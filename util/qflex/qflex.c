#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "qemu/osdep.h"
#include "qemu/thread.h"

#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qapi/qmp/qerror.h"
#include "qemu/option_int.h"
#include "qemu/main-loop.h"

#include "exec/log.h"
#include "qflex/qflex.h"
#include "qflex/qflex-traces.h"

QflexState_t qflexState = {
    .inst_done = false,
    .broke_loop = false,
    .singlestep = false,
    .exit_main_loop = false,
    .skip_interrupts = false,
    .log_inst = false,
    .config = {
        .sim_cycles = 0,
        .cores_count = 0,
        .config_path = NULL,
        .sim_path = NULL,
        .debug_mode = NULL,
        .is_trace = false,
        .is_timing = false,
    },
};



static int qflex_singlestep_with_retry(CPUState *cpu, bool retry) {
    int ret = 0;
    uint64_t pc_ss = QFLEX_GET_ARCH(pc)(cpu);
    uint32_t asid = QFLEX_GET_ARCH(asid)(cpu);
    uint64_t pc_ss_after;

    if(qflexState.log_inst) {
        qemu_log("CPU[%i]:ASID[%x]:", cpu->cpu_index, asid); 
        QFLEX_GET_ARCH(log_inst)(cpu);
    }

    ret = qflex_cpu_step(cpu);

    pc_ss_after = QFLEX_GET_ARCH(pc)(cpu);
    if(pc_ss == pc_ss_after) {
        if(retry) {
            printf("QFlex singlestep went wrong twice in a row: ret = %x\n", ret);
            qemu_log("QFlex singlestep went wrong twice in a row: ret = %x\n", ret);
            assert(false);
        } else {
            printf("QFlex singlestep went wrong, did not advance a step: ret = %x\n ---- Retrying\n", ret);
            qemu_log("QFlex singlestep went wrong, did not advance a step: ret = %x\n ---- Retrying\n", ret);
            ret = qflex_singlestep_with_retry(cpu, true);
        }
    }

    return ret;
}

int qflex_singlestep(CPUState *cpu) {
    return qflex_singlestep_with_retry(cpu, false);
}

static void qflex_prepare_singlestep(void) {
    CPUState *cpu;
    CPU_FOREACH(cpu) {
        cpu_single_step(cpu, SSTEP_ENABLE);
        qatomic_mb_set(&cpu->exit_request, 0);
    }
}

int qflex_adaptative_execution(CPUState *cpu) {
    qflexState.exit_main_loop = false;
    if (qflexState.config.is_timing || qflexState.singlestep) {
            qflex_prepare_singlestep();
    }
    while(1) {
        if (qflexState.config.is_timing) {
            qflex_sim_callbacks.start_timing();
        } else if (qflexState.singlestep) {
            qflex_singlestep(cpu);
        }
        else if(!qflexState.exit_main_loop) {
            break;
        } 
    }

    CPUState *cs;
    CPU_FOREACH(cs) {
        cpu_single_step(cs, 0);
    }
    return 0;
}