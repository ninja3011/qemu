#include "stdint.h"
#include "stdbool.h"
#include "qflex-trace-decoder.h"

typedef struct TraceParams {

} TraceParams;

/**
 * extract32:
 * @value: the value to extract the bit field from
 * @start: the lowest bit in the bit field (numbered from 0)
 * @length: the length of the bit field
 *
 * Extract from the 32 bit input @value the bit field specified by the
 * @start and @length parameters, and return it. The bit field must
 * lie entirely within the 32 bit word. It is valid to request that
 * all 32 bits are returned (ie @length 32 and @start 0).
 *
 * Returns: the value of the bit field extracted from the input value.
 */
static inline uint32_t extract32(uint32_t value, int start, int length)
{
    assert(start >= 0 && length > 0 && length <= 32 - start);
    return (value >> start) & (~0U >> (32 - length));
}

static void disas_ldst_excl(DisasContext *s, uint32_t insn)
{
    int rt = extract32(insn, 0, 5);
    int rn = extract32(insn, 5, 5);
    int rt2 = extract32(insn, 10, 5);
    int rs = extract32(insn, 16, 5);
    int is_lasr = extract32(insn, 15, 1);
    int o2_L_o1_o0 = extract32(insn, 21, 3) * 2 | is_lasr;
    int size = extract32(insn, 30, 2);

    switch (o2_L_o1_o0) {
    case 0x0: /* STXR */
    case 0x1: /* STLXR */
        clean_addr = gen_mte_check1(s, cpu_reg_sp(s, rn),
                                    true, rn != 31, size);
        gen_store_exclusive(s, rs, rt, rt2, clean_addr, size, false);
        return;

    case 0x4: /* LDXR */
    case 0x5: /* LDAXR */
        if (rn == 31) {
            gen_check_sp_alignment(s);
        }
        clean_addr = gen_mte_check1(s, cpu_reg_sp(s, rn),
                                    false, rn != 31, size);
        s->is_ldex = true;
        gen_load_exclusive(s, rt, rt2, clean_addr, size, false);
        if (is_lasr) {
            tcg_gen_mb(TCG_MO_ALL | TCG_BAR_LDAQ);
        }
        return;

    case 0x8: /* STLLR */
        if (!dc_isar_feature(aa64_lor, s)) {
            break;
        }
        /* StoreLORelease is the same as Store-Release for QEMU.  */
        /* fall through */
    case 0x9: /* STLR */
        /* Generate ISS for non-exclusive accesses including LASR.  */
        if (rn == 31) {
            gen_check_sp_alignment(s);
        }
        tcg_gen_mb(TCG_MO_ALL | TCG_BAR_STRL);
        clean_addr = gen_mte_check1(s, cpu_reg_sp(s, rn),
                                    true, rn != 31, size);
        /* TODO: ARMv8.4-LSE SCTLR.nAA */
        GEN_QFLEX_HELPER(qflex_mem_trace_gen_helper(), GEN_HELPER(qflex_pre_mem)( 
					 cpu_env, clean_addr, tcg_const_i32(MMU_DATA_STORE), tcg_const_i32(1 << size)));
        do_gpr_st(s, cpu_reg(s, rt), clean_addr, size | MO_ALIGN, true, rt,
                  disas_ldst_compute_iss_sf(size, false, 0), is_lasr);
        GEN_QFLEX_HELPER(qflex_mem_trace_gen_helper(), GEN_HELPER(qflex_post_mem)( 
					 cpu_env, clean_addr, tcg_const_i32(MMU_DATA_STORE), tcg_const_i32(1 << size)));
        return;

    case 0xc: /* LDLAR */
        if (!dc_isar_feature(aa64_lor, s)) {
            break;
        }
        /* LoadLOAcquire is the same as Load-Acquire for QEMU.  */
        /* fall through */
    case 0xd: /* LDAR */
        /* Generate ISS for non-exclusive accesses including LASR.  */
        if (rn == 31) {
            gen_check_sp_alignment(s);
        }
        clean_addr = gen_mte_check1(s, cpu_reg_sp(s, rn),
                                    false, rn != 31, size);
        /* TODO: ARMv8.4-LSE SCTLR.nAA */
        GEN_QFLEX_HELPER(qflex_mem_trace_gen_helper(), GEN_HELPER(qflex_pre_mem)( 
					 cpu_env, clean_addr, tcg_const_i32(MMU_DATA_LOAD), tcg_const_i32(1 << size)));
        do_gpr_ld(s, cpu_reg(s, rt), clean_addr, size | MO_ALIGN, false, true,
                  rt, disas_ldst_compute_iss_sf(size, false, 0), is_lasr);
        GEN_QFLEX_HELPER(qflex_mem_trace_gen_helper(), GEN_HELPER(qflex_post_mem)( 
					 cpu_env, clean_addr, tcg_const_i32(MMU_DATA_LOAD), tcg_const_i32(1 << size)));

        tcg_gen_mb(TCG_MO_ALL | TCG_BAR_LDAQ);
        return;

    case 0x2: case 0x3: /* CASP / STXP */
        if (size & 2) { /* STXP / STLXP */
            if (rn == 31) {
                gen_check_sp_alignment(s);
            }
            if (is_lasr) {
                tcg_gen_mb(TCG_MO_ALL | TCG_BAR_STRL);
            }
            clean_addr = gen_mte_check1(s, cpu_reg_sp(s, rn),
                                        true, rn != 31, size);
            gen_store_exclusive(s, rs, rt, rt2, clean_addr, size, true);
            return;
        }
        if (rt2 == 31
            && ((rt | rs) & 1) == 0
            && dc_isar_feature(aa64_atomics, s)) {
            /* CASP / CASPL */
            gen_compare_and_swap_pair(s, rs, rt, rn, size | 2);
            return;
        }
        break;

    case 0x6: case 0x7: /* CASPA / LDXP */
        if (size & 2) { /* LDXP / LDAXP */
            // addr = r[n]
            gen_load_exclusive(s, rt, rt2, clean_addr, size, true);
            return;
        }
        if (rt2 == 31
            && ((rt | rs) & 1) == 0) {
            /* CASPA / CASPAL */
            gen_compare_and_swap_pair(s, rs, rt, rn, size | 2);
            return;
        }
        break;

    case 0xa: /* CAS */
    case 0xb: /* CASL */
    case 0xe: /* CASA */
    case 0xf: /* CASAL */
        if (rt2 == 31 && dc_isar_feature(aa64_atomics, s)) {
            gen_compare_and_swap(s, rs, rt, rn, size);
            return;
        }
        break;
    }
    unallocated_encoding(s);
}
static void disas_ldst(TraceParams *s, uint32_t insn)
{
    switch (extract32(insn, 24, 6)) {
    case 0x08: /* Load/store exclusive */
        disas_ldst_excl(s, insn);
        break;
    case 0x18: case 0x1c: /* Load register (literal) */
        disas_ld_lit(s, insn);
        break;
    case 0x28: case 0x29:
    case 0x2c: case 0x2d: /* Load/store pair (all forms) */
        disas_ldst_pair(s, insn);
        break;
    case 0x38: case 0x39:
    case 0x3c: case 0x3d: /* Load/store register (all forms) */
        disas_ldst_reg(s, insn);
        break;
    case 0x0c: /* AdvSIMD load/store multiple structures */
        disas_ldst_multiple_struct(s, insn);
        break;
    case 0x0d: /* AdvSIMD load/store single structure */
        disas_ldst_single_struct(s, insn);
        break;
    case 0x19:
        if (extract32(insn, 21, 1) != 0) {
            disas_ldst_tag(s, insn);
        } else if (extract32(insn, 10, 2) == 0) {
            disas_ldst_ldapr_stlr(s, insn);
        } else {
            // unallocated_encoding(s);
        }
        break;
    default:
        unallocated_encoding(s);
        break;
    }
}

static bool aarch64_insn_get_params(TraceParams *s, uint32_t insn)
{

    switch (extract32(insn, 25, 4)) {
    case 0x0:
        break;
    case 0x1: case 0x3: /* UNALLOCATED */
        // unallocated_encoding;
        break;
    case 0x2: /* SVE */
        break;
    case 0x8: case 0x9: /* Data processing - immediate */
        break;
    case 0xa: case 0xb: /* Branch, exception generation and system insns */
        break;
    case 0x4:
    case 0x6:
    case 0xc:
    case 0xe:      /* Loads and stores */
        disas_ldst(s, insn);
        break;
    case 0x5:
    case 0xd:      /* Data processing - register */
        break;
    case 0x7:
    case 0xf:      /* Data processing - SIMD and floating point */
        break;
    default:
        assert(FALSE); /* all 15 cases should be handled above */
        break;
    }
    return false;
}