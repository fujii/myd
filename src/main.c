/*
 *  MyD
 * 
 * copyright (c) 2000 Hironori FUJII 
 * 
 */
#include <config.h>

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <curses.h>
#include <getopt.h>

#include "myd.h"
#include "getsel.h"

#define PROGRAM "MyD"

char dic_filename[1024];
char *pdic;
char *prompt = PROGRAM ": ";
chtype attr_word, attr_mean, attr_keyword;
chtype blank;
int display_start;          /* スクロール用 */ 
#define SCROLL_STEP 4
#define BEGIN_MEAN 8  /* 意味の表示開始位置 */
#define checking_time 500

struct option longopts[] = {
  {"dictionary", required_argument, 0, 'd'},
  {"theme",      required_argument, 0, 't'},
  {"help",       no_argument,       0, 'h'},
  {"version",    no_argument,       0, 'v'},
  {0,            0,                 0,  0 }
};

void usage(char *prog){
  printf(
	 PROGRAM " ver " VERSION "\n"
	 "辞書検索プログラム\n"
	 "usage: %s [-d <dic_filename>] [--dictionary <dic_filename>]\n"
	 "          [-t <num>] [--theme <num>] \n"
	 "          [-h] [--help]\n"
	 "          [-v] [--version]\n"
	 , prog);
  exit(0);
}


int is_kanji(char c){
  return c & 0x80;
}

void display(){
  int s, i;
  int maxy, maxx;
  int x, y; 
  char *word = keyword.word;

  move(1, 0);
  clrtobot();

  getmaxyx(stdscr, maxy, maxx);
  if(maxx < BEGIN_MEAN + 4){
    addstr("Window is too narrow!");
    return;
  }

  if(word[0] == '\0'){
    return;
  }

  while(!kw_is_hit()){
    display_start = 0;
    search();
  }

  s = keyword.begin + display_start;

  while(s < keyword.hit + keyword.begin){
    /*  単語の表示  */
    attrset(attr_word);
    printw("%s", item_array[s].word);

    /* 意味の表示  */
    attrset(attr_mean);

    addch(blank);

    i = 0;
    while(item_array[s].mean[i]){
      getyx(stdscr, y, x);
      if(x < BEGIN_MEAN)
	move(y, BEGIN_MEAN);

      if(is_kanji(item_array[s].mean[i])){
	getyx(stdscr, y, x);
	if(x == maxx - 1)
	  addch(blank);
	else if(x == maxx-2 && y == maxy-1)
	  addch(blank);
	else{
	  addnstr(&item_array[s].mean[i], 2);
	  i += 2;
	}
      }else  /* 1 バイト文字  */
	addch(item_array[s].mean[i++]);

      if(y == maxy -1 && x == maxx -1)
	break;

    }
    if(y == maxy - 1)
      break;
    addch('\n');
    s++;
  }
}

void print_prompt(){
  attrset(attr_keyword);
  mvaddstr(0, 0, prompt);
}

void print_keyword(){
    move(0, strlen(prompt));
    clrtoeol();
    attrset(attr_keyword);
    addstr(keyword.word);
    move(0, keyword.cursor + strlen(prompt));
}

void main_loop(){
  int c;
  char *pc;

  pc = check_sel();
  if(pc){
    kw_set_str(pc);
    display();
  }

  while(1){ 

    print_keyword();
    refresh();

    c = getch();

    if(isprint(c)){
      kw_ins_char(c);
    }else{
      switch(c){
      case 1:  /* ^A */
	kw_cur_head();
	break;
      case 2:  /* ^B */
      case KEY_LEFT:
	kw_cur_back();
	break;
      case 5:  /* ^E */
	kw_cur_tail();
	break;
      case 6:  /* ^F */
      case KEY_RIGHT:
	kw_cur_forward();
	break;

      case 8:  /* ^H */
      case KEY_BACKSPACE:
	kw_back_space();
	break;

      case 16: /* ^P */
      case KEY_UP:
	display_start -= SCROLL_STEP;
	if(display_start < 0)
	  display_start = 0;
	break;
      case 14: /* ^N */
      case KEY_DOWN:
	display_start += SCROLL_STEP;
	if(display_start >= keyword.hit)
	  display_start = keyword.hit - 1;
	break;

      case 4:  /* ^D */
      case KEY_DC:
      case 0x7f:
	kw_del_char();
	break;

      case 21: /* ^U */
	kw_clear();
	break;
      case 11: /* ^K */
	kw_kill();
	break;

      case 12: /* ^L */
      case KEY_RESIZE:
	print_prompt();
	break;

      case 24: /* ^X */
	return;
	
      case ERR: /* timeout */
	pc = check_sel();
	if(pc){
	  kw_set_str(pc);
	  break;
	}else
	  continue;

      default:
	break;
      }
    }
    display();
  }  
}

void finish(int s){
  get_sel_end();
  free(pdic);
  free(item_array);
  endwin();
  exit(s);
}

int main(int argc, char *argv[]){
  int theme = 0;
  struct stat st;
  int i;
  
  for (i=SIGHUP;i<=SIGTERM;i++)
    if (signal(i,SIG_IGN)!=SIG_IGN)
      signal(i, finish);

  strcpy(dic_filename, getenv("HOME"));
  strcat(dic_filename, "/");
  strcat(dic_filename, HOME_DIC_FILENAME);
  if(stat(dic_filename, &st) == -1){
    strcpy(dic_filename, SYSTEM_DIC_PATH);
  }

  while(1){
    int c;
    c = getopt_long(argc, argv, "d:t:hv", longopts, 0);
    if(c == -1)
      break;
    switch(c){
    case 'd':
      strcpy(dic_filename, optarg);
      break;
      
    case 't':
      theme = atoi(optarg);
      break;

    case 'h':
      usage(argv[0]);
    case 'v':
      printf(PROGRAM " version " VERSION "\n");
      exit(0);
    case ':':
      usage(argv[0]);
    case '?':
      usage(argv[0]);
    }
  }

  if(!read_dic(dic_filename))
    exit(1);

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  timeout(checking_time);

  if(!has_colors())
    theme = 0;

  switch(theme){
  case 0:   /*  mono  */
    attr_word = A_BOLD;
    attr_mean = 0;
    attr_keyword = A_BOLD;
    blank = ' ';
    break;


  case 1:    /*  light */
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_WHITE);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    attr_word = COLOR_PAIR(1) | A_BOLD;
    attr_mean = COLOR_PAIR(2);
    attr_keyword = A_BOLD;
    blank = ' ' | COLOR_PAIR(2);
    bkgdset(blank);
    break;

  case 2:   /*  dark  */
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);
    attr_word = COLOR_PAIR(1) | A_BOLD;
    attr_mean = COLOR_PAIR(2);
    attr_keyword = A_BOLD;
    blank = ' ' | COLOR_PAIR(2);
    bkgdset(blank);
    break;

  case 3:  /* light for kterm */
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);
    attr_word = COLOR_PAIR(1) | A_BOLD | A_REVERSE;
    attr_mean = COLOR_PAIR(2) | A_REVERSE;
    attr_keyword = COLOR_PAIR(2) | A_REVERSE;
    blank = ' ' | COLOR_PAIR(2)| A_REVERSE | A_BOLD;
    bkgdset(blank);
    break;

  }

  print_prompt();

  get_sel_init();

  main_loop();

  finish(0);
  return 0;
}
