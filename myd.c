/*
 *  MyD
 * 
 * copyright (c) 2000 Hironori FUJII 
 * 
 */

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <curses.h>
#include <getopt.h>

#define PROGRAM "MyD"
#define VERSION "1.0"

char dic_filename[1024];
#define SYSTEM_DIC_FILENAME "/usr/local/share/myd.dic"
#define HOME_DIC_FILENAME ".myd.dic"
char *pdic;
char *prompt = "Search: ";
chtype attr_word, attr_mean;
int display_start;          /* スクロール用 */ 
#define SCROLL_STEP 4
#define BEGIN_MEAN 8  /* 意味の表示開始位置 */

struct item{       /* 項目用構造体 */
  char *word;
  char *mean;
};

struct item *item_array;  /* 項目の配列 */
int n_item;               /* 総項目数 */

struct {           /* 二分探索用構造体 */
  char word[256];  /* 検索文字列 */
  int cursor;      /* 検索文字列のカーソル位置 */
  int f;           /* 二分探索のあたま */
  int l;           /* 二分探索のしっぽ */
  int begin;       /* ヒット開始位置 */
  int hit;         /* ヒット数 */
} keyword;

struct option longopts[] = {
  {"dictionary", required_argument, NULL, 'd'},
  {"mono",       no_argument,       NULL, 'm'},
  {"help",       no_argument,       NULL, 'h'},
  {"version",    no_argument,       NULL, 'v'},
  {NULL,         0,                 NULL,  0 }
};

void usage(char *prog){
  printf(
	 PROGRAM " ver " VERSION "\n"
	 "辞書検索プログラム\n"
	 "usage: %s [-d <dic_filename>] [--dictionary <dic_filename>]\n"
	 "          [-m] [--mono]\n"
	 "          [-h] [--help]\n"
	 "          [-v] [--version]\n"
	 , prog);
  exit(0);
}

int cmp_item(const void *a, const void *b){
  return strcasecmp(((struct item *) a ) -> word,
		    ((struct item *) b ) -> word);
}

int read_dic(){
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
}

void sig_handler(int s){
  free(pdic);
  free(item_array);
  endwin();
  exit(1);
}

void kw_change(){
  keyword.f = 0;
  keyword.l = n_item - 1;
  keyword.begin = -1;
  keyword.hit = 0;
  timeout(0);
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
    if(strcasecmp(item_array[m].word, word) >= 0){
      keyword.l = m;
    }else{
      keyword.f = m;
    }
  }else{
    if(strncasecmp(item_array[f].word, word, strlen(word)))
      keyword.begin = l;
    else
      keyword.begin = f;

    f = keyword.begin;
    while(!strncasecmp(item_array[f].word, word, strlen(word))){
      f++;
      keyword.hit ++;
      if(f == n_item)
	break;
    }
  }
}

int is_kanji(char c){
  return c & 0x80;
}

void display(){
  int s, i;
  int maxy, maxx;
  int x, y; 
  char *word = keyword.word;

  move(2, 0);
  clrtobot();

  getmaxyx(stdscr, maxy, maxx);
  if(maxx < BEGIN_MEAN + 4){
    addstr("Window is too narrow!");
    return;
  }

  if(word[0] == '\0'){
    timeout(-1);
    return;
  }
  if(!kw_is_hit()){
    display_start = 0;
    search();
    return;
  }
  timeout(-1);

  s=keyword.begin + display_start;

  while(s < keyword.hit + keyword.begin){
    /*  単語の表示  */
    attrset(attr_word);
    printw("%s", item_array[s].word);

    /* 意味の表示  */
    attrset(attr_mean);

    addch(' ');

    i = 0;
    while(item_array[s].mean[i]){
      getyx(stdscr, y, x);
      if(x < BEGIN_MEAN)
	move(y, BEGIN_MEAN);

      if(is_kanji(item_array[s].mean[i])){
	getyx(stdscr, y, x);
	if(x == maxx - 1)
	  addch(' ');
	else if(x == maxx-2 && y == maxy-1)
	  addch(' ');
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

void print_title(){
  int x, y, i;
  getmaxyx(stdscr, y, x);
  attrset(A_REVERSE);
  move(0, 0);
  hline('=', x);
  mvaddstr(0, 3, " " PROGRAM " ver." VERSION " ");
}

void print_prompt(){
  attrset(0);
  mvaddstr(1, 0, prompt);
}

void print_keyword(){
    attrset(A_BOLD);
    move(1, strlen(prompt));
    clrtoeol();
    addstr(keyword.word);
    move(1, keyword.cursor + strlen(prompt));
}

void main_loop(){
  int i=0, c;

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
	print_title();
	print_prompt();
	break;
	
      default:
	break;
      }
    }
    display();
  }  
}

main(int argc, char *argv[]){
  int mono=0;
  struct stat st;
  int i;
  
  for (i=SIGHUP;i<=SIGTERM;i++)
    if (signal(i,SIG_IGN)!=SIG_IGN)
      signal(i,sig_handler);

  strcpy(dic_filename, getenv("HOME"));
  strcat(dic_filename, "/");
  strcat(dic_filename, HOME_DIC_FILENAME);
  if(stat(dic_filename, &st) == -1){
    strcpy(dic_filename, SYSTEM_DIC_FILENAME);
  }

  while(1){
    int c;
    c = getopt_long(argc, argv, "d:mhv", longopts, NULL);
    if(c == -1)
      break;
    switch(c){
    case 'd':
      strcpy(dic_filename, optarg);
      break;
      
    case 'm':
      mono = 1;
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

  if(!read_dic())
    exit(1);

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  timeout(0);

  if(has_colors() && !mono){
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    attr_word = COLOR_PAIR(1) | A_BOLD;
    attr_mean = COLOR_PAIR(0);
  }else{
    attr_word = A_BOLD;
    attr_mean = 0;
  }

  print_title();
  print_prompt();

  main_loop();

  free(pdic);
  free(item_array);
  endwin();
  exit(0);
}
