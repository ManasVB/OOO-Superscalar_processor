// This file includes the In-order fetch/dispatch engine
#include <iostream>
#include <cstdint>

#include "dispatch.h"

extern bool trace_file_read;
extern uint64_t instruction_count;

bool Fetch(FILE *FP, unsigned long int width) {

  int op_type, dest, src1, src2;  // Variables are read from trace file
  uint64_t pc; // Variable holds the pc read from input file

  uint32_t line_count = 0;

  while(line_count < width) {
    if(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF) {
      printf("%lx %d %d %d %d\n", pc, op_type, dest, src1, src2);
      ++line_count;
      ++instruction_count;
    } else {
      trace_file_read = true;
      break;
    }
  }

  return trace_file_read;
}

void Decode() {}

void Rename() {}

void RegRead() {}

void Dispatch() {}
