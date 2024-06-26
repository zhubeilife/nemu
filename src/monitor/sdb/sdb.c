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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

#include <memory/paddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

//  继续运行被暂停的程序
static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  printf("%s-NEMU: has a good night!\n", ANSI_FMT(str(__GUEST_ISA__), ANSI_FG_YELLOW ANSI_BG_RED));
  nemu_state.state = NEMU_QUIT;
  return -1;
}

// 单步执行, si [N] 让程序单步执行N条指令后暂停执行,当N没有给出时, 缺省为1
static int cmd_si(char *args) {
  uint64_t step = 1;
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  if (arg != NULL) {
    // or use sscanf(arg, "%d", &step);
    step = atoi(arg);
  }
  // TODO(attention here)
  // NEMU默认会把单步执行的指令打印出来(这里面埋了一些坑, 你需要RTFSC看看指令是在哪里被打印的), 这样你就可以验证单步执行的效果了.
  cpu_exec(step);
  return 0;
}

// 打印程序状态, info SUBCMD,	info r: 打印寄存器状态, info w: 打印监视点
static int cmd_info(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  if (arg == NULL)
  {
    printf("Please Give subcmd, r for regs, w for watch point\n");
  }
  else if (strcmp(arg, "r") == 0)
  {
    isa_reg_display();
  }
  else if (strcmp(arg, "w") == 0)
  {
    // TODO: implement
    printf("Sorry waiting implemention!\n");
  }
  else
  {
    printf("Unknown command '%s'\n", arg);
  }

  return 0;
}

// 扫描内存, x N EXPR, x 10 $esp	求出表达式EXPR的值, 将结果作为起始内存地址, 以十六进制形式输出连续的N个4字节
static int cmd_x(char *args) {
  /* extract the first argument */
  char *arg_size = strtok(NULL, " ");
  if (arg_size == NULL)
  {
    printf("Please Give size and expr\n");
    return 0;
  }
  int size = atoi(arg_size);
  if (size <= 0)
  {
    printf("Please Give right size\n");
    return 0;
  }

  char *arg_expr = strtok(NULL, " ");
  // TODO(now just think expr is just a int)
  int base_addr = strtol(arg_expr, NULL, 16);

  if (!in_pmem(base_addr) || !in_pmem(base_addr + 4*size))
  {
    printf("Memroy add is out of boundry\n");
    return 0;
  }

  printf("memory from %x to %x\n", base_addr, base_addr + 4*size);
  for (int i = 0; i < size; i++)
  {
    printf(FMT_WORD "  ", paddr_read(base_addr + 4*i, 4));
  }
  printf("\n");

  return 0;
}

// 表达式求值, p EXPR,	p $eax + 1	求出表达式EXPR的值, EXPR支持的运算请见调试中的表达式求值小节
static int cmd_p(char *args) {
  if (args == NULL)
  {
    printf("Please Give Right Expr!\n");
    return 0;
  }

#ifndef TEST_EXPR
  bool ret;
  word_t result = expr(args, &ret);

  printf("Result: %u\n", result);
  return 0;

#else
  // for test expr
  char *arg_size = strtok(NULL, " ");
  if (arg_size == NULL)
  {
    printf("Please Give size and expr\n");
    return 0;
  }
  int input_result = atoi(arg_size);

  char *expr_args = arg_size + strlen(arg_size) + 1;
  bool ret;
  word_t result = expr(expr_args, &ret);

  if (result == input_result)
  {
    printf("right!\n");
  }
  else
  {
    printf("wrong!\n");
    Assert(0, "get wrong");
  }
  printf("Result: %d\n", result);
  return 0;
#endif

}

// 设置监视点,	w EXPR	w *0x2000,	当表达式EXPR的值发生变化时, 暂停程序执行
static int cmd_w(char *args) {

  return 0;
}

// 删除监视点, d N,	d 2	删除序号为N的监视点
static int cmd_d(char *args) {

  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Step Instruction", cmd_si },
  { "info", "Infomation", cmd_info },
  { "x", "Examines the data located in memory at address", cmd_x },
  { "p", "Prints the value which the indicated expression evaluates to as a hexadecimal number", cmd_p },
  { "w", "Add Watch Point", cmd_w },
  { "d", "Delte Watch Point", cmd_d },
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
