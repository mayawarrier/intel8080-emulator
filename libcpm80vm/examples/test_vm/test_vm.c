
#include <stdio.h>
#include <string.h>

#include "i8080.h"
#include "vm.h"
#include "vm_bios.h"
#include "vm_devices.h"
#include "test_vm.h"

struct cpm80_vm_simple
{
	struct vconsole {
		FILE *sin;
		FILE *sout;
	} vcon_device;

	struct vdisk {
		unsigned sector, track;
		char *disk; /* 243KB */
	}
	vdisk_devices[SIMPLEVM_MAX_DISKS];

	struct i8080 cpu;
	struct cpm80_vm base_vm;
	struct cpm80_serial_ldevice lcon;
	struct cpm80_disk_ldevice ldisks[SIMPLEVM_MAX_DISKS];
};

#define VM_243K (248832)

/* does nothing. always succeeds. */
static int vdev_noinit(void *dev) { (void)dev; return 0; }

/* 
 * Virtual console.
 * This version maps directly to stdin/stdout.
 * Microcomputer-specific control sequences would best be handled here.
 * Note that CP/M 2.2 expects these functions to block until successful.
 */
static int vcon_status(void *dev)
{
	struct vconsole *vcon = (struct vconsole *)dev;
	return ferror(vcon->sin) || ferror(vcon->sout);
}
static char vcon_in(void *dev)
{
	char c;
	struct vconsole *vcon = (struct vconsole *)dev;
	/* blocks until success */
	do { c = (char)getc(vcon->sin); } while (ferror(vcon->sin));
	return c;
}
static void vcon_out(void *dev, char c)
{
	struct vconsole *vcon = (struct vconsole *)dev;
	/* blocks until success */
	do { putc(c, vcon->sout); } while (ferror(vcon->sout));
}

/*
 * Virtual disk(s).
 * This version emulates the disks in memory.
 * CP/M requires each disk format/drive to have a
 * "Disk Parameter Header" (disk->dph_addr),
 * See vm_bios.h/cpm80_bios_define_disks().
 */
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
	struct vdisk *ddev = (struct vdisk *)dev;
	const unsigned long bindex = (unsigned long)ddev->sector * ddev->track;
	memcpy(buf, ddev->disk + bindex, 128 * sizeof(char));
	return 0;
}
static int vdisk_writel(void *dev, char buf[128], int deblock_code)
{
	(void)deblock_code;
	/* don't bother de-blocking - we're writing to memory which is
	 * bound to be many times faster than disk access anyway */
	struct vdisk *ddev = (struct vdisk *)dev;
	const unsigned long bindex = (unsigned long)ddev->sector * ddev->track;
	memcpy(ddev->disk + bindex, buf, 128 * sizeof(char));
	return 0;
}

#define get_svm_memory_ptr(cpu) ((i8080_word_t *)((struct cpm80_vm *)((cpu)->udata))->udata)

static i8080_word_t vm_memory_read(const struct i8080 *cpu, i8080_addr_t a)
{
	return *(get_svm_memory_ptr(cpu) + a);
}
static void vm_memory_write(const struct i8080 *cpu, i8080_addr_t a, i8080_word_t w)
{
	*(get_svm_memory_ptr(cpu) + a) = w;
}

static int set_svm_memory_ptr(struct cpm80_vm_simple *const svm, const struct cpm80_vm_simple_params *params)
{
	if (!params->working_memory) return -1;
	svm->base_vm.udata = params->working_memory;
	return 0;
}

static int
set_svm_disk_memory_bufs(struct cpm80_vm_simple *const svm, const struct cpm80_vm_simple_params *params)
{
	int ndisks = params->ndisks;
	i8080_word_t *memory_buf = params->disk_memory;
	if (ndisks < 1 || ndisks > SIMPLEVM_MAX_DISKS || !memory_buf) return -1;

	struct vdisk *vdisk_devs = svm->vdisk_devices;

	ndisks--;
	while (ndisks > -1) {
		vdisk_devs[ndisks].disk = memory_buf;
		memory_buf += VM_243K;
		ndisks--;
	}

	return 0;
}

static void simple_vm_init_console(struct cpm80_vm_simple *const svm)
{
	struct cpm80_serial_ldevice *const con = &svm->lcon;
	con->dev = &svm->vcon_device;
	con->init = vdev_noinit;
	con->status = vcon_status;
	con->in = vcon_in;
	con->out = vcon_out;
}

static void simple_vm_init_disk(struct cpm80_vm_simple *const svm, int disknum, cpm80_addr_t dph_addr)
{
	struct cpm80_disk_ldevice *const disk = svm->ldisks + disknum;
	disk->dev = svm->vdisk_devices + disknum;
	disk->dph_addr = dph_addr;
	disk->init = vdev_noinit;
	disk->set_sector = vdisk_setsec;
	disk->set_track = vdisk_settrk;
	disk->readl = vdisk_readl;
	disk->writel = vdisk_writel;
}

/* A list of parameters for 4 IBM 3740 SD 8" floppy disks. See vm_bios.h.
 * These numbers were extracted from the Intel MDS-800 BIOS for CP/M 2.2:
 * http://www.gaby.de/cpm/manuals/archive/cpm22htm/axa.asm */
static const unsigned disk_param_data[] =
{
	/* disk 0 */
	10, 0, 1, 26, 6, 1024, 243, 64, 64, 2, 1,
	2, 1, 0, /* disk 1 */
	2, 2, 0, /* disk 2 */
	2, 3, 0  /* disk 3 */
};
/* as formatted for vm_bios.h */
static const unsigned *const vm_disk_params[] =
{
	disk_param_data,
	disk_param_data + 11,
	disk_param_data + 14,
	disk_param_data + 17
};

/* base_vm->cpu must be initialized first */
static int simple_vm_init_all_disks(struct cpm80_vm_simple *const svm, i8080_addr_t cpm_origin, int ndisks)
{
	cpm80_addr_t dph_addrs[SIMPLEVM_MAX_DISKS];
	/* right below the BIOS jump table */
	const cpm80_addr_t disk_defns_begin = cpm_origin + 0x1633;
	/* same as in MDS-800 I/O drivers; should be enough clearance */
	const cpm80_addr_t disk_bdos_ram_begin = cpm_origin + 0x186e;

	/* Define disk parameter headers for CP/M.
	 * This is a port of the CP/M 2.0 disk re-definition library to C.
	 * See vm_bios.h for more details */
	int err = cpm80_bios_redefine_disks(&svm->base_vm, ndisks,
		vm_disk_params, disk_defns_begin, disk_bdos_ram_begin, dph_addrs);
	if (err) return err;

	int i;
	for (i = 0; i < ndisks; ++i) {
		simple_vm_init_disk(svm, i, dph_addrs[i]);
	}

	return 0;
}

int simple_vm_init(struct cpm80_vm_simple *const svm,
	const struct cpm80_vm_simple_params *params, const i8080_word_t cpmbin[0x1633])
{
	int err;
	struct i8080 *const cpu = &svm->cpu;
	struct cpm80_vm *base_vm = &svm->base_vm;

	/* init sample_vm */
	err = set_svm_memory_ptr(svm, params);
	if (err) return err;
	err = set_svm_disk_memory_bufs(svm, params);
	if (err) return err;
	svm->vcon_device.sin = stdin;
	svm->vcon_device.sout = stdout;

	/* init cpu */
	i8080_init(cpu);
	cpu->memory_read = vm_memory_read;
	cpu->memory_write = vm_memory_write;
	
	/* init base_vm */
	base_vm->cpu = cpu;
	base_vm->memsize = params->maxaddr / 1024 + 1;
	/* the usual case, but may depend on manufacturer */
	base_vm->cpm_origin = (base_vm->memsize - 7) * 1024;
	err = cpm80_vm_init(base_vm);
	if (err) return err;

	/* load cpm binary */
	memcpy(params->working_memory + base_vm->cpm_origin, cpmbin, 0x1633 * sizeof(i8080_word_t));

	/* init disks and console */
	err = simple_vm_init_all_disks(svm, (i8080_addr_t)base_vm->cpm_origin, params->ndisks);
	if (err) return err;
	base_vm->ndisks = params->ndisks;
	base_vm->disks = svm->ldisks;
	simple_vm_init_console(svm);
	base_vm->con = &svm->lcon;

	/* unused base_vm devices */
	base_vm->lst = 0;
	base_vm->rdr = 0;
	base_vm->pun = 0;

	return 0;
}

int simple_vm_start(struct cpm80_vm_simple *const svm)
{
	struct i8080 *cpu = &svm->cpu;
	struct cpm80_vm *base_vm = &svm->base_vm;

	/* power on! */
	int err = cpm80_vm_poweron(base_vm);
	if (err) return err;

	/* execute instructions until poweroff or fatal error */
	while (!i8080_next(cpu));

	return cpu->exitcode;
}
