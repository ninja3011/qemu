#include "qemu/osdep.h"
#include "cpu.h"
#include "internals.h"

#include "qflex-helper.h"
#include "qflex/qflex-arch.h"
#include "qflex/qflex.h"
#include "exec/log.h"

#include "cpregs.h"

/* qflex/qflex-arch.h
 */
#define ENV(cpu) ((CPUARMState *) cpu->env_ptr)

uint64_t QFLEX_GET_ARCH(pc)(CPUState *cs) { return ENV(cs)->pc; }
int      QFLEX_GET_ARCH(el)(CPUState *cs) { return arm_current_el(ENV(cs)); }

/**
 * NOTE: Layout ttbrN register: (src: https://developer.arm.com/docs/ddi0595/h/aarch64-system-registers/ttbr0_el1)
 * ASID:  bits [63:48]
 * BADDR: bits [47:1]
 * CnP:   bit  [0]
 */
uint64_t QFLEX_GET_ARCH(asid)(CPUState *cs) {
    if (false /* TODO: when do we need ttbr1 ? */) {
        return ENV(cs)->cp15.ttbr0_ns >> 48;
    } else {
        return ENV(cs)->cp15.ttbr1_ns >> 48;
    }
}

uint64_t QFLEX_GET_ARCH(asid_reg)(CPUState *cs) {
    if (false /* TODO: when do we need ttbr1 ? */) {
        return ENV(cs)->cp15.ttbr0_ns;
    } else {
        return ENV(cs)->cp15.ttbr1_ns;
    }
}

uint64_t QFLEX_GET_ARCH(tid)(CPUState *cs) {
    int curr_el = arm_current_el(ENV(cs));
    if(curr_el == 0) {
        return ENV(cs)->cp15.tpidrurw_ns;
    } else {
        return ENV(cs)->cp15.tpidrprw_ns;
    }
}

uint64_t QFLEX_GET_ARCH(reg)(CPUState *cs, int reg_index) {
    assert(reg_index <= 31 && reg_index >= 0);
    return ENV(cs)->xregs[reg_index];
}

void QFLEX_SET_ARCH(reg)(CPUState *cs, int reg_index, uint64_t value) {
    assert(reg_index <= 31 && reg_index >= 0);
    ENV(cs)->xregs[reg_index] = value;
}


uint64_t QFLEX_GET_ARCH(vfp_reg)(CPUState *cs, int reg_index) {
    assert(reg_index <= 63 && reg_index >= 0);
    return ENV(cs)->vfp.zregs[reg_index >> 1].d[reg_index & 1];
}

void QFLEX_GET_ARCH(log_inst)(CPUState *cs) {
    FILE* logfile = qemu_log_trylock();
    target_disas(logfile, cs, QFLEX_GET_ARCH(pc)(cs), 4);
    qemu_log_unlock(logfile);
}

void QFLEX_GET_ARCH(log_inst_buffer)(CPUState *cs, uint64_t addr, char **buf_ptr) {
    //target_disas_buffer(buf_ptr, cs, addr, 4);
    char *buf = plugin_disas_pc(cs, QFLEX_GET_ARCH(pc)(cs), 4);
    char *newstr = malloc(strlen(buf) + 2);
    strcpy(newstr, buf);
    strcat(newstr, "\n");
    free(buf);
    *buf_ptr = newstr;
}

uint64_t gva_to_hva(CPUState *cs, uint64_t addr, int access_type) {
    return gva_to_hva_arch(cs, addr, (MMUAccessType) access_type);
}

uint64_t gva_to_gpa(CPUState *cs, uint64_t addr, int access_type) {
    return gva_to_gpa_arch(cs, addr, (MMUAccessType) access_type);
}

uint64_t gva_to_hva_with_asid(uint64_t asid_reg, uint64_t vaddr, int access_type) {
    CPUState *cs = first_cpu;
    return gva_to_hva_arch_with_asid(cs, vaddr, (MMUAccessType) access_type, asid_reg);
}

void qflex_print_state_asid_tid(CPUState* cs) {
    CPUARMState *env = cs->env_ptr;
    qemu_log("[MEM]CPU%" PRIu32 "\n"
             "  TTBR0:0[0x%04" PRIx64 "]:1[0x%4" PRIx64 "]:2[0x%4" PRIx64 "]:3[0x%4" PRIx64 "]\n"
             "  TTBR1:0[0x%04" PRIx64 "]:1[0x%4" PRIx64 "]:2[0x%4" PRIx64 "]:3[0x%4" PRIx64 "]\n"
             "  CXIDR:0[0x%04" PRIx64 "]:1[0x%4" PRIx64 "]:2[0x%4" PRIx64 "]:3[0x%4" PRIx64 "]\n"
             "  TPIDR:0[0x%016" PRIx64 "]:1[0x%016" PRIx64 "]:2[0x%016" PRIx64 "]:3[0x%016" PRIx64 "]\n"
             "  TPIDX:0[0x%016" PRIx64 "]:R[0x%016" PRIx64 "]\n",
             cs->cpu_index,
             env->cp15.ttbr0_el[0], env->cp15.ttbr0_el[1], env->cp15.ttbr0_el[2], env->cp15.ttbr0_el[3],
             env->cp15.ttbr1_el[0], env->cp15.ttbr1_el[1], env->cp15.ttbr1_el[2], env->cp15.ttbr1_el[3],
             env->cp15.contextidr_el[0], env->cp15.contextidr_el[1], env->cp15.contextidr_el[2], env->cp15.contextidr_el[3],
             env->cp15.tpidr_el[0], env->cp15.tpidr_el[1], env->cp15.tpidr_el[2], env->cp15.tpidr_el[3],
             env->cp15.tpidruro_ns, env->cp15.tpidrro_el[0]);
}

void qflex_dump_archstate_log(CPUState *cpu, char **buf_ptr) {
    char *buf = *buf_ptr;
    buf += sprintf(buf, "CPU[0x%02d]ASID[0x%08lx]:PC[0x%016lx]\n"
             "NZCV[0x%x]",
             cpu->cpu_index, QFLEX_GET_ARCH(asid)(cpu), QFLEX_GET_ARCH(pc)(cpu),
             QFLEX_GET_ARCH(nzcv)(cpu));
    for (int reg = 0; reg < 32; reg++) {
        buf += sprintf(buf, "X%02i[%016lx]\n", reg, QFLEX_GET_ARCH(reg)(cpu, reg));
    }
}

uint32_t QFLEX_GET_ARCH(pstate)(CPUState *cs) {
    return pstate_read(ENV(cs));
}

uint32_t QFLEX_GET_ARCH(nzcv)(CPUState *cs) {
    CPUARMState *env = cs->env_ptr;
    uint32_t nzcv = (pstate_read(env) >> 28) & 0xF;
    return nzcv;
}

uint64_t QFLEX_GET_ARCH(sp_el)(CPUState *cs, int el) {
    return ENV(cs)->sp_el[el];
}

uint64_t QFLEX_GET_ARCH(sctlr)(CPUState *cs, int el) {
    return ENV(cs)->cp15.sctlr_el[el];
}

uint64_t QFLEX_GET_ARCH(ttbr0)(CPUState *cs, int el) {
    return ENV(cs)->cp15.ttbr0_el[el];
}

uint64_t QFLEX_GET_ARCH(ttbr1)(CPUState *cs, int el) {
    return ENV(cs)->cp15.ttbr1_el[el];
}

uint64_t QFLEX_GET_ARCH(tcr)(CPUState *cs, int el) {
    return ENV(cs)->cp15.tcr_el[el];
}

uint64_t QFLEX_GET_ARCH(isa_reg)(CPUState *cs, int isar) {
    switch (isar) {
    case ISAR_ID_ISAR0:
        return ARM_CPU(cs)->isar.id_isar0;
    case ISAR_ID_ISAR1:
        return ARM_CPU(cs)->isar.id_isar1;
    case ISAR_ID_ISAR2:
        return ARM_CPU(cs)->isar.id_isar2;
    case ISAR_ID_ISAR3:
        return ARM_CPU(cs)->isar.id_isar3;
    case ISAR_ID_ISAR4:
        return ARM_CPU(cs)->isar.id_isar4;
    case ISAR_ID_ISAR5:
        return ARM_CPU(cs)->isar.id_isar5;
    case ISAR_ID_ISAR6:
        return ARM_CPU(cs)->isar.id_isar6;
    case ISAR_ID_MMFR0:
        return ARM_CPU(cs)->isar.id_mmfr0;
    case ISAR_ID_MMFR1:
        return ARM_CPU(cs)->isar.id_mmfr1;
    case ISAR_ID_MMFR2:
        return ARM_CPU(cs)->isar.id_mmfr2;
    case ISAR_ID_MMFR3:
        return ARM_CPU(cs)->isar.id_mmfr3;
    case ISAR_ID_MMFR4:
        return ARM_CPU(cs)->isar.id_mmfr4;
    case ISAR_ID_MMFR5:
        return ARM_CPU(cs)->isar.id_mmfr5;
    case ISAR_ID_PFR0:
        return ARM_CPU(cs)->isar.id_pfr0;
    case ISAR_ID_PFR1:
        return ARM_CPU(cs)->isar.id_pfr1;
    case ISAR_ID_PFR2:
        return ARM_CPU(cs)->isar.id_pfr2;
    case ISAR_MVFR0:
        return ARM_CPU(cs)->isar.mvfr0;
    case ISAR_MVFR1:
        return ARM_CPU(cs)->isar.mvfr1;
    case ISAR_MVFR2:
        return ARM_CPU(cs)->isar.mvfr2;
    case ISAR_ID_DFR0:
        return ARM_CPU(cs)->isar.id_dfr0;
    case ISAR_ID_DFR1:
        return ARM_CPU(cs)->isar.id_dfr1;
    case ISAR_DBGDIDR:
        return ARM_CPU(cs)->isar.dbgdidr;
    case ISAR_DBGDEVID:
        return ARM_CPU(cs)->isar.dbgdevid;
    case ISAR_DBGDEVID1:
        return ARM_CPU(cs)->isar.dbgdevid1;
    case ISAR_ID_AA64ISAR0:
        return ARM_CPU(cs)->isar.id_aa64isar0;
    case ISAR_ID_AA64ISAR1:
        return ARM_CPU(cs)->isar.id_aa64isar1;
    case ISAR_ID_AA64PFR0:
        return ARM_CPU(cs)->isar.id_aa64pfr0;
    case ISAR_ID_AA64PFR1:
        return ARM_CPU(cs)->isar.id_aa64pfr1;
    case ISAR_ID_AA64MMFR0:
        return ARM_CPU(cs)->isar.id_aa64mmfr0;
    case ISAR_ID_AA64MMFR1:
        return ARM_CPU(cs)->isar.id_aa64mmfr1;
    case ISAR_ID_AA64MMFR2:
        return ARM_CPU(cs)->isar.id_aa64mmfr2;
    case ISAR_ID_AA64DFR0:
        return ARM_CPU(cs)->isar.id_aa64dfr0;
    case ISAR_ID_AA64DFR1:
        return ARM_CPU(cs)->isar.id_aa64dfr1;
    case ISAR_ID_AA64ZFR0:
        return ARM_CPU(cs)->isar.id_aa64zfr0;
    case ISAR_ID_AA64SMFR0:
        return ARM_CPU(cs)->isar.id_aa64smfr0;
    case ISAR_RESET_PMCR_EL0:
        return ARM_CPU(cs)->isar.reset_pmcr_el0;
    default:
    // Argument index not found
        assert(false);
    };
    assert(false);
}

uint64_t QFLEX_GET_ARCH(sysreg)(CPUState *cs, uint8_t op0, uint8_t op1,
                                uint8_t op2, uint8_t crn, uint8_t crm) {
    const ARMCPRegInfo *ri;
    ARMCPU *cpu = ARM_CPU(cs);
    CPUARMState *env = cs->env_ptr;

    ri = get_arm_cp_reginfo(cpu->cp_regs,
                            ENCODE_AA64_CP_REG(CP_REG_ARM64_SYSREG_CP,
                                               crn, crm, op0, op1, op2));

    if (!ri) {
        // Unknown register; this might be a guest error or a QEMU
        // unimplemented feature.
        //
        qemu_log_mask(LOG_UNIMP, "ERROR: read access to unsupported AArch64 "
                      "system register op0:%d op1:%d crn:%d crm:%d op2:%d\n",
                      op0, op1, crn, crm, op2);
        return 0;
    }

    // Check access permissions
    if (!cp_access_ok(arm_current_el(env), ri, true)) {
        qemu_log("ERROR: access to sysreg with wrong permissions");
        return 0;
    }

    if (ri && !(ri->type & ARM_CP_NO_RAW)) { 
        return read_raw_cp_reg(env, ri);
    } else { // Msutherl: do it the slow way by linear searching if previous encoding didn't work
        for (size_t i = 0; i < cpu->cpreg_array_len; i++) {
            uint32_t regidx = kvm_to_cpreg_id(cpu->cpreg_indexes[i]);
            ri = get_arm_cp_reginfo(cpu->cp_regs, regidx);
            if (ri->opc0 == op0 &&
                ri->opc1 == op1 &&
                ri->opc2 == op2 &&
                ri->crn == crn &&
                ri->crm == crm && 
                !(ri->type & ARM_CP_NO_RAW)) {
                return read_raw_cp_reg(env,ri);
            }
        }
    }
    qemu_log("ERROR: QEMU did not recognize sysreg case");
    return 0;
}
