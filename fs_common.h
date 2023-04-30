#ifndef _fs_common_h
#define  _fs_common_h

// THIS FILE IS DIFFERENT FOR EACH Project I DO;


#define DEBUG_CFG_DATA_SSH     0x0001
#define DEBUG_TX_RX_SSH        0x0002
#define DEBUG_RX_RS232         0x0004
#define DEBUG_TX_RS232         0x0008
#define DEBUG_RX_KB            0x0010
#define DEBUG_RX_FTELNET       0x0100
#define DEBUG_TX_FTELNET       0x0200
#define DEBUG_RX_FTELNET_CMD   0x0400
#define DEBUG_TX_FTELNET_CMD   0x0800


#define DEBUG(x)   if ((g_Debug & x) == x)
extern uint64_t  g_Debug ;

#define V_SILENT      0
#define V_NORMAL      1
#define V_LOG         1
#define V_INFORMATION 2
#define V_WARNING     2
#define V_DEBUG       3
#define VERBOSE(x)  if ( g_Verbose >= x)
extern uint64_t  g_Verbose; 


#ifndef TRUE
#define TRUE 1
#endif




extern char RED[];    //  = { 0x1b,'[','1',';','3','1','m',0x00 };
extern char GREEN[];  //  = { 0x1b,'[','1',';','3','2','m',0x00 };
extern char YELLOW[]; //  = { 0x1b,'[','1',';','3','3','m',0x00 };
extern char BLUE[];   //  = { 0x1b,'[','1',';','3','4','m',0x00 };
extern char MAGENTA[]; // = { 0x1b,'[','1',';','3','5','m',0x00 };
extern char CYAN[];   //  = { 0x1b,'[','1',';','3','6','m',0x00 };
extern char NC[];     //  = { 0x1b,'[','0','m',0x00 };

#endif
