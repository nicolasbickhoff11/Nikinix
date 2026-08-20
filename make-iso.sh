#!/bin/bash

curl -L https://github.com/Limine-Bootloader/Limine/releases/latest/download/limine-binary.tar.gz | gunzip | tar -xf -

# Build "limine" utility.
make -C limine-binary

# Create a directory which will be our ISO root.
mkdir -p iso_root

# Copy the relevant files over.
mkdir -p iso_root/boot
cp -v bin/nikinix iso_root/boot/
tar -cf initrd.tar -C initrd .
cp -v initrd.tar iso_root/boot/
mkdir -p iso_root/boot/limine
cp -v limine.conf limine-binary/limine-bios.sys limine-binary/limine-bios-cd.bin \
      limine-binary/limine-uefi-cd.bin iso_root/boot/limine/

# Create the EFI boot tree and copy Limine's EFI executables over.
mkdir -p iso_root/EFI/BOOT
cp -v limine-binary/BOOTX64.EFI iso_root/EFI/BOOT/
cp -v limine-binary/BOOTIA32.EFI iso_root/EFI/BOOT/

# Create the bootable ISO.
xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
        -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        iso_root -o nikinix.iso

# Install Limine stage 1 and 2 for legacy BIOS boot.
./limine-binary/limine bios-install nikinix.iso
