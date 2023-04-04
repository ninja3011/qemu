
#include "qflex/qflex.h"
#include "qflex/custom-instrumentation.h"
#include "qflex/qflex-log.h"

QFlexGen_t qflexGen = {
    .example = false,
    .max_calls = 0,
    .nb_calls = 0
};

/** 
 * If the helper is inserted on every function callbacks, it will slowdown
 * considerably the program.
 * We might want only to enable callbacks on specific moments.
 * 
 * TB cache should be cleaned by calling `qflex_tb_flush()` when the flag is set.
 * This will ensure that previously decoded instructions will redecode 
 * and insert the helper.
 */
void qflex_gen_example_set(bool set, uint64_t nb_insn)
{
    qflexGen.example = set;
    qflexGen.max_calls = nb_insn;
    qflexGen.nb_calls = 0;
    qflex_tb_flush();
}

/**
 * We have 3 example:
 * TAG_INSTRUCTION_DECODED: Helper callback on every start of instruction decoded
 * TAG_EXCEPTION_RETURN: Helper callback on every exception return instruction
 * TAG_MTE_OPERATION: Helpter on every(not sure?) time we check for the MTE tag
 * 
 * NOTE: for the `qflexGen.nb_calls` variable, in case you run with Multi-threaded
 *       TCG, it will most likely not be exact because of concurrent additions.
 *       Multiple threads might be updating the `nb_calls` at the same time resulting 
 *       in data races.
 */
void qflex_example_callback(uint64_t cpu_index, uint64_t arg1, uint64_t arg2)
{
    switch(arg1) {
        case TAG_INSTRUCTION_DECODED:
            qemu_log("CPU[%"PRId64"]: Decoded instruction with PC[0x%016"PRIx64"]\n", cpu_index, arg2);
            break;
        case TAG_EXCEPTION_RETURN:
            qemu_log("CPU[%"PRId64"]: Executed ERET with dest PC[0x%016"PRIx64"]\n", cpu_index, arg2);
            break;
        case TAG_MTE_OPERATION:
            qemu_log("CPU[%"PRId64"]: Checking MTE for ADDR[0x%016"PRIx64"]\n", cpu_index, arg2);
            break;
    }

    qflexGen.nb_calls++;
    if(qflexGen.max_calls <= qflexGen.nb_calls) {
        // Stop executing helpers
        qflex_gen_example_set(false, 0);
    }
}