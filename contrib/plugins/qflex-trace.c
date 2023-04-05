/*
 * Copyright (C) 2021, Mahmoud Mandour <ma.mandourr@gmail.com>
 *
 * License: GNU GPL, version 2 or later.
 *   See the COPYING file in the top-level directory.
 */

#include <inttypes.h>
#include <stdio.h>
#include <glib.h>

#include <qemu-plugin.h>
// #include "qflex-api.h"

#define STRTOLL(x) g_ascii_strtoll(x, NULL, 10)

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

static enum qemu_plugin_mem_rw rw = QEMU_PLUGIN_MEM_RW;

static GHashTable *miss_ht;

static GMutex hashtable_lock;
//static GRand *rng;

typedef struct {
    uint64_t vaddr_pc;
    uint64_t haddr_pc;
} InsnData;

static void vcpu_mem_access(unsigned int vcpu_index, qemu_plugin_meminfo_t info,
                            uint64_t vaddr, void *userdata)
{
    struct qemu_plugin_hwaddr *hwaddr;
    hwaddr = qemu_plugin_get_hwaddr(info, vaddr);
    if (hwaddr && qemu_plugin_hwaddr_is_io(hwaddr)) {
        // We still trace io memory accesses?
        // return;
    }
    uint64_t haddr = qemu_plugin_hwaddr_phys_addr(hwaddr);
    printf("%lx", haddr);
}

static void vcpu_insn_exec(unsigned int vcpu_index, void *userdata)
{
    InsnData *insn = ((InsnData *) userdata);
    uint64_t haddr_pc = insn->haddr_pc;
    uint64_t vaddr_pc = insn->vaddr_pc;
    printf("%lx%lx", haddr_pc, vaddr_pc);
}

static void vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
    size_t n_insns;
    size_t i;
    InsnData *data;

    n_insns = qemu_plugin_tb_n_insns(tb);
    for (i = 0; i < n_insns; i++) {
        struct qemu_plugin_insn *insn = qemu_plugin_tb_get_insn(tb, i);
        uint64_t haddr_pc = (uint64_t) qemu_plugin_insn_haddr(insn);
        uint64_t vaddr_pc = (uint64_t) qemu_plugin_insn_vaddr(insn);

        /*
         * Instructions might get translated multiple times, we do not create
         * new entries for those instructions. Instead, we fetch the same
         * entry from the hash table and register it for the callback again.
         */
        g_mutex_lock(&hashtable_lock);
        data = g_hash_table_lookup(miss_ht, GUINT_TO_POINTER(haddr_pc));
        if (data == NULL) {
            data = g_new0(InsnData, 1);
            data->haddr_pc = haddr_pc;
            data->vaddr_pc = vaddr_pc;
            g_hash_table_insert(miss_ht, GUINT_TO_POINTER(haddr_pc),
                               (gpointer) data);
        }
        g_mutex_unlock(&hashtable_lock);

        qemu_plugin_register_vcpu_mem_cb(insn, vcpu_mem_access,
                                         QEMU_PLUGIN_CB_NO_REGS,
                                         rw, data);

        qemu_plugin_register_vcpu_insn_exec_cb(insn, vcpu_insn_exec,
                                               QEMU_PLUGIN_CB_NO_REGS, data);
    }
}

static void insn_free(gpointer data)
{
    InsnData *insn = (InsnData *) data;
    g_free(insn);
}

static void plugin_exit(qemu_plugin_id_t id, void *p)
{
    g_hash_table_destroy(miss_ht);
}

typedef enum {
    OP_X,
    OP_1,
    OP_2,
    OP_3
} op_types_t;

QEMU_PLUGIN_EXPORT
int qemu_plugin_install(qemu_plugin_id_t id, const qemu_info_t *info,
                        int argc, char **argv)
{
    op_types_t op = OP_X;
    uint64_t value = 0;
    bool flag_set = false;

    for (int i = 0; i < argc; i++) {
        char *opt = argv[i];
        g_autofree char **tokens = g_strsplit(opt, "=", 2);

        if (g_strcmp0(tokens[0], "value") == 0) {
            value = STRTOLL(tokens[1]);
        } else if (g_strcmp0(tokens[0], "flag_set") == 0) {
            if (!qemu_plugin_bool_parse(tokens[0], tokens[1], &flag_set)) {
                fprintf(stderr, "boolean argument parsing failed: %s\n", opt);
                return -1;
            }
        } else if (g_strcmp0(tokens[0], "op_type") == 0) {
            if (g_strcmp0(tokens[1], "op1") == 0) {
                op = OP_1;
            } else if (g_strcmp0(tokens[1], "op2") == 0) {
                op = OP_2;
            } else if (g_strcmp0(tokens[1], "op3") == 0) {
                op = OP_3;
            } else {
                fprintf(stderr, "invalid op: %s\n", opt);
                return -1;
            }
        } else {
            fprintf(stderr, "option parsing failed: %s\n", opt);
            return -1;
        }
    }

    //qemu_plugin_qflex_get_callbacks();
    // Insert callback on translation block generation
    qemu_plugin_register_vcpu_tb_trans_cb(id, vcpu_tb_trans);
    // Insert callback on program exit
    qemu_plugin_register_atexit_cb(id, plugin_exit, NULL);

    g_autoptr(GString) params_log = g_string_new("");
    g_string_append_printf(params_log, "option parsing success: value[0x%016lx]:flag_set[%s]:op_type[%i]\n", value, flag_set ? "true" : "false", op);
    qemu_plugin_outs(params_log->str);


    miss_ht = g_hash_table_new_full(NULL, g_direct_equal, NULL, insn_free);

    return 0;
}
