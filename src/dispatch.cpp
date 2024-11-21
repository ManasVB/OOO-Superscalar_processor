// This file includes the In-order fetch/dispatch engine
#include <iostream>
#include <cstdint>
#include <vector>
#include <algorithm>

#include "dispatch.h"

using namespace std;

#define DE_REG  bundle[0]
#define RN_REG  bundle[1]
#define RR_REG  bundle[2]
#define DI_REG  bundle[3]

extern vector<ROB> rob;
extern vector<RMT> rmt;
extern vector<IQ> iq;

extern vector<uint32_t> wakeup;

extern bool trace_read_complete;
extern uint64_t total_instruction_count;
extern uint32_t head, tail;

/* bundle is a matrix of size 4 x width; for 4 pipeline registers in the dispatch engine
 * The no. of rows is 4 because there are 4 pipeline regs in this engine: DE, RN, RR, DI
*/
vector<vector<pipeline_regs_d>> bundle(4);

void Fetch(FILE *FP, unsigned long int width) {

  int op_type, dest, src1, src2;  // Variables are read from trace file
  uint64_t pc; // Variable holds the pc read from input file

  uint32_t line_count = 0;
  uint8_t latency = 0;

  /* If there are more instructions in the trace file and if DE is empty (can accept
   * a new decode bundle), then fetch up to WIDTH instructions from the trace file
   * into DE. Fewer than WIDTH instructions will be fetched only if the trace file
   * has fewer than WIDTH instructions left.
   */ 
  if(DE_REG.empty() && !trace_read_complete) {
    while(line_count < width) {
      if(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF) {
        if(op_type == 0)
          latency = 1;
        else if(op_type == 1)
          latency = 2;
        else
          latency = 5;

        // printf("%lx %d %d %d %d\n", pc, op_type, dest, src1, src2);
        DE_REG.push_back({pc, op_type, dest, src1, src2, false, false, total_instruction_count, latency});
        ++line_count;
        ++total_instruction_count;
      } else {
        trace_read_complete = true;
        break;
      }
    }
  }
}

void Decode() {

  /* If RN is empty (can accept a new rename bundle), then advance the decode bundle
   * from DE to RN. Else do nothing.
  */ 
  if(!DE_REG.empty() && RN_REG.empty()) {
      RN_REG = DE_REG;
      DE_REG.clear();
  }
}

void Rename(unsigned long int rob_size) {

  // Check if ROB has enough free entries to accept the entire rename bundle
  bool is_enough_entries = (tail + RN_REG.size())%rob_size > head;
  
  /* If RR is empty and enough entries exist, process the rename bundle and advance
   * if from RN to RR. Else do nothing.
  */
  if(!RN_REG.empty() && RR_REG.empty() && is_enough_entries) {

    for(auto &instr: RN_REG) {

      // allocate an entry in the ROB for the instruction
      rob[tail] = {.rdy = false, .dst = instr.dst};

      // rename its source registers if not from ARF, else set them as ready
      (rmt[instr.src1].valid) ? instr.src1 = rmt[instr.src1].ROB_tag : instr.src1_rdy = true;

      (rmt[instr.src2].valid) ? instr.src2 = rmt[instr.src2].ROB_tag : instr.src2_rdy = true;
      
      // if destination register exist, update RMT and update instr dst with ROB_Tag
      if(instr.dst != -1) {
        rmt[instr.dst] = {.valid = true, .ROB_tag = tail};
      }
      instr.dst = tail; //dst now becomes dst_tag

      tail = (tail + 1)%rob_size;
    }
    
    RR_REG = RN_REG;
    RN_REG.clear();
  }
}

void RegRead() {

  if(!RR_REG.empty() && DI_REG.empty()) {

    // check wakeup from execute
    if(!wakeup.empty()) {
      for(auto &instr : RR_REG) {
        for(auto &wakeup_dst: wakeup) {
          if(!instr.src1_rdy && instr.src1 == (int)wakeup_dst)
            instr.src1_rdy = true;
          if(!instr.src2_rdy && instr.src2 == (int)wakeup_dst)
            instr.src2_rdy = true;
        }
      }
    }

    DI_REG = RR_REG;
    RR_REG.clear();
  }
}

void Dispatch() {

  if(!DI_REG.empty()) {

    // Check if IQ has enough free entries to accept the entire dispatch bundle
    bool is_enough_entries = false;
    uint8_t free_entries_count = 0;
    for(auto &itr: iq) {
      if(!itr.valid)  // valid bit = 0, means free entry
        ++free_entries_count;
      if(free_entries_count >= DI_REG.size()) {
        is_enough_entries = true;
        break;
      }
    }

    if(is_enough_entries) {
      uint8_t bundle_count = 0;
      for(auto &itr: iq) {
        if(!itr.valid) { // valid bit = 0, means free entry
          itr.valid = true;
          itr.dst_tag = DI_REG[bundle_count].dst;

          // check wakeup from execute
          if(!wakeup.empty()) {
            for(auto &wakeup_dst: wakeup) {
              if(!(DI_REG[bundle_count].src1_rdy) && DI_REG[bundle_count].src1 == (int)wakeup_dst)
                DI_REG[bundle_count].src1_rdy = true;
              if(!(DI_REG[bundle_count].src2_rdy) && DI_REG[bundle_count].src2 == (int)wakeup_dst)
                DI_REG[bundle_count].src2_rdy = true;              
            }
          }

          itr.rs1_rdy = DI_REG[bundle_count].src1_rdy;
          itr.rs2_rdy = DI_REG[bundle_count].src2_rdy;

          if(!DI_REG[bundle_count].src1_rdy)
            itr.rs1_tag = DI_REG[bundle_count].src1;

          if(!DI_REG[bundle_count].src2_rdy)
            itr.rs2_tag = DI_REG[bundle_count].src2;

          itr.age = DI_REG[bundle_count].age;

          itr.latency = DI_REG[bundle_count].latency;

          ++bundle_count;
        }

        if(bundle_count == DI_REG.size())
          break;
      }

      // Sort the issue queue according on age in ascending order
      std::sort(iq.begin(), iq.end(), [](const IQ &a, const IQ&b) {return a.age < b.age;});

      // for(auto &instr: iq) {
      //   cout << "valid: " << instr.valid << " dst_tag: " << instr.dst_tag << " age: " << instr.age << "rs ready: " << instr.rs1_rdy << instr.rs2_rdy << endl;
      // }
      // cout << endl;

      DI_REG.clear();
    }
  }
}
