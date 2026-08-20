#include <stdint.h>
#include <stddef.h>

#include "user/cr3.h"
#include "user/exec.h"
#include "map.h"
#include "string.h"
#include "pmm.h"
#include "print.h"
#include "msr.h"


/*uint8_t code[] = {
    // sys_write(1, "root@nikinix:~#", 15)

    // mov eax, 0
    0xB8, 0x00, 0x00, 0x00, 0x00,

    // mov edi, 1
    0xBF, 0x01, 0x00, 0x00, 0x00,

    // lea rsi, [rip + 33]
    0x48, 0x8D, 0x35, 0x21, 0x00, 0x00, 0x00,

    // mov edx, 15
    0xBA, 0x0F, 0x00, 0x00, 0x00,

    // syscall
    //0x0F, 0x05,
    0xCD, 0x80,

    // sys_write(1, "root@nikinix:~#", 15)

    // mov eax, 0
    0xB8, 0x00, 0x00, 0x00, 0x00,

    // mov edi, 1
    0xBF, 0x01, 0x00, 0x00, 0x00,

    // lea rsi, [rip + 9]
    0x48, 0x8D, 0x35, 0x09, 0x00, 0x00, 0x00,

    // mov edx, 15
    0xBA, 0x0F, 0x00, 0x00, 0x00,

    //0xCC,
    // syscall
    //0x0F, 0x05,
    0xCD, 0x80,

    // Loop infinito
    0xEB, 0xFE,

    // Dados
    'r','o','o','t','@','n','i','k','i','n','i','x',
    ':','~','#','\0'
};*/


uint64_t *current_user_cr3 = 0;


void init_userspace()
{
    //kprintf("A\n");


    current_user_cr3 = setup_user_cr3();


    //kprintf("B\n");




    /*
     * =========================
     * 2. USER STACK PAGE
     * =========================
     */

    uint64_t stack_phys = bitmap_alloc(1);


    /*
     * Página da stack:
     *
     * 0x700000 - 0x700fff
     *
     * RSP começa em:
     *
     * 0x701000
     */

    uint64_t stack_virt = 0x00007fffffffe000;

    uint64_t stack_top = stack_virt + 4096;

    //kprintf("BUG?\n");

    map_generic(
        current_user_cr3,
        stack_virt,
        stack_phys,
        PAGE_PRESENT |
        PAGE_RW |
        PAGE_USER
    );


    //kprintf("D\n");


    /*
     * =========================
     * 3. ENTER USERSPACE
     * =========================
     *
     * SYSCALL/SYSRET setup:
     *
     * RCX = user RIP
     * R11 = user RFLAGS
     *
     * RSP precisa ser configurado
     * antes do sysretq.
     */

    uint64_t cr3_phys =
        virt_to_phys(current_user_cr3);

    /*uint64_t efer = rdmsr(0xC0000080);

    kprintf("EFER: %lx\n", efer);*/

    uint64_t rip = load_elf(current_user_cr3, "/init");
    kprintf("RIP: 0x%lx\n", rip);
    kprintf("LOADING init (\"/init\")!\n");


    asm volatile(
        "cli\n"

        "mov %0, %%cr3\n"

        "push $0x23\n"
	"push %1\n"
	"push $0x202\n"
	"push $0x1B\n"
	"push %2\n"

        "iretq\n"

        :
        :
        "r"(cr3_phys),
        "r"(stack_top),
        "r"(rip)

        :
        "rcx",
        "r11",
        "memory"
    );


    /*
     * nunca retorna
     */
}
