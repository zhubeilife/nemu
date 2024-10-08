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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#define ARRLEN(arr) (int)(sizeof(arr) / sizeof(arr[0]))

// this should be enough
static char buf[65536] = {};
static int buf_index = 0;
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  int result = %s; "
"  printf(\"%%d\", result); "
"  return 0; "
"}";

int choose(int range)
{
  return rand() % range;
}

void gen_num()
{
  // max 32 bit num is 4294967295, use 10bytes
  uint32_t num = (uint32_t)choose(100);
  sprintf(buf+buf_index, "%3d", num);
  buf_index = buf_index + 3;
}

void gen_rand_op()
{
  switch (choose(3))
  {
  case 0:
    buf[buf_index] = '+';
    break;
  case 1:
    buf[buf_index] = '-';
    break;
  case 2:
    buf[buf_index] = '*';
    break;
  // ignore to generate divide reuslt
  // case 3:
  //   buf[buf_index] = '/';
  //   break;
  }
  buf_index++;
}

void gen(char c)
{
  buf[buf_index] = c;
  buf_index++;
}

void gen_rand_expr()
{
  if (buf_index > ARRLEN(buf))
  {
    return;
  }
  switch (choose(3))
  {
  case 0:
    gen_num();
    break;
  case 1:
    gen('(');
    gen_rand_expr();
    gen(')');
    break;
  default:
    gen_rand_expr();
    gen_rand_op();
    gen_rand_expr();
    break;
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  for (int i = 0; i < loop;) {

    buf_index = 0;
    gen_rand_expr();
    buf[buf_index+1] = '\0';
    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc -Werror /tmp/.code.c -o /tmp/.expr > /dev/null 2>&1");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    if(fp == NULL) continue;

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("p %d %s\n", result, buf);

    i++;
  }
  return 0;
}
