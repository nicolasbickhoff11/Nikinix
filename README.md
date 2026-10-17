Nikinix is a feature-rich hobby OS in progress, they have some bugs and failures.

## Current State
- FBCON (1024x768x32) ✅
- Limine Boot ✅
- Initrd ✅
- RamFS ✅
- VFS ✅
- ELF Loader ✅
- ACPI ✅
- Reboot/Shutdown via ACPI ✅
- APIC ✅
- Bitmap Allocator ✅
- VMM ✅
- Userspace ✅
- TAR initrd format ✅
- LAPIC Timer ✅
- TSC ✅
- Type 1 and 0 SMBIOS ✅
- PCIe ✅
- MSR ✅
- IDT ✅
- syscalls ✅
- list dirs ✅
- unix-style errno ✅
- shell ✅
- syscall instruction ✅
- OPEN, and File Operations syscalls 🚧
- Scheduler 🚧
- Fork and Process Operations syscalls ❌
- SMAP / SMEP ❌

## TODO
🚧 switch to copy_to_user and write_to_user
🚧 SMAP / SMEP

## How to Compile
run make and make iso

## How to Run
use the prebuilt nikinix iso in release or use your builded nikinix iso in QEMU, Vbox, etc

but for run imediatelly run: make run

warning: this OS needs PCIe support and ACPI and (I/O)APIC and LAPIC support

## License
This project is licensed under the GNU General Public License v3.0 – see the [LICENSE](LICENSE) file for details.
