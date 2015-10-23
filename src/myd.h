struct item{       /* �����ѹ�¤�� */
  char *word;
  char *mean;
};

extern struct item *item_array;  /* ���ܤ����� */
extern int n_item;               /* ����ܿ� */

#define MAX_WORD_LEN 256

struct _keyword{           /* ��ʬõ���ѹ�¤�� */
  char word[MAX_WORD_LEN];  /* ����ʸ���� */
  int cursor;      /* ����ʸ����Υ���������� */
  int f;           /* ��ʬõ���Τ����� */
  int l;           /* ��ʬõ���Τ��ä� */
  int begin;       /* �ҥåȳ��ϰ��� */
  int hit;         /* �ҥåȿ� */
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

