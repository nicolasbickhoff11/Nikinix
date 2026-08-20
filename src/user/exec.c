#include "elf.h"
#include "vfs/vfs.h"
#include "print.h"
#include "map.h"
#include <stdint.h>

#define DivRoundUp(x, y) (((x) + (y) - 1) / (y))

uint64_t load_elf(uint64_t *user_cr3, const char *path) {
    if (!user_cr3 || !path) return -1;
    vnode_t *file = NULL;
    int r = vfs_lookup_path(vfs_root, path, &file);
    if (r < 0) return -2;
    if (!S_ISREG(file->mode)) return -3;
    mode_t mode = file->mode & 0777;
    if (!(mode & 0111)) return -4;
    Elf64_Ehdr ehdr = {0};
    r = vfs_read(file, &ehdr, sizeof(ehdr), 0);
    if (r < 0) return -5;
    if (ehdr.e_ident[0] != 0x7F ||
        ehdr.e_ident[1] != 'E' || 
        ehdr.e_ident[2] != 'L' ||
        ehdr.e_ident[3] != 'F')
        return -6;

    if (ehdr.e_ident[EI_CLASS] != ELFCLASS64 ||
        ehdr.e_ident[EI_DATA] != ELFDATA2LSB ||
        ehdr.e_ident[EI_VERSION] != EV_CURRENT ||
        ehdr.e_ident[EI_OSABI] != ELFOSABI_SYSV)
        return -7;

    if (ehdr.e_type != ET_EXEC ||
        ehdr.e_machine != EM_X86_64 ||
        ehdr.e_version != EV_CURRENT)
        return -8;
    
    Elf64_Phdr *p = phys_to_virt(malloc(ehdr.e_phentsize * ehdr.e_phnum));
    if (!p) return -9;

    r = vfs_read(file, p, ehdr.e_phentsize * ehdr.e_phnum, ehdr.e_phoff);
    if (r < 0) return -10;

    Elf64_Phdr *entry = p;
    for (uint16_t i = 0; i < ehdr.e_phnum; i++) {
        if (entry->p_type != PT_LOAD) continue;
        uint64_t flags = 0;
        if (entry->p_flags & PF_R) flags |= PAGE_PRESENT;
        if (entry->p_flags & PF_W) flags |= PAGE_RW;
        if (!(entry->p_flags & PF_X)) flags |= PAGE_NX;
        flags |= PAGE_USER;
        uint64_t base = bitmap_alloc(DivRoundUp(entry->p_memsz, 4096));
        r = vfs_read(file, phys_to_virt(base), entry->p_filesz, entry->p_offset);
        if (r < 0) return -12;
        r = map_range_generic(user_cr3, entry->p_vaddr, base, flags, entry->p_memsz);
        if (r < 0) return -13;
        memset(phys_to_virt(base) + entry->p_filesz, 0x00, entry->p_memsz - entry->p_filesz);
        entry++;
    }

    free(p);
    return ehdr.e_entry;
}