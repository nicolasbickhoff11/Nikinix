/*
    coded by Danoni631
    Crash display
*/

#include <stdint.h>
#include "print.h"
#include "panic.h"

void panic_screen(const char* error)
{
    clear_video();
    puts_fd1("Nikinix has encountered a critical error and was powered off to avoid damaging the machine.\n\n");
    puts_fd1("If this is the first time that you see this screen, restart the machine.\n");
    puts_fd1("Else if the problems persists, contact the creators via GitHub.\n\n");
    puts_fd1("Nicolas's Github: nicolasbickhoff11.\nDanilo's Github: Danoni631\n\n");
    
    puts_fd1
    (
      "Possible causes:\n - An unexpected event disrupted kernel execution.\n - A critical system component failed to respond.\n - Illegal instructions or unauthorized memory access occurred.\n\n"
    );

    puts_fd1("Error code:\n");
    puts_fd1(error);
}
