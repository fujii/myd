struct item{       /* 項目用構造体 */
  char *word;
  char *mean;
};

extern struct item *item_array;  /* 項目の配列 */
extern int n_item;               /* 総項目数 */

#define MAX_WORD_LEN 256

struct _keyword{           /* 二分探索用構造体 */
  char word[MAX_WORD_LEN];  /* 検索文字列 */
  int cursor;      /* 検索文字列のカーソル位置 */
  int f;           /* 二分探索のあたま */
  int l;           /* 二分探索のしっぽ */
  int begin;       /* ヒット開始位置 */
  int hit;         /* ヒット数 */
  int match_len;
};

extern struct _keyword keyword;

int read_dic(char *dic_filename);
void kw_change();
int kw_is_hit();
void kw_cur_head();
void kw_cur_tail();
void kw_cur_back();
void kw_cur_forward();
void kw_back_space();
void kw_del_char();
void kw_ins_char(int ch);
void kw_set_str(char *str);
void kw_kill();
void kw_clear();
void search();

