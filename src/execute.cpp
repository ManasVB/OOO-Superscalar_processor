// This file includes the Out-of-order issue/execute engine
#include <iostream>
#include <stdint.h>
#include <vector>

#include "execute.h"

using namespace std;

extern vector<ROB> rob;
extern vector<RMT> rmt;
extern vector<IQ> iq;

extern vector<vector<pipeline_regs_e>> execute_list;

// Issue up to WIDTH oldest instructions from the IQ
void Issue(unsigned long int width) {
  uint8_t instrs_removed = 0;

  for(auto &instr: iq) {
    if(instr.valid && instr.rs1_rdy && instr.rs2_rdy) {

      // Remove the instruction from the IQ.
      instr.valid = false;

      //Add the instruction to the execute_list
      execute_list[0][instrs_removed] = {.dst_tag = instr.dst_tag, .src1 = instr.rs1_tag, \
        .src2 = instr.rs2_tag, .latency = instr.latency};
      
      ++instrs_removed;
    }
    if(instrs_removed == width) {
      break;
    }
  }
}

void Execute() {}

void Writeback() {}

void Retire() {}
