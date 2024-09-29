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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I,   // immediate
  TYPE_U,   // upper immediate
  TYPE_S,   // store
  TYPE_R,   // register
  TYPE_B,   // branch
  TYPE_J,   // jump
  TYPE_N,   // none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immB() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 12) | BITS(i, 7, 7) << 11 | BITS(i, 30, 25) << 5 | BITS(i, 11, 8) << 1; } while(0)
#define immJ() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 20) | BITS(i, 19, 12) << 12 | BITS(i, 20, 20) << 11 | BITS(i, 30, 21) << 1; } while(0)

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I:
      src1R();
      immI();
      break;
    case TYPE_U:
      immU();
      break;
    case TYPE_S:
      src1R();
      src2R();
      immS();
      break;
    case TYPE_R:
      src1R();
      src2R();
    case TYPE_B:
      src1R();
      src2R();
      immB();
      break;
    case TYPE_J:
      immJ();
      break;
    case TYPE_N: break;
    default: panic("unsupported type = %d", type);
  }
}

void log_ftrace(bool is_func_call, vaddr_t current_pc, vaddr_t next_pc);

void ftrace_jump(uint32_t inst_val,  vaddr_t current_pc, vaddr_t next_pc)
{
    // 00008067          	jalr	zero,0(ra)
    if (inst_val == 0x00008067)
    {
        // ret
        log_ftrace(false, current_pc, next_pc);
    }
    else
    {
        // call func
        log_ftrace(true, current_pc, next_pc);
    }
}

static int decode_exec(Decode *s) {
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  int rd = 0; \
  word_t src1 = 0, src2 = 0, imm = 0; \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}
  // instruction pattern
  // INSTPAT(模式字符串, 指令名称, 指令类型, 指令执行操作);
  // 模式字符串中只允许出现4种字符:
  //    0表示相应的位只能匹配0
  //    1表示相应的位只能匹配1
  //    ? 表示相应的位可以匹配0或1
  //    空格是分隔符, 只用于提升模式字符串的可读性, 不参与匹配
  // rd, src1, src2和imm中, 它们分别代表目的操作数的寄存器号码, 两个源操作数和立即数.
  INSTPAT_START();
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = src1 + src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = src1 - src2);
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, R(rd) = ((sword_t)src1 < (sword_t)src2) ? 1 : 0);
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, R(rd) = (src1 < src2) ? 1 : 0);
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, R(rd) = src1 & src2);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = src1 | src2);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = src1 ^ src2);
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, R(rd) = src1 << BITS(src2, 4, 0));
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , R, R(rd) = src1 >> BITS(src2, 4, 0));
  // here can reference to nemu/test/test_SEXT.cpp
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra    , R, R(rd) = (sword_t)src1 >> BITS(src2, 4, 0));

  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, R(rd) = src1 * src2);
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R, R(rd) = (((int64_t)(sword_t)src1 * (int64_t)(sword_t)src2) >> 32));
  // INSTPAT("0000001 ????? ????? 010 ????? 01100 11", mulhsu , R, R(rd) = src1 + src2);
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu  , R, R(rd) = (((uint64_t)src1 * (uint64_t)src2) >> 32));
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , R, R(rd) = (sword_t)src1 / (sword_t)src2);
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , R, R(rd) = src1 / src2);
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R, R(rd) = (sword_t)src1 % (sword_t)src2);
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , R, R(rd) = src1 % src2);


  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti   , I, R(rd) = ((sword_t)src1 < (sword_t)imm) ? 1 : 0);
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, R(rd) = (src1 < imm) ? 1 : 0);
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, R(rd) = src1 & imm);
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori    , I, R(rd) = src1 | imm);
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, R(rd) = src1 ^ imm);
  INSTPAT("0000000 ????? ????? 001 ????? 00100 11", slli   , I, R(rd) = src1 << BITS(imm, 4, 0));
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli   , I, R(rd) = src1 >> BITS(imm, 4, 0));
  //INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai   , I, R(rd) = (sword_t)src1 >> BITS(imm, 4, 0));
  INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai   , I, imm = BITS(imm, 4, 0); R(rd) = (SEXT(BITS(src1, 31, 31), 1)) << (32 - imm) | (src1 >> imm));
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, s->dnpc = (src1 + imm)&(~((vaddr_t)1)); R(rd) = s->pc + 4; IFDEF(CONFIG_FTRACE, ftrace_jump(s->isa.inst.val, s->pc, s->dnpc)););
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));               // TODO: 这里imm需要按有符号算吗？比如应该是减？还是这里都是补码?
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, R(rd) = Mr(src1 + imm, 2));
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb     , I, R(rd) = SEXT(Mr(src1 + imm, 1), 8));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, R(rd) = SEXT(Mr(src1 + imm, 2), 16));
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, R(rd) = SEXT(Mr(src1 + imm, 4), 32));

  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm);

  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, src2));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2));

  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, s->dnpc = s->pc + imm; R(rd) = s->pc + 4;IFDEF(CONFIG_FTRACE, ftrace_jump(s->isa.inst.val, s->pc, s->dnpc)););

  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, if (src1 == src2) { s->dnpc = s->pc + imm; });
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, if (src1 != src2) { s->dnpc = s->pc + imm; });
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt    , B, if ((sword_t)src1 <  (sword_t)src2) { s->dnpc = s->pc + imm; });
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, if ((sword_t)src1 >= (sword_t)src2) { s->dnpc = s->pc + imm; });
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu   , B, if (src1 <  src2) { s->dnpc = s->pc + imm; });
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu   , B, if (src1 >= src2) { s->dnpc = s->pc + imm; });

  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();


  // above code is equivalent to:
  /*
  { const void ** __instpat_end = &&__instpat_end_;
    do {
      uint64_t key, mask, shift;
      pattern_decode("??????? ????? ????? ??? ????? 00101 11", 38, &key, &mask, &shift);
      if ((((uint64_t)s->isa.inst.val >> shift) & mask) == key) {
        {
          decode_operand(s, &rd, &src1, &src2, &imm, TYPE_U);
          R(rd) = s->pc + imm;
        }
        goto *(__instpat_end);
      }
    } while (0);
    // ...
    __instpat_end_: ; }
  */

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
