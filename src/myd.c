/*
 *  MyD
 * 
 * copyright (c) 2000 Hironori FUJII 
 * 
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "myd.h"

struct item *item_array;  /* ¹àÌÜ¤ÎÇÛÎó */
int n_item;               /* Áí¹àÌÜ¿ô */


int cmp_item(const void *a, const void *b){
  return strcasecmp(((struct item *) a ) -> word,
		    ((struct item *) b ) -> word);
}

struct _keyword keyword;

int read_dic(char *dic_filename){
  struct stat st;
  int dic_size;
  char *pdic;
  int fd;
  int i;
  char *p;

  if(stat(dic_filename, &st) == -1){
    perror(dic_filename);
    return 0;
  }
  dic_size = st.st_size;

  pdic = (char*)malloc(dic_size + 1);
  if(!pdic){
    perror("malloc");
    return 0;
  }

  fd = open(dic_filename, 0);
  if(fd < 0){
    perror("open dictionary");
    free(pdic);
    return 0;
  }

  if(read(fd, pdic, dic_size) != dic_size){
    perror("read dictionary");
    free(pdic);
    close(fd);
    return 0;
  }
  pdic[dic_size] = '\0';

  /* count  words */
  for(i=0;i<dic_size;i++){
    if(pdic[i] == '\n')
      n_item ++;
  }
  n_item /= 2;

  item_array = (struct item *)calloc(sizeof(struct item), n_item);
  if(!item_array){
    perror("calloc");
    free(pdic);
    close(fd);
    return 0;
  }

  p = pdic;
  for(i=0;i<n_item;i++){
    item_array[i].word = p;
    while(*p != '\r' && *p != '\n')
      p++;
    if(*p == '\r'){
      *p++ = '\0';
      p++;
    }else
      *p++ = '\0';
      
    item_array[i].mean = p;
    while(*p != '\r' && *p != '\n')
      p++;
    if(*p == '\r'){
      *p++ = '\0';
      p++;
    }else
      *p++ = '\0';
  }

  qsort(item_array, n_item, sizeof(struct item), cmp_item);

  return 1;
}


void kw_change(){
  keyword.f = 0;
  keyword.l = n_item - 1;
  keyword.begin = -1;
  keyword.hit = 0;
  keyword.match_len = strlen(keyword.word);
}

int kw_is_hit(){
  if(keyword.begin != -1)
    return 1;
  return 0;
}

void kw_cur_head(){
  keyword.cursor = 0;
}

void kw_cur_tail(){
  keyword.cursor = strlen(keyword.word);
}

void kw_cur_back(){
  if(keyword.cursor > 0)
    keyword.cursor --;
}

void kw_cur_forward(){
  if(keyword.cursor < strlen(keyword.word))
    keyword.cursor ++;
}

void kw_del_char(){
  memmove(keyword.word + keyword.cursor,
	  keyword.word + keyword.cursor + 1,
	  strlen(keyword.word + keyword.cursor));
  kw_change();
}

void kw_back_space(){
  if(keyword.cursor == 0)
    return;
  kw_cur_back();
  kw_del_char();
}

void kw_ins_char(int ch){
  memmove(keyword.word + keyword.cursor + 1,
	  keyword.word + keyword.cursor,
	  strlen(keyword.word + keyword.cursor) + 1);
  keyword.word[keyword.cursor++] = ch;
  kw_change();
}

void kw_set_str(char *str){
  strncpy(keyword.word, str, MAX_WORD_LEN);
  keyword.cursor = strlen(str);
  kw_change();
}

void kw_kill(){
  if(keyword.word[keyword.cursor] == '\0')
    return;
  keyword.word[keyword.cursor] = '\0';
  kw_change();
}

void kw_clear(){
  kw_cur_head();
  kw_kill();
}

void search(){
  int f = keyword.f;
  int l = keyword.l;
  int m;
  char *word = keyword.word;

  if(l - f > 1){
    m = (f + l) / 2;
    if(strncasecmp(item_array[m].word, word, keyword.match_len) >= 0){
      keyword.l = m;
    }else{
      keyword.f = m;
    }
  }else{
    if(strncasecmp(item_array[f].word, word, keyword.match_len))
      keyword.begin = l;
    else
      keyword.begin = f;

    f = keyword.begin;
    while(!strncasecmp(item_array[f].word, word, keyword.match_len)){
      f++;
      keyword.hit ++;
      if(f == n_item)
	break;
    }
    if(keyword.hit == 0 && keyword.match_len > 1){
      keyword.match_len --;
      keyword.l = keyword.begin;
      keyword.f = 0;
      keyword.begin = -1;
    }

  }
}
