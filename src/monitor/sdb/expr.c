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
#include <memory/paddr.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

// here a clever choice to let TK_NOTYPE from 256 to avoid
// conflict with asic charater
enum {
  TK_NOTYPE = 256, TK_EQ, TK_DEC_NUM, TK_HEX_NUM,
      TK_REG_NAME, TK_NEGATIVE, TK_DEREFENCE, TK_LESS_EQ,
      TK_LOGIC_AND,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"-", '-'},           // minus
  {"\\*", '*'},         // multi
  {"\\/", '/'},         // sub
  {"\\(", '('},         // left bracket
  {"\\)", ')'},         // right bracket
  {"==", TK_EQ},        // equal
  {"<=", TK_LESS_EQ},   // less equal
  {"&&", TK_LOGIC_AND}, // logic and
  {"\\$\\$?[0-9A-Za-z]+", TK_REG_NAME},         // reg name
  {"0[xX][0-9A-Fa-f]+", TK_HEX_NUM},        // hex number
  {"[[:digit:]]+", TK_DEC_NUM},             // dec number
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

// tokens数组用于按顺序存放已经被识别出的token信息
static Token tokens[32] __attribute__((used)) = {};
// nr_token指示已经被识别出的token数目.
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          default:
            if (substr_len > FIELD_ARRLEN_SIZEOF(Token, str) - 1)
            {
              Assert(0, "sub str is too large with: len: %d: %.*s", substr_len, substr_len, substr_start);
            }
            if (nr_token < ARRLEN(tokens))
            {
              tokens[nr_token].type = rules[i].token_type;
              memcpy(tokens[nr_token].str, substr_start, substr_len);
              tokens[nr_token].str[substr_len] = '\0';
              nr_token++;
            }
            else
            {
              printf("expr is large than %d\n", ARRLEN(tokens));
              return false;
            }
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_inter_parentehese(int start, int end)
{
  // generate the array
  int parent_arry[ARRLEN(tokens)] = {};
  int left_parent = 0;
  int right_parent = 0;
  for (int i = start; i <= end; i++)
  {
    int op_type = tokens[i].type;
    if (op_type == ')' || op_type == '(')
    {
      op_type == '(' ? left_parent++ : right_parent++;
      parent_arry[i] = op_type;
    }
    else
    {
      // just use TK_NOTYPE as ident
      parent_arry[i] = TK_NOTYPE;
    }
  }

  // before start there should be same
  if (left_parent != right_parent)
  {
    return false;
  }
  else if (left_parent == 0)
  {
    return true;
  }

  // check left ones
  bool find_pair = false;
  do
  {
    find_pair = false;
    for (int i = start; i <= end; i++)
    {
      if (parent_arry[i] == '(')
      {
        for(int j = i + 1; j <= end; j++)
        {
          if (parent_arry[j] == '(')
          {
            break;
          }
          else if (parent_arry[j] == ')')
          {
            find_pair = true;
            left_parent--;
            parent_arry[i] = TK_NOTYPE;
            parent_arry[j] = TK_NOTYPE;
            break;
          }
        }
      }
    }
  } while (find_pair);

  return left_parent == 0 ? true : false;
}

bool check_parentheses(int p, int q) {
  // first the who exp is surround by ()
  if (tokens[p].type != '(' || tokens[q].type != ')')
  {
    return false;
  }

  return check_inter_parentehese(p+1, q-1);
}

bool check_negavitve(int p, int q)
{
  if ((q - p) == 1 && tokens[p].type == '-')
  {
    return true;
  }
  else
  {
    return false;
  }
}

void show_debug_info(int p, int q)
{
  int size = 0;
  int p_size = 0;
  int q_size = 0;
  printf("\n");
  for (int i = 0; i < nr_token; i++)
  {
    printf("%s ", tokens[i].str);
    if (p == i) p_size = size;
    if (q == i) q_size = size;
    size += strlen(tokens[i].str) + 1;
  }
  printf("\n");
  printf("%*c", p_size, ' ');
  printf("^");
  printf("%*c", q_size - p_size -1, ' ');
  printf("^");
  printf("\n");
  printf("debug info p: %d, q: %d\n", p, q);
}

bool get_main_op(int p, int q, int *main_pos)
{
  int pos = p;
  int type = TK_NOTYPE;

  for (int i = p; i <= q; i++)
  {
    int op_type = tokens[i].type;
    switch (op_type)
    {
      // not deal something like (+1) where + means positive
      case '+':
      case '-':
        type = op_type;
        pos = i;
        break;
      case '*':
      case '/':
        if (type != '+' && type != '-')
        {
          type = op_type;
          pos = i;
        }
        break;
      case '(':
        // find coresponding ')'
          int left_count = 1;
          int right_count = 0;
          for (int j = i+1; j <= q; j++)
          {
            int temp_token = tokens[j].type;
            if (temp_token == '(')
            {
              left_count ++;
            }
            else if (temp_token == ')')
            {
              right_count++;
            }
            if (left_count == right_count)
            {
              i = j;
              break;
            }
          }
        break;
      case ')':
        // should not happend
        show_debug_info(p,q);
        Assert(0, "Should not happend");
        return false;
        break;
    }
  }

  if (type == TK_NOTYPE)
  {
    printf("find no main op");
    show_debug_info(p, q);
    return false;
  }
  // printf("debug: get main on %d %s\n", pos, tokens[pos].str);
  *main_pos = pos;
  return true;
}

word_t eval(int p, int q, bool *success)
{
  if (p > q) {
    /* Bad expression */
    Assert(0, "Bad expression pos p: %d, q: %d", p, q);
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    switch (tokens[p].type)
    {
      // TODO: here not deal with if strol fail
    case TK_HEX_NUM:
      return strtol(tokens[p].str, NULL, 16);;
      break;
    case TK_DEC_NUM:
      return strtol(tokens[p].str, NULL, 10);
      break;
    case TK_REG_NAME:
      word_t reg_val = isa_reg_str2val(tokens[p].str + 1, success);
      if (*success)
      {
        return reg_val;
      }
      else
      {
        printf("Can't get reg %s value", tokens[p].str + 1);
        return 0;
      }
      break;
    default:
      *success = false;
      printf("Expression is not a number with %s", tokens[p].str);
      return 0;
    }
  }
  else if (check_parentheses(p, q)) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1, success);
  }
  else if (q - p ==1)
  {
    switch (tokens[p].type)
    {
    case TK_NEGATIVE:
      return -eval(q, q, success);
      break;
    case TK_DEREFENCE:
      paddr_t addr = eval(q, q, success);
      if (*success)
      {
        if (in_pmem(addr))
        {
          return paddr_read(addr, 4);
        }
        else
        {
          *success = false;
          return 0;
        }
      }
      else
      {
        return 0;
      }
      break;
    default:
      Assert(0, "should not happend");
    }
  }
  else {
    int main_pos = p;
    if (get_main_op(p, q, &main_pos))
    {
      int op_type = tokens[main_pos].type;
      word_t val1 = eval(p, main_pos - 1, success);
      word_t val2 = eval(main_pos + 1, q, success);

      switch (op_type) {
        case '+': return val1 + val2;
        case '-': return val1 - val2;
        case '*': return val1 * val2;
        case '/': return val1 / val2;
        default: Assert(0, "Not support op type %s", tokens[main_pos].str);
      }
    }
    else
    {
      *success = false;
      printf("Can't get main op with p: %d, q: %d", p, q);
      return 0;
    }
  }
}

bool check_pre_type(int i)
{
  return tokens[i].type == '+' || tokens[i].type == '-' || tokens[i].type == '*' || tokens[i].type == '/' || tokens[i].
    type == '(';
}

word_t expr(char *e, bool *success) {
  // first set success is true, any thing wrong happend
  // will set it to false
  *success = true;
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  // maybe need to check after type is a num
  for (int i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '-' && (i == 0 || check_pre_type(i-1)) )
    {
      tokens[i].type = TK_NEGATIVE;
    }
    else if (tokens[i].type == '*' && (i == 0 || check_pre_type(i-1)) )
    {
      tokens[i].type = TK_DEREFENCE;
    }
  }

  return eval(0, nr_token - 1, success);
}
