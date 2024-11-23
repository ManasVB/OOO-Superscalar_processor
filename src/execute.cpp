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

extern bool is_done;

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
  wakeup.clear();

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

  uint32_t instructions_retired = 0;

  while((head != tail) || is_rob_full) {
    if(rob[head].rdy) {

      Payload *pl_print = rob[head].metadata;
      
      // Update RMT
      int check_rmt_entry = rob[head].dest;

      if(check_rmt_entry != -1) {
        if(rmt[check_rmt_entry].valid && rmt[check_rmt_entry].ROB_tag == head) {
          rmt[check_rmt_entry].valid = false;
        }
      }

      printf("%lu  ", pl_print->age);
      printf("fu{%d}  ", pl_print->op_type);
      printf("src{%d,%d}  ", pl_print->src1, pl_print->src2);
      printf("dst{%d}  ", pl_print->dest);

      if((rob[head].metadata)->age == (unsigned)final_instruction_number) {is_done = true; break;}

      // Increment head pointer
      head = (head + 1)%rob_size;
      
      if(is_rob_full)
        is_rob_full = false;

      ++instructions_retired;
    } else
      break;

    if(instructions_retired == width)
      break;
  }
}
