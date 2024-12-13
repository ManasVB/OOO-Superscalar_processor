#pragma once

#include <cstdint>

#include "sim_proc.h"
typedef IQ pipeline_regs_e; // Execute list functional units

extern void Issue(unsigned long int);

extern void Execute(void);

extern void Writeback(void);

extern void Retire(unsigned long int, unsigned long int);
