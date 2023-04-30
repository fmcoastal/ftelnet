#ifndef _ftelnet_h
#define _ftelnet_h


#define IP_ADDR_STRING_SIZE 64

typedef struct ftelnet_data_struct {
      char           name[128];
      char           dst_ip[IP_ADDR_STRING_SIZE];
      int            port;
      int            socket;       /* Socket  */
      int            verbosity;
      int            fd_log;       // if not null, write data to this
      int            interactive;  // if interactive code has been called
      node_t *       p_cmd_snd_list; // cmd lists
      node_t *       p_cmd_rcv_list; // cmd list
      frbuff *       ptx_buf;
      frbuff *       prx_buf;
      pthread_t      tid_rx;
      pthread_t      tid_tx;
      int            done ;      // signal thread to end
} ftelnet_data_t ;



 // if you error, you can figure out which connection base on "name"
ftelnet_data_t * ftelnet_create(char * name ) ;

void ftelnet_set_dest_ip( ftelnet_data_t * p, char* dst_ip );
void ftelnet_set_port( ftelnet_data_t * p, int port );
void ftelnet_set_verbosity( ftelnet_data_t * p, int verbosity );

int  ftelnet_init_session( ftelnet_data_t *  p);

int  ftelnet_open_session( ftelnet_data_t * p );

int ftelnet_close_session(ftelnet_data_t * p);
void ftelnet_distroy_session(ftelnet_data_t * p);

int ftelnet_check_channel( ftelnet_data_t * p);
int ftelnet_get_c(ftelnet_data_t * p, char * c);
int ftelnet_put_c(ftelnet_data_t * p, char c);

// otehr functions

void  ftelnet_print_ftelnet_data_t( ftelnet_data_t * p );
frbuff * ftelnet_get_tx_ring( ftelnet_data_t * p );
frbuff * ftelnet_get_rx_ring( ftelnet_data_t * p );


// COMMAND INFORMATION

////////////////////////////////////////
//  Send telnet control Message
//    format the string m w/ size m_sz into a ftelnet_cmd_t and
//    send the message
////////////////////////////////////////
int64_t ftelnet_send_ctrl_message(ftelnet_data_t *p, char * m,int m_sz);

////////////////////////////////////////
// _ Send telnet control Message
////////////////////////////////////////
int64_t  _ftelnet_send_ctrl_message(ftelnet_data_t *p, ftelnet_cmd_t *cmd );





#define CMD  255   // 0xff Command  
#define DONT 254   // 0xfe Dont  Enable 
#define DO   253   // 0xfd Offer to Enable 
#define WONT 252   // 0xfc Not Offering to Enable 
#define WILL 251   // 0xfb Offering to Enable 

#define NEW_ENVIONRMENT_OPTION 0x27
#define ENVIONRMENT_OPTION     0x24
#define TERMINAL_SPEED         0x20
#define NEGOTIATE_WIDOW_SIZE   0x1f
#define TERMINAL_TYPE          0x18
#define SUPRESS_GO_AHEAD       0x03
#define ECHO                   0x01 
#if 0
https://www.rfc-editor.org/rfc/rfc854.html
https://www.geeksforgeeks.org/introduction-to-telnet/

Commands of Telnet are identified by a prefix character, Interpret As 
Command (IAC) with code 255. IAC is followed by command and option codes.

The basic format of the command is as shown in the following figure :

       IAC Command_Code Option_Code
i

Character  Decimal    Binary    Meaning 
TERMINAL_TYPE 
WILL	251	11111011	1. Offering to enable. 
                                2. Accepting a request to enable. 
 
WON’T	252	11111100	1. Rejecting a request to enable. 
                                2. Offering to disable. 
                                3. Accepting a request to disable. 
 
DO	253	11111101`	1. Approving a request to enable. 
                                2. Requesting to enable. 
 
DON’T	254	11111110	1. Disapproving a request to enable. 
                                2. Approving an offer to disable. 
                                3. Requesting to disable. 


Code   Option               Meaning 
0	Binary	           It interprets as 8-bit binary transmission.
1	Echo	           It will echo the data that is received on one side to the other side.
3	Suppress go ahead  It will suppress go ahead signal after data.
5	Status	           It will request the status of TELNET.
6	Timing mark	   It defines the timing marks.
8	Line width	   It specifies the line width.
9	Page size	   It specifies the number of lines on a page.
24	Terminal type	   It set the terminal type.
32	Terminal speed	   It set the terminal speed.
34	Line mode	   It will change to the line mode.

#endif




#endif


