#ifndef _ACPI_SHUTDOWN_H
#define _ACPI_SHUTDOWN_H 1

extern uint8_t slp_typa;
extern uint8_t slp_typb;

int find_shutdown_values();
void acpi_shutdown();

#endif