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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  char expr[32];
  word_t data;
  /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP head_node,free_node;
static WP *head = &head_node, *free_ = &free_node;
//把head和free设置为一个头结点感觉更方便
/*
static void  traverse_and_count(){
  WP* traverse = head->next;
  int count=0;
  while(traverse)
  {
    count++;
    traverse=traverse->next;
  }
  printf("总共有 %d 个监视点，还可再创建 %d 个监视点",count,32-count);
}
*/

bool query_wp(int no)
{
  WP *query = head->next;
  while(query)
  {
    if(query->NO == no)
    {
      printf("No.%d watchpoint is founded,its value is: %u",no,query->data);
      return true;
    }
    query=query->next;
  }
  return false;
}

void delete_wp(int no){
  WP *p=head,*d;
  while(p->next)
  {
    if(p->next->NO == no)
    {
      d=p->next;
      p->next=p->next->next;

      d->next=free_->next;
      free_->next=d;
      printf("已取消%d号监视点\n",no);
      return;
    }
    p=p->next;
  }
  printf("找不到%d号监视点\n",no);
}

//这里使用头插法
void new_wp(char *str_expr){
  static int count_no =0;
  if(free_->next == NULL)
    assert(0);
  WP *newwp = free_->next;
  free_->next = free_->next->next;
  newwp->next = head->next;
  head->next=newwp;
  
  newwp->NO = count_no++;
  strcpy(newwp->expr,str_expr);
  bool t=true;
  newwp->data = expr(newwp->expr,&t);
  printf("watchponit %d : %s has setted!,current value:%u\n",newwp->NO,newwp->expr,newwp->data);
}
/*
static void free_wp(){
  if(head->next == NULL)
    assert(0);
  
  WP *temp= head->next;
  head->next=head->next->next;
  temp->next=free_->next;
  free_->next=temp;
}
*/
void list_all_wp(){
  WP* h = head->next;
  if (!h) {
    puts("No watchpoints.");
    return;
  }
  printf("%-8s%-8s%-8s\n", "Num", "What","data");
  while (h) {
    printf("%-8d%-8s%-8u\n", h->NO, h->expr,h->data);
    h = h->next;
  }
}

void wp_difftest(bool *flag){
  WP *i = head->next;
  bool t=true;
  while(i)
  {
    word_t new_data = expr(i->expr,&t);
    if(i->data != new_data)
    {
      *flag = true;
      printf("watchpoint %d : %s\n"
              "Old value = %u\n"
              "New value = %u\n",i->NO,i->expr,i->data,new_data);
      i->data = new_data;
    }
    i=i->next;
  }

}

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  //使用头结点感觉更方便
  free_->next = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

