
#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "i8080.h"
#include "vm.h"
#include "vm_types.h"
#include "vm_bios.h"
#include "vm_devices.h"
#include "vm_sample.h"

#define VM_MAX_DISKS (4)
#define VM_243K (248832)

static struct cpm80_sample_vm
{
	i8080_word_t *memory;
	i8080_addr_t memory_maxaddr;
	int memory_ksize;

	struct vm_vconsole {
		FILE *sin;
		FILE *sout;
	} vcon_device;

	struct vm_vibm3740 {
		unsigned sector, track;
		char *disk; /* 243KB */
	}
	vdisk_devices[VM_MAX_DISKS];

	struct i8080 cpu;
	struct cpm80_vm base_vm;
	struct cpm80_serial_ldevice lcon;
	struct cpm80_disk_ldevice ldisks[VM_MAX_DISKS];
}
SAMPLE_VM =
{
	.memory = 0,
	.vcon_device =
	{
		.sin = stdin,
		.sout = stdout
	},
	.vdisk_devices =
	{
		{.sector = 0,.track = 0,.disk = 0 },
		{.sector = 0,.track = 0,.disk = 0 },
		{.sector = 0,.track = 0,.disk = 0 },
		{.sector = 0,.track = 0,.disk = 0 }
	}
};

static i8080_word_t vm_memory_read(i8080_addr_t a) { return VM_MEMORY[a]; }
static void vm_memory_write(i8080_addr_t a, i8080_word_t w) { VM_MEMORY[a] = w; }

/* does nothing */
static int vm_vdev_noinit(void *) { return 0; }

static int vm_vcon_status(void *dev)
{
	struct vm_vconsole *cdev = (struct vm_vconsole *)dev;
	return ferror(cdev->sin) || ferror(cdev->sout);
}

static char vm_vcon_in(void *dev)
{
	char c;
	struct vm_vconsole *cdev = (struct vm_vconsole *)dev;
	/* blocks until success */
	do { c = (char)getc(cdev->sin); } while (ferror(cdev->sin));
	return c;
}

static void vm_vcon_out(void *dev, char c)
{
	struct vm_vconsole *cdev = (struct vm_vconsole *)dev;
	/* blocks until success */
	do { putc(c, cdev->sout); } while (ferror(cdev->sout));
}

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

static int vm_vdisk_writel(void *dev, char buf[128], int /* deblock_code */)
{
	/* don't bother de-blocking - we're writing to memory and this is 
	 * bound to be many times faster than disk access anyway */
	struct vm_vibm3740 *ddev = (struct vm_vibm3740 *)dev;
	const unsigned long bindex = (unsigned long)ddev->sector * ddev->track;
	memcpy(ddev->disk + bindex, buf, 128 * sizeof(char));
	/* always succeeds */
	return 0;
}

void sample_vm_set_working_memory(i8080_word_t *memory, i8080_addr_t maxaddr)
{
	SAMPLE_VM.memory = memory;
	SAMPLE_VM.memory_maxaddr = maxaddr;
	SAMPLE_VM.memory_ksize = maxaddr / 1024 + 1;
}

void sample_vm_set_disk_memory(char *memory, int ndisks)
{
	struct vm_vibm3740 *vdisk_devs = SAMPLE_VM.vdisk_devices;

	if (ndisks > 0 && ndisks <= VM_MAX_DISKS) {
		SAMPLE_VM.base_vm.ndisks = ndisks;

		ndisks--;
		while (ndisks > -1) {
			vdisk_devs[ndisks].disk = memory;
			memory += VM_243K;
			ndisks--;
		}
	}
}

static void sample_vm_init_console(void)
{
	struct cpm80_serial_ldevice *const con = &SAMPLE_VM.lcon;
	con->dev = &SAMPLE_VM.vcon_device;
	con->init = vm_vdev_noinit;
	con->status = vm_vcon_status;
	con->in = vm_vcon_in;
	con->out = vm_vcon_out;
}

static void sample_vm_init_disk(int disknum, cpm80_addr_t dph_addr)
{
	struct cpm80_disk_ldevice *const disk = SAMPLE_VM.ldisks + disknum;
	disk->dev = SAMPLE_VM.vdisk_devices + disknum;
	disk->dph_addr = dph_addr;
	disk->init = vm_vdev_noinit; /* DPH already created */
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
static int sample_vm_init_all_disks(void)
{
	cpm80_addr_t dph_addrs[VM_MAX_DISKS];
	/* right below the BIOS jump table */
	const cpm80_addr_t disk_defns_begin = base_vm->cpm_origin + 0x1633;
	/* same as in MDS-800 I/O drivers; should be enough clearance */
	const cpm80_addr_t disk_bdos_ram_begin = base_vm->cpm_origin + 0x186e;

	/* Define disk parameters in BIOS memory in the correct format for CP/M.
	 * This is a C translation of the CP/M 2.0 disk re-definition library. 
	 * See vm_bios.h for more details */
	int err = cpm80_bios_redefine_disks(base_vm, SAMPLE_VM.base_vm.ndisks,
		vm_disk_params, disk_defns_begin, disk_bdos_ram_begin, dph_addrs);
	if (err) return err;

	int i;
	for (i = 0; i < SAMPLE_VM.base_vm.ndisks; ++i) {
		sample_vm_init_disk(i, dph_addrs[i]);
	}

	SAMPLE_VM.base_vm.disks = SAMPLE_VM.ldisks;

	return 0;
}

int sample_vm_init(void)
{
	struct cpm80_vm *base_vm = &SAMPLE_VM.base_vm;
	struct i8080 *cpu = &SAMPLE_VM.cpu;

	/* no memory to boot from! */
	if (!SAMPLE_VM.memory || !SAMPLE_VM.vdisk_devices[0].disk)
		return -1;

	/* init cpu */
	i8080_init(cpu);
	cpu->memory_read = vm_memory_read;
	cpu->memory_write = vm_memory_write;
	
	/* init base_vm */
	base_vm->cpu = cpu;
	base_vm->memsize = SAMPLE_VM.memory_ksize;
	/* the usual case, but may depend on manufacturer */
	base_vm->cpm_origin = (SAMPLE_VM.memory_ksize - 7) * 1024;
	err = cpm80_vm_init(base_vm);
	if (err) return err;

	/* init disks and console */
	err = sample_vm_init_all_disks();
	if (err) return err;
	sample_vm_init_console();	

	/* assign base_vm devices */
	base_vm->con = &SAMPLE_VM.lcon;
	base_vm->lst = 0;
	base_vm->rdr = 0;
	base_vm->pun = 0;

	return 0;
}

static void poweroff_handler(int sig)
{
	signal(sig, SIG_IGN); /* prevent a second signal from arriving */
	cpm80_vm_poweroff(&SAMPLE_VM.base_vm);
	signal(sig, poweroff_handler);
}

int sample_vm_start(void)
{
	struct i8080 *cpu = &SAMPLE_VM.cpu;
	struct cpm80_vm *base_vm = &SAMPLE_VM.base_vm;

	/* power on! */
	int err = cpm80_vm_poweron(base_vm);
	if (err) return err;

	signal(SIGINT, poweroff_handler);

	/* execute instructions until poweroff or fatal error */
	while (!i8080_next(cpu));

	return cpu->exitcode;
}
