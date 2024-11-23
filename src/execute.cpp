// This file includes the Out-of-order issue/execute engine
#include <iostream>
#include <stdint.h>
#include <vector>

#include "execute.h"

using namespace std;

extern vector<ROB> rob;
extern vector<RMT> rmt;
extern vector<IQ> iq;

extern vector<uint32_t> wakeup;

vector<pipeline_regs_e> execute_list;
vector<pipeline_regs_e> WB_Reg;

extern uint32_t head, tail;
extern bool is_rob_full;

extern uint64_t total_instruction_count;
extern uint64_t total_cycle_count;
extern int64_t final_instruction_number;

// Issue up to WIDTH oldest instructions from the IQ
void Issue(unsigned long int width) {

  uint8_t instructions_removed = 0;

  for(auto &instr : iq) {
    if(instr.valid && instr.payload.src1_rdy && instr.payload.src2_rdy) {
      ++instructions_removed;

      // Issue stage begin cycle
      instr.payload.begin_cycle[5] = total_cycle_count;
      
      // Remove instruction from the IQ
      instr.valid = false;

      // Add to the execution list
      for(auto &exec_itr: execute_list) {
        if(!exec_itr.valid) { // valid means free entry
          exec_itr.valid = true;
          exec_itr.payload = instr.payload;
          break;
        }
      }
      ++instructions_removed;
    }

    if(instructions_removed > width)
      break;
  }
}

void Execute() {

}

// Process the writeback bundle in WBFor each instruction in WB, mark the instruction as “ready” in its entry in the ROB.
void Writeback() {

}

void Retire(unsigned long int rob_size, unsigned long int width) {

}
