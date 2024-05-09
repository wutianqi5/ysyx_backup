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
#include "memory/vaddr.h"

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

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_p(char *args){
  bool success = true;
  word_t res = expr(args,&success);

  if(!success){
    puts("ERROR");
    return -1;
  }
  else{
    printf("Result is : %u\n",res);
  }
  return 0;
}

static int cmd_x(char *args){
  if (args == NULL)
    return -1;
  
  char *num = strtok(args," ");
  char *expr = strtok(NULL," ");

  if(num == NULL || expr == NULL)
  {
    printf("Parameter Error\n");
    return -1;
  }
  int n = strtol(num,NULL,10);
  vaddr_t addr = strtol(expr,NULL,16);

  for(int i=0;i<n;i++)
  {
    word_t data = vaddr_read(addr+i*4,4);
    printf("0x%08x",addr+i*4);
    printf("  0x%08x\n",data);
  }
  return 0;

  
  
}

void list_all_wp();
static int cmd_info(char *args){
  if(strcmp(args,"r")==0){
    isa_reg_display();
  }

  if(strcmp(args,"w")==0){
    list_all_wp();
  }

  return 0;
}

static int cmd_si(char *args) {
  if (args == NULL){
    cpu_exec(1);
  }
  else{
    int num_execution = atoi(args);
    cpu_exec(num_execution);
  }
  return 0;
}

void new_wp(char *args);
void delete_wp(int no);
static int cmd_w(char *args){
  if(args==NULL)
    return 1;
  new_wp(args);
  return 0;
}
static int cmd_d(char *args){
  int n=strtol(args,NULL,10);
  delete_wp(n);
  return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  {"si","Pause the program after executing N instructions in one step,when N is not given, it defaults to 1",cmd_si},
  {"info","info r,print register status; info w,print watch point information",cmd_info},
  {"x","x N EXPR,Find the value of the expression EXPR and use the result as the starting memory address, \
  output N consecutive 4 bytes in hexadecimal format",cmd_x},
  {"p","p EXPR,Get the value of the expression EXPR. ",cmd_p},
  {"w","w EXPR,Pause program execution when the value of the expression EXPR changes",cmd_w},
  {"d","d N,Delete watchpoint with serial number N",cmd_d}

  /* TODO: Add more commands */

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
        if (cmd_table[i].handler(args) < 0) { 
          if (strcmp(cmd, "q") == 0) {
            nemu_state.state = NEMU_QUIT; // set "QUIT" state when q 优雅的退出
          }
          return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

static void test_expr(int test){
  if(!test) return;
  FILE *fp = fopen("/home/pan/ysyx-workbench/nemu/tools/gen-expr/build/input","r");
  if(!fp) perror("文件不存在！");
  unsigned int correct_num;
  char expr_str[50000];
  while(true){
    int ret=fscanf(fp,"%u %s",&correct_num,expr_str);
    if(ret != 2) break;
    bool success = true;
    word_t expr_res = expr(expr_str,&success);
    assert(success);
    if(correct_num != expr_res)
    {
      printf("正确值应该是：%u,而算出的值为:%u",correct_num,expr_res);
    }
  }
  fclose(fp);
  printf("Pass");
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();
  test_expr(0);
  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
