#include <stdint.h>
#include <stdio.h>
#include <libssh/libssh.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>          // for thread Items
#include <errno.h>            // for error handling 
#include "frbuff.h"
#include "ftty.h"
#include "fs_common.h"
#include "flist.h"
#include "ftelnet_cmd.h"
#include "ftelnet.h"

// Socket Items
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "mkaddr.h"
// end socket Items

#include "fprintbuff.h"


void *  ftelnet_rx_thread(void* arg);
void *  ftelnet_tx_thread(void* arg);
static void  bail(const char *on_what);


#define STANDALONE_TELNET
#ifdef STANDALONE_TELNET

kb_data * gpkb;
#endif


#define WAI() printf("%d %s-%s\n",__LINE__,__FILE__,__FUNCTION__)
//#define WAI()

#define WAIC(x) printf("%s%d %s-%s%s\n",x,__LINE__,__FILE__,__FUNCTION__,NC)


//                                                 cmd   subcmd
//Server sends  Will supress - go ahead    ->ff  fb    03
//Response     Do Supress-go ahead         ->ff  fd    03
// Server sends  Will Echo                 ->ff  fb    01
// response      do   Echo                 ->ff  fd    01
// Server sends  Will binary Transmission  ->ff  fb    00
// response      do  ibinary Transmission  ->ff  fd    00

// server sends screen data.  

//Server sends  Will supress - go ahead    ->ff  fb    03
//Response     Do Supress-go ahead         ->ff  fd    03
// Server sends  Will Echo                 ->ff  fb    01
// response      do   Echo                 ->ff  fd    01


#define SOCKET_RX_BUF_SZ  2048
void *  ftelnet_rx_thread(void* arg)
{
//    int  n;
    int  i;
    int  r;
    int  j;
    int z;
    int64_t err = 0;
    struct sockaddr_in adr;           /* AF_INET */
    unsigned int sz = sizeof(adr);
    char dgram[ SOCKET_RX_BUF_SZ ];   /* Recv buffer */
#ifdef TIMESTAMP
// for time stamps
    time_t rawtime ;
    struct tm * timeinfo;
#endif

    ftelnet_data_t  * p =( ftelnet_data_t *)arg;
    printf("Starting Rx Thread: %s\n",p->name);

    while( p->done == 0) // this will be set by teh back door
    {
        // start socket read
       sz = sizeof adr;
       z = recvfrom(p->socket,      /* Socket */
             dgram,                 /* Receiving buffer */
             SOCKET_RX_BUF_SZ,      /* Max recv buf size */
             0,                     /* Flags: no options */
             (struct sockaddr *)&adr,     /* Addr */
             &sz);                        /* Addr len, in & out */
        if( z == 0 )
        {
            p->done = 1;
            printf("[%s] recvfrom() returned 0 -> connection closed\n",__FUNCTION__);
            continue;
        }

#ifdef TIMESTAMP
        time (&rawtime);
        timeinfo = localtime (&rawtime);
        printf("c%d:%d:%d %s:%u %s\n",timeinfo->tm_hour,timeinfo->tm_min
                               ,timeinfo->tm_sec
                               , inet_ntoa(adr.sin_addr)
                               ,(unsigned)ntohs(adr.sin_port)
                               ,dgram);
#endif

        if ( z > 0 ) // we have data
        {
            //WAIC(CYAN); 
            if ( (uint8_t)dgram[0] == (uint8_t)0xff )   // it is a control message
            {
                ftelnet_cmd_t cmd;
                uint8_t * ptr ;
                i = 0;
                j = 0;
                while ( i < z )  
                { 
 //                   WAIC(CYAN); 
 //                   printf(" z = %d, \n",z); 
                    ptr = (uint8_t *) (&(cmd.msg[0]));
                    *ptr++ = (uint8_t)( dgram[i++]);
                    j = 1;
                    while ( ( (uint8_t)dgram[i] != (uint8_t)0xff ) && (i < z ))
                    {
//                        printf(" %d,  0x%02x \n",i,dgram[i]); 
                        *ptr++ = (uint8_t)(dgram[i++]);
                        j++;
                    }
 //                   printf(" i = %d, -- 0x%02x   j=%d \n",i,dgram[i],j); 
                    if ((( (uint8_t)dgram[i] == (uint8_t)0xff ) && (i < z )) || ( i == z ) )
                    {
                       cmd.sz=j;
                       if (g_Debug > 0)
                       {
                           printf("%s",CYAN);
                           printf("[%s] cmd Message \n",__FUNCTION__);
                           ft_cmd_print(&cmd);
                           printf("%s",NC);
                       }

                       err = ListPushEnd( &(p->p_cmd_rcv_list),  &cmd, sizeof(ftelnet_cmd_t),ft_cmd_copy);
                       if ( err != 0)
                           printf ("[%s] ListPushEnd returned error %ld  \n",__FUNCTION__,err );
                    }
                }

//                PrintBuff((unsigned char *)&dgram[0],z,0);
            }
            else
            {
               for( i = 0 ; i < z ; i++)
               {
                  DEBUG( DEBUG_RX_FTELNET )
                  {
                  if ( dgram[i] < 0x20 )
                      printf ("[%s] 0x%02x  c: .  \n", __FUNCTION__,dgram[i]);   // what did we fetch?
                  else
                      printf ("[%s] 0x%02x  c:%c  \n", __FUNCTION__,dgram[i],dgram[i]);   // what did we fetch?
                  }
                  r =  RBuffPut(p->prx_buf, (void *) (&dgram[i]) );    /* put value int ring buffer */
                  if ( r != 0)
                  {
                      int j;
                      char lc;
                      char * lpc = &lc ;
                      char buff[1025];
                      if ( r == 5)
                      {
                          r = 0;
                          printf("%d:%s-%s   RX_BUFF_FULL - flush 1000 bytes off top %d\n",__LINE__,__FILE__,__FUNCTION__,r);
                          for( j= 0 ; j  < 1024  && r == 0 ; j++)
                          {
                              r = RBuffFetch(p->prx_buf, (void **) (&lpc) );
                              if ( r == 0)
                                  buff[j]=*lpc;
                              else
                                  printf ( "Flush prx_buf and answer not 0?? %d\n",r);
                          }
                          PrintBuff((unsigned char *)&buff[0],j,0);
                       }
                       else
                       {
                           printf("%d:%s-%s RBuffPut error: %d \n",__LINE__,__FILE__,__FUNCTION__,r);
                       }
                  }  
               } // data - for loop
                
            }  // if data or cmd
        } // we have data
    } // while done
    printf("End of Rx Thread: %s\n",p->name);
    pthread_exit ((void *)2);
}



void *  ftelnet_tx_thread(void* arg)
{
     ftelnet_data_t * p =( ftelnet_data_t *)arg;
     char c;
     char * pc = &c ;
     int r;
     int z;
     int64_t err;
     struct sockaddr_in adr_srvr;       /* AF_INET */
     ftelnet_cmd_t cmd_out;

 ftelnet_print_ftelnet_data_t(p);      

     printf("Starting Tx Thread %s\n",p->name);
     while( p->done == 0 )
     {
        err =ListPopHead( &(p->p_cmd_snd_list),(void *)  &cmd_out, ft_cmd_copy);
        if ( err != LST_LIST_IS_EMPTY )
        {
             if (g_Debug > 1 )
             {
//               WAI();
                 printf("%s",GREEN);
                 printf("[%s] telnet cmd message to send \n",__FUNCTION__);            
                 ft_cmd_print((void *) &cmd_out);   
                 printf("%s",NC);
             }
             /*
             * Send format string to server:
             */
              z = sendto( p->socket,   /* Socket to send result */
                  &(cmd_out.msg[0]),     /* The datagram result to snd */
                  cmd_out.sz,      /* The datagram lngth */
                  0,               /* Flags: no options */
                 (struct sockaddr *)&adr_srvr,/* addr */
                  sizeof adr_srvr );  /* Server address length */
              if ( z < 0 ) 
              {
                  printf("[%s] \"sendto()\" returned %d \n",__FUNCTION__,z); 
                  bail(__FUNCTION__);   
              }
        }
        r = RBuffFetch( p->ptx_buf ,(void **) &pc); //fetch value out of ring buffer */
        if ( r == 0)
        {
            //WAIC(BLUE);
            DEBUG(DEBUG_TX_FTELNET)
            {
                 printf("[%d:%s] tx %c  0x%x\n",__LINE__,__FUNCTION__,*pc,*pc);
            }

            z = 1;
            /*
             * Send format string to server:
             */
            z = sendto(p->socket,                 /* Socket to send result */
                   pc,                            /* The datagram result to snd */
                   z ,                            /* The datagram lngth */
                   0,                             /* Flags: no options */
                   (struct sockaddr *)&adr_srvr,  /* addr */
                     sizeof adr_srvr  );          /* Server address length */
            if ( z < 0 )
                     printf( "bail   2");   
   //i449             bail("sendto(2)");
   
        }
        else
        {
            usleep(500);    // let the OS have the machine if noting to go out.
        }
     }
     printf("Ending Tx Thread: %s\n",p->name);
     pthread_exit ((void *)2);
}




/*****
*  create storage for an TELNET SESSSION
 */
ftelnet_data_t * ftelnet_create(char * name )
{
    ftelnet_data_t * p;
    p=(ftelnet_data_t *) malloc(sizeof(ftelnet_data_t));
    if ( p == NULL)
    {
        printf("%d:%s-%s  unable to malloc ftelnet_data_t\n",__LINE__,__FILE__,__FUNCTION__);
        return NULL;
    }

    memset( p,0,sizeof(ftelnet_data_t));
    // could set up some default here, especially if unitialization causes
    //             a seg fault;
    if ( name != NULL)
    {
        strcpy(p->name,name);
    }

    return p;
}


void ftelnet_set_dest_ip( ftelnet_data_t * p, char* dst_ip)     {strcpy(p->dst_ip ,dst_ip);}
void ftelnet_set_verbosity( ftelnet_data_t * p, int verbosity ) {p->verbosity = verbosity;}
void ftelnet_set_port( ftelnet_data_t * p, int port )           {p->port = port ; }

////////////////////////////////////////
// _ Send telnet control Message
////////////////////////////////////////
int64_t  _ftelnet_send_ctrl_message(ftelnet_data_t *p, ftelnet_cmd_t *cmd )
{
     int64_t err=0; 
//   WAI()     

    err = ListPushEnd( &(p->p_cmd_snd_list),  cmd,sizeof(ftelnet_cmd_t), ft_cmd_copy);
    if ( err != 0)
        printf ("[%s]  ListPushHead returned error %ld  \n",__FUNCTION__,err );

    // show what is pending to be sent.
    if ( g_Debug > 1 )
    {
        printf("%s",MAGENTA);
        ListPrint(  p->p_cmd_snd_list , &ft_cmd_print  );
        printf("%s",NC);
    }
    return err;
}
 
////////////////////////////////////////
//  Send telnet control Message
//    format the string m w/ size m_sz into a ftelnet_cmd_t and
//    send the message  
////////////////////////////////////////
int64_t ftelnet_send_ctrl_message(ftelnet_data_t *p, char * m,int m_sz)
{
     int64_t  i;
     ftelnet_cmd_t cmd; 
     uint8_t * ptr = (uint8_t *) (&(cmd.msg[0]));
     
     // check size is not greater than 128 characters

     for ( i = 0 ;  i < m_sz ; i ++)
     {
         *ptr++ = (uint8_t) *m++;
     }
     cmd.sz=m_sz;
     // stuff the message in the cmd queue
     return  _ftelnet_send_ctrl_message(p, &cmd );
}


int ftelnet_open_channel(ftelnet_data_t * p)
{
//   int rc;
   int result;
   int err;
   struct sockaddr_in    addr_server;

#ifdef SUPPORT_BIND
   struct sockaddr_in    addr_bind;     
   int                   port_bind = 0;
   char                  ip_addr_bind[IP_ADDR_STRING_SIZE] = {0};
#endif

   WAI();
   RbuffInitialize ( &(p->ptx_buf), &result, sizeof(char),8192, 32);
   if( result != 0)
   {
        printf("Failed to allocate Tx Buffer  %d \n",result);
//        cleanup(pg_db);
        return -2;
    }

    RbuffInitialize ( &(p->prx_buf), &result, sizeof(char),8192, 32);
    if( result != 0)
    {
        printf("Failed to allocate Rx Buffer  %d \n",result);
//        cleanup(pg_db);
        return -3;
    }

//    DEBUG( DEBUG_CFG_DATA_SSH )
    {
       ftelnet_print_ftelnet_data_t(p);
    }


     err = pthread_create(&(p->tid_rx), NULL, &ftelnet_rx_thread, p);
     if (err != 0)
         printf("\ncan't create thread :[%s]", strerror(err));
     else
         printf("\n Telnet Rx Thread created successfully\n");

     err = pthread_create(&(p->tid_tx), NULL, &ftelnet_tx_thread, p);
     if (err != 0)
         printf("\ncan't create thread :[%s]", strerror(err));
     else
         printf("\n Telnet Tx Thread created successfully\n");

    usleep(1000000);  // for now sleep 1 sec till threads run.  
        // future, rework and have thread signal it is running!

    WAI();


    // Socket creation here

     /*
      * Create a TCP socket to use:
      */
     p->socket = socket(AF_INET,SOCK_STREAM,0);
     if ( p->socket < 0 )
     {
         WAI();
         bail("cannot create Socket"); 
     }
     printf("Socket Handle:  %d\n",p->socket);

//    end open the tcp socket


//   create structure describing Server you want to connect to      
     memset(&addr_server,0,sizeof(addr_server));

     addr_server.sin_family = AF_INET;
     addr_server.sin_addr.s_addr = inet_addr(p->dst_ip);
     addr_server.sin_port = htons(p->port);

     if ( addr_server.sin_addr.s_addr == INADDR_NONE )
         bail("bad address.");

     printf("Server to connect to %s  port %d\n",p->dst_ip,p->port);

#ifdef SUPPORT_BIND
//////////////////////////////////////////////////////////
// begin  bind to a specific host interface (if desired) 

     if ( ip_addr_bind[0] != 0x00 )
     {
         int SoReUseAddr = 0;  // ??? setsocket opt SO_REUSE_ADDR
         int z;



         /* Addr on cmdline: */
        // bind_addr = argv[2];
        // adr_bind_port = 9091;
         printf("My interface IP:  %s  port %d\n",ip_addr_bind,port_bind);

         memset(&addr_bind,0,sizeof addr_bind);
         addr_bind.sin_family = AF_INET;
         addr_bind.sin_port   = htons( port_bind);
         addr_bind.sin_addr.s_addr = inet_addr(ip_addr_bind);

         if ( addr_bind.sin_addr.s_addr == INADDR_NONE )
         {
            bail("bad bind address.");
         }     

         if( SoReUseAddr != 0)  // a 4th argument should force an error on second routine
         {
            printf(" setsocket opt SO_REUSE_ADDR\n");
            /*
            * Allow multiple listeners on the
            * broadcast address:
            */
            z = setsockopt(p->socket,
              SOL_SOCKET,
              SO_REUSEADDR,
              &SoReUseAddr,
              sizeof SoReUseAddr );

              if ( z == -1 )
                bail("setsockopt(SO_REUSEADDR)");
          }
      /*
       * Bind our socket to the broadcast address:
       */
 //      printadr("addr_bind",&addr_bind);
 //      printf("    len_bind  %d\n",len_bind);

         z = bind(p->socket,
            (struct sockaddr *)&addr_bind,
             sizeof (addr_bind));

         if ( z == -1 )
            bail("bind(2)");

     } // end bind address supplied

// end  bind to a specific host interface (if desired) 
#endif


    WAI();
    err= connect( p->socket, (struct sockaddr *)&addr_server, sizeof(addr_server)) ;
    if( err != 0)
    {
       printf("\n Error : Connect Failed %d \n",err);
       return 1;
    } 
    printf("\n Connect returned  %d \n",err);

   return 0;
}

#if 0
150 // returns true (!=0) if the channel is open and EOF has not been received
151 int ftelnet_check_channel( ftelnet_data_t * p)
152 {
153     return (ssh_channel_is_open(p->channel) && !ssh_channel_is_eof(p->channel    ));
154 }
155
#endif
int ftelnet_get_c(ftelnet_data_t * p, char * c)
{
   char lc;
   char *lpc=&lc;
   int r;

   r = RBuffFetch(p->prx_buf, (void **) (&lpc) );
   if ( r == 0)
   {
           *c=*lpc;
           return 0;  // one character received
   }        
   return 1;  // no data present
}



int ftelnet_put_c(ftelnet_data_t * p, char c)
{
   int r;

//   WAI();
   r =  RBuffPut(p->ptx_buf, (void *) (&c) );    /* put value int ring buffer */
   if ( r != 0)
        printf("error writing  %d\n",r);

   return r;
}


int  ftelnet_close_session( ftelnet_data_t * p )
{
   WAI();
//210   ssh_channel_send_eof(p->channel);
//211   ssh_channel_close( p->channel);
//212   ssh_channel_free( p->channel);
//213   p->channel = NULL;
   p->done = 1;

   if(p->socket != 0)
   {
       close(p->socket);
       p->socket = 0;
   }
   return 0;
}

void ftelnet_distroy_session(ftelnet_data_t * p )
{
   if ( p != NULL)
   {
       ftelnet_close_session(p);

//       if( p->session != NULL)
//       {
//           ssh_disconnect( p->session);
//           ssh_free ( p->session);
//           p->session = NULL ;
//       }

       if( p->prx_buf != NULL) { free (p->prx_buf); p->prx_buf=NULL;}
       if( p->ptx_buf != NULL) { free (p->ptx_buf); p->ptx_buf=NULL;}
    }
}




void ftelnet_print_ftelnet_data_t(ftelnet_data_t *p)
{
     printf(" name :          %s\n",p->name);
     printf(" dst_ip :        %s\n",p->dst_ip);
     printf(" port :          %d\n",p->port);
     printf(" socket :        %d\n",p->socket);
     printf(" verbosity:      %d\n",p->verbosity);
     printf(" fd_log :        %d\n",p->fd_log);
     printf(" interactive:    %d\n",p->interactive);
     printf(" p_cmd_snd_list: %p\n",p->p_cmd_snd_list);
     printf(" p_cmd_rcv_list: %p\n",p->p_cmd_rcv_list);
//     printf(" session:       %p\n",p->session);
//     printf(" channel:       %p\n",p->channel);
     printf(" tx:            %p\n",p->ptx_buf);
     printf(" rx:            %p\n",p->ptx_buf);
     printf(" done:          %d\n",p->done);

}


int ServiceReceivedTelnetCmds( ftelnet_data_t *p )
{
    int64_t        done  = 0 ;
    int64_t        error = 0 ;
    ftelnet_cmd_t  rx_cmd;
 
    while (done == 0)
    {
        error =ListPopHead( &(p->p_cmd_rcv_list),(void *) &rx_cmd, ft_cmd_copy);
        if ( error != LST_LIST_IS_EMPTY )
        {
            if (g_Debug > 1 )
            {
                 WAIC(YELLOW);
                 //printf("[%s] cmd from remote \n",__FUNCTION__);            
                 //ft_cmd_print((void *) &rx_cmd );   
            }

            // check if the cmd is a "Will"  and tell the far end to "don't"

            
            if ( rx_cmd.msg[2] == 0x03)   // supress Go ahead
            {
                 if ( rx_cmd.msg[1] != 0xfb )  // Will 
                 {                     
                     rx_cmd.msg[1] = 0xfe ;    // 254  "Don't" 
                     error = _ftelnet_send_ctrl_message( p, &rx_cmd ) ;
                     if ( error != 0 )
                         printf("[%s] failed to add send message <1>  %ld\n",__FUNCTION__,error);
                 }
            }
            else if ( rx_cmd.msg[2] == 0x00 )   // binary transfer  
            {
                 // the second byte of the Rx CMd will be 
                 if (( rx_cmd.msg[1] == 0xfb ) ||  ( rx_cmd.msg[1] == 0xfd ))  // 251  will or do  
                 {
                     rx_cmd.msg[1] = 0xfe ;    // 254  "Don't" to far end        
                     error = _ftelnet_send_ctrl_message( p, &rx_cmd ) ;
                     if ( error != 0 )
                         printf("[%s] failed to add send message %ld\n",__FUNCTION__,error);
                 }
             }
             else  // all other options 
             {  
                  if ( rx_cmd.msg[1] == 0xfb )   // if "WILL"
                  {
                      rx_cmd.msg[1] = 0xfe ;    // 254  "Don't" 
                      error = _ftelnet_send_ctrl_message( p, &rx_cmd ) ;
                      if ( error != 0 )
                          printf("[%s] failed to add send message <1>  %ld\n",__FUNCTION__,error);
                  }
             }
//             WAI();
         }
         else
         {
            done = 1;  // no more RX requests
         }
    } // end while done == 0
    return 0;
}



#ifdef STANDALONE_TELNET
char RED[]     = { 0x1b,'[','1',';','3','1','m',0x00 };
char GREEN[]   = { 0x1b,'[','1',';','3','2','m',0x00 };
char YELLOW[]  = { 0x1b,'[','1',';','3','3','m',0x00 };
char BLUE[]    = { 0x1b,'[','1',';','3','4','m',0x00 };
char MAGENTA[] = { 0x1b,'[','1',';','3','5','m',0x00 };
char CYAN[]    = { 0x1b,'[','1',';','3','6','m',0x00 };
char NC[]      = { 0x1b,'[','0','m',0x00 };


ftelnet_data_t * gp_telnet;

uint64_t  g_Debug = 0;
uint64_t  g_error = 0;
// need to use the escape key to send special charactes
// this is because getchar does not return Ctrl Sequences
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void  cleanup(ftelnet_data_t  *p)
{
    ftelnet_distroy_session(p);

}

/*
 * This function reports the error and
 * exits back to the shell:
 */
static void
bail(const char *on_what) {
    fputs(strerror(errno),stderr);
    fputs(": ",stderr);
    fputs(on_what,stderr);
    fputc('\n',stderr);
    exit(1);
}


void print_usage(void)
{
   printf("ftelnet- App /src   \n");
   printf("-i <server_ip_addr>   EG: 10.75.1.77 (default)\n");
   printf("-p <server_port>      EG: 7011       (default)\n");
   printf("-l                    open/append log file \n");
   printf("-v <value>            Verbose Setting  \n");
   printf("-d <0xDebugFlags>     Debug Flags  \n");
   printf("example: \n");
   printf("./ftelnet   -i 10.75.1.79 -p 23  -d 2 -v 1 \n");
   printf("./ftelnet  \n");

   printf("\n");
}

char     g_outFileName[1024];
int      g_WriteLogFile = 0;
FILE *   g_fpOut;
uint64_t g_Verbose;

int main(int argc, char ** argv)
{
      int option  ;
//    int64_t err;
      char target_ip[64] ={0};
      int  target_port = 0;
//    mode=3;
//    if ( argc >= 2) mode=atoi(argv[1]);

 #if 0
    strcpy(target_ip,"10.75.1.79");   // 4 port power controller
    target_port = 23 ;
#else
    strcpy(target_ip,"10.75.1.77");   // terminal server spot
    target_port = 7011;;
#endif
    strcpy(g_outFileName,"log.txt");    

while ((option = getopt(argc, argv,"v:li:d:p:ih")) != -1) {
       switch (option) {
//          case 'm' : g_Menu=1 ;          // configure to bind w/ SO_REUSEADDR
//                break;
            case 'v' : g_Verbose=atoi(optarg) ;   // Set the verbose level
                break;
            case 'l' : g_WriteLogFile = 1; // write/append all  communications to log
                break;
            case 'i' : strcpy(target_ip,optarg);       // server ip addr
                break;
            case 'p' : target_port = atoi(optarg);    // server  port
                break;
            case 'd' : g_Debug = atoi(optarg);        // g_Debug setting
                break;
            case 'h' :                               // Print Help
            default:
                print_usage();
//                exit(EXIT_FAILURE);
                return(1);
       }
   }

   printf("Command Line Arguments:\n");
   printf("       %24s Telnet server IP address\n" ,target_ip);
   printf("       %24d Telnet server port\n"       ,target_port);
   printf("       %24d Verbose\n"         ,(int)g_Verbose);
   printf("  0x%024lx   Debug\n"          ,g_Debug);
   printf("       %24d Log Enabled\n"     ,g_WriteLogFile);
   printf("       %24s Log File Name\n"   ,g_outFileName);

   if ( g_WriteLogFile != 0)
   {
//
//  Open an Output LOG FIle if give a name
//
       g_fpOut = fopen (g_outFileName, "a+");
       if (g_fpOut == NULL)
       {
           printf ("Unable to open file %s\n", g_outFileName);
           return (-5);
       }
   }






printf(" Target:\n");
printf("     IP: %s\n",target_ip);
printf("   Port: %d\n",target_port);


    gpkb = ftty_kb_create();
    ftty_kb_start( gpkb );

    // Initialize Default Values & Data Structures
    gp_telnet = ftelnet_create( target_ip );
    ftelnet_set_dest_ip(   gp_telnet , target_ip );
    ftelnet_set_port(      gp_telnet , target_port );
    ftelnet_set_verbosity( gp_telnet , 0 );

    ftelnet_print_ftelnet_data_t ( gp_telnet );


// Start threads and connect to the target

    ftelnet_open_channel( gp_telnet);

// done opening the tcp connection
    {
      uint64_t err = 0;
      unsigned char  cmd[] = { CMD, WILL, NEGOTIATE_WIDOW_SIZE, \
                               CMD, WILL, TERMINAL_SPEED, \
                               CMD, WILL, TERMINAL_TYPE, \
                               CMD, WILL, NEW_ENVIONRMENT_OPTION, \
                               CMD, DO,   ECHO, \
                               CMD, WILL, SUPRESS_GO_AHEAD, \
                               CMD, DO,   SUPRESS_GO_AHEAD }; 
      int cmd_sz=3*7;
 
      err = ftelnet_send_ctrl_message( gp_telnet, (char *)cmd , cmd_sz);
      if ( err != 0 )
         printf("[%s] failed to add send message %ld\n",__FUNCTION__,err);

    }

   {
    int done = 0;
    int r;
    char c;
          printf("ctrl-C to exit \n\r");
          while( done == 0)
          {
              r =  ftty_kb_fetch( gpkb, &c) ;
              if ( r == 0 )
              {
                  if( c < 0x20)
                      if ( c == 0x0d)  printf("\n\r");
                      else if ( c == 0x0a)  printf("\r");
                      else
                          printf("<%02x>",c);
                  else
                      printf("%c",c);
  //                printf("in:  %c %x \n",*pc,*pc);
  //                if ( c == 0x31) 
                  ftelnet_put_c(gp_telnet, c );
                  if ( c == 0x03) done =1;
              }

              r = ftelnet_get_c( gp_telnet, &c);
              if ( r == 0)
              {
                  printf("%c",c);
              }
              ServiceReceivedTelnetCmds(gp_telnet);



           }

}

     // need to make shure the threads are closed before calling below
     ftty_kb_stop( gpkb );
     ftty_kb_distroy(  gpkb );

     ftelnet_distroy_session ( gp_telnet );



     cleanup(gp_telnet);
     restore_tty();

     if (g_fpOut != NULL)
     {
          fclose (g_fpOut );
     }

     printf("\ndone\n");

     return 0;

 }

#endif





