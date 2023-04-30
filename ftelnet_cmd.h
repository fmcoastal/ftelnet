#ifndef _f_telnet_cmd_h
#define _f_telnet_cmd_h


typedef struct ftelnet_cmd_struct {
int64_t     sz;
uint8_t  msg[128];  // need to think about this .
} ftelnet_cmd_t;


void ft_cmd_print(void * pdata);
void ft_cmd_copy(void * Dst, void * Src);
int64_t ft_cmd_compare_s(void * A, void * B);
int64_t ft_compare_d(void * A, void * B);






#endif
