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

#include <isa.h>

extern char* find_record_func_name(vaddr_t next_pc);
extern int find_record_func_sym(vaddr_t next_pc);

static inline void etrace() {
#ifdef CONFIG_ETRACE
  log_write("[ETRACE]: deal exception No: %d", cpu.csr.mcause);
  int index = find_record_func_sym(cpu.csr.mepc);
  if (index == -1) {
    log_write("\n");
  }
  else {
    log_write(", when running func: %s\n", find_record_func_name(cpu.csr.mepc));
  }
#endif
}

// riscv32触发异常后硬件的响应过程如下:
// 将当前PC值保存到mepc寄存器
// 在mcause寄存器中设置异常号
// 从mtvec寄存器中取出异常入口地址
// 跳转到异常入口地址
word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
  cpu.csr.mcause = NO;
  // switch (NO) {
  //   case 11: epc += 4;break;
  // }
  cpu.csr.mepc = epc;

  cpu.csr.mstatus.part.MPP = 3;
  cpu.csr.mstatus.part.MPIE = cpu.csr.mstatus.part.MIE;
  cpu.csr.mstatus.part.MIE = 0;

  etrace();

  return cpu.csr.mtvec;
}

word_t isa_query_intr() {
  return INTR_EMPTY;
}
