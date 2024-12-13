#pragma once

#include <cstdio>

#include "sim_proc.h"

typedef Payload pipeline_regs_d; // Dispatch engine pipeline registers

extern void Fetch(FILE *fp, unsigned long int);

extern void Decode(void);

extern void Rename(unsigned long int);

extern void RegRead(void);

extern void Dispatch(void);
