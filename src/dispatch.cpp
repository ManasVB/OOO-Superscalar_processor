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

extern bool trace_read_complete;
extern bool is_done;

extern uint64_t total_instruction_count;
extern uint64_t total_cycle_count;
extern int64_t final_instruction_number;

extern uint32_t head, tail;
extern bool is_rob_full;

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

        // Set latency according to op_type
        if(op_type == 0)
          latency = 1;
        else if(op_type == 1)
          latency = 2;
        else
          latency = 5;

        DE_REG.push_back({total_instruction_count, op_type, latency, dest, false, src1, false, src2, {total_cycle_count,0}});

        ++line_count;
        ++total_instruction_count;
      } else {
        final_instruction_number = total_instruction_count;
        trace_read_complete = true;
        break;
      }
    }
  }
}

void Decode() {
  if(!DE_REG.empty()) {
    
    // timestamp[3] is the time spent in decode stage
    for(auto &instr: DE_REG) {
      ++instr.timestamp[3];
    }

    if(RN_REG.empty()) {
      for(auto &instr: DE_REG) {

        // timestamp[1] is the time spent in fetch stage
        instr.timestamp[1] = 1;

        // Decode stage begin cycle = fetch + 1
        instr.timestamp[2] = instr.timestamp[0] + instr.timestamp[1];

      }

      RN_REG = DE_REG;
      DE_REG.clear();
    }
  }
}

void Rename(unsigned long int rob_size) {

  if(!RN_REG.empty()) {
    uint32_t free_spots = 0;

    if(!is_rob_full) {
      if(tail >= head)
        free_spots = rob_size - (tail - head);
      else
        free_spots = head - tail;
    }

    bool is_rob_free = (RN_REG.size() <= free_spots);

    // timestamp[5] is the time spent in rename stage
    for(auto &instr: RN_REG) {
      ++instr.timestamp[5];
    }

    if(RR_REG.empty() && is_rob_free) {
      for(auto &instr: RN_REG) {

        // Rename stage begin cycle = Decode begin + decode spent
        instr.timestamp[4] = instr.timestamp[2] + instr.timestamp[3];

        // allocate an entry in the ROB for the instruction
        rob[tail] = {.rdy = false, .dest = instr.dest, .src1 = instr.src1, .src2 = instr.src2};

        // rename its source registers
        if(instr.src1 != -1) {
          instr.src1 = (rmt[instr.src1].valid) ? rmt[instr.src1].ROB_tag : -1;
        }

        if(instr.src2 != -1) {
          instr.src2 = (rmt[instr.src2].valid) ? rmt[instr.src2].ROB_tag : -1;
        }

        if(instr.dest != -1) {
          rmt[instr.dest] = {.valid = true, .ROB_tag = tail};
        }
        instr.dest = tail; //dst now becomes dst_tag

        tail = (tail + 1)%rob_size;

        if(head == tail) {
          is_rob_full = true;
        }
      }

      RR_REG = RN_REG;
      RN_REG.clear();
    }
  }
}

void RegRead() {
  if(!RR_REG.empty()) {

    // timestamp[7] is the time spent in RegRead stage
    for(auto &instr: RR_REG) {
      ++instr.timestamp[7];
    }

    if(DI_REG.empty()) {
      for(auto &instr: RR_REG) {
        
        // RegRead stage begin cycle = RN begin + RN timespent
        instr.timestamp[6] = instr.timestamp[4] + instr.timestamp[5];

        if(instr.src1 == -1) {
          instr.src1_rdy = true;
        }

        if(instr.src2 == -1) {
          instr.src2_rdy = true;
        }

        if(instr.src1 != -1)
          if(rob[instr.src1].rdy)
            instr.src1_rdy = true;

        if(instr.src2 != -1)
          if(rob[instr.src2].rdy)
            instr.src2_rdy = true;

      }

      DI_REG = RR_REG;
      RR_REG.clear();
    }
  }
}

void Dispatch() {
  if(!DI_REG.empty()) {

    // timestamp[9] is the time spent in Dispatch stage
    for(auto &instr: DI_REG) {
      ++instr.timestamp[9];
    }

    uint8_t iq_free_entries = 0;
    bool is_iq_free = false;

    //check for free entries in the IQ
    for(auto &iq_itr: iq) {
      if(!iq_itr.valid)
        ++iq_free_entries;

      if(iq_free_entries >= DI_REG.size()) {
        is_iq_free = true;
        break;
      }
    }

    
    if(is_iq_free) {

      uint8_t bundle_count = 0;

      for(auto &iq_itr: iq) {

         if(!iq_itr.valid) {
          iq_itr.valid = true;

          // Dispatch stage begin cycle
          DI_REG[bundle_count].timestamp[8] = DI_REG[bundle_count].timestamp[6] + DI_REG[bundle_count].timestamp[7];

          // Add to IQ
          iq_itr.age = DI_REG[bundle_count].age;
          iq_itr.payload = DI_REG[bundle_count];

          ++bundle_count;
         }

        if(bundle_count == DI_REG.size())
          break; 
      }

      // Sort the issue queue according on age in ascending order
      std::sort(iq.begin(), iq.end(), [](const IQ &a, const IQ&b) {return a.age < b.age;});
      
      DI_REG.clear();
    }
  }
}
