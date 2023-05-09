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
uint64_t QFLEX_GET_ARCH(vfp_reg)(CPUState *cs, int reg_index);
void	 QFLEX_GET_ARCH(log_inst)(CPUState *cs);
void	 QFLEX_GET_ARCH(log_inst_buffer)(CPUState *cs, uint64_t addr, char **buf_ptr);
uint32_t QFLEX_GET_ARCH(pstate)(CPUState *cs);
uint32_t QFLEX_GET_ARCH(nzcv)(CPUState *cs);
uint64_t QFLEX_GET_ARCH(sysreg)(CPUState *cs, uint8_t op0, uint8_t op1,
                                uint8_t op2, uint8_t crn, uint8_t crm);
uint64_t QFLEX_GET_ARCH(sp_el)(CPUState *cs, int el);
uint64_t QFLEX_GET_ARCH(sctlr)(CPUState *cs, int el);
uint64_t QFLEX_GET_ARCH(ttbr0)(CPUState *cs, int el);
uint64_t QFLEX_GET_ARCH(ttbr1)(CPUState *cs, int el);
uint64_t QFLEX_GET_ARCH(tcr)(CPUState *cs, int el);
uint64_t QFLEX_GET_ARCH(isa_reg)(CPUState *cs, int isar);

#ifndef QFLEX_ARM_ISA_REG
enum {
  ISAR_ID_ISAR0,
  ISAR_ID_ISAR1,
  ISAR_ID_ISAR2,
  ISAR_ID_ISAR3,
  ISAR_ID_ISAR4,
  ISAR_ID_ISAR5,
  ISAR_ID_ISAR6,
  ISAR_ID_MMFR0,
  ISAR_ID_MMFR1,
  ISAR_ID_MMFR2,
  ISAR_ID_MMFR3,
  ISAR_ID_MMFR4,
  ISAR_ID_MMFR5,
  ISAR_ID_PFR0,
  ISAR_ID_PFR1,
  ISAR_ID_PFR2,
  ISAR_MVFR0,
  ISAR_MVFR1,
  ISAR_MVFR2,
  ISAR_ID_DFR0,
  ISAR_ID_DFR1,
  ISAR_DBGDIDR,
  ISAR_DBGDEVID,
  ISAR_DBGDEVID1,
  ISAR_ID_AA64ISAR0,
  ISAR_ID_AA64ISAR1,
  ISAR_ID_AA64PFR0,
  ISAR_ID_AA64PFR1,
  ISAR_ID_AA64MMFR0,
  ISAR_ID_AA64MMFR1,
  ISAR_ID_AA64MMFR2,
  ISAR_ID_AA64DFR0,
  ISAR_ID_AA64DFR1,
  ISAR_ID_AA64ZFR0,
  ISAR_ID_AA64SMFR0,
  ISAR_RESET_PMCR_EL0,
};
#define QFLEX_ARM_ISA_REG
#endif

bool QFLEX_GET_ARCH(has_irq)(CPUState *cs);

void qflex_dump_archstate_log(CPUState *cpu, char **buf_ptr);


// Guest VA to Guest PA
uint64_t gva_to_gpa(CPUState *cs, uint64_t addr, int access_type);
uint64_t gva_to_hva(CPUState *cs, uint64_t addr, int access_type);
uint64_t gva_to_hva_with_asid(uint64_t asid_reg, uint64_t vaddr, int access_type);

#endif /* QFLEX_ARCH_H */
