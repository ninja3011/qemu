#include "qemu/osdep.h"
#include "qemu/thread.h"

#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qapi/qmp/qerror.h"
#include "qemu/option_int.h"
#include "qemu/config-file.h"
#include "qemu/qemu-options.h"
#include "qemu/main-loop.h"
#include "hw/boards.h"
#include "qemu/option.h"

#include "qemu/log.h"

#include "sysemu/tcg.h"

#include "qflex/qflex-config.h"
#include "qflex/qflex.h"
#include "qflex/qflex-log.h"
#include "qflex/qflex-traces.h"

#include "qflex/libqflex/qflex-api.h"
#include "qflex/libqflex/flexus_proxy.h"

QemuOptsList qemu_qflex_opts = {
    .name = "qflex",
    .merge_lists = true,
    .head = QTAILQ_HEAD_INITIALIZER(qemu_qflex_opts.head),
    .desc = {
        {
            .name = "singlestep",
            .type = QEMU_OPT_BOOL,

        },
        {
            .name = "sim-path",
            .type = QEMU_OPT_STRING,
        },
        {
            .name = "sim-config-path",
            .type = QEMU_OPT_STRING,
        },
        {
            .name = "debug",
            .type = QEMU_OPT_STRING,
        },
        {
            .name = "mode",
            .type = QEMU_OPT_STRING,
        },
        {
            .name = "core-count",
            .type = QEMU_OPT_NUMBER,

        },
        {
            .name = "cycles",
            .type = QEMU_OPT_NUMBER,

        },
        { /* end of list */ }
    },
};

QemuOptsList qemu_qflex_gen_mem_trace_opts = {
    .name = "qflex-gen-mem-trace",
    .merge_lists = true,
    .head = QTAILQ_HEAD_INITIALIZER(qemu_qflex_gen_mem_trace_opts.head),
    .desc = {
        {
            .name = "core_count",
            .type = QEMU_OPT_NUMBER,
        },
        { /* end of list */ }
    },
};

static void qflex_configure(QemuOpts *opts, Error **errp) {
    qflexState.singlestep = qemu_opt_get_bool(opts, "singlestep", false);
    qflexState.config.sim_cycles = qemu_opt_get_number(opts, "cycles", 1000);
    qflexState.config.cores_count = qemu_opt_get_number(opts, "core-count", -1);
    qflexState.config.is_timing = false;

    const char *config_path = qemu_opt_get(opts, "sim-config-path");
    const char *sim_path = qemu_opt_get(opts, "sim-path");
    const char *mode_opt = qemu_opt_get(opts, "mode");
    const char *debug_mode = qemu_opt_get(opts, "debug");

    qflexState.config.config_path = strdup(config_path);
    qflexState.config.sim_path = strdup(sim_path);
    if (debug_mode) {
        qflexState.config.debug_mode = strdup(debug_mode);
    } else {
        qflexState.config.debug_mode = NULL;
    }

    int error = 0;
    // Set trace or timing
    if (!mode_opt) {
        error_report("argument for `mode=(trace|timing)` missing, assuming `mode=trace`\n");
        qflexState.config.is_timing = false;
        qflexState.config.is_trace = true;
    } else {
        qflexState.config.is_timing = strcmp(mode_opt, "timing") == 0;
        qflexState.config.is_trace = strcmp(mode_opt, "trace") == 0;
        if (!(qflexState.config.is_timing || qflexState.config.is_trace)) {
          error_setg(errp, "argument for `mode=(trace|timing)` is wrong: %s\n",
                     mode_opt);
          error |= 1;
        }
    }

    if (!qflexState.config.sim_path) {
        error_report("ERROR:`sim-path=PATH` is required to point to QFlex library");
        error |= 1;
    }
    if (!qflexState.config.config_path) {
        error_report("ERROR:`sim-config-path=PATH` is required to point to QFlex uArch config");
        error |= 1;
    }

    if (qflexState.config.is_timing) {
        qflexState.singlestep = true;
        qflexState.exit_main_loop = true;
        qflexState.log_inst = true;
    }

    if (qflexState.singlestep) {
        if (!singlestep) {
            error_report("ERROR:`singlestep` and `timing` available only with `-singlestep`, enabling now.");
            singlestep = true;
        }
    }
 

    if (error) {
        exit(EXIT_FAILURE);
    }
}

void qflex_init(Error **errp) {
    int error = 0;
    if (qflexState.singlestep) {
        if (!tcg_enabled()) {
            error_report("ERROR:`singlestep` and `timing` available only with TCG.");
            error |= 1;
        }
        if (!qemu_loglevel_mask(CPU_LOG_TB_NOCHAIN)) {
            error_report("ERROR:`singlestep` and `timing` available only with `-d nochain`, setting `-d nochain` flag");
            qemu_loglevel |= CPU_LOG_TB_NOCHAIN;
        }
        if (!singlestep) {
            error_report("ERROR:`singlestep` and `timing` available only with `-singlestep`, should be set before");
            error |= 1;
        }
    }
   
    // Load QFlex library
    bool success = flexus_dynlib_load(qflexState.config.sim_path);
    if (success) {
        fprintf(stderr, "<%s:%i> Flexus Simulator set!.\n", basename(__FILE__),
                __LINE__);
    } else {
        error_report("ERROR:simulator could not be set.!.\n");
        error |= 1;
    }

    if (error) {
        exit(EXIT_FAILURE);
    }

    // Init QFlex values
    QFLEX_TO_QEMU_API_t hooks_from_qemu;
    QEMU_TO_QFLEX_CALLBACKS_t hooks_to_qemu;
    qflex_api_init(qflexState.config.is_timing, qflexState.config.sim_cycles);
    QFLEX_API_get_Interface_Hooks(&hooks_from_qemu);
    qflex_init_fn(&hooks_from_qemu, &hooks_to_qemu, qflexState.config.cores_count, qflexState.config.config_path, qflexState.config.debug_mode);
    QEMU_API_set_Interface_Hooks(&hooks_to_qemu);
}

static void qflex_log_configure(const char *opts) {
    int mask;
    mask = qflex_str_to_log_mask(opts);
    if (!mask) {
        qflex_print_log_usage(opts, stdout);
        exit(EXIT_FAILURE);
    }
    qflex_set_log(mask);
}

static void qflex_gen_mem_trace_configure(QemuOpts *opts, Error **errp) {
    int core_count = qemu_opt_get_number(opts, "core_count", 1);
	qflex_mem_trace_init(core_count);
}

int qflex_parse_opts(int index, const char *optarg, Error **errp) {
    QemuOpts *opts;

    switch(index) {
    case QEMU_OPTION_qflex:
        opts = qemu_opts_parse_noisily(
            qemu_find_opts("qflex"), optarg, false);
        if (!opts) { exit(EXIT_FAILURE); }
        qflex_configure(opts, errp);
        qemu_opts_del(opts);
        break;
    case QEMU_OPTION_qflex_d:
        qflex_log_configure(optarg);
        break;
    case QEMU_OPTION_qflex_gen_mem_trace:
        opts = qemu_opts_parse_noisily(
                qemu_find_opts("qflex-gen-mem-trace"), optarg, false);
        if(!opts) { exit(EXIT_FAILURE); }
        qflex_gen_mem_trace_configure(opts, errp);
        qemu_opts_del(opts);
        break;
    default:
        return 0;
    }
    return 1;
}
