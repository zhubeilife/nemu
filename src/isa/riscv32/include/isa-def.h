/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>

typedef union {
  struct {
    uint32_t UIE: 1, SIE: 1, WPRI_0: 1, MIE: 1;
    uint32_t UPIE: 1, SPIE: 1, WPRI: 1, MPIE: 1;
    uint32_t SPP: 1, WPRI_1_2: 2, MPP: 2, FS: 2;
    uint32_t XS: 2, MPRV: 1, SUM: 1, MXR: 1;
    uint32_t TVM: 1, TW: 1, TSR: 1, WPRI_3_10: 8, SD: 1;
  } part;
  word_t val;
} mstatus_t;

// CSR Control and Status Register
typedef struct {
  mstatus_t mstatus;   // 0x0300
  word_t mtvec;     // 0x0305
  word_t mepc;      // 0x0341
  word_t mcause;    // 0x0342
}csr_t;

typedef struct {
  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
  vaddr_t pc;
  csr_t csr;
} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);

// decode
typedef struct {
  uint32_t inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);

#define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)

#endif
