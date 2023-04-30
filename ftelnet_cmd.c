#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "ftelnet_cmd.h"



// references:
//     https://www.rfc-editor.org/rfc/rfc854.html
//     https://www.ibm.com/docs/en/zos/2.4.0?topic=problems-telnet-commands-options
//     http://www.tcpipguide.com/free/t_TelnetProtocolCommands-3.htm
void ft_cmd_print(void * pdata)
{
int i;
uint8_t * ptr;
    ftelnet_cmd_t * p =(ftelnet_cmd_t *) pdata;
    ptr = p->msg;
    printf("        %p   sz: %ld   msg:  ",p,p->sz);
    for ( i = 0 ; i < p->sz ;  i++)
    {
         if ( i%8 == 0 ) printf("\n         ");
         printf("0x%02x ",*ptr++ ); 
    } 
    printf("\n         ");  
    if( p->msg[1] == (uint8_t) 0xfe )       printf("Don't");   //254
    else if( p->msg[1] == (uint8_t) 0xfd )  printf("   Do");   //253
    else if( p->msg[1] == (uint8_t) 0xfc )  printf("Won't");   //252
    else if( p->msg[1] == (uint8_t) 0xfb )  printf(" Will");   //251
    else if( p->msg[1] == (uint8_t) 0xfa )  printf("   SB");   //250
    else if( p->msg[1] == (uint8_t) 0xf9 )  printf("   GA");   //249
    else if( p->msg[1] == (uint8_t) 0xf8 )  printf("   EL");   //248
    else if( p->msg[1] == (uint8_t) 0xf7 )  printf("   EC");   //247
    else if( p->msg[1] == (uint8_t) 0xf6 )  printf("  AYT");   //246
    else if( p->msg[1] == (uint8_t) 0xf5 )  printf("   A0");   //245
    else if( p->msg[1] == (uint8_t) 0xf4 )  printf("   IP");   //244
    else if( p->msg[1] == (uint8_t) 0xf3 )  printf("  BRK");   //243
    else if( p->msg[1] == (uint8_t) 0xf3 )  printf("   DM");   //242
    else if( p->msg[1] == (uint8_t) 0xf3 )  printf("  NOP");   //241
    else if( p->msg[1] == (uint8_t) 0xf3 )  printf("   SE");   //240
    else                          printf(" %d  ",p->msg[1]);   // ???

    // option_codes
    if( p->msg[2] == (uint8_t) 0x2C )       printf(" New Environment Option");   
    else if( p->msg[2] == (uint8_t) 0x27 )  printf(" New Environment Option");   
    else if( p->msg[2] == (uint8_t) 0x20 )  printf(" Terminal Speed");                     
    else if( p->msg[2] == (uint8_t) 0x1f )  printf(" Negotiate About Window Size ");                     
    else if( p->msg[2] == (uint8_t) 0x18 )  printf(" Terninal Type");                     
    else if( p->msg[2] == (uint8_t) 0x03 )  printf(" Supress Go Ahead");         
    else if( p->msg[2] == (uint8_t) 0x02 )  printf(" Reconnection");   
    else if( p->msg[2] == (uint8_t) 0x01 )  printf(" Echo");   
    else if( p->msg[2] == (uint8_t) 0x00 )  printf(" Binary Transmission");   
    else                          printf(" %02x %d  ",p->msg[2],p->msg[2]);   // ???
    printf("\n");
}


void ft_cmd_copy(void * Dst, void * Src)
{
    memcpy(Dst,Src,sizeof(ftelnet_cmd_t));
}



int64_t ft_cmd_compare_s(void * A, void * B)
{
    return strcmp( (char *)((( ftelnet_cmd_t*)A)->msg) , (char *)((( ftelnet_cmd_t*)B)->msg));
}

int64_t ft_compare_d(void * A, void * B)
{
   return ! ( (( ftelnet_cmd_t*)A)->sz == (( ftelnet_cmd_t*)B)->sz );
}




