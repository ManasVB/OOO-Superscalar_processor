// This file includes the Out-of-order issue/execute engine
#include <iostream>
#include <stdint.h>
#include <vector>
#include <algorithm>
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
extern bool is_done;

extern uint64_t total_cycle_count;
extern uint64_t total_instruction_count;
extern uint64_t final_instruction_count;
// Issue up to WIDTH oldest instructions from the IQ
void Issue(unsigned long int width) {
  uint8_t instrs_removed = 0;

  for(auto &instr: iq) {
     
    if(instr.valid && instr.rs1_rdy && instr.rs2_rdy) {

      rob[instr.dst_tag].payload.begincycle[5] = total_cycle_count;

      // Remove the instruction from the IQ.
      instr.valid = false;

      //Add the instruction to the execute_list
      for(auto &itr: execute_list) {
        if(!itr.valid) {  // not valid means free entry
          itr = { .dst_tag = instr.dst_tag, .src1 = instr.rs1_tag, \
            .src2 = instr.rs2_tag, .latency = instr.latency, .valid = true};
          
          break;
        }
      }
      ++instrs_removed;
    }
    if(instrs_removed == width) {
      break;
    }
  }
}

void Execute() {
  
  uint8_t WB_Reg_Counter = 0;
  // wakeup.clear();

  // Remove the instruction from the execute_list, which is finishing and add to WB_Reg
  for(auto &instr: execute_list) {
    if(instr.valid && instr.latency == 1) {

      rob[instr.dst_tag].payload.begincycle[6] = total_cycle_count;

      // Add to WB_Reg
      WB_Reg[WB_Reg_Counter] = instr;
      ++WB_Reg_Counter;

      // Remove from execute_list
      instr.valid = false;

      // Wakeup dependent sources in the Issue Queue
      for(auto &iq_itr: iq) {
        if(iq_itr.valid && !iq_itr.rs1_rdy && iq_itr.rs1_tag == instr.dst_tag) {
          iq_itr.rs1_rdy = true;
        }

        if(iq_itr.valid && !iq_itr.rs2_rdy && iq_itr.rs2_tag == instr.dst_tag) {
          iq_itr.rs2_rdy = true;
        }
      }

      // Send a wakeup signal to RR and DI
      wakeup.push_back(instr.dst_tag);

    } else if(instr.valid && instr.latency > 1) {
      --(instr.latency);
    }
  }
}

// Process the writeback bundle in WB. For each instruction in WB, mark the instruction as “ready” in its entry in the ROB.
void Writeback() {
  for(auto &instr: WB_Reg) {
    if(instr.valid) {
      rob[instr.dst_tag].rdy = true;
      rob[instr.dst_tag].payload.begincycle[7] = total_cycle_count;
      instr.valid = false;
    }
  }
}

void Retire(unsigned long int rob_size, unsigned long int width) {

  uint32_t instructions_retired = 0;

  while((head != tail) || is_rob_full) {
    if(rob[head].rdy) {

      // cout << rob[head].payload.age << endl;
      rob[head].payload.begincycle[8] = rob[head].payload.begincycle[7] + 1;

      Payload pl_print = rob[head].payload;

      // Update RMT
      int check_rmt_entry = rob[head].dst;

      if(check_rmt_entry != -1) {
        if(rmt[check_rmt_entry].valid && rmt[check_rmt_entry].ROB_tag == head) {
          rmt[check_rmt_entry].valid = false;
        }
      }

      wakeup.erase(std::remove(wakeup.begin(), wakeup.end(), head), wakeup.end());

      printf("%lu  ", pl_print.age);
      printf("fu{%d}  ", pl_print.op_type);
      printf("src{%d,%d}  ", pl_print.src1, pl_print.src2);
      printf("dst{%d}  ", pl_print.dst);
      printf("FE{%lu,%lu} ", pl_print.begincycle[0], (pl_print.begincycle[1] - pl_print.begincycle[0]));
      printf("DE{%lu,%lu}  ", pl_print.begincycle[1], (pl_print.begincycle[2] - pl_print.begincycle[1]));
      printf("RN{%lu,%lu}  ", pl_print.begincycle[2], (pl_print.begincycle[3] - pl_print.begincycle[2]));
      printf("RR{%lu,%lu}  ", pl_print.begincycle[3], (pl_print.begincycle[4] - pl_print.begincycle[3]));
      printf("DI{%lu,%lu}  ", pl_print.begincycle[4], (pl_print.begincycle[5] - pl_print.begincycle[4]));
      printf("IS{%lu,%lu}  ", pl_print.begincycle[5], (pl_print.begincycle[7] - rob[head].payload.latency - pl_print.begincycle[5]));
      printf("EX{%lu,%u}  ", (pl_print.begincycle[7] - rob[head].payload.latency), rob[head].payload.latency);
      printf("WB{%lu,%lu}  ", pl_print.begincycle[7], (pl_print.begincycle[8] - pl_print.begincycle[7]));
      printf("RT{%lu,%lu}\n", pl_print.begincycle[8], ((total_cycle_count - pl_print.begincycle[8]))+1);

      if(rob[head].payload.age == final_instruction_count) {is_done = true; break;}


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
