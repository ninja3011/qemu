#include "qemu/osdep.h"
#include "qemu/thread.h"

#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qapi/qmp/qerror.h"
#include "qemu/option_int.h"
#include "qemu/config-file.h"
#include "qemu/qemu-options.h"
#include "qemu/main-loop.h"

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
    if (qflexState.singlestep) {
        int error = 0;
        if (!tcg_enabled()) {
            error_report("`singlestep` available only with TCG.");
            error |= 1;
        }
        if (!qemu_loglevel_mask(CPU_LOG_TB_NOCHAIN)) {
            error_report("`singlestep` available only with `-d nochain`.");
            error |= 1;
        }
        if (error)
            exit(EXIT_FAILURE);
    }
    flexus_dynlib_load("mypath/failure");
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
