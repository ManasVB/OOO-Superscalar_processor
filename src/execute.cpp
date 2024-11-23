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
      
      // Add to the execution list
      for(auto &exec_itr: execute_list) {
        if(!exec_itr.valid) { // valid means free entry
          exec_itr = instr;
          break;
        }
      }

      // Remove instruction from the IQ
      instr.valid = false;
    }

    if(instructions_removed > width)
      break;
  }
}

void Execute() {

  uint8_t WB_Reg_Counter = 0;
  // wakeup.clear();

  for(auto &instr: execute_list) {
    if(instr.valid) {
      if(instr.payload.latency == 1) {

        WB_Reg[WB_Reg_Counter] = instr;
        ++WB_Reg_Counter;

        // Remove from execute_list
        instr.valid = false;

        // Wakeup dependent sources in the Issue Queue
        for(auto &iq_itr: iq) {
          if(iq_itr.valid && !iq_itr.payload.src1_rdy && iq_itr.payload.src1 == instr.payload.dest) {
            iq_itr.payload.src1_rdy = true;
          }

          if(iq_itr.valid && !iq_itr.payload.src2_rdy && iq_itr.payload.src2 == instr.payload.dest) {
            iq_itr.payload.src2_rdy = true;
          }
        }

        // Send a wakeup signal to RR and DI
        wakeup.push_back(instr.payload.dest);

      } else if(instr.payload.latency > 1) {
        --(instr.payload.latency);
      }
    }
  }
}

// Process the writeback bundle in WBFor each instruction in WB, mark the instruction as “ready” in its entry in the ROB.
void Writeback() {
  for(auto &instr: WB_Reg) {
    if(instr.valid) {

      // Writeback stage begin cycle
      instr.payload.begin_cycle[7] = total_cycle_count;

      // WB stage begin cycle = Ex stage begin cycle + latency
      instr.payload.begin_cycle[6] = total_cycle_count - instr.payload.latency;

      rob[instr.payload.dest].rdy = true;
      instr.valid = false;
    }
  }
}

void Retire(unsigned long int rob_size, unsigned long int width) {
  
}
