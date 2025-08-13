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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <cpu/difftest.h>

#include <memory/paddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
void show_watch_point();
bool add_new_expr_wp(char *args_expr);
bool del_wp_no(int NO);

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
    show_watch_point();
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
  bool success;
  word_t base_addr = expr(arg_expr, &success);
  if (!success)
  {
    printf("Can't get expr %s", arg_expr);
    return 0;
  }

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

  printf("Result hex: %x dec: %d\n", result, result);
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
  char *arg_expr = strtok(NULL, " ");
  add_new_expr_wp(arg_expr);
  return 0;
}

// 删除监视点, d N,	d 2	删除序号为N的监视点
static int cmd_d(char *args) {
  char *arg_no = strtok(NULL, " ");
  if (arg_no == NULL)
  {
    printf("Please Give no\n");
    return 0;
  }
  int no = atoi(arg_no);
  if (no < 0)
  {
    printf("Please Give right no\n");
    return 0;
  }
  del_wp_no(no);

  return 0;
}

static int cmd_detach(char *args) {
  IFDEF(CONFIG_DIFFTEST, difftest_detach());
  printf("Difftest detach\n");
  return 0;
}

static int cmd_attach(char *args) {
  IFDEF(CONFIG_DIFFTEST, difftest_attach());
  printf("Difftest attach\n");
  return 0;
}

void load_user_elf(char* file_name, int file_offset);
static int cmd_ftrace(char *args) {
  /* extract the first argument */

  char *arg_filename = strtok(NULL, " ");
  if (arg_filename == NULL)
  {
    printf("Please Give file path and offset\n");
    return 0;
  }

  char *arg_size = strtok(NULL, " ");
  if (arg_size == NULL)
  {
    printf("Please Give file path and offset\n");
    return 0;
  }
  int size = atoi(arg_size);
  if (size <= 0)
  {
    printf("Please Give right size\n");
    return 0;
  }

  Log("Load user elf: %s with offset: %d", arg_filename, size);
  load_user_elf(arg_filename, size);
  return 0;
}

// 比较暴力的直接把cpu和内存的都给保存下来了
// TODO未经过测试
static int cmd_save(char *args) {
  printf("Save current status to file\n");
  FILE *fptr;
  fptr = fopen("nemu_screenshot", "wb");
  
  if (fptr == NULL) {
    printf("Failed to open file for writing\n");
    return 0;
  }
  
  // Save CPU state
  fwrite(&cpu, sizeof(CPU_state), 1, fptr);
  
  // Save physical memory by reading it in chunks
  // We need to read from CONFIG_MBASE to CONFIG_MBASE + CONFIG_MSIZE
  
  uint8_t buffer[4096]; // 4KB buffer
  paddr_t addr = CONFIG_MBASE;
  size_t remaining = CONFIG_MSIZE;
  
  while (remaining > 0) {
    size_t chunk_size = (remaining < 4096) ? remaining : 4096;
    
    // Read memory in chunks
    for (size_t i = 0; i < chunk_size; i += 4) {
      if (i + 4 <= chunk_size) {
        word_t data = paddr_read(addr + i, 4);
        buffer[i] = data & 0xFF;
        buffer[i + 1] = (data >> 8) & 0xFF;
        buffer[i + 2] = (data >> 16) & 0xFF;
        buffer[i + 3] = (data >> 24) & 0xFF;
      } else {
        // Handle remaining bytes
        for (size_t j = i; j < chunk_size; j++) {
          buffer[j] = paddr_read(addr + j, 1) & 0xFF;
        }
      }
    }
    
    fwrite(buffer, 1, chunk_size, fptr);
    addr += chunk_size;
    remaining -= chunk_size;
  }
  
  fclose(fptr);
  printf("Status saved to nemu_screenshot\n");
  return 0;
}

// TODO 如果difftest打开了，需要坐下恢复
static int cmd_load(char *args) {
  printf("Load status from file\n");
  FILE *fptr;
  fptr = fopen("nemu_screenshot", "rb");
  
  if (fptr == NULL) {
    printf("Failed to open file for reading\n");
    return 0;
  }
  
  // Load CPU state
  fread(&cpu, sizeof(CPU_state), 1, fptr);
  
  // Load physical memory by writing it in chunks
  uint8_t buffer[4096]; // 4KB buffer
  paddr_t addr = CONFIG_MBASE;
  size_t remaining = CONFIG_MSIZE;
  
  while (remaining > 0) {
    size_t chunk_size = (remaining < 4096) ? remaining : 4096;
    
    // Read chunk from file
    size_t bytes_read = fread(buffer, 1, chunk_size, fptr);
    if (bytes_read != chunk_size) {
      printf("Failed to read memory data from file\n");
      fclose(fptr);
      return 0;
    }
    
    // Write memory in chunks
    for (size_t i = 0; i < chunk_size; i += 4) {
      if (i + 4 <= chunk_size) {
        word_t data = buffer[i] | (buffer[i + 1] << 8) | 
                     (buffer[i + 2] << 16) | (buffer[i + 3] << 24);
        paddr_write(addr + i, 4, data);
      } else {
        // Handle remaining bytes
        for (size_t j = i; j < chunk_size; j++) {
          paddr_write(addr + j, 1, buffer[j]);
        }
      }
    }
    
    addr += chunk_size;
    remaining -= chunk_size;
  }
  
  fclose(fptr);
  printf("Status loaded from nemu_screenshot\n");
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
  { "detach", "quit Diff Test", cmd_detach },
  { "attach", "open Diff Test", cmd_attach },
  { "save", "save current status", cmd_save },
  { "load", "load save status", cmd_load },
  { "ft", "load user function elf", cmd_ftrace },
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
