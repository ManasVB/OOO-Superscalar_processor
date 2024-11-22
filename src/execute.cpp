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

// Issue up to WIDTH oldest instructions from the IQ
void Issue(unsigned long int width) {
  uint8_t instrs_removed = 0;

  for(auto &instr: iq) {
     
    if(instr.valid && instr.rs1_rdy && instr.rs2_rdy) {

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
  wakeup.clear();

  // Remove the instruction from the execute_list, which is finishing and add to WB_Reg
  for(auto &instr: execute_list) {
    if(instr.valid && instr.latency == 1) {

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

// Process the writeback bundle in WBFor each instruction in WB, mark the instruction as “ready” in its entry in the ROB.
void Writeback() {
  for(auto &instr: WB_Reg) {
    if(instr.valid) {
      rob[instr.dst_tag].rdy = true;
      instr.valid = false;
    }
  }
}

void Retire(unsigned long int rob_size, unsigned long int width) {

  uint32_t instructions_retired = 0;

  while((head != tail) || is_rob_full) {
    if(rob[head].rdy) {

      // // Update RMT
      // for(auto &itr: rmt) {
      //   if(itr.valid && itr.ROB_tag == head) {
      //     itr.valid = false;
      //     break;
      //   }
      // }
      
      // Update RMT
      int check_rmt_entry = rob[head].dst;

      if(check_rmt_entry != -1) {
        if(rmt[check_rmt_entry].valid && rmt[check_rmt_entry].ROB_tag == head) {
          rmt[check_rmt_entry].valid = false;
        }
      }
      
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
