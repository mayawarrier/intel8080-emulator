
#include <stdio.h>
#include <string.h>

#include "i8080.h"
#include "vm.h"
#include "vm_bios.h"
#include "vm_devices.h"
#include "simple_vm_c90_noalloc.h"

#define VM_243K (248832)

/* always succeeds; does nothing */
static int vm_vdev_noinit(void *dev) { (void)dev; return 0; }

/* 
 * Virtual console.
 * This version just maps directly to stdin/stdout.
 * Microcomputer-specific control sequences would best be handled here.
 * Note that CP/M 2.2 expects these functions to block.
 */
static int vm_vcon_status(void *dev)
{
	struct vm_vconsole *cdev = (struct vm_vconsole *)dev;
	return ferror((FILE *)cdev->sin) || ferror((FILE *)cdev->sout);
}
static char vm_vcon_in(void *dev)
{
	char c;
	struct vm_vconsole *cdev = (struct vm_vconsole *)dev;
	/* blocks until success */
	do { c = (char)getc((FILE *)cdev->sin); } while (ferror((FILE *)cdev->sin));
	return c;
}
static void vm_vcon_out(void *dev, char c)
{
	struct vm_vconsole *cdev = (struct vm_vconsole *)dev;
	/* blocks until success */
	do { putc(c, (FILE *)cdev->sout); } while (ferror((FILE *)cdev->sout));
}

/*
 * Virtual disk(s).
 * This version just emulates the disks in memory.
 * CP/M requires each disk format/drive to have a
 * "Disk Parameter Header" (disk->dph_addr).
 * This is generated in simple_vm_init_all_disks().
 * See also vm_bios.h/cpm80_bios_redefine_disks().
 */
static void vm_vdisk_setsec(void *dev, unsigned sector)
{
	struct vm_vibm3740 *ddev = (struct vm_vibm3740 *)dev;
	ddev->sector = sector;
}
static void vm_vdisk_settrk(void *dev, unsigned track)
{
	struct vm_vibm3740 *ddev = (struct vm_vibm3740 *)dev;
	ddev->track = track;
}
static int vm_vdisk_readl(void *dev, char buf[128])
{
	struct vm_vibm3740 *ddev = (struct vm_vibm3740 *)dev;
	const unsigned long bindex = (unsigned long)ddev->sector * ddev->track;
	memcpy(buf, ddev->disk + bindex, 128 * sizeof(char));
	/* always succeeds */
	return 0;
}
static int vm_vdisk_writel(void *dev, char buf[128], int deblock_code)
{
	(void)deblock_code;
	/* don't bother de-blocking - we're writing to memory which is
	 * bound to be many times faster than disk access anyway */
	struct vm_vibm3740 *ddev = (struct vm_vibm3740 *)dev;
	const unsigned long bindex = (unsigned long)ddev->sector * ddev->track;
	memcpy(ddev->disk + bindex, buf, 128 * sizeof(char));
	/* always succeeds */
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

	struct vm_vibm3740 *vdisk_devs = svm->vdisk_devices;

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
	con->init = vm_vdev_noinit;
	con->status = vm_vcon_status;
	con->in = vm_vcon_in;
	con->out = vm_vcon_out;
}

static void simple_vm_init_disk(struct cpm80_vm_simple *const svm, int disknum, cpm80_addr_t dph_addr)
{
	struct cpm80_disk_ldevice *const disk = svm->ldisks + disknum;
	disk->dev = svm->vdisk_devices + disknum;
	disk->dph_addr = dph_addr;
	disk->init = vm_vdev_noinit;
	disk->set_sector = vm_vdisk_setsec;
	disk->set_track = vm_vdisk_settrk;
	disk->readl = vm_vdisk_readl;
	disk->writel = vm_vdisk_writel;
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
