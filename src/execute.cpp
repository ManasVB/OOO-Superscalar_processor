// This file includes the Out-of-order issue/execute engine
#include <iostream>
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <cassert>

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

      rob[instr.payload.dest].metadata = instr.payload;
      
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

    if(instructions_removed == width)
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

      uint8_t latency = 0;

      // Writeback stage begin cycle
      instr.payload.begin_cycle[7] = total_cycle_count;

      if(instr.payload.op_type == 0)
        latency = 1;
      else if(instr.payload.op_type == 1)
        latency = 2;
      else
        latency = 5;

      // WB stage begin cycle = Ex stage begin cycle + latency
      instr.payload.begin_cycle[6] = total_cycle_count - latency;

      // Retire stage begin cycle
      instr.payload.begin_cycle[8] = instr.payload.begin_cycle[7] + 1;

      rob[instr.payload.dest].metadata = instr.payload;

      rob[instr.payload.dest].rdy = true;
      // rob[instr.payload.dest].metadata = &(instr.payload);
      instr.valid = false;
    }
  }
}

void Retire(unsigned long int rob_size, unsigned long int width) {

  uint32_t instructions_retired = 0;

  while((head != tail) || is_rob_full) {
    if(rob[head].rdy) {

      Payload pl_print = (rob[head].metadata);
      
      // Update RMT
      int check_rmt_entry = rob[head].dest;

      if(check_rmt_entry != -1) {
        assert(rmt[check_rmt_entry].valid);
        
        if(rmt[check_rmt_entry].ROB_tag == head) {
          rmt[check_rmt_entry].valid = false;
        }
      }

      wakeup.erase(std::remove(wakeup.begin(), wakeup.end(), head), wakeup.end());

      printf("%lu  ", pl_print.age);
      printf("fu{%d}  ", pl_print.op_type);
      printf("src{%d,%d}  ", rob[head].src1, rob[head].src2);
      printf("dst{%d}  ", rob[head].dest);
      printf("FE{%lu,%lu} ", pl_print.begin_cycle[0], (pl_print.begin_cycle[1] - pl_print.begin_cycle[0]));
      printf("DE{%lu,%lu}  ", pl_print.begin_cycle[1], (pl_print.begin_cycle[2] - pl_print.begin_cycle[1]));
      printf("RN{%lu,%lu}  ", pl_print.begin_cycle[2], (pl_print.begin_cycle[3] - pl_print.begin_cycle[2]));
      printf("RR{%lu,%lu}  ", pl_print.begin_cycle[3], (pl_print.begin_cycle[4] - pl_print.begin_cycle[3]));
      printf("DI{%lu,%lu}  ", pl_print.begin_cycle[4], (pl_print.begin_cycle[5] - pl_print.begin_cycle[4]));
      printf("IS{%lu,%lu}  ", pl_print.begin_cycle[5], (pl_print.begin_cycle[6] - pl_print.begin_cycle[5]));
      printf("EX{%lu,%lu}  ", (pl_print.begin_cycle[6]), (pl_print.begin_cycle[7] - pl_print.begin_cycle[6]));
      printf("WB{%lu,%u}  ", pl_print.begin_cycle[7], 1);
      printf("RT{%lu,%lu}", pl_print.begin_cycle[8], ((total_cycle_count - pl_print.begin_cycle[8]))+1);
      printf("\n");

      if((rob[head].metadata).age == (unsigned)final_instruction_number) {is_done = true; break;}

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
