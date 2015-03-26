/* Copyright 2015, Quasilinear Research, All rights reserved. */

/**
 * Q (Quasilinear) Instruction Set Extension Implementation
 * 
 * The environment must define the Q_DRIVER variable naming a shared library
 * to load. Execution of 'custom0' instructions will be delegated to this library
 * via the API defined in 'qio.h'. Libraries should implement 'qio_init' and
 * 'qio_exec'.
 *
 **/

#include "qio.h"

#include "extension.h"
#include "rocc.h"
#include "mmu.h"
#include <dlfcn.h>
#include <cstring>
#include <cstdlib>

static qio_exec_t qio_exec;

static reg_t current_pc;
static processor_t* current_proc;     

static uint64_t get_pc() {
  return current_pc;
}

static uint64_t get_xpr(uint8_t which) {
  return current_proc->get_state()->XPR[which];
}

static uint64_t get_fpr(uint8_t which) {
  return current_proc->get_state()->FPR[which];
}

static uint64_t get_mem(uint64_t addr) {
  return current_proc->get_mmu()->load_uint64(addr);
}

static uint32_t get_insn(uint64_t addr){ 
  return current_proc->get_mmu()->load_insn(addr).insn.bits();
}

static qio_callbacks_t callbacks = {
  .get_pc = get_pc,
  .get_xpr = get_xpr,
  .get_fpr = get_fpr,
  .get_mem = get_mem,
  .get_insn = get_insn,
};

static reg_t custom(processor_t* p, insn_t insn, reg_t pc) {
  current_proc = p;
  current_pc = pc;
  rocc_insn_union_t u;
  u.i = insn;
  reg_t xs1 = u.r.xs1 ? RS1 : -1;
  reg_t xs2 = u.r.xs2 ? RS2 : -1;
  reg_t xd = qio_exec(u.r.funct, xs1, xs2);
  if(u.r.xd) {
    WRITE_RD(xd);
  }
  return pc+4;
}

class q_ext_t : public extension_t
{
  public:

    const char* name() { return "q_ext"; }

    std::vector<insn_desc_t> get_instructions()
    {
      std::vector<insn_desc_t> insns;
      insns.push_back((insn_desc_t){0x0b, 0x7f, &::illegal_instruction, custom});
      return insns;
    }

    std::vector<disasm_insn_t*> get_disasms()
    {
      std::vector<disasm_insn_t*> disasms;
      return disasms;
    }

  q_ext_t()
  {

    char* lib_name = getenv("Q_DRIVER");
    if (lib_name == NULL) {
      fprintf(stderr, "Q_DRIVER environment variable should name a shared library\n");
      exit(-1);
    }

    void* lib_handle = dlopen(lib_name, RTLD_LAZY);
    if (lib_handle == NULL) {
      fprintf(stderr, "couldn't open '%s' (from Q_DRIVER)\n", lib_name);
      exit(-1);
    }

    qio_init_t qio_init = (qio_init_t)dlsym(lib_handle,"qio_init");
    if (qio_init == NULL) {
      fprintf(stderr, "driver '%s' did not export 'qio_init' symbol\n", lib_name);
      exit(-1);
    }

    qio_exec = (qio_exec_t)dlsym(lib_handle,"qio_exec");
    if (qio_exec == NULL) {
      fprintf(stderr, "driver '%s' did not export 'qio_exec' symbol\n", lib_name);
      exit(-1);
    }

    qio_init(&callbacks);

  }

};

REGISTER_EXTENSION(q_ext, []() { return new q_ext_t; })
