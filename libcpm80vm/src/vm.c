
#include "i8080.h"
#include "i8080_opcodes.h"
#include "vm_devices.h"
#include "vm_bios.h"
#include "vm_callno.h"
#include "vm_types.h"
#include "vm.h"

#if defined (__STDC__) && !defined(__STDC_VERSION__)
	#define inline
#endif

extern int vmsprintf(char *dest, const char *format, const void *const *args); /* strutil.c */

static inline void vmputs(struct cpm80_serial_ldevice *const con, const char *str)
{
	char c;
	while ((c = *str) != '\0') {
		con->out(con->dev, c);
		str++;
	}
}

static char VM_strbuf[128];
static const char VM_err_fmt[] = "\r\n>>VM error: %s\r\n";

static int bios_monitor(struct i8080 *cpu)
{
	struct cpm80_vm *vm = (struct cpm80_vm *)cpu->udata;
	const cpm80_addr_t bios_table_begin = vm->cpm_origin + 0x1600;

	/* Translate CP/M's bios table to VM bios_call() */
	int callno = (int)(cpu->pc - bios_table_begin) / 3;

	if (callno > 16 || callno < 0) {
		/* huh? how did this happen? */
		void *fargs[] = { &cpu->pc };
		vmsprintf(VM_strbuf + 64, "Unexpected monitor call at 0x%04x", fargs);
		fargs[0] = VM_strbuf + 64;
		vmsprintf(VM_strbuf, VM_err_fmt, fargs);
		vmputs(vm->con, VM_strbuf);

		return VM80_UNEXPECTED_MONITOR_CALL;
	} 
	else return vm->bios_call(vm, callno);
}

static struct i8080_monitor vm_monitor = { .on_rst7 = bios_monitor,.on_halt_changed = 0 };

int cpm80_vm_init(struct cpm80_vm *const vm)
{
	int okay = 0;
	if (vm->cpu && vm->cpu->memory_read && vm->cpu->memory_write &&
		vm->memsize > 0 && vm->memsize < 65 &&
		(vm->memsize == 64 || (unsigned)vm->cpm_origin < (unsigned)vm->memsize * 1024)) {
		okay = 1;
	}
	if (!okay) return -1;

	vm->cpu->udata = vm;
	vm->cpu->monitor = &vm_monitor;
	vm->bios_call = cpm80_bios_call;
	vm->is_poweron = 0;
	
	const cpm80_addr_t bios_table_begin = vm->cpm_origin + 0x1600;

	/* set up monitor calls */
	int i; cpm80_addr_t tbl_ptr = bios_table_begin;
	for (i = 0; i < 17; ++i) {
		vm->cpu->memory_write(vm->cpu, tbl_ptr, i8080_RST_7);
		tbl_ptr += 0x0003;
	}

	/* JMP to cold boot on reset */
	i8080_word_t boot_ptr_lo = (i8080_word_t)(bios_table_begin & 0xff);
	i8080_word_t boot_ptr_hi = (i8080_word_t)((bios_table_begin >> 8) & 0xff);
	vm->cpu->memory_write(vm->cpu, 0x0000, i8080_JMP);
	vm->cpu->memory_write(vm->cpu, 0x0001, boot_ptr_lo);
	vm->cpu->memory_write(vm->cpu, 0x0002, boot_ptr_hi);

	return 0;
}

static inline int is_serial_device_initialized(struct cpm80_serial_ldevice *ldev)
{
	return (ldev && ldev->init && ldev->in && ldev->out && ldev->status);
}

static inline int is_disk_device_initialized(struct cpm80_disk_ldevice *disk)
{
	return (disk && disk->init && disk->readl && disk->writel && disk->set_sector && disk->set_track);
}

static void fatal_io_write(const struct i8080 *cpu, i8080_addr_t port, i8080_word_t word)
{
	port &= 0xff; /* actual port is only 8 bits! */
	struct cpm80_vm *vm = (struct cpm80_vm *)cpu->udata;

	void *fargs[] = { &word, &port };
	vmsprintf(VM_strbuf + 64, "Unhandled write 0x%02x to port %u", fargs);
	fargs[0] = VM_strbuf + 64;
	vmsprintf(VM_strbuf, VM_err_fmt, fargs);
	vmputs(vm->con, VM_strbuf);

	vm->cpu->mexitcode = VM80_UNHANDLED_IO;
}

static i8080_word_t fatal_io_read(const struct i8080 *cpu, i8080_addr_t port)
{
	port &= 0xff; /* actual port is only 8 bits! */
	struct cpm80_vm *vm = (struct cpm80_vm *)cpu->udata;

	void *fargs[] = { &port };
	vmsprintf(VM_strbuf + 64, "Unhandled read from port %u", fargs);
	fargs[0] = VM_strbuf + 64;
	vmsprintf(VM_strbuf, VM_err_fmt, fargs);
	vmputs(vm->con, VM_strbuf);

	vm->cpu->mexitcode = VM80_UNHANDLED_IO;
	return 0;
}

static i8080_word_t fatal_interrupt_read(const struct i8080 *cpu)
{
	struct cpm80_vm *vm = (struct cpm80_vm *)cpu->udata;

	void *fargs[] = { "Unhandled interrupt" };
	vmsprintf(VM_strbuf, VM_err_fmt, fargs);
	vmputs(vm->con, VM_strbuf);

	vm->cpu->mexitcode = VM80_UNHANDLED_INTR;
	return 0;
}

int cpm80_vm_poweron(struct cpm80_vm *const vm)
{
	if (!vm || !is_serial_device_initialized(vm->con) || !vm->bios_call ||
		!vm->disks || vm->ndisks < 1 || !is_disk_device_initialized(vm->disks) ||
		!vm->cpu || !vm->cpu->memory_read || !vm->cpu->memory_write || vm->cpu->mexitcode != 0) {
		return -1;
	}

	/* Calls to NULL handlers fail silently.
	 * Instead, catch any attempt to do so and print debug info */
	if (!vm->cpu->io_read) vm->cpu->io_read = fatal_io_read;
	if (!vm->cpu->io_write) vm->cpu->io_write = fatal_io_write;
	if (!vm->cpu->interrupt_read) vm->cpu->interrupt_read = fatal_interrupt_read;

	i8080_reset(vm->cpu);
	vm->is_poweron = 1;

	return 0;
}

static i8080_word_t poweroff(const struct i8080 *cpu)
{
	struct cpm80_vm *vm = (struct cpm80_vm *)cpu->udata;
	vm->cpu->interrupt_read = vm->prev_ih; /* restore original */
	vm->cpu->mexitcode = VM80_POWEROFF;
	vm->is_poweron = 0;
	return i8080_NOP;
}

void cpm80_vm_poweroff(struct cpm80_vm *const vm)
{
	vm->prev_ih = vm->cpu->interrupt_read; /* save original */
	vm->cpu->interrupt_read = poweroff;
	vm->cpu->inte = 1; /* force interrupts on */
	i8080_interrupt(vm->cpu);
}

#define lsbit(buf) (buf & (unsigned)1) 
#define concatenate(byte1, byte2) ((unsigned)(byte1) << 8 | byte2)

#define get_bios_args_bc(cpu) concatenate((cpu)->b, (cpu)->c)
#define get_bios_args_de(cpu) concatenate((cpu)->d, (cpu)->e)

static inline void set_bios_returnval(struct i8080 *const cpu, unsigned retval)
{
	cpu->h = (i8080_word_t)((retval & 0xff00) >> 8);
	cpu->l = (i8080_word_t)(retval & 0xff);
}

#define serial_init(ldev) ((ldev)->init((ldev)->dev))
#define serial_in(cpu, ldev) ((cpu)->a = (i8080_word_t)((ldev)->in((ldev)->dev)))
#define serial_out(cpu, ldev) ((ldev)->out((ldev)->dev, (char)((cpu)->c)))
#define serial_stat(cpu, ldev) ((cpu)->a = ((ldev)->status((ldev)->dev)) ? 0xff : 0x00)
#define set_track(ldev, track) ((ldev)->set_track((ldev)->dev, track))
#define set_sector(ldev, sector) ((ldev)->set_sector((ldev)->dev, sector))
#define disk_setstat(cpu, st) ((cpu)->a = ((st) == -1) ? 0xff : (i8080_word_t)(st))

static inline void disk_select(struct cpm80_vm *const vm)
{
	int selected = (int)vm->cpu->c;
	struct cpm80_disk_ldevice *const disk = vm->disks + selected;

	/* generate disk param block if possible */
	if (!lsbit(vm->cpu->e) && disk->init(disk->dev)) {
		set_bios_returnval(vm->cpu, 0);
		return;
	}

	set_bios_returnval(vm->cpu, disk->dph_addr);
	vm->sel_disk = selected;
}

static inline int dma_out_of_bounds(int memsize, cpm80_addr_t dma_addr)
{
	if (memsize >= 64) {
		return 65535 - (unsigned)dma_addr < 127;
	} else {
		return (unsigned)memsize * 1024 - (unsigned)dma_addr < 128;
	}
}

static void fatal_dma(struct cpm80_vm *const vm, cpm80_addr_t dma_addr)
{
	void *fargs[] = { &dma_addr };
	vmsprintf(VM_strbuf + 64, "DMA out of bounds: 0x%04x", fargs);
	fargs[0] = VM_strbuf + 64;
	vmsprintf(VM_strbuf, VM_err_fmt, fargs);
	vmputs(vm->con, VM_strbuf);

	vm->cpu->mexitcode = VM80_DMA_FATAL;
}

static inline void disk_set_track(struct cpm80_vm *const vm)
{
	struct cpm80_disk_ldevice *const disk = vm->disks + vm->sel_disk;
	set_track(disk, get_bios_args_bc(vm->cpu));
}

static inline void disk_set_sector(struct cpm80_vm *const vm)
{
	struct cpm80_disk_ldevice *const disk = vm->disks + vm->sel_disk;
	set_sector(disk, get_bios_args_bc(vm->cpu));
}

static inline void disk_home(struct cpm80_vm *const vm)
{
	struct cpm80_disk_ldevice *const disk = vm->disks + vm->sel_disk;
	set_track(disk, 0);
}

static inline void disk_read(struct cpm80_vm *const vm)
{
	if (dma_out_of_bounds(vm->memsize, vm->dma_addr)) {
		fatal_dma(vm, vm->dma_addr);
		return;
	}

	char buf[128];
	struct cpm80_disk_ldevice *const disk = vm->disks + vm->sel_disk;
	int err = disk->readl(disk->dev, buf);

	disk_setstat(vm->cpu, err);
	if (err) return;

	unsigned i;
	const unsigned addr = (unsigned)vm->dma_addr;
	for (i = 0; i < 128; ++i) {
		vm->cpu->memory_write(vm->cpu, (i8080_addr_t)(addr + i), buf[i]);
	}
}

static inline void disk_write(struct cpm80_vm *const vm)
{
	if (dma_out_of_bounds(vm->memsize, vm->dma_addr)) {
		fatal_dma(vm, vm->dma_addr);
		return;
	}

	char buf[128];
	unsigned i;
	const unsigned addr = (unsigned)vm->dma_addr;
	for (i = 0; i < 128; ++i) {
		buf[i] = (char)vm->cpu->memory_read(vm->cpu, (i8080_addr_t)(addr + i));
	}

	struct cpm80_disk_ldevice *const disk = vm->disks + vm->sel_disk;
	int err = disk->writel(disk->dev, buf, (int)vm->cpu->c);
	disk_setstat(vm->cpu, err);
}

static inline void disk_sectran(struct cpm80_vm *const vm)
{
	unsigned sector = get_bios_args_bc(vm->cpu);
	unsigned lookup_table_ptr = get_bios_args_de(vm->cpu);

	unsigned tsector;
	if (lookup_table_ptr != 0) {
		tsector = (unsigned)vm->cpu->memory_read(vm->cpu, (i8080_addr_t)(lookup_table_ptr + sector));
	} else {
		/* CP/M bug; some routines forget to check for NULL:
		 * http://cpuville.com/Code/CPM-on-a-new-computer.html
		 * Return sector as is. */
		tsector = sector;
	}

	set_bios_returnval(vm->cpu, tsector);
}

static const char BOOT_signon_fmt[] = "\r\ngithub.com/mayawarrier/intel8080-emulator\r\n%uK CP/M VERS %s\r\n\n";

int cpm80_bios_call(struct cpm80_vm *const vm, int callno)
{
	int err = 0;
	struct i8080 *const cpu = vm->cpu;

	switch (callno)
	{
		case BIOS_BOOT:
		{
			/* initialize attached devices */
			if (vm->con) serial_init(vm->con);
			if (vm->lst) serial_init(vm->lst);
			if (vm->pun) serial_init(vm->pun);
			if (vm->rdr) serial_init(vm->rdr);

			/* print signon message */
			void *fargs[] = { &vm->memsize, CPM80_BIOS_VERSION };
			vmsprintf(VM_strbuf, BOOT_signon_fmt, fargs);
			vmputs(vm->con, VM_strbuf);

			/* C must be 0 to select disk A as boot drive.
			 * CP/M 2.2 Alteration guide, pg 17:
			 * https://archive.org/details/bitsavers_digitalResationGuide1979_3864305/page/n19/mode/2up */
			cpu->c = 0;
		} 
		/* fall through */

		case BIOS_WBOOT:
		{
			/* Create wboot and bdos entry points at 0x0 and 0x5 for user code */
			cpm80_addr_t wboot_entry = vm->cpm_origin + 0x1603;
			cpu->memory_write(cpu, 0x0000, i8080_JMP);
			cpu->memory_write(cpu, 0x0001, (i8080_word_t)(wboot_entry & 0xff));
			cpu->memory_write(cpu, 0x0002, (i8080_word_t)((wboot_entry & 0xff00) >> 8));
			cpm80_addr_t bdos_entry = vm->cpm_origin + 0x0806;
			cpu->memory_write(cpu, 0x0005, i8080_JMP);
			cpu->memory_write(cpu, 0x0006, (i8080_word_t)(bdos_entry & 0xff));
			cpu->memory_write(cpu, 0x0007, (i8080_word_t)((bdos_entry & 0xff00) >> 8));

			/* jump to command processor */
			cpu->pc = (i8080_addr_t)vm->cpm_origin;
			break;
		}
			
		case BIOS_CONST:  if (vm->con) serial_stat(cpu, vm->con); break;
		case BIOS_LISTST: if (vm->lst) serial_stat(cpu, vm->lst); break;

		case BIOS_CONIN:  if (vm->con) serial_in(cpu, vm->con); break;
		case BIOS_CONOUT: if (vm->con) serial_out(cpu, vm->con); break;
		case BIOS_LIST:   if (vm->lst) serial_out(cpu, vm->lst); break;
		case BIOS_PUNCH:  if (vm->pun) serial_out(cpu, vm->pun); break;
		case BIOS_READER: if (vm->rdr) serial_in(cpu, vm->rdr); break;

		case BIOS_SELDSK:  disk_select(vm); break;
		case BIOS_HOME:    disk_home(vm); break;
		case BIOS_SETTRK:  disk_set_track(vm); break;
		case BIOS_SETSEC:  disk_set_sector(vm); break;
		case BIOS_READ:    disk_read(vm); break;
		case BIOS_WRITE:   disk_write(vm); break;
		case BIOS_SECTRAN: disk_sectran(vm); break;

		case BIOS_SETDMA: vm->dma_addr = (cpm80_addr_t)get_bios_args_bc(cpu); break;

		default: err = -1; /* unrecognized */
	}

	return err;
}

static inline void clear(struct cpm80_vm *const vm, cpm80_addr_t start, unsigned nbytes)
{
	unsigned i;
	const unsigned end = (unsigned)start + nbytes;
	for (i = (unsigned)start; i < end; ++i) {
		vm->cpu->memory_write(vm->cpu, (i8080_addr_t)i, 0x00);
	}
}

static inline void write_word(struct i8080 *const cpu, cpm80_addr_t addr, cpm80_word_t word)
{
	i8080_word_t lo = (i8080_word_t)(word & 0x00ff), hi = (i8080_word_t)((word & 0xff00) >> 8);
	cpu->memory_write(cpu, (i8080_addr_t)addr++, hi);
	cpu->memory_write(cpu, (i8080_addr_t)addr, lo);
}

/* described in vm_bios.h */
struct disk_params
{
	unsigned dn, dm, fsc, lsc, skf,
		bls, dks, dir, cks, ofs, k16;
};

struct disk_param_block
{
	/* Sectors per track, Disk size max, Directory max, Checksum size, Track offset */
	cpm80_word_t spt, dsm, drm, cks, off;
	/* Block shift, Block mask, Extent mask, Allocation bitmap bytes 0 & 1 */
	cpm80_byte_t bsh, blm, exm, al0, al1;
};

struct disk_header
{
	/* Addresses of: sector translate table, directory buffer,
	 * disk parameter block, checksum vector, allocation vector */
	cpm80_addr_t xlt, dirbuf, dpb, csv, alv;
};

/* Disks that depend on/refer to previous or yet to be defined disks */
struct disk_reference
{
	/* Refers to disk? */
	int to;
	/* allocation vector size &
	 * checksum vector size of referred disk */
	unsigned als, css;
};

static void get_disk_param_block(struct disk_params *params, struct disk_param_block *param_block)
{
	/* end - start + 1 */
	param_block->spt = (cpm80_word_t)(params->lsc - params->fsc + 1);

	param_block->dsm = (cpm80_word_t)(params->dks - 1);
	param_block->drm = (cpm80_word_t)(params->dir - 1);
	/* 2 checksum bits per directory entry */
	param_block->cks = (cpm80_word_t)(params->cks / 4);
	param_block->off = (cpm80_word_t)params->ofs;

	int i;
	/* blk_shift is the # of bits to shift left a block number to get its first sector
	 * blk_mask masks a sector number to get its offset into the block it resides in
	 * For eg. blk_shift=3, blk_mask=0x07 implies 8 * 128 = 1K blocks
	 *         blk_shift=4, blk_mask=0x0f implies 16 * 128 = 2K blocks
	 *         blk_shift=5, blk_mask=0x1f implies 32 * 128 = 4K blocks
	 *         and so on. 
	 */
	unsigned blk_shift = 0, blk_mask = 0;
	unsigned sectors_per_block = params->bls / 128;
	for (i = 0; i < 16; ++i) {
		if (sectors_per_block == 1) break;
		blk_shift += 1;
		blk_mask = (blk_mask << 1) | 1;
		sectors_per_block >>= 1;
	}

	param_block->bsh = (cpm80_byte_t)blk_shift;
	param_block->blm = (cpm80_byte_t)blk_mask;

	/* Files consist of one or more logical "extents," each of which span 128 sectors (16K).
	 * A directory/file entry can map up to 16 blocks (i.e. 2K blocks => 2 extents/entry, 
	 * 3K blocks => 3 extents/entry and so on). extent_mask limits an extent index to the 
	 * maximum per entry (eg. 0-2 for 3K blocks). */
	unsigned extent_mask = 0;
	if (params->k16 != 0) {
		unsigned kilobytes_per_block = params->bls / 1024;
		for (i = 0; i < 16; ++i) {
			if (kilobytes_per_block == 1) break;
			extent_mask = (extent_mask << 1) | 1;
			kilobytes_per_block >>= 1;
		}
		/* The entry allocation map is 16 bytes - each byte is the address of a block. 
		 * For more than 256 blocks a disk, we need two bytes per address => half the
		 * blocks => half the extents. */
		if (params->dks > 256) extent_mask >>= 1;
	}

	param_block->exm = (cpm80_byte_t)extent_mask;

	/* Generate the directory allocation bitmap.
	 * Bitmap of blocks allocated to the disk directory. */
	unsigned dir_alloc = 0;
	unsigned dir_remaining = params->dir;
	unsigned dir_block_size = params->bls / 32;
	for (i = 0; i < 16; ++i) {
		if (dir_remaining == 0) break;
		dir_alloc = (dir_alloc >> 1) | 0x8000; /* fills from the left */
		if (dir_remaining > dir_block_size) dir_remaining -= dir_block_size;
		else dir_remaining = 0;
	}

	param_block->al0 = (cpm80_byte_t)(dir_alloc >> 8);
	param_block->al1 = (cpm80_byte_t)(dir_alloc & 0xff);
}

/* order: spt, bsh, blm, exm, dsm, drm, al0, al1, cks, off. */
static void write_disk_param_block(struct cpm80_vm *const vm, struct disk_param_block *param_block, cpm80_addr_t dest)
{
	write_word(vm->cpu, dest, param_block->spt); dest += 2;
	vm->cpu->memory_write(vm->cpu, dest, param_block->bsh); dest += 1;
	vm->cpu->memory_write(vm->cpu, dest, param_block->blm); dest += 1;
	vm->cpu->memory_write(vm->cpu, dest, param_block->exm); dest += 1;
	write_word(vm->cpu, dest, param_block->dsm); dest += 2;
	write_word(vm->cpu, dest, param_block->drm); dest += 2;
	vm->cpu->memory_write(vm->cpu, dest, param_block->al0); dest += 1;
	vm->cpu->memory_write(vm->cpu, dest, param_block->al1); dest += 1;
	write_word(vm->cpu, dest, param_block->cks); dest += 2;
	write_word(vm->cpu, dest, param_block->off);
}

/* Euclid's greatest common divisor algorithm */
static unsigned gcd(unsigned a, unsigned b)
{
	unsigned rem;
	while ((rem = b % a) > 0) {
		b = a;
		a = rem;
	}
	return a;
}

/* The sector translate table maps logical sectors to physical sectors.
 * For faster disk access, physical sectors that map a file's logical sectors 
 * may be skewed over the disk so the disk seek can reach them quicker as the disk spins.
 * for eg. a skew factor of 2 generates the mapping 0->0, 1->2, 2->4, 3->6... etc,
 * wrapping around at the end of the track ...127->254, 128->1, 129->3 and so on.
 * Returns size of table. */
static unsigned
write_sector_translate_table(struct cpm80_vm *const vm, struct disk_params *params, struct disk_param_block *param_block, cpm80_addr_t buf_ptr)
{
	unsigned skew = params->skf;
	unsigned sectors = param_block->spt;
	unsigned sector_offset = params->fsc;

	/* # of mappings to generate before overlapping already mapped sectors.
	 * for eg. with spt=72 and skf=10:
	 * 0,10,20,...70,8,18,...68,6,16,...66,4,14,...64,2,12,...62,0,10...
	 *                                                          ^^^
	 * an overlap occurs every 5 rotations (every 36 mapped sectors)
	 */
	const unsigned num_before_overlap = sectors / gcd(sectors, skew);

	/* sector to start mapping at every rotation */
	unsigned next_base = 0;

	unsigned i;
	unsigned next_sector = next_base, num_assigned = 0;
	for (i = 0; i < sectors; ++i) {
		unsigned sector_alloc = next_sector + sector_offset;

		if (sectors < 256) {
			vm->cpu->memory_write(vm->cpu, buf_ptr, (i8080_word_t)(sector_alloc & 0xff));
			buf_ptr += 1;
		} else {
			/* need two byte address */
			write_word(vm->cpu, buf_ptr, (cpm80_word_t)sector_alloc);
			buf_ptr += 2;
		}

		next_sector += skew;
		if (next_sector >= sectors) next_sector -= sectors;
		num_assigned++;

		if (num_assigned == num_before_overlap) {
			next_base += 1;
			next_sector = next_base;
			num_assigned = 0;
		}
	}

	return sectors;
}

/* order: xlt, 0, 0, 0, dirbuf, dpb, csv, alv */
static void write_disk_header(struct cpm80_vm *const vm, struct disk_header *header, cpm80_addr_t dest)
{
	write_word(vm->cpu, dest, (cpm80_word_t)header->xlt); dest += 2;
	write_word(vm->cpu, dest, 0x0000); dest += 2;
	write_word(vm->cpu, dest, 0x0000); dest += 2; /* 3 blank words used as a workspace by CP/M */
	write_word(vm->cpu, dest, 0x0000); dest += 2;
	write_word(vm->cpu, dest, (cpm80_word_t)header->dirbuf); dest += 2;
	write_word(vm->cpu, dest, (cpm80_word_t)header->dpb); dest += 2;
	write_word(vm->cpu, dest, (cpm80_word_t)header->csv); dest += 2;
	write_word(vm->cpu, dest, (cpm80_word_t)header->alv);
}

/* Returns 0 if parameter list if complete i.e. does not depend on another disk, 1 if it does, and -1 for a invalid list. */
static int extract_disk_params(const unsigned *param_list, const unsigned num_params, struct disk_params *params)
{
	if (num_params == 10) {
		params->dn = param_list[1];
		params->fsc = param_list[2];
		params->lsc = param_list[3];
		params->skf = param_list[4];
		params->bls = param_list[5];
		params->dks = param_list[6];
		params->dir = param_list[7];
		params->cks = param_list[8];
		params->ofs = param_list[9];
		params->k16 = param_list[10];
		return 0;
	} else if (num_params == 2) {
		params->dn = param_list[1];
		params->dm = param_list[2];
		return 1;
	}
	return -1;
}

#define DIRBUF_LEN (128)
#define DPB_LEN (15)
/* Default (minimum) length of disk header. Adjust if necessary. */
#define DPH_LEN (16)

int cpm80_bios_define_disks(struct cpm80_vm *const vm, const int ndisks,
	const unsigned *const *disk_params, cpm80_addr_t disk_header_ptr, cpm80_addr_t disk_ram_ptr, cpm80_addr_t *disk_dph_out)
{
	/* all disks use the same dirbuf */
	const cpm80_addr_t dirbuf_all = disk_ram_ptr;
	clear(vm, disk_ram_ptr, DIRBUF_LEN);
	disk_ram_ptr += DIRBUF_LEN;

	struct disk_header headers[CPM80_MAX_DISKS];
	
	int i;
	/* some disks may depend on others and share parameter blocks with them */
	struct disk_reference refers[CPM80_MAX_DISKS];
	for (i = 0; i < ndisks; ++i) refers[i].to = -1;

	cpm80_addr_t disk_param_block_ptr = disk_header_ptr + ndisks * DPH_LEN;

	struct disk_params params;
	struct disk_param_block param_block;
	for (i = 0; i < ndisks; ++i) {
		const unsigned *param_list = disk_params[i];
		const unsigned num_params = param_list[0];

		int does_depend = extract_disk_params(param_list, num_params, &params);
		if (does_depend == -1) return -1;

		headers[i].dirbuf = dirbuf_all;

		if (does_depend) {
			/* disk dn depends on disk dm */
			refers[params.dn].to = (int)params.dm;
			/* but dm may not be defined yet, so process dn later */
			continue;
		}

		/* compute disk allocation & checksum vector sizes */
		unsigned css = params.cks / 4;
		unsigned als = params.dks / 8;
		if (als % 8 != 0) als++; /* ciel */
		unsigned vtotal = als + css;
		refers[i].als = als;
		refers[i].css = css;

		/* clear space for vectors */
		headers[i].alv = disk_ram_ptr;
		headers[i].csv = disk_ram_ptr + (cpm80_addr_t)als;
		clear(vm, disk_ram_ptr, vtotal);
		disk_ram_ptr += (cpm80_addr_t)vtotal;

		/* write disk parameter block */
		headers[i].dpb = disk_param_block_ptr;
		get_disk_param_block(&params, &param_block);
		write_disk_param_block(vm, &param_block, disk_param_block_ptr);
		disk_param_block_ptr += DPB_LEN;

		/* write translate table */
		if (params.skf != 0) {
			headers[i].xlt = disk_param_block_ptr;
			unsigned xlt_size = write_sector_translate_table(vm, &params, &param_block, disk_param_block_ptr);
			disk_param_block_ptr += (cpm80_addr_t)xlt_size;
		} else {
			/* no skew. table is NULL */
			headers[i].xlt = 0x0000;
		}
	}

	/* process reference definitions */
	for (i = 0; i < ndisks; ++i) {
		int dn = i;
		int dm = refers[i].to;
		if (dm != -1) {
			/* use same param block and translate table */
			headers[dn].dpb = headers[dm].dpb;
			headers[dn].xlt = headers[dm].xlt;
			/* clear fresh space, each disk gets its own vectors */
			unsigned als = refers[dm].als, css = refers[dm].css;
			unsigned vtotal = als + css;
			headers[i].alv = disk_ram_ptr;
			headers[i].csv = disk_ram_ptr + (cpm80_addr_t)als;
			clear(vm, disk_ram_ptr, vtotal);
			disk_ram_ptr += (cpm80_addr_t)vtotal;
		}
	}

	/* write out disk headers */
	for (i = 0; i < ndisks; ++i) {
		disk_dph_out[i] = disk_header_ptr;
		write_disk_header(vm, headers + i, disk_header_ptr);
		disk_header_ptr += DPH_LEN;
	}

	return 0;
}
