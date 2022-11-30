#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/helper-proto.h"
#include "exec/log.h"
#include "exec/exec-all.h"
#include "exec/helper-arch.h"
#include "qflex/qflex-arch.h"


bool HELPER(vcpu_is_userland)(CPUState *cpu) { return arm_current_el(ENV(cpu)) == 0; }
uint16_t HELPER(vcpu_get_asid)(CPUState *cpu) { return QFLEX_GET_ARCH(asid)(cpu); }
