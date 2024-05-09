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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <ctype.h>
#include "memory/vaddr.h"

enum {
  TK_NOTYPE = 256, TK_EQ,TK_NUM,TK_16NUM,
  TK_UNEQ,TK_AND,TK_DEREF,TK_REG

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"-",'-'},
  {"\\*",'*'},
  {"/",'/'},
  {"\\$[a-z]+[0-9]*",TK_REG},
  {"0[Xx][0-9a-fA-F]+",TK_16NUM},  //匹配16进制数
  {"[[:digit:]]+",TK_NUM},    //数字
  {"\\(",'('},
  {"\\)",')'},
  {"==", TK_EQ},        // equal
  {"!=", TK_UNEQ},
  {"&&", TK_AND},
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

static Token tokens[50000] __attribute__((used)) = {};
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

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        if (rules[i].token_type == TK_NOTYPE)
          break;

        tokens[nr_token].type = rules[i].token_type;
        strncpy(tokens[nr_token].str,substr_start,substr_len);
        tokens[nr_token].str[substr_len] = '\0';
        //注意，str超过32位后会溢出！这里没有写应对溢出的方法，我的想法是将str[32]改为指针
        
        switch (rules[i].token_type) {
          case '*':
            if(nr_token==0 || ((tokens[nr_token-1].type != TK_16NUM && tokens[nr_token-1].type != TK_NUM && tokens[nr_token-1].type != ')' && tokens[nr_token-1].type!=TK_REG)) )
            {
              tokens[nr_token].type = TK_DEREF;
            }
          break;

        }
        nr_token++;
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


//字符栈
typedef struct{
  Token* top;
  Token* base;
  int stacksize;
}SqStack;

//操作数栈
typedef struct
{
  /* data */
  word_t *base;
  word_t *top;
  int stacksize;
}NStack;
//字符栈初始化
static int InitStack (SqStack *S){
  S->base = (Token *) malloc(32 * sizeof(Token));
  if(!S->base) return 1;
  S->top = S->base;
  S->stacksize = 32;
  return 0;
}
//操作数栈初始化
static int InitStack_N(NStack *S){
  S->top=S->base = (word_t *)malloc(32 * sizeof(word_t));
  if(!S->base) return 1;
  S->stacksize = 32;
  return 0;
}

//字符栈入栈
static int Push(SqStack *S,Token e){
  if(S->top - S->base >= S->stacksize){
    Token *new_base = realloc(S->base,(S->stacksize + 8)*sizeof(Token));
    if(!new_base) return 1;
    else S->base=new_base;

    S->top = S->base +S->stacksize;
    S->stacksize += 8;
  }
  *S->top = e;
  S->top++;
  return 0;
}

//运算数栈入栈
static int Push_N(NStack *S,word_t e){
  if (S->top - S->base >= S->stacksize){
    word_t *new_base = (word_t*)realloc(S->base, (S->stacksize + 8)*sizeof(word_t));
    if(!new_base) return 1;
    else S->base = new_base;
    S->top = S->base+ S->stacksize;
    S->stacksize += 8;
  }
  *S->top = e;
  S->top++;
  return 0;
}
//字符栈出栈
static int Pop(SqStack *S,Token *e){
  if(S->top == S->base) return 1;
  S->top--;
  *e = *S->top;
  return 0;
}

//运算数栈出栈
static int Pop_N(NStack *S,word_t *e){
  if(S->top == S->base) return 1;
  S->top--;
  *e = *S->top;
  return 0;
}

static int StackEmpty(SqStack *S){
  if(S->top == S->base)
    return 1;
  else return 0;
}



//str是中缀表达式，p是转换得到的后缀表达式
static void in2post(){
  SqStack s;
  InitStack(&s);
  Token e;
  int i,j=0;
  
  for(i=0;i<nr_token;++i)
  {
    switch (tokens[i].type){
      case TK_NUM:
      case TK_16NUM:
        tokens[j++]=tokens[i];
        break;
      
      
      case TK_REG:
        tokens[j]=tokens[i];
        word_t num=isa_reg_str2val(&tokens[i].str[1],NULL);
        sprintf(tokens[j].str,"%u",num);
        tokens[j].type=TK_NUM;
        ++j;
        break;

      case '(':
        Push(&s,tokens[i]);
        break;
      //注意出栈时接收变量的e是个**
      case ')':
        while((s.top-1)->type != '(')
        {
          Pop(&s,&e);
          tokens[j++]= e;
        }
        Pop(&s,&e);//左扩号直接出去即可
        break;
      
      case '+':
      case '-':
        while(!StackEmpty(&s) && (s.top-1)->type != '(')
        {
          Pop(&s,&e);
          tokens[j++]=e;
        }
        Push(&s,tokens[i]);
        break;
      
      case '*':
      case '/':
      case TK_EQ:
      case TK_UNEQ:
      case TK_AND:
        while(!StackEmpty(&s) && (s.top-1)->type !='(' && (s.top-1)->type !='+' && (s.top-1)->type !='-')
        {
          Pop(&s,&e);
          tokens[j++]=e;
        }
        Push(&s,tokens[i]);
        break;
      
      case TK_DEREF:
        Push(&s,tokens[i]);
        break;
      

    }
  }
  //中缀表达式遍历完毕，把残余在栈中的输出即可
  while(!StackEmpty(&s))
  {
    Pop(&s,&e);
    if(e.type!='(')
    {
      tokens[j++]=e;
    }
  }
  nr_token = j;//后缀Tokens的长度，后缀表达式不含括号所以变小了
}

static word_t cal_post(){
  int i;
  NStack n;
  InitStack_N(&n);
  word_t a,b,e;
  for(i=0;i<nr_token;++i)
  {
    switch(tokens[i].type)
    {
      case TK_16NUM:
      case TK_NUM:
        e=strtoul(tokens[i].str,NULL,0);
        Push_N(&n,e);
        break;

      case '+':
        Pop_N(&n,&b);
        Pop_N(&n,&a);
        Push_N(&n,a+b);
        break;
      
      case '-':
        Pop_N(&n,&b);
        Pop_N(&n,&a);
        Push_N(&n,a-b);
        break;
      
      case '*':
        Pop_N(&n,&b);
        Pop_N(&n,&a);
        Push_N(&n,a*b);
        break;
      case '/':
        Pop_N(&n,&b);
        Pop_N(&n,&a);
        Push_N(&n,a/b);
        break;
      case TK_EQ:
        Pop_N(&n,&b);
        Pop_N(&n,&a);
        Push_N(&n,a==b);
        break;
      case TK_UNEQ:
        Pop_N(&n,&b);
        Pop_N(&n,&a);
        Push_N(&n,a!=b);
        break;
      case TK_AND:
        Pop_N(&n,&b);
        Pop_N(&n,&a);
        Push_N(&n,a&&b);
        break;
      case TK_DEREF:
        Pop_N(&n,&b);
        e = vaddr_read(b,4);
        Push_N(&n,e);
        break;
    }
  }
  Pop_N(&n,&e);
  return e;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  in2post();
  word_t res= cal_post();

  return res;
}
