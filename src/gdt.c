#include <stdint.h>
#include "gdt.h"
#include "tss.h"

// Tamanho ajustado para 7 entradas (índices de 0 a 6). 
// O suficiente para abrigar a TSS de 16 bytes nativa de 64 bits.
struct gdt_segment    gdt[7] = { 0 };
struct gdt_descriptor gdtr = { 0 };

void setup_gdt() {
    // gdt[0] - Null Descriptor (Exigido pelo hardware)
    gdt[0].access_byte          = 0x00;
    gdt[0].limit_high_and_flags = 0x00;

    // gdt[1] - Kernel Code (Seletor 0x08)
    gdt[1].limit_low            = 0xFFFF;
    gdt[1].access_byte          = 0x9A;
    gdt[1].limit_high_and_flags = 0xAF; // Long Mode (L=1, D=0), Granularidade 4K (G=1)

    // gdt[2] - Kernel Data (Seletor 0x10)
    gdt[2].limit_low            = 0xFFFF;
    gdt[2].access_byte          = 0x92;
    gdt[2].limit_high_and_flags = 0xCF; // Granularidade 4K (G=1), Tamanho 32-bit padrão (D/B=1)

    // === SEÇÃO DE USUÁRIO CASADA COM O SYSRETQ ===
    
    // gdt[3] - User Data (Seletor 0x18 -> Carregado via SYSRETQ com RPL 3 vira 0x1B)
    gdt[3].limit_low            = 0xFFFF;
    gdt[3].access_byte          = 0xF2; // Privilégio Ring 3 (DPL=3), Segmento de Dados R/W
    gdt[3].limit_high_and_flags = 0xCF;

    // gdt[4] - User Code (Seletor 0x20 -> Carregado via SYSRETQ com RPL 3 vira 0x23)
    gdt[4].limit_low            = 0xFFFF;
    gdt[4].access_byte          = 0xFA; // Privilégio Ring 3 (DPL=3), Segmento de Código R/X
    gdt[4].limit_high_and_flags = 0xAF; // Long Mode ativo (L=1)

    // === SISTEMA - DESCRITOR DE TSS EXPANDIDO (16 BYTES TOTAIS) ===
    
    uint32_t tss_limit = sizeof(tss) - 1;
    uint64_t tss_addr  = (uint64_t)&tss;

    // gdt[5] - TSS (Task State Segment) - Primeira Metade (Seletor 0x28)
    gdt[5].limit_low            = tss_limit & 0xFFFF;
    gdt[5].base_low             = tss_addr & 0xFFFF;
    gdt[5].base_mid             = (tss_addr >> 16) & 0xFF;
    gdt[5].access_byte          = 0x89; // Presente, Tipo 9 (TSS de 64-bit disponível), DPL=0
    gdt[5].limit_high_and_flags = (tss_limit >> 16) & 0x0F;
    gdt[5].base_high            = (tss_addr >> 24) & 0xFF;

    // gdt[6] - TSS (Task State Segment) - Segunda Metade (Extensão de 64 bits)
    // Sobrescrevemos o descritor inteiro mapeando-o como os bits superiores do endereço
    uint64_t* tss_extended = (uint64_t*)&gdt[6];
    *tss_extended = (tss_addr >> 32) & 0xFFFFFFFFFFFFFFULL; // Bits 32:63 do endereço. Atributos ficam zerados.

    // Configuração do Ponteiro da GDTR
    gdtr.size   = (sizeof(struct gdt_segment) * 7) - 1; 
    gdtr.offset = (uint64_t)gdt;
}
