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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  char expr[64];
  word_t value;
  struct watchpoint *next;
} WP;

static WP wp_pool[NR_WP] = {};
static size_t free_wp_size = NR_WP;
// head用于组织使用中的监视点结构, free_用于组织空闲的监视点结构
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

void show_watch_point()
{
  printf("=== WatchPioint ===\n");
  printf("%4s %10s %14s\n", "NO", "expr", "vaule");
  WP * t_head = head;
  while (t_head != NULL)
  {
    printf("%4d %10s     " FMT_WORD"\n", t_head->NO, t_head->expr, t_head->value);
    t_head = t_head->next;
  }
  printf("\n");
}

bool check_wp_value_chage()
{
  bool changed = false;
  WP * t_head = head;
  while (t_head != NULL)
  {
    bool sucess = false;
    word_t new_value = expr(t_head->expr, &sucess);
    if (new_value != t_head->value && sucess)
    {
      changed = true;
      t_head->value = expr(t_head->expr, &sucess);
    }
    t_head = t_head->next;
  }
  return changed;
}

void show_wp_debug_info()
{
  printf("\n============---Watch Point Pool---============\n");

  printf("head: ");
  WP * t_head = head;
  while (t_head != NULL)
  {
    printf("-->%d ", t_head->NO);
    t_head = t_head->next;
  }
  printf("\n");

  printf("free: ");
  WP * t_free = free_;
  while (t_free != NULL)
  {
    printf("-->%d ", t_free->NO);
    t_free = t_free->next;
  }
  printf("\n");

  printf("\n============---end---============\n");
}

bool new_wp(WP **wp, char *expr_str)
{
  if (free_wp_size == 0)
  {
    return false;
  }
  // get topesst free node
  *wp = free_;
  // free node --
  free_wp_size--;
  free_ = (*wp)->next;
  // set wp next to null and add to tail of head
  (*wp)->next = NULL;
  strcpy((*wp)->expr, expr_str);
  // set value
  bool sucess = false;
  (*wp)->value = expr(expr_str, &sucess);

  // add wp to head
  if (head == NULL)
  {
    head = *wp;
  }
  else
  {
    WP *h_wp = head;
    while(h_wp->next != NULL)
    {
      h_wp = h_wp->next;
    }
    h_wp->next = *wp;
  }

  return true;
}

bool add_new_expr_wp(char *args_expr)
{
  bool sucess = false;
  expr(args_expr, &sucess);
  if (!sucess)
  {
    printf("Can't find expr: %s\n", args_expr);
    return false;
  }
  WP* wp;
  if (new_wp(&wp, args_expr))
  {
    printf("Add watch point NO:%d %s\n", wp->NO, args_expr);
    return true;
  }
  else
  {
    return false;
  }
}

bool free_wp(WP **wp, int delete_no)
{
  if (head == NULL)
  {
    show_wp_debug_info();
    Assert(0, "the head is null should not can free!");
    return false;
  }

  // find pre wp at head
  bool find_pre = false;
  if (head->NO == delete_no)
  {
    *wp = head;
    head = (*wp)->next;
    find_pre = true;
  }
  else
  {
    WP *pre = head;
    while (pre->next != NULL)
    {
      if (pre->next->NO == delete_no)
      {
        find_pre = true;
        *wp = pre->next;
        pre->next = (*wp)->next;
        break;
      }
      else
      {
        pre = pre->next;
      }
    }
  }

  if (!find_pre)
  {
    show_wp_debug_info();
    printf("not find wp NO:%d\n", delete_no);
    return false;
  }

  // place back wp at head of free
  free_wp_size++;
  if (free_ == NULL)
  {
    free_ = *wp;
    (*wp)->next = NULL;
  }
  else
  {
    WP * t_free = free_;
    free_ = *wp;
    free_->next = t_free;
  }

  return true;
}

bool del_wp_no(int NO)
{
  WP *delete_wp;
  if (free_wp(&delete_wp, NO))
  {
    printf("Delete watch point: %d %s\n", delete_wp->NO, delete_wp->expr);
    return true;
  }
  else
  {
    return false;
  }
}
