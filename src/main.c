#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "print.h"
#include "pmm.h"
#include "map.h"
#include "pic.h"
#include "idt.h"
#include "io.h"
#include "gdt.h"
#include "pit.h"
#include "cpu.h"
#include "rsdp_addr.h"
#include "string.h"
#include "rtc.h"
#include "msr.h"
#include "logo.h"
#include "acpi/reboot.h"
#include "acpi/shutdown.h"
#include "smbios.h"
#include "tss.h"
#include "tsc.h"
#include "acpi/acpi.h"
#include "acpi/fadt.h"
#include "acpi/pcie.h"
#include "syscall/enable.h"
#include "user/init.h"
#include "acpi/apic.h"
#include "malloc.h"
#include "vfs/vfs.h"

uint64_t ram_size_usable = 0;

char buf[8192];

extern void isr_keyboard();
extern void isr_timer();
extern void isr_page_fault();
extern void isr_general_fault();
extern void isr_ud();
extern void syscall_entry();
extern void isr_breakpoint();
extern void isr_double_fault();

uint64_t framebuffer        = 0;
uint32_t framebuffer_height = 0;
uint32_t framebuffer_width  = 0;
uint32_t framebuffer_pitch  = 0;
uint8_t  framebuffer_bpp    = 0;

uint64_t hhdm_offset = 0;

mem_t mem_avaliable[8192] = { 0 };
uint64_t mem_count = 0;

void play_sound(uint32_t freq) {
    uint32_t div = 1193180 / freq;

    outb(0x43, 0xB6);

    outb(0x42, div & 0xFF);
    outb(0x42, (div >> 8) & 0xFF);

    outb(0x61, inb(0x61) | 3);
}

void sleep(int n) {
    for (volatile int i = 0; i < n * 100000; i++);
}

void stop_sound() {
    outb(0x61, inb(0x61) & 0xFC);
}

// Set the base revision to 6, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_smbios_request smbios_request = {
    .id = LIMINE_SMBIOS_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_tsc_frequency_request tsc_frequency_request = {
    .id = LIMINE_TSC_FREQUENCY_REQUEST_ID,
    .revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

LIMINE_PTR(struct limine_memmap_response *) memmap_ptr;

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.

uint64_t stack = 0;

__attribute__((naked))
void reload(void) {
    asm volatile (
	"mov $0x10, %%ax\n"
	"mov %%ax, %%ds\n"
	"mov %%ax, %%es\n"
	"mov %%ax, %%fs\n"
	"mov %%ax, %%gs\n"
	"mov %%ax, %%ss\n"
	"mov %0, %%rsp\n"
	"call kmain\n"
	:
	: "r"(stack)
    );
}

void _start(void) {
    asm volatile ("mov %%rsp, %0" : "=r"(stack));
    setup_gdt();
    asm volatile ("lgdt %0" :: "m"(gdtr));
    asm volatile (
        "pushq $0x08\n"
	"lea reload(%%rip), %%rax\n"
	"pushq %%rax\n"
	"lretq\n"
	:
	:
	: "rax"
    );
}

uint64_t ram_total = 0;
extern void tar_list(void *initrd, uint64_t initrd_size);

#include <stdint.h>

static inline uint64_t read_cr0(void)
{
    uint64_t value;
    asm volatile ("mov %%cr0, %0" : "=r"(value));
    return value;
}

static inline void write_cr0(uint64_t value)
{
    asm volatile ("mov %0, %%cr0" :: "r"(value) : "memory");
}

static inline uint64_t read_cr4(void)
{
    uint64_t value;
    asm volatile ("mov %%cr4, %0" : "=r"(value));
    return value;
}

static inline void write_cr4(uint64_t value)
{
    asm volatile ("mov %0, %%cr4" :: "r"(value) : "memory");
}

void sse2_init(void)
{
    uint64_t cr0 = read_cr0();

    cr0 &= ~(1ULL << 2);  // EM = 0
    cr0 |=  (1ULL << 1);  // MP = 1

    write_cr0(cr0);

    uint64_t cr4 = read_cr4();

    cr4 |= (1ULL << 9);   // OSFXSR
    cr4 |= (1ULL << 10);  // OSXMMEXCPT

    write_cr4(cr4);
}

void kmain(void) {

    sse2_init();
    uint64_t first_tsc = rdtsc();
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    struct limine_framebuffer *fb_struct = framebuffer_request.response->framebuffers[0];
    struct limine_hhdm_response *hhdm = hhdm_request.response;
    struct limine_memmap_response *memmap = memmap_request.response;
    struct limine_rsdp_response *rsdp = rsdp_request.response;
    struct limine_smbios_response *smbios = smbios_request.response;
    struct limine_module_response *modules = module_request.response;
    struct limine_tsc_frequency_response *tsc = tsc_frequency_request.response;

    memmap_ptr = memmap;

    struct limine_file *initrd = modules->modules[0];

    entry_32 = (uint64_t)smbios->entry_32;
    entry_64 = (uint64_t)smbios->entry_64;

    framebuffer        = (uint64_t)fb_struct->address;
    framebuffer_height = (uint32_t)fb_struct->height;
    framebuffer_width  = (uint32_t)fb_struct->width;
    framebuffer_pitch  = (uint32_t)fb_struct->pitch;
    framebuffer_bpp    = (uint8_t)fb_struct->bpp;

    hhdm_offset = hhdm->offset;

    rsdp_phys_address = (uint64_t)virt_to_phys(rsdp->address);

    uint64_t entry_count = memmap->entry_count;

    volatile uint32_t *fb_ptr = (volatile uint32_t*)framebuffer;

    uint64_t c = 0;
    for (uint64_t i = 0; i < entry_count; i++) {
         if (c >= 8192) break;
         struct limine_memmap_entry *entry = memmap->entries[i];
         if (entry->type == LIMINE_MEMMAP_USABLE) {
             mem_avaliable[c].base = entry->base;
             mem_avaliable[c].length = entry->length;
             c++;
         }
    }
    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        ram_total += entry->length;
    }

    mem_count = c;

    mem_from_alloc = (mem_avaliable[0].base + 4095) & ~4095;
    mem_end        = ((mem_from_alloc + mem_avaliable[0].length) + 4095) & ~4095;

    for (size_t y = 0; y < framebuffer_height; y++) {
        for (size_t x = 0; x < framebuffer_width; x++) {
            fb_ptr[y * (framebuffer_pitch / 4) + x] = 0x00000000;
        }
    }

    uint64_t pml4_phys = 0;

    asm volatile ("mov %%cr3, %0" : "=r"(pml4_phys));
    pml4 = (uint64_t*)(hhdm_offset + pml4_phys);

    init_serial();
    //PitInit();

    kprintf("Hello from Nikinix!\n");
    kprintf("[LOG] Remapping PIC....\n");
    pic_remap();
    kprintf("[LOG] Setting PIC....\n");
    clear_irq_mask(0);
    clear_irq_mask(1);
    clear_irq_mask(2);
    kprintf("[LOG] Setting IDT....\n");
    set_idt_entry((uint64_t)isr_keyboard, 0x21, 0x08, 0x00, 0x8E);
    set_idt_entry((uint64_t)isr_timer, 0x20, 0x08, 0x00, 0x8E);
    set_idt_entry((uint64_t)isr_page_fault, 0x0E, 0x08, 0x00, 0x8E);
    set_idt_entry((uint64_t)isr_general_fault, 0x0D, 0x08, 0x00, 0x8E);
    set_idt_entry((uint64_t)isr_ud, 0x06, 0x08, 0x00, 0x8E);
    set_idt_entry((uint64_t)syscall_entry, 0x80, 0x08, 0x00, 0xEE);
    set_idt_entry((uint64_t)isr_breakpoint, 0x03, 0x08, 0x00, 0xEF);
    set_idt_entry((uint64_t)isr_double_fault, 0x08, 0x08, 0x00, 0x8E);

    kprintf("[LOG] Loading IDT....\n");
    load_idt();

    asm volatile ("sti");
    kprintf("[LOG] Framebuffer address: 0x%lx\n", framebuffer);
    kprintf("[LOG] Framebuffer height: %d\n", framebuffer_height);
    kprintf("[LOG] Framebuffer width: %d\n", framebuffer_width);
    kprintf("[LOG] Framebuffer pitch: %d\n", framebuffer_pitch);
    kprintf("[LOG] Framebuffer resoluction: %dx%dx%d\n", framebuffer_width, framebuffer_height, framebuffer_bpp);
    kprintf("[LOG] HHDM offset: 0x%lx\n", hhdm_offset);
    kprintf("[LOG] RSDP PHYS Address: 0x%lx\n", rsdp_phys_address);

    kprintf("[LOG] Initrd Address: 0x%lx\n", initrd->address);
    
    kprintf("[LOG] TSC Frequency: %lld\n", tsc->frequency);

    uint64_t apic_base = rdmsr(0x1B);
    apic_base |= (1 << 11);
    wrmsr(0x1B, apic_base);
    apic_base = rdmsr(0x1B);

    lapic_address = apic_base & 0xFFFFF000;
    lapic_virt = phys_to_virt(lapic_address);
    kprintf("[LOG] LAPIC PHYS Address: 0x%lx\n", lapic_address);

    kprintf("2 Seconds...\n");

    uint64_t current_tsc = rdtsc();
    uint64_t future_tsc = current_tsc + (2 * tsc->frequency);

    while (rdtsc() < future_tsc) {
        asm volatile("pause");
    }
    
    kprintf("OK!\n");

    char buf[49];
    cpu_name(buf);
    kprintf("[LOG] CPU Name: %s\n", buf);

    smbios_init();
    kprintf("[LOG] Manufacturer: %s\n[LOG] Product Name: %s\n[LOG] Version: %s\n", info.manufacturer, info.productname, info.version);
    if (info.serialnumber) {
        kprintf("[LOG] Serial Number: %s\n", info.serialnumber);
    }
    //if (memcmp(info.productname, "VirtualBox", 10) == 0 || memcmp(info.manufacturer, "VMware, Inc.", 12) == 0 || memcmp(info.manufacturer, "QEMU", 4) == 0) {
    //    kprintf("[TROLL] KKKK Voce so usa vm porque voce e um pobretao kkkkk e nao tem dinheiro.\n");
    //}
    kprintf("[LOG] BIOS Vendor: %s\n[LOG] BIOS Version: %s\n[LOG] BIOS Release Date: %s\n", info.bios_vendor, info.bios_version, info.bios_release_date);

    pmm_init();
    init_acpi();
    setup_tss();
    heap = (mem_avaliable[1].base + 4095) & ~4095;
    heap_end = (mem_avaliable[1].base + mem_avaliable[0].length + 4095) & ~4095;

    kprintf("[LOG] Testing Bitmap Allocator...\n");
    uint64_t page = bitmap_alloc(1);
    kprintf("[LOG] PHYS test PAGE: 0x%lx\n", page);
    kprintf("[LOG] Trying to freeing this...\n");
    kprintf("[LOG] Return: %d\n", bitmap_free(page, 1));
    kprintf("[LOG] Testing Double Free Protection...\n");
    kprintf("[LOG] Return: %d\n", bitmap_free(page, 1));
    kprintf("[LOG] Testing Reserved Page/Frame Protection (BITMAP BUFFER PAGE)...\n");
    kprintf("[LOG] Return: %d\n", bitmap_free(bitmap.bitmap_phys, 1));
    kprintf("[LOG] Testing Reserved Page/Frame Protection (BITMAP BUFFER LASTPAGE)...\n");
    kprintf("[LOG] Return: %d\n", bitmap_free(bitmap.bitmap_phys + bitmap.bitmapAllocSizeinBytes - 4095, 1));

    kprintf("[LOG] Kernel BITMAP Struct PHYS: 0x%lx\n", get_phys_generic(pml4, &bitmap));

    kprintf("[LOG] Testing Malloc...\n");
    void *p = malloc(256);
    kprintf("[LOG] Addr: %p\n", p);
    kprintf("[LOG] Trying to free...\n");
    free(p);
    kprintf("[LOG] OK!\n");

    kprintf("[LOG] Starting VFS and RamFS\n");
    vfs_init();
    mount_t *mnt = phys_to_virt(create_ramfs());
    vfs_mount("/", mnt);
    vnode_t *root = mnt->root;
    kprintf("[LOG] Testing RamFS and VFS layer creating a file!\n");
    vnode_t *file = NULL;

tar_list(initrd->address, initrd->size);

vfs_lookup_path(root, "test.txt", &file);

char other_buffer[256];

ssize_t n = vfs_read(file, other_buffer, sizeof(other_buffer) - 1, 0);
if (n < 0) kprintf("[LOG] error in read of file /test.txt: ret: %d!\n", n);

if (n >= 0)
    other_buffer[n] = '\0';

kprintf("[LOG] Content of /test.txt: %s\n", other_buffer);

    asm volatile ("ltr %0" :: "r"(0x28));

    map_range_generic(pml4, lapic_virt, lapic_address, PAGE_PRESENT | PAGE_RW, 4096);
    map_range_generic(pml4, io_apic_virt, io_apic_address, PAGE_PRESENT | PAGE_RW, 4096);
    uint32_t id = lapic_read(0x020);

    kprintf("[LOG] LAPIC ID: %u\n", id);
    tsc_frequency = tsc->frequency;
    calibrate_lapic_timer(10);

    disable_pic();

    uint64_t entry = 0;

// Vetor da IDT
entry |= 0x21ULL;

// Delivery Mode = Fixed (bits 8-10 = 000)
// Destination Mode = Physical (bit 11 = 0)

// Polaridade padrão: Active High (bit 13 = 0)
// Trigger padrão: Edge (bit 15 = 0)

// Mask = 0 (habilitado, bit 16 = 0)

// Destino: LAPIC ID
uint32_t lapic_id_reg = lapic_read(0x020);
uint8_t apic_id = (lapic_id_reg >> 24) & 0xFF;

// Para xAPIC físico
entry |= ((uint64_t)apic_id << 56);

// Aplicar overrides do MADT somente se existirem
if (irq1_polarity == 3) {
    entry |= (1ULL << 13); // Active Low
}

if (irq1_trigger == 3) {
    entry |= (1ULL << 15); // Level Trigger
}

kprintf("IOAPIC entry: %llx\n", entry);

    //ioapic_write(0x12, 0x00000021); // LOW da IRQ/GSI 1
//ioapic_write(0x13, 0x00000000); // HIGH

//uint32_t low = ioapic_read(0x12);
//uint32_t high = ioapic_read(0x13);

//kprintf("LOW=%x HIGH=%x\n", low, high);

    kprintf("IOAPIC ID: 0x%x\n", ioapic_read(0));
    kprintf("IOAPIC VER: 0x%x\n", ioapic_read(1));
    ioapic_write(0x10 + gsi_keyboard_ps2 * 2, entry); // LOW da IRQ/GSI 1
    ioapic_write(0x11 + gsi_keyboard_ps2 * 2, entry >> 32); // HIGH
    ioapic_write_redir(gsi_keyboard_ps2, entry);

    uint64_t redir = ioapic_read_redir(gsi_keyboard_ps2);
    kprintf("IOREDTBL[1]=%llx\n", redir);

    lapic_write(0xF0, lapic_read(0xF0) | 0x100);

    struct FADT *fadt = get_fadt();
    uint8_t century_reg = fadt->Century;

    if (!century_reg) {
        century_reg = 0x32; // default
        kprintf("[RTC] ACPI did not provide century register, using CMOS 0x32 fallback\n");
    }

    kprintf("[LOG] Century register is 0x%x\n", century_reg);

    uint8_t regB = read_cmos_register(0x0B);
    uint8_t is_bcd = !(regB & 0x04);

    struct rtc_time rtc = {0};
    century = read_cmos_register(century_reg);

    if (is_bcd) {
        century = bcd_to_bin(century);
    }

    //read_rtc(&rtc);
    //kprintf("[LOG] time: %d/%d/%d %02d:%02d:%02d\n", rtc.day, rtc.month, rtc.year, rtc.hour, rtc.min, rtc.sec);
    kprintf("[LOG] Finding Shutdown Values in ACPI....\n");

    find_shutdown_values();

    kprintf("[LOG] Activating syscall instruction....\n");
    setup_syscall();

    kprintf("[LOG] Listing PCIe Tree\n");

    read_pcie_addr();

    pci_scan_bus(get_pcie(), 0);

    check_24_hour();

    current_tsc = rdtsc();

    uint64_t ms = ((current_tsc - first_tsc) * 1000) / tsc->frequency;

    kprintf("[LOG] Uptime: %llu ms\n", ms);

    read_rtc(&rtc);
    kprintf("[LOG] time: %d/%d/%d %02d:%02d:%02d           ", rtc.day, rtc.month, rtc.year, rtc.hour, rtc.min, rtc.sec);

    //while (1) {
    //    read_rtc(&rtc);
    //    kprintf("\r[LOG] time: %d/%d/%d %02d:%02d:%02d                 ", rtc.day, rtc.month, rtc.year, rtc.hour, rtc.min, rtc.sec);
    //}
    init_userspace();

    /*while (1)
    {
        kprintf("[LOG] Shutdowning...");
        acpi_shutdown();
    }
    kprintf("[LOG] Rebooting...");
    acpi_reboot();*/

    /*int dst_x = (framebuffer_width - 128) / 2;
    int dst_y = (framebuffer_height - 128) / 2;
    for (int y = 0; y < 128; y++) {
        memcpy(
            (uint8_t*)fb_ptr + (dst_y + y) * framebuffer_pitch + dst_x * 4,
            logo_raw + y * 128 * 4,
            128 * 4
        );
    }*/
    while (1) {
        asm volatile ("sti; hlt");
        if (!last_char)
            continue;

        if (last_char == '\b') {
            cursor_x -= 8;
            if (cursor_x < 0) cursor_x = 0;
            fb_print_char(' ');
            cursor_x -= 8;
            com1_puts("\b \b");
        } else {
            kprintf("%c", last_char);
        }
        last_char = 0;
    }

    // We're done, just hang...
    hcf();
}
