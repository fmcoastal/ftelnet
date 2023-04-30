#ifndef _f_tty_h
#define _f_tty_h



void   test_tty(void);

typedef struct kb_data_struct
{
    int done;
    int debug;
    int verbosity;
    pthread_t   tid_kb;
    frbuff * kb_buff;      // ring buffer for keyboard

} kb_data ;


void   reconfig_tty(void);
void   restore_tty(void);

kb_data * ftty_kb_create(void);
int  ftty_kb_start( kb_data * p);
int  ftty_kb_stop( kb_data * p );
int  ftty_kb_distroy(  kb_data * p);

// return 0 if char is good,  else,
//  return value comes from rbuff fetch function
int  ftty_kb_fetch(  kb_data * p, char * c);

 void ftty_print_kb_data(kb_data * p,char * string);
#endif
