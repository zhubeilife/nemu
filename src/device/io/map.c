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
#include <memory/host.h>
#include <memory/vaddr.h>
#include <device/map.h>

#define IO_SPACE_MAX (32 * 1024 * 1024)

static uint8_t *io_space = NULL;
static uint8_t *p_space = NULL;

uint8_t* new_space(int size) {
  uint8_t *p = p_space;
  // page aligned;
  size = (size + (PAGE_SIZE - 1)) & ~PAGE_MASK;
  p_space += size;
  assert(p_space - io_space < IO_SPACE_MAX);
  return p;
}

static bool check_bound(IOMap *map, paddr_t addr) {
  if (map == NULL) {
    Log("%s address (" FMT_PADDR ") is out of bound at pc = " FMT_WORD, ANSI_FMT("Quit Due to", ANSI_FG_YELLOW), addr, cpu.pc);
    return true;
    //Assert(map != NULL, "address (" FMT_PADDR ") is out of bound at pc = " FMT_WORD, addr, cpu.pc);
  } else {
    if (!(addr <= map->high && addr >= map->low)) {
      Log("%s address (" FMT_PADDR ") is out of bound {%s} [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
          ANSI_FMT("Quit Due to", ANSI_FG_YELLOW), addr, map->name, map->low, map->high, cpu.pc);
      return true;
    }
    // Assert(addr <= map->high && addr >= map->low,
        // "address (" FMT_PADDR ") is out of bound {%s} [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
        // addr, map->name, map->low, map->high, cpu.pc);
  }
  return false;
}

static void invoke_callback(io_callback_t c, paddr_t offset, int len, bool is_write) {
  if (c != NULL) { c(offset, len, is_write); }
}

void init_map() {
  io_space = malloc(IO_SPACE_MAX);
  assert(io_space);
  p_space = io_space;
}

void log_device(IOMap *map, bool is_write)
{
  if (is_write)
  {
    log_write("@@@ write ");
  }
  else
  {
    log_write("@@@ read  ");
  }

  log_write("%s @@@\n", map->name);
}

void set_nemu_state(int state, vaddr_t pc, int halt_ret);

word_t map_read(paddr_t addr, int len, IOMap *map) {
  assert(len >= 1 && len <= 8);
  if (check_bound(map, addr)) {
    set_nemu_state(NEMU_ABORT, cpu.pc, -1);
    return 0;
  }
  paddr_t offset = addr - map->low;
  IFDEF(CONFIG_DTRACE, log_device(map, false));
  invoke_callback(map->callback, offset, len, false); // prepare data to read
  word_t ret = host_read(map->space + offset, len);
  return ret;
}

void map_write(paddr_t addr, int len, word_t data, IOMap *map) {
  assert(len >= 1 && len <= 8);
  if (check_bound(map, addr)) {
    set_nemu_state(NEMU_ABORT, cpu.pc, -1);
    return;
  }
  paddr_t offset = addr - map->low;
  host_write(map->space + offset, len, data);
  IFDEF(CONFIG_DTRACE, log_device(map, true));
  invoke_callback(map->callback, offset, len, true);
}
