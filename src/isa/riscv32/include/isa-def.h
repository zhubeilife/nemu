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

/*
Arch/ABI    Instruction           System  Ret  Ret  Error    Notes
                                  call #  val  val2
───────────────────────────────────────────────────────────────────
arm/OABI    swi NR                -       r0   -    -        2
arm/EABI    swi 0x0               r7      r0   r1   -
arm64       svc #0                w8      x0   x1   -
i386        int $0x80             eax     eax  edx  -
ia64        break 0x100000        r15     r8   r9   r10      1, 6
mips        syscall               v0      v0   v1   a3       1, 6
riscv       ecall                 a7      a0   a1   -
*/

typedef enum {
  //mcause 的最高位在发生中断时置 1,发生同步异常时置 0
  INSTRUCTION_ADDRESS_MISALIGNED = 0,  // 指令地址未对齐
  INSTRUCTION_ACCESS_FAULT       = 1,  // 指令访问故障
  ILLEGAL_INSTRUCTION            = 2,  // 非法指令
  BREAKPOINT                     = 3,  // 断点
  LOAD_ADDRESS_MISALIGNED        = 4,  // 加载地址未对齐
  LOAD_ACCESS_FAULT              = 5,  // 加载访问故障
  STORE_ADDRESS_MISALIGNED       = 6,  // 存储地址未对齐
  STORE_ACCESS_FAULT             = 7,  // 存储访问故障
  ENVIRONMENT_CALL_FROM_U_MODE   = 8,  // 用户模式的环境调用
  ENVIRONMENT_CALL_FROM_S_MODE   = 9,  // 管理模式的环境调用（若存在）
  ENVIRONMENT_CALL_FROM_M_MODE   = 11, // 机器模式的环境调用
  INSTRUCTION_PAGE_FAULT         = 12, // 指令页面故障
  LOAD_PAGE_FAULT                = 13, // 加载页面故障
  STORE_PAGE_FAULT               = 15, // 存储页面故障
  // 中断的最高位为1，这里使用更大的数值表示
  INTERRUPT_MACHINE_TIMER        = 0x80000007, // 机器定时器中断
  INTERRUPT_MACHINE_EXTERNAL     = 0x8000000b  // 机器外部中断
} RiscV_ExceptionCode;

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
