
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <limits.h>

#include "vm.h"
#include "i8080.h"
#include "internal/vm_bios.h"
#include "internal/vm_devices.h"
#include "test_vm.h"

#if UINT_MAX >= 0xffffffff
typedef unsigned uint32;
#elif ULONG_MAX >= 0xffffffff
typedef unsigned long uint32;
#else
#error "Cannot find a 32-bit type."
#endif

struct cpm80_test_vm
{
	struct vconsole {
		FILE *sin;
		FILE *sout;
	} vcon;

	struct vdisk {
		/* first sector, sectors/track */
		unsigned fsc, spt;
		unsigned sector, track;
		char *disk; /* 256256 bytes */
	} *vdisks;

	struct i8080 cpu;
	struct cpm80_vm base_vm;
	struct cpm80_serial_ldevice lcon;
	struct cpm80_disk_ldevice *ldisks;
};

/* does nothing. always succeeds. */
static int vdev_noinit(void *dev) { (void)dev; return 0; }

/* 
 * Virtual console.
 * Maps to stdin/stdout.
 * Microcomputer-specific character/control
 * sequences would best be handled here.
 * CP/M 2.2 expects these functions to block until successful.
 */
static int vcon_init(void *dev)
{
	struct vconsole *vcon = (struct vconsole *)dev;
	vcon->sin = stdin; vcon->sout = stdout;
	return 0;
}
static int vcon_status(void *dev)
{
	(void)dev;
	/* Single char operations are expected to
	 * be atomic, so the console is technically 
	 * always "ready," and we can't check if a future
	 * I/O stream call will fail. So always succeed by
	 * default. vcon_in() and vcon_out() will block if
	 * necessary. */
	return 0;
}
static char vcon_in(void *dev)
{
	char c; struct vconsole *vcon = (struct vconsole *)dev;
	clearerr(vcon->sin);
	do { c = (char)getc(vcon->sin); } while (ferror(vcon->sin));
	return c;
}
static void vcon_out(void *dev, char c)
{
	struct vconsole *vcon = (struct vconsole *)dev;
	clearerr(vcon->sout); 
	do { putc(c, vcon->sout); } while (ferror(vcon->sout));
}
static void init_console(struct cpm80_test_vm *const vm)
{
	vm->lcon.dev = &vm->vcon;
	vm->lcon.init = vcon_init;
	vm->lcon.status = vcon_status;
	vm->lcon.in = vcon_in;
	vm->lcon.out = vcon_out;
	vm->base_vm.con = &vm->lcon;
}

/*
 * Virtual disk(s).
 * CP/M requires each disk format or drive to
 * have a "Disk Parameter Header" (disk->dph_addr).
 * Parameters for an 8" 250K IBM-PC compatible floppy follow.
 *
 * Headers are generated from this with a call to
 * vm_bios.h/cpm80_bios_define_disks() (a port of the CP/M 2.0
 * disk re-definition library to C. See documentation in vm_bios.h
 * for more details).
 * 
 * Numbers were taken from the Intel MDS-800 BIOS for CP/M 2.2:
 * http://www.gaby.de/cpm/manuals/archive/cpm22htm/axa.asm
 */
static const unsigned IBM_PC_250K_params[] = {
	/* (77/26/128 format with 2 reserved tracks. ~243K usable) */
	10, 0, 1, 26, 6, 1024, 243, 64, 64, 2, 1
};

static void vdisk_setsec(void *dev, unsigned sector)
{
	((struct vdisk *)dev)->sector = sector;
}
static void vdisk_settrk(void *dev, unsigned track)
{
	((struct vdisk *)dev)->track = track;
}
static int vdisk_readl(void *dev, char buf[128])
{
	struct vdisk *vd = (struct vdisk *)dev;
	const uint32 pos = ((vd->track * vd->spt) + vd->sector - vd->fsc) * 128;
	memcpy(buf, vd->disk + pos, 128 * sizeof(char));
	return 0;
}
static int vdisk_writel(void *dev, char buf[128], int deblock_code)
{
	(void)deblock_code;
	/* don't bother de-blocking - any modern I/O is bound
	 * to be many times faster than floppy disk I/O anyway */
	struct vdisk *vd = (struct vdisk *)dev;
	const uint32 pos = ((vd->track * vd->spt) + vd->sector - vd->fsc) * 128;
	memcpy(vd->disk + pos, buf, 128 * sizeof(char));
	return 0;
}

/* base_vm.cpu must be initialized first */
static int init_all_disks(struct cpm80_test_vm *const vm, 
	const struct cpm80_test_vm_params *params, i8080_addr_t cpm_origin)
{
	int ndisks = params->ndisks;
	if (ndisks < 1 || ndisks > CPM80_MAX_DISKS || !params->disks) 
		return -1;

	int err = -1; /* err code if something fails */
	vm->vdisks = malloc(ndisks * sizeof(struct vdisk));
	vm->ldisks = malloc(ndisks * sizeof(struct cpm80_disk_ldevice));
	if (!vm->vdisks || !vm->ldisks) goto on_failure;

	/* init vdisks */
	int i;
	struct vdisk *vdisk;
	for (i = 0; i < ndisks; ++i) {
		vdisk = vm->vdisks + i;
		if (!params->disks[i]) goto on_failure;
		vdisk->disk = params->disks[i];
		vdisk->fsc = IBM_PC_250K_params[2];
		vdisk->spt = IBM_PC_250K_params[3] - vdisk->fsc + 1;
		/* probably starts at the beginning? */
		vdisk->sector = vdisk->fsc;
		vdisk->track = 0; 
	}

	/* set up params for n disks */
	cpm80_addr_t dphs[CPM80_MAX_DISKS];
	const unsigned *disk_params[CPM80_MAX_DISKS];
	unsigned dupe_disks[3 * (CPM80_MAX_DISKS - 1)];

	disk_params[0] = IBM_PC_250K_params; /* at least 1 disk */
	if (ndisks > 1) {
		int j, pindex = 0;
		for (j = 1; j < ndisks; ++j) {
			disk_params[j] = dupe_disks + pindex;
			dupe_disks[pindex++] = 2; /* num elems */
			dupe_disks[pindex++] = j; /* disk number */
			dupe_disks[pindex++] = 0; /* duplicates of disk 0 */
		}
	}
	/* same as in MDS-800 BIOS */
	const cpm80_addr_t defns_start = cpm_origin + 0x1633;
	const cpm80_addr_t bdos_ram_start = cpm_origin + 0x186e;

	/* Generate disk headers from params. */
	err = cpm80_bios_define_disks(&vm->base_vm, ndisks, disk_params, defns_start, bdos_ram_start, dphs);
	if (err) goto on_failure;

	/* init ldisks */
	int k;
	struct cpm80_disk_ldevice *ldisk;
	for (k = 0; k < ndisks; ++k) {
		ldisk = vm->ldisks + k;
		ldisk->dev = vm->vdisks + k;
		ldisk->dph_addr = dphs[k];

		ldisk->init = vdev_noinit;
		ldisk->set_sector = vdisk_setsec;
		ldisk->set_track = vdisk_settrk;
		ldisk->readl = vdisk_readl;
		ldisk->writel = vdisk_writel;
	}

	vm->base_vm.ndisks = ndisks;
	vm->base_vm.disks = vm->ldisks;
	return 0;

on_failure:
	if (vm->vdisks) free(vm->vdisks);
	if (vm->ldisks) free(vm->ldisks);
	return err;
}

#define memory_ptr(cpu) ((i8080_word_t *)((struct cpm80_vm *)((cpu)->udata))->udata)

static i8080_word_t vm_memory_read(const struct i8080 *c, i8080_addr_t a) { return *(memory_ptr(c) + a); }
static void vm_memory_write(const struct i8080 *c, i8080_addr_t a, i8080_word_t w) { *(memory_ptr(c) + a) = w; }

struct cpm80_test_vm *test_vm_create(const struct cpm80_test_vm_params *params)
{
	/* basic param checks */
	if (!params->memory ||
	#if UINT_MAX != 0xffff
		params->maxaddr > 65535 ||
	#endif
		!params->cpmbin ||
		params->cpmbin_size > params->maxaddr) {
		return NULL;
	}

	struct cpm80_test_vm *vm = malloc(sizeof(struct cpm80_test_vm));
	if (!vm) return NULL;
	vm->ldisks = NULL;
	vm->vdisks = NULL;

	/* load cpm binary */
	int memsize = params->maxaddr / 1024 + 1;
	cpm80_addr_t cpm_origin = (memsize - 7) * 1024;
	memcpy(params->memory + cpm_origin, params->cpmbin, params->cpmbin_size * sizeof(i8080_word_t));

	/* init cpu */
	i8080_init(&vm->cpu);
	vm->cpu.memory_read = vm_memory_read;
	vm->cpu.memory_write = vm_memory_write;
	
	/* init cpm80_vm */
	vm->base_vm.cpu = &vm->cpu;
	vm->base_vm.memsize = memsize;
	vm->base_vm.cpm_origin = cpm_origin;
	vm->base_vm.udata = params->memory; /* store memory ptr */
	int err = cpm80_vm_init(&vm->base_vm);
	if (err) {
		free(vm);
		return NULL;
	}

	/* init console and disks */
	init_console(vm);
	err = init_all_disks(vm, params, cpm_origin);
	if (err) {
		free(vm);
		return NULL;
	}

	/* unused devices */
	vm->base_vm.lst = 0;
	vm->base_vm.rdr = 0;
	vm->base_vm.pun = 0;

	return vm;
}

void test_vm_destroy(struct cpm80_test_vm *vm)
{
	if (vm) {
		if (vm->ldisks) free(vm->ldisks);
		if (vm->vdisks) free(vm->vdisks);
		free(vm);
	}
}

int test_vm_start(struct cpm80_test_vm *const vm)
{
	/* power on! */
	int err = cpm80_vm_poweron(&vm->base_vm);
	if (err) return err;

	struct i8080 *cpu = &vm->cpu;
	/* execute instructions till poweroff/error */
	while (!i8080_next(cpu));

	return cpu->mexitcode;
}
