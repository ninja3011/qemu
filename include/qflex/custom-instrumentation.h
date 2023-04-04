#include "qflex.h"

typedef struct QFlexGen_t {
    bool example;
    uint64_t max_calls;
    uint64_t nb_calls;
} QFlexGen_t;

extern QFlexGen_t qflexGen;

void qflex_gen_example_set(bool set, uint64_t nb_insn);
void qflex_example_callback(uint64_t cpu_index, uint64_t arg1, uint64_t arg2);
#define TAG_INSTRUCTION_DECODED 0x100
#define TAG_EXCEPTION_RETURN    0x200
#define TAG_MTE_OPERATION       0x400
