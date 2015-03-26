/* Copyright 2015, Quasilinear Research, All rights reserved. */

/**
 * Q (Quasilinear) Instruction Set Extension Implementation
 * 
 * If the Q_DRIVER environment variable is defined, the shared library names
 * will be loaded and execution of 'custom0' instructions will be delegated to
 * it via the API defined in 'qio.h'. Libraries should implement 'qio_init' and
 * 'qio_exec'.
 *
 * If Q_DRIVER is not defined, an internal fallback driver will be used instead.
 * The fallback driver reads from the file identified by the Q_TRACE environment
 * variable. The file should contain a sequence of values to be returned, one
 * per line, represented in hex with a leading '0x'. If the external trace file
 * is not defined or the given trace file has been exhausted, the value zero
 * will be used instead.
 *
 **/

#include "qio.h"

#include "rocc.h"
#include "mmu.h"
#include <dlfcn.h>
#include <cstring>
#include <cstdlib>

// forward declaration of callback functions
uint64_t qio_get_reg(void* instance, uint8_t which);
uint64_t qio_get_mem(void* instance, uint64_t addr);
uint64_t qio_get_instruction(void* instance, uint64_t addr);

class q_rocc_t : public rocc_t
{
 public:
  const char* name() { return "q_rocc"; }

  reg_t custom0(rocc_insn_t insn, reg_t xs1, reg_t xs2)
  {
    return qio_exec(insn.funct, xs1, xs2); // let the driver handle this
  }

  q_rocc_t()
  {

    char* lib_name = getenv("Q_DRIVER");
    // if lib_name is NULL, we'll end up looking up the dummy implementation below

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

    qio_callbacks_t callbacks = {
      .get_reg = qio_get_reg,
      .get_mem = qio_get_mem,
      .get_instruction = qio_get_instruction,
    };

    qio_init(this, callbacks);

  }

  friend uint64_t qio_get_reg(void* instance, uint8_t which);
  friend uint64_t qio_get_mem(void* instance, uint64_t addr);
  friend uint64_t qio_get_instruction(void* instance, uint64_t addr);

 private:
  qio_exec_t qio_exec = NULL;
};

// callbacks defined outside of class so that they can be exported to driver 

uint64_t qio_get_reg(void* instance, uint8_t which) {
  q_rocc_t* q = (q_rocc_t*)instance;
  state_t* state = q->p->get_state();
  if (which == 0) {
    return state->pc;
  } else if (which < NXPR) {
    return state->XPR[which];
  } else if (which < NXPR + NFPR) {
    return state->FPR[which - NXPR];
  } else {
    return 0;
  }
}

uint64_t qio_get_mem(void* instance, uint64_t addr) {
  q_rocc_t* q = (q_rocc_t*)instance;
  return q->p->get_mmu()->load_uint64(addr);
}

uint64_t qio_get_instruction(void* instance, uint64_t addr) {
  q_rocc_t* q = (q_rocc_t*)instance;
  return q->p->get_mmu()->load_insn(addr).insn.bits();
}

// fallback driver implementation
extern "C" {

  static FILE* q_trace_file;
  void qio_init(void* instance, qio_callbacks_t) {
    char* q_trace_filename = getenv("Q_TRACE");
    if (q_trace_filename != NULL) {
      q_trace_file = fopen(q_trace_filename, "r");
    }
  }
  uint64_t qio_exec(uint8_t funct, uint64_t xs1, uint64_t xs2) {
    if (q_trace_file != NULL) {
      uint64_t res;
      if (fscanf(q_trace_file, "0x%llx\n", &res) > 0) {
        return res;
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  }
}

REGISTER_EXTENSION(q_rocc, []() { return new q_rocc_t; })
