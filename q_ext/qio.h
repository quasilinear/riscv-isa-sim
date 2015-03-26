/* Copyright 2015, Quasilinear Research, All rights reserved. */

/**
 * Q (Quasilinear) Instruction Set Extension Driver Interface
 * 
 * Dynamically loadable drivers should export 'qio_init' and 'qio_exec'
 * functions with the types below.
 *
 **/

#ifdef __cplusplus
  #include <cinttypes>
#else
  #include <inttypes.h>
#endif

typedef struct {
  uint64_t (*get_pc)();
  uint64_t (*get_xpr)(uint8_t which);
  uint64_t (*get_fpr)(uint8_t which);
  uint64_t (*get_mem)(uint64_t addr);
  uint32_t (*get_insn)(uint64_t addr);
} qio_callbacks_t;

typedef void (*qio_init_t)(qio_callbacks_t* callbacks);
typedef uint64_t (*qio_exec_t)(uint8_t funct, uint64_t xs1, uint64_t xs2);
