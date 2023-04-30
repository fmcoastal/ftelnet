/* mkaddr.h
 *
 * Make a socket address:
 */

#ifndef _mkaddr_h
#define _mkaddr_h


/*
 * Create an AF_INET Address:
 *
 * ARGUMENTS:
 *  1.  addr    Ptr to area
 *              where address is
 *              to be placed.
 *  2.  addrlen Ptr to int that
 *              will hold the final
 *              address length.
 *  3.  str_addr The input string
 *              format hostname, and
 *              port.
 *  4.  protocol The input string
 *              indicating the
 *              protocol being used.
 *              NULL implies "tcp".
 * RETURNS:
 *  0   Success.
 *  -1  Bad host part.
 *  -2  Bad port part.
 *
 * NOTES:
 *  "*" for the host portion of the
 *  address implies INADDR_ANY.
 *
 *  "*" for the port portion will
 *  imply zero for the port (assign
 *  a port number).
 *
 * EXAMPLES:
 *  "www.lwn.net:80"
 *  "localhost:telnet"
 *  "*:21"
 *  "*:*"
 *  "ftp.redhat.com:ftp"
 *  "sunsite.unc.edu"
 *  "sunsite.unc.edu:*"
 */
int
mkaddr(void *addr,
  int *addrlen,
  char *str_addr,
  char *protocol);

/* print a socketaddr_in struct 
 *    addr is point to the variable
 *    varname is the name of the variable*/
 void printadr(char* varname,void *addr);

#endif

