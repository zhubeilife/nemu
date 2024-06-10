/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
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
#include "local-include/reg.h"

/*
+-----------+---------------+-------------------------+--------------------------+
|   Name    |  ABI Mnemonic |         Meaning         |  Preserved across calls? |
+-----------+---------------+-------------------------+--------------------------+
| x0        |  zero         |  Zero                   |   —(Immutable)           |
| x1        |  ra           |  Return address         |  No                      |
| x2        |  sp           |  Stack pointer          |  Yes                          |
| x3        |  gp           |  Global pointer         |   — (Unallocatable)      |
| x4        |  tp           |  Thread pointer         |   — (Unallocatable)      |
| x5 - x7   |  t0 - t2      |  Temporary registers    |  No                      |
| x8 - x9   |  s0 - s1      |  Callee-saved registers |  Yes                     |
| x10 - x17 |  a0 - a7      |  Argument registers     |  No                      |
| x18 - x27 |  s2 - s11     |  Callee-saved registers |  Yes                     |
| x28 - x31 |  t3 - t6      |  Temporary registers    |  No                      |
+-----------+---------------+-------------------------+--------------------------+
*/

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
  printf("regs:\n");
  printf("(%s: " FMT_WORD  ")  ", "pc", cpu.pc);
  for (int i = 0; i < MUXDEF(CONFIG_RVE, 16, 32); i++)
  {
    printf("(%s: " FMT_WORD ")  ", regs[i], cpu.gpr[i]);
  }
  printf("\n");
}

word_t isa_reg_str2val(const char *s, bool *success) {
  return 0;
}
