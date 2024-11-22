#pragma once

#include <cstdint>

#include "sim_proc.h"
typedef struct{
  uint32_t dst_tag;
  uint32_t src1;
  uint32_t src2;
  uint8_t latency;
  bool valid;
} pipeline_regs_e; // Execute list functional units

extern void Issue(unsigned long int);

extern void Execute(void);

extern void Writeback(void);

extern void Retire(unsigned long int, unsigned long int);
