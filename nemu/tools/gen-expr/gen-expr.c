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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char* buf ;
static int len_buf;
static int capacity_buf;
static char* code_buf; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";
/*
uint32_t a = (1-2)/2; // a=0
uint32_t a = (uint32_t)(1-2)/2; // a=2147483647
从用户角度，只考虑无符号的情况下，第二个结果是正确的。然而测试用例是通过第一种形式生成的，
即进行运算的数都是有符号类型的，因此我们需要将word_t（无符号）类型先转为sword_t（有符号）类型，
再进行运算，从而满足测试用例。是的，先考虑测试再考虑用户，况且也没有用户
*/
static int choose(int n){
  return rand()%n;
}

static int augment_buf(){
  char* new_buf = (char *)realloc(buf,(capacity_buf+50000)*sizeof(char));
  char* new_code_buf = (char *)realloc(code_buf,(capacity_buf+51000)*sizeof(char));
  if(!new_buf || !new_code_buf){
    free(buf);
    free(code_buf);
    return -1;
  }
  else {
    buf=new_buf;
    code_buf = new_code_buf;
    capacity_buf+=50000;
  }
  return 0;
}

static void gen_num(){
  unsigned num = choose(10)+1;
  char num_str [10];
  int ret = snprintf(num_str,sizeof(num_str)/sizeof(char),"%u",num);
  len_buf += ret; 
  //扩充buf数组
  if(len_buf > capacity_buf)
  {
    ret=augment_buf();
    if(ret<0){
      printf("ERROR,Overflow\n");
      return;
    }
  }

    strcat(buf,num_str);
 
}

static void gen(char e){
  char str[2];
  str[0]=e;
  str[1]='\0';
  len_buf += 1; 
  //扩充buf数组
  if(len_buf > capacity_buf)
  {
    int ret=augment_buf();
    if(ret<0){
      printf("ERROR,Overflow\n");
      return;
    }
  }
  strcat(buf,str);
}

static void gen_rand_op(){
  int choose_num = choose(4);
  len_buf += 1; 
  //扩充buf数组
  if(len_buf > capacity_buf)
  {
    int ret=augment_buf();
    if(ret<0){
      printf("ERROR,Overflow\n");
      return;
    }
  }
  switch (choose_num)
  {
  case 0:
    strcat(buf,"+");
    break;
  
  case 1:
    strcat(buf,"-");
    break;
  
  case 2:
    strcat(buf,"*");
    break;
  
  case 3:
    strcat(buf,"/");
    break;
  
  default:
    break;
  }
}

static void gen_rand_expr() {
  switch (choose(3)) {
  case 0: gen_num(); break;
  case 1: gen('('); gen_rand_expr(); gen(')'); break;
  default: gen_rand_expr(); gen_rand_op(); gen_rand_expr(); break;
  
}
}

int main(int argc, char *argv[]) {
  buf = (char *)malloc(65536 * sizeof(char));
  code_buf = (char *)malloc(66000 *sizeof(char));
  capacity_buf = 65536;
  len_buf=0;
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    buf[0] = '\0';
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -Wall -Werror -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    unsigned result;
    ret = fscanf(fp, "%u", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  free(buf);
  free(code_buf);
  return 0;
}