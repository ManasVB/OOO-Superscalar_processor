// This file includes the Out-of-order issue/execute engine
#include <iostream>
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <cassert>

#include "execute.h"
#include "dispatch.h"

using namespace std;

extern vector<ROB> rob;
extern vector<RMT> rmt;
extern vector<IQ> iq;

extern vector<vector<pipeline_regs_d>> bundle;

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
    if(instr.valid) {

      if(instr.payload.src1_rdy && instr.payload.src2_rdy) {
        ++instructions_removed;

        // Issue stage begin cycle
        instr.payload.timestamp[10] = instr.payload.timestamp[8] + instr.payload.timestamp[9];
        
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
    }

    if(instructions_removed == width)
      break;
  }
}

void Execute() {

  uint8_t WB_Reg_Counter = 0;

  for(auto &instr: execute_list) {
    if(instr.valid) {

      // timestamp[13] is the time spent in execute stage
      ++instr.payload.timestamp[13];

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

        // Wakeup dependent sources in the DI bundle
        for(auto &DI_itr : bundle[3]) {
          if(!DI_itr.src1_rdy && DI_itr.src1 == instr.payload.dest) {
            DI_itr.src1_rdy = true;
          }

          if(!DI_itr.src2_rdy && DI_itr.src2 == instr.payload.dest) {
            DI_itr.src2_rdy = true;
          }          
        }

        // Wakeup dependent sources in the RR bundle
        for(auto &RR_itr : bundle[2]) {
          if(!RR_itr.src1_rdy && RR_itr.src1 == instr.payload.dest) {
            RR_itr.src1_rdy = true;
          }

          if(!RR_itr.src2_rdy && RR_itr.src2 == instr.payload.dest) {
            RR_itr.src2_rdy = true;
          }          
        }

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
      instr.payload.timestamp[14] = total_cycle_count;

      // Execute stage begin cycle
      instr.payload.timestamp[12] = instr.payload.timestamp[14] - instr.payload.timestamp[13];

      // IQ time spent
      instr.payload.timestamp[11] = instr.payload.timestamp[12] - instr.payload.timestamp[10];

      // timestamp[15] is the time spent in Writeback stage
      ++instr.payload.timestamp[15];

      // copy all the timestamps to ROB
      rob[instr.payload.dest].metadata = instr.payload;

      rob[instr.payload.dest].rdy = true;

      instr.valid = false;
    }
  }
}

void Retire(unsigned long int rob_size, unsigned long int width) {

  uint32_t instructions_retired = 0;

  while((head != tail) || is_rob_full) {
    if(rob[head].rdy) {

      // Retire stage begin cycle
      rob[head].metadata.timestamp[16] = rob[head].metadata.timestamp[14] + rob[head].metadata.timestamp[15];

      Payload pl_print = (rob[head].metadata);
      
      // Update RMT
      int check_rmt_entry = rob[head].dest;

      if(check_rmt_entry != -1) {
        assert(rmt[check_rmt_entry].valid);
        
        if(rmt[check_rmt_entry].ROB_tag == head) {
          rmt[check_rmt_entry].valid = false;
        }
      }

      printf("%lu  ", pl_print.age);
      printf("fu{%d}  ", pl_print.op_type);
      printf("src{%d,%d}  ", rob[head].src1, rob[head].src2);
      printf("dst{%d}  ", rob[head].dest);
      printf("FE{%lu,%lu} ", pl_print.timestamp[0], pl_print.timestamp[1]);
      printf("DE{%lu,%lu}  ", pl_print.timestamp[2], pl_print.timestamp[3]);
      printf("RN{%lu,%lu}  ", pl_print.timestamp[4], pl_print.timestamp[5]);
      printf("RR{%lu,%lu}  ", pl_print.timestamp[6], pl_print.timestamp[7]);
      printf("DI{%lu,%lu}  ", pl_print.timestamp[8], pl_print.timestamp[9]);
      printf("IS{%lu,%lu}  ", pl_print.timestamp[10], pl_print.timestamp[11]);
      printf("EX{%lu,%lu}  ", pl_print.timestamp[12], pl_print.timestamp[13]);
      printf("WB{%lu,%lu}  ", pl_print.timestamp[14], pl_print.timestamp[15]);
      printf("RT{%lu,%lu}", pl_print.timestamp[16], ((total_cycle_count - pl_print.timestamp[16]))+1);
      printf("\n");

      if((rob[head].metadata).age == (unsigned)(final_instruction_number-1)) {is_done = true; break;}

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
