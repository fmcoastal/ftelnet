#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <termios.h>
#include <memory.h>   // for memset
#include <pthread.h>   // for memset
#include "frbuff.h" 
#include "fs_common.h"

#include "ftty.h" 

//#define STANDALONE_TTY

//#define WAI() printf("%d:%s %s\n",__LINE__,__FILE__,__FUNCTION__)
#define WAI() 
int g_ftty_Debug=0;

void *  ftty_kb_thread(void* arg);

/* the OS holds control till a character is hit,
   the OS returns a character when hit, It does
   not wait for <cr> to be input
*/

/* need create a mode where a thread that sits 
   on getch and puts the results  in a fifo which
   I can pull from when I want to look and not 
   stall my other processing.  
*/ 

int            g_valid_tty_opts_backup=0;
struct termios g_tty_opts_backup={0};


void  reconfig_tty(void)
{
     struct termios  tty_opts_raw;

     if (!isatty(STDIN_FILENO)) {
       printf("Error: stdin is not a TTY\n");
       exit(1);
     }
     printf("stdin is %s\n", ttyname(STDIN_FILENO));

     // Back up current TTY settings
     tcgetattr(STDIN_FILENO, &g_tty_opts_backup);
     // set the flag     
     g_valid_tty_opts_backup=1;

     if( g_ftty_Debug != 0 )
     {
         printf(" c_oflag::OPOST   = %d \n", ( !! ( g_tty_opts_backup.c_oflag & OPOST ) ));
         printf(" c_oflag::ONLCR   = %d \n", ( !! ( g_tty_opts_backup.c_oflag & ONLCR ) ));
         printf(" c_oflag::XTABS   = %d \n", ( !! ( g_tty_opts_backup.c_oflag & XTABS ) ));
         printf(" c_oflag::ENOENT  = %d \n", ( !! ( g_tty_opts_backup.c_oflag & ENOENT ) ));
     }
   
     // Change TTY settings to raw mode
     // https://www.qnx.com/developers/docs/6.5.0SP1.update/com.qnx.doc.neutrino_lib_ref/c/cfmakeraw.html
     // The cfmakeraw() function sets the terminal attributes as follows:

     //     termios_p->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
     //     termios_p->c_oflag &= ~OPOST;
     //     termios_p->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
     //     termios_p->c_cflag &= ~(CSIZE|PARENB);
     //     termios_p->c_cflag |= CS8; 

     // https://www.gnu.org/software/libc/manual/html_node/Mode-Data-Types.html
     cfmakeraw(&tty_opts_raw);

     if( g_ftty_Debug != 0 )
     {
         printf(" c_oflag::OPOST   = %d \n", ( !! ( tty_opts_raw.c_oflag & OPOST ) ));
         printf(" c_oflag::ONLCR   = %d \n", ( !! ( tty_opts_raw.c_oflag & ONLCR ) ));
         printf(" c_oflag::XTABS   = %d \n", ( !! ( tty_opts_raw.c_oflag & XTABS ) ));
         printf(" c_oflag::ENOENT  = %d \n", ( !! ( tty_opts_raw.c_oflag & ENOENT ) ));
     }
     // leave the output (stdout) behavoir the same :-)
     tty_opts_raw.c_oflag =  g_tty_opts_backup.c_oflag ;

     tcsetattr(STDIN_FILENO, TCSANOW, &tty_opts_raw);
}


void   restore_tty(void)
{
   if ( g_valid_tty_opts_backup != 0 )
   {
       // Restore previous TTY settings
       tcsetattr(STDIN_FILENO, TCSANOW, &g_tty_opts_backup);
       // clear the flag
       g_valid_tty_opts_backup = 0;
   }
}

void ftty_print_kb_data(kb_data * p,char * string)
{
     printf(" kb_data:  %s  %p  \n",string,p);
     printf("   int done           %d  \n",p->done);
     printf("   int debug          %d  \n",p->debug);
     printf("   int verbocity      %d  \n",p->verbosity);
     printf("   pthread_t tid_kb   %lx \n",p->tid_kb);
     printf("   frbuff * kb_buff   %p  \n",p->kb_buff);
}


 void *  ftty_kb_thread(void* arg)
 {
     kb_data * pkb =(kb_data *)arg;
     char c;
     int r;
     int done;   // there is a problem I need to figure out.  if some other thread kill the
                 // program, we can be sitting inside the getchar() function.  
                 // not clear if teh restor_tty() function get called which leave the console
                 // in a funky state. 
                 // also need to make sure the "restore_tty()" is safe if called multiple times

     WAI();
     printf("%sStarting kb  Thread%s\n",CYAN,NC);
     reconfig_tty();          // configure getchar() to return every keystroke
     done =0;
//     ftty_print_kb_data( pkb, "pdb");
     while( ( pkb->done == 0 ) && (done == 0))
     {
        c = getchar();
        r = RBuffPut( pkb->kb_buff,(void *) &c); //push value into ring buffer */
//        printf(" r: %d  %c \n",r,c);
         if ( r == 0)
        {
            if(g_ftty_Debug != 0)
            {
                 printf("\n[%s] %c  0x%x\n",__FUNCTION__,c,c);
            }
            if( c == 0x03) done=1;
        }
     }
     restore_tty();
     printf("%sEnding kb Thread%s\n",RED,NC);
     pkb->done=1 ;      
     return (void *)1;
 }

kb_data * ftty_kb_create(void)
{
    kb_data * p;
    p=(kb_data *) malloc(sizeof(kb_data));
    if ( p == NULL)
    {
        printf("%d:%s-%s  unable to malloc kb_data\n",__LINE__,__FILE__,__FUNCTION__);
        return NULL;
    }

    memset( p,0,sizeof(kb_data));
    // could set up some default here, especially if unitialization causes
    //             a seg fault;

    return p;
}


int  ftty_kb_start( kb_data * p)
{
     int err;

     err = RbuffInitialize ( &(p->kb_buff), &err, sizeof(char),8192, 32);  //
     if( err != 0)
     {
          printf("Failed to allocate Keyboard Buffer  %d \n",err);
          return -1;
     }
      err = pthread_create(&(p->tid_kb), NULL, &ftty_kb_thread, p); 
      if (err != 0)
           printf("\ncan't create thread :[%s]\n", strerror(err));
      else
           printf("\n Thread created successfully\n");

   return 0;
}
int  ftty_kb_stop( kb_data * p )
{
   // test someone pass us NULL
   if ( p == NULL)  return (-3);

   p->done=1;
   // wait for thread to stop
   sleep (1); 
   
   if ( p->kb_buff != NULL)
   {
         RbuffClose (p->kb_buff);
         p->kb_buff=NULL;
   }      
   return 0;
}
int  ftty_kb_distroy(  kb_data * p)
{
   // test someone pass us NULL
   if ( p != NULL)  
   {
       ftty_kb_stop (p);
       free (p);
       p = 0;
   }
   return 0;
}

/////////////////////////////////////////////
// return 0 if char is good,  else, 
//  return value comes from rbuff fetch function
int  ftty_kb_fetch(  kb_data * p, char * c)
{
   int    r;
   char   mc ;
   char * pc ;

   pc = &mc;
   r = RBuffFetch( p->kb_buff,(void **) &pc);
   if ( r == 0 )
   {
//        printf("[%s] r: %d   c: %c %02x\n",__FUNCTION__,r,*pc,*pc);
        *c = *pc;
        return 0;
//      printf("in:  %c %x \n",*pc,*pc);
//        if ( *pc == 0x03) done =1;
   }
   return r;
}


void test_tty(void)
{
    printf("hit 'ctrl-C' to exit \r\n");
    // Read and print characters from stdin
     int c, i = 1;
     for (c = getchar(); c != 3; c = getchar()) {
         printf("%d. 0x%02x (0%02o)\r\n", i++, c, c);
    }
    printf("You typed 0x03 (003). Exiting.\r\n");
 
}


#ifdef  STANDALONE_TTY
char RED[]     = { 0x1b,'[','1',';','3','1','m',0x00 };
char GREEN[]   = { 0x1b,'[','1',';','3','2','m',0x00 };
char YELLOW[]  = { 0x1b,'[','1',';','3','3','m',0x00 };
char BLUE[]    = { 0x1b,'[','1',';','3','4','m',0x00 };
char MAGENTA[] = { 0x1b,'[','1',';','3','5','m',0x00 };
char CYAN[]    = { 0x1b,'[','1',';','3','6','m',0x00 };
char NC[]      = { 0x1b,'[','0','m',0x00 };

kb_data * gpkb = NULL;

  
int  main(int arc, char ** argv)
{
int done=0;


     gpkb = ftty_kb_create();
     ftty_print_kb_data( gpkb, "after init");
 
     ftty_kb_start( gpkb );
     ftty_print_kb_data( gpkb, "after start");

         printf("ctrl-C to exit \n\r");
         while( done == 0)
         {
             char c;
             int r;
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
                 if ( c == 0x03) done =1;
             }
          }


     ftty_kb_stop( gpkb );
     ftty_kb_distroy(  gpkb );

//    reconfig_tty();
//    test_tty();
//    restore_tty();
  
  }
  #endif
