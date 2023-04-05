#ifndef QFLEX_ARCH_H
#define QFLEX_ARCH_H

#include "qemu/osdep.h"

/* This file contains functions specific to architectural state.
 * These functions should be defined in target/arch/qflex-helper.c in
 * order to access architecture specific values
 */

// functions define to get CPUArchState specific values
#define QFLEX_GET_ARCH(name) glue(qflex_get_arch_, name)
#define QFLEX_SET_ARCH(name) glue(qflex_set_arch_, name)

uint64_t QFLEX_GET_ARCH(pc)(CPUState *cs);
uint64_t QFLEX_GET_ARCH(asid)(CPUState *cs);
uint64_t QFLEX_GET_ARCH(asid_reg)(CPUState *cs);
uint64_t QFLEX_GET_ARCH(tid)(CPUState *cs);
int      QFLEX_GET_ARCH(el)(CPUState *cs);
uint64_t QFLEX_GET_ARCH(reg)(CPUState *cs, int reg_index);
void     QFLEX_SET_ARCH(reg)(CPUState *cs, int reg_index, uint64_t value);
void	 QFLEX_GET_ARCH(log_inst)(CPUState *cs);
void	 QFLEX_GET_ARCH(log_inst_buffer)(CPUState *cs, uint64_t addr, char **buf_ptr);
uint32_t QFLEX_GET_ARCH(pstate)(CPUState *cs);
uint32_t QFLEX_GET_ARCH(nzcv)(CPUState *cs);
uint64_t QFLEX_GET_ARCH(sysreg)(CPUState *cs, uint8_t op0, uint8_t op1,
                                uint8_t op2, uint8_t crn, uint8_t crm);
uint64_t QFLEX_GET_ARCH(sp_el)(CPUState *cs, int el);
uint64_t QFLEX_GET_ARCH(sctlr)(CPUState *cs, int el);
bool QFLEX_GET_ARCH(has_irq)(CPUState *cs);

void qflex_dump_archstate_log(CPUState *cpu, char **buf_ptr);

uint64_t gva_to_hva(CPUState *cs, uint64_t addr, int access_type);
uint64_t gva_to_hva_with_asid(uint64_t asid_reg, uint64_t vaddr, int access_type);

#endif /* QFLEX_ARCH_H */
