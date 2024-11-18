#ifndef SIM_PROC_H
#define SIM_PROC_H

#include <cstdint>
#include <map>

#pragma pack(1)
typedef struct proc_params{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
}proc_params;

typedef struct {
    uint8_t dst;    // R0-R66
    uint32_t pc;
    bool rdy;
} ROB;

typedef struct {
    bool valid;
    uint32_t dst_tag;

    bool rs1_rdy;
    uint32_t rs1_tag;

    bool rs2_rdy;
    uint32_t rs2_tag;
} IQ;

typedef struct {
    bool valid;
    uint32_t ROB_tag;
} RMT;


#endif
