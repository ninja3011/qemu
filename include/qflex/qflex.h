#ifndef QFLEX_H
#define QFLEX_H

#include <stdbool.h>

#include "qemu/osdep.h"
#include "qemu/thread.h"

#include "qflex/qflex-log.h"
#include "qflex/qflex-arch.h"

#define QFLEX(name)       glue(qflex_, name)

typedef struct QflexConfig_t {
    uint64_t sim_cycles;
    int cores_count;
    const char *config_path;
    const char *sim_path;
    bool modeIsTiming;
} QflexConfig_t;
 
typedef struct QflexState_t {
    bool singlestep;
    bool inst_done;
    bool broke_loop;
    bool exit_main_loop;
    bool skip_interrupts;
    bool log_inst;
    QflexConfig_t config;
} QflexState_t;

#ifndef MemoryAccessType
// See cpu.h to match MMUAccessType
typedef enum MemoryAccessType {
    DATA_LOAD  = 0,
    DATA_STORE = 1,
    INST_FETCH = 2
} MemoryAccessType;
#define MemoryAccessType
#endif

extern QflexState_t qflexState;

/** qflex_singlestep
 * This functions executes a single instruction (as TB) before returning.
 * It sets the broke_loop flag if it broke the main execution loop (in cpu-exec.c)
 */
int qflex_singlestep(CPUState *cpu);

/** qflex_adaptative_execution
 * This executes normal qemu till a different mode flag is set.
 */
int qflex_adaptative_execution(CPUState *cpu);

/** qflex_cpu_step (cpus.c)
 */
int qflex_cpu_step(void *arg);

/* Located in cpu-exec.c
 * Because QFLEX relies sometimes on added TB instrumentation
 * Flush already decoded TBs such that the extra TCG insts are generated
 */
void qflex_tb_flush(void);
#endif /* QFLEX_H */
