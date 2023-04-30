/**********************************************************************|
|                                                                      |
| File: frbuff.C                                                       |
| Author:
| Date:
| Description: Generic ring buffer routine.
|              used on framers for holding IRQ Data
|                                                                      |
|----------------------------------------------------------------------|

      
        Date     Name        Description of change
        x/xx/xx  F Smith
***********************************************************************/
#undef FRBUFF_DEBUG
//#define FRBUFF_DEBUG

#include <stdint.h>    // for uint64_t
#include <stdlib.h>    // for malloc free & NULL
#include <memory.h>    // for memcpy
#define fastmemcpy memcpy
#include <stdio.h>     // for sprintf
#include "frbuff.h"

#ifdef F_CPP
/////////////////////////////////////////////////////////////////////////////////
// Constructor 
/////////////////////////////////////////////////////////////////////////////////
cRbuffBase::cRbuffBase(int *result,int ElementSize,int Elements,int Pad)  //  
{
    
    if(Pad >= Elements)
    {
        *result = RBUFF_BAD_PARAMETERS;
        return;
    }

    m_ElementSize = ElementSize;
    m_ArraySize   = Elements;
    m_size        =  m_ElementSize * m_ArraySize ;  // calculate size
    

    if((m_base = malloc(m_size)) == NULL)
    {
        *result = FRBUFF_UNABLE_TO_ALLOCATE_BUFFER;
        return;
    }
    m_Pad = m_ElementSize * Pad; 
    m_inptr  = m_base ;                        /* reset the buffer */
    m_outptr = m_base ;                        /* reset the buffer */
    m_end = (void *)((int)m_base + m_size) ;                  // calculate end 
#ifdef FRBUFF_DEBUG
    RBuffPrintMembers("Debug Boot");    
#endif
    *result = NO_ERROR;
}



/////////////////////////////////////////////////////////////////////////////////
//   Distructor
//             free the memory allocated by initializing the ring buffer
/////////////////////////////////////////////////////////////////////////////////
cRbuffBase::~cRbuffBase()           //  
{
    free(m_base);
}

/* ----------------------------------------------------------------------
put data into ring buffer
-2 buffer full  data loss
-1 buffer full
0 success
-----------------------------------------------------------------------*/
int cRbuffBase::RBuffPut(void * data)    /* put value int ring buffer */
{
    void *tmpptr;
    

#ifdef FRBUFF_DEBUG
    RBuffPrintPointers("Put");
#endif

    fastmemcpy(m_inptr,data,m_ElementSize); /* copy the data */


    tmpptr = (void *)((int)m_inptr + m_ElementSize);   /* increment the buffer */
    
    if( tmpptr == m_end )     // check for wrap
    {
        tmpptr = m_base ;     // reset the buffer 
    }
    if( tmpptr == m_outptr )  // test buff full ?
    {
        return (FRBUFF_FULL);  // return FRBUFF_FULL 
    }
    
    m_inptr = tmpptr;  /* update the pointer */
    
    tmpptr = (void *)((int)tmpptr + m_Pad) ;  // check for overflow - add the pad
    
    if(tmpptr >= m_end )   // if past the end wrap please
    {
        tmpptr = (void *)((int)tmpptr - m_size);
    }
    
    if(tmpptr == m_outptr)   // if equal to outptr, flag gettng full
    {
        return FRBUFF_GETTING_FULL;          /* return buffer getting full */
    }


    return(0);
}
/* ----------------------------------------------------------------------
put data into ring buffer and allows the buffer to wrap and lose data
- use this function when you want to document something and want to
save only the last n pieces of data
-2 buffer full  data loss
-1 buffer full
0 success
-----------------------------------------------------------------------*/
int cRbuffBase::SWBuffPut(void * data)    /* put value int ring buffer */
{
    void * tmpptr;
    
    fastmemcpy(m_inptr,data,m_ElementSize); // put the data in the buffer 
    
    tmpptr = (void *)((int)m_inptr + m_ElementSize);   /* increment the buffer */
    
    if( tmpptr == m_end )
    {
        tmpptr = m_base ;    /* reset the buffer */
    }
    
    m_inptr = tmpptr;  /* update the pointer */
    return(0);
}

/* ----------------------------------------------------------------------
fetch data from the ring buffer
-1 buffer full
0 success
-----------------------------------------------------------------------*/
int cRbuffBase::RBuffFetch(void ** data) //fetch value out of ring buffer */
{
    void * tmpptr;
    
#ifdef FRBUFF_DEBUG
    RBuffPrintPointers("Fetch");
#endif
    tmpptr = (void *)((int)m_outptr + m_ElementSize);   /* increment the output buffer */
    
    if( tmpptr == m_end )   // check if pointing past the end
    {
        tmpptr = m_base ;    /* reset the buffer */
    }
    
    if( m_inptr == m_outptr )    /* test buff full ?*/
    {
#ifdef FRBUFF_DEBUG
        RBuffPrintPointers(" - No Data");
#endif
        return (FRBUFF_NO_DATA_AVAILABLE);  /* return -1 to flag no data */
    }

    *data = m_outptr ;  /* put the data in the buffer */
    m_outptr = tmpptr;
    return(0);
}

/* ----------------------------------------------------------------------
fills in data with the byte which the out ptr is pointing to.
also calculates the number of bytes available in the ring buffer
if data = 0x03 and function returns 0  - ignore the data;
else the data is good and what will be returned when fetch is called
0 empty
number of bytes else
-----------------------------------------------------------------------*/
int cRbuffBase::RBuffPeek(void ** data)
{
    register int dif;
    *data = m_outptr;     /* fill in the data */
    
    dif = (int)m_inptr - (int)m_outptr;  /* calc the buffer available */
    if (dif < 0) dif += m_size;   /* adjust for wrap */
    return(dif/m_ElementSize);
}



/* ----------------------------------------------------------------------
calculates the number of bytes available in the ring buffer
0 empty
number of bytes else
-----------------------------------------------------------------------*/
int cRbuffBase::RBuffAmountOfData(void)
{
    register int dif;
    dif = (int)m_inptr - (int)m_outptr;
    if (dif < 0)dif +=m_size;
    return(dif/m_ElementSize);
}


/* ----------------------------------------------------------------------
Print the input and output pointer values
-----------------------------------------------------------------------*/
void cRbuffBase::RBuffPrintPointers(char * String)
{
    char tmpstr[120];
    sprintf(tmpstr,"\n%s in:0x%08x out:0x%08x ",String,m_inptr,m_outptr);
    rtosPrint("%s",tmpstr);
}

/* ----------------------------------------------------------------------
Print the member values 
-----------------------------------------------------------------------*/
void cRbuffBase::RBuffPrintMembers(char * String)
{
    char tmpstr[120];
    sprintf(tmpstr,"\n%s",String); 
    rtosPrint("%s",tmpstr);
    sprintf(tmpstr,"\n   ElementSz  0x%04x            ArraySize 0x%04x",m_ElementSize,m_ArraySize); 
    rtosPrint("%s",tmpstr);
    sprintf(tmpstr,"\n   Pad        0x%04x            size %08x",m_Pad,m_size); 
    rtosPrint("%s",tmpstr);
    sprintf(tmpstr,"\n   Base       0x%08x        end 0x%08x\n",m_base,m_end);
    rtosPrint("%s",tmpstr);
    sprintf(tmpstr,"\n   in         0x%08x        out 0x%08x",m_inptr,m_outptr);
    rtosPrint("%s",tmpstr);
}

#else
// --------------------------- c code -------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
// Constructor 
/////////////////////////////////////////////////////////////////////////////////
int RbuffInitialize (frbuff ** pbuff,int *result, int ElementSize,int Elements,int Pad)  //
{

    if(Pad >= Elements)
    {
        *result = FRBUFF_BAD_PARAMETERS;
        return *result;
    }

    // see if I allocate the data structure or not
    if(*pbuff == NULL)
    {

        // allocate a structure
        if((*pbuff  = malloc(sizeof(frbuff))) == NULL)
        {
            *result = FRBUFF_UNABLE_TO_ALLOCATE_HANDLE;
            return *result;
        }
        
        (*pbuff)->allocatedStruct = 1;   // I will be 

        (*pbuff)->base = NULL ; // force next check to allocate a buffer
    }
    else
    {
        (*pbuff)->allocatedStruct = 0 ;   // Structure was Allocated
    }

    // now that I have as 
    (*pbuff)->ElementSize = ElementSize;
    (*pbuff)->ArraySize   = Elements;
    (*pbuff)->size        = (*pbuff)->ElementSize * (*pbuff)->ArraySize ;  // calculate size
 
    // allocate the data buffer
    if ((*pbuff)->base == NULL)
    {
        (*pbuff)->allocatedBuffer = 1;              // flag that I am allocating it
        //allocate a buffer for the Ring Buffer
        if(((*pbuff)->base = malloc((*pbuff)->size)) == NULL)
        {
            (*pbuff)->allocatedBuffer = 0;  // make sure close does not try to free this!
            RbuffClose(*pbuff);
            *result = FRBUFF_UNABLE_TO_ALLOCATE_BUFFER;
            return *result;
        }
    }
    else
    {
        (*pbuff)->allocatedBuffer = 0 ;   // Buffer was Allocated
    }
 

    (*pbuff)->Pad    =  (*pbuff)->ElementSize * Pad; 
    (*pbuff)->inptr  =  (*pbuff)->base ;                        /* reset the buffer */
    (*pbuff)->outptr =  (*pbuff)->base ;                        /* reset the buffer */
    (*pbuff)->end = (void *)((uint64_t)((*pbuff)->base) + ((*pbuff)->size)) ;   // calculate end 
#ifdef FRBUFF_DEBUG
    RBuffPrintMembers(*pbuff,"Debug Boot");    
#endif
    *result = NO_ERROR;
    return *result;
}



/////////////////////////////////////////////////////////////////////////////////
//   Distructor
//             free the memory allocated by initializing the ring buffer
/////////////////////////////////////////////////////////////////////////////////
int RbuffClose (frbuff * pbuff) 
{
    if(pbuff->allocatedBuffer == 1)  free( pbuff->base);
    if(pbuff->allocatedStruct == 1)  free( pbuff);
    return 0;
}

/* ----------------------------------------------------------------------
put data into ring buffer
-2 buffer full  data loss
-1 buffer full
0 success
-----------------------------------------------------------------------*/
int RBuffPut(frbuff * pbuff, void * data)    /* put value int ring buffer */
{

#ifdef FRBUFF_DEBUG
    RBuffPrintPointers(pbuff,"Put");
#endif

    fastmemcpy(pbuff->inptr,data,pbuff->ElementSize); /* copy the data */


    pbuff->tmpptrP = (void *)((uint64_t)pbuff->inptr + pbuff->ElementSize);   /* increment the buffer */
    
    if( pbuff->tmpptrP == pbuff->end )     // check for wrap
    {
        pbuff->tmpptrP = pbuff->base ;     // reset the buffer 
    }
    if( pbuff->tmpptrP == pbuff->outptr )  // test buff full ?
    {
        return (FRBUFF_FULL);  // return FRBUFF_FULL 
    }
    
    pbuff->inptr = pbuff->tmpptrP;  /* update the pointer */
    
    pbuff->tmpptrP = (void *)((uint64_t)pbuff->tmpptrP + pbuff->Pad) ;  // check for overflow - add the pad
    
    if(pbuff->tmpptrP >= pbuff->end )   // if past the end wrap please
    {
        pbuff->tmpptrP = (void *)((uint64_t)pbuff->tmpptrP - pbuff->size);
    }
    
    if(pbuff->tmpptrP == pbuff->outptr)   // if equal to outptr, flag gettng full
    {
        return FRBUFF_GETTING_FULL;          /* return buffer getting full */
    }


    return(0);
}
/* ----------------------------------------------------------------------
put data into ring buffer and allows the buffer to wrap and lose data
- use this function when you want to document something and want to
save only the last n pieces of data
-2 buffer full  data loss
-1 buffer full
0 success
-----------------------------------------------------------------------*/
int RBuffSWBuffPut(frbuff * pbuff, void * data)    /* put value int ring buffer */
{
    
    fastmemcpy(pbuff->inptr,data,pbuff->ElementSize); // put the data in the buffer 
    
    pbuff->tmpptrP = (void *)((uint64_t)pbuff->inptr + pbuff->ElementSize);   /* increment the buffer */
    
    if( pbuff->tmpptrP == pbuff->end )
    {
        pbuff->tmpptrP = pbuff->base ;    /* reset the buffer */
    }
    
    pbuff->inptr = pbuff->tmpptrP;  /* update the pointer */
    return(0);
}

/* ----------------------------------------------------------------------
fetch data from the ring buffer
-1 buffer full
0 success
-----------------------------------------------------------------------*/
int RBuffFetch(frbuff * pbuff, void ** data) //fetch value out of ring buffer */
{
    
#ifdef FRBUFF_DEBUG
    RBuffPrintPointers(pbuff,"Fetch");
#endif
    pbuff->tmpptrF = (void *)((uint64_t)pbuff->outptr + pbuff->ElementSize);   /* increment the output buffer */
    
    if( pbuff->tmpptrF == pbuff->end )   // check if pointing past the end
    {
        pbuff->tmpptrF = pbuff->base ;    /* reset the buffer */
    }
    
    if( pbuff->inptr == pbuff->outptr )    /* test buff full ?*/
    {
#ifdef FRBUFF_DEBUG
        RBuffPrintPointers(pbuff," - No Data");
#endif
        return (FRBUFF_NO_DATA_AVAILABLE);  /* return -1 to flag no data */
    }

    *data = pbuff->outptr ;  /* put the data in the buffer */
    pbuff->outptr = pbuff->tmpptrF;
    return(0);
}

/* ----------------------------------------------------------------------
fills in data with the byte which the out ptr is pointing to.
also calculates the number of bytes available in the ring buffer
if data = 0x03 and function returns 0  - ignore the data;
else the data is good and what will be returned when fetch is called
0 empty
number of bytes else
-----------------------------------------------------------------------*/
int RBuffPeek(frbuff * pbuff, void ** data)
{
    *data = pbuff->outptr;     /* fill in the data */
    
    pbuff->dif = (uint64_t)pbuff->inptr - (uint64_t)pbuff->outptr;  /* calc the buffer available */
    if (pbuff->dif < 0) pbuff->dif += pbuff->size;   /* adjust for wrap */
    return(pbuff->dif/pbuff->ElementSize);
}



/* ----------------------------------------------------------------------
calculates the number of bytes available in the ring buffer
0 empty
number of bytes else
-----------------------------------------------------------------------*/
int RBuffAmountOfData(frbuff * pbuff)
{
    pbuff->dif = (uint64_t)pbuff->inptr - (uint64_t)pbuff->outptr;
    if (pbuff->dif < 0)pbuff->dif += pbuff->size;
    return(pbuff->dif/pbuff->ElementSize);
}


/* ----------------------------------------------------------------------
Print the input and output pointer values
-----------------------------------------------------------------------*/
void RBuffPrintPointers(frbuff * pbuff, char * String)
{
    sprintf(pbuff->tmpstr,"\n%s in:0x%p out:0x%p ",String,pbuff->inptr,pbuff->outptr);
    rtosPrint("%s",pbuff->tmpstr);
}

/* ----------------------------------------------------------------------
Print the member values 
-----------------------------------------------------------------------*/
void RBuffPrintMembers(frbuff * pbuff, char * String)
{
    sprintf(pbuff->tmpstr,"\n%s",String); 
    rtosPrint("%s",pbuff->tmpstr);
    sprintf(pbuff->tmpstr,"\n   ElementSz  0x%04x            ArraySize 0x%04x",pbuff->ElementSize,pbuff->ArraySize); 
    rtosPrint("%s",pbuff->tmpstr);
    sprintf(pbuff->tmpstr,"\n   Pad        0x%04x            size %08x",pbuff->Pad,pbuff->size); 
    rtosPrint("%s",pbuff->tmpstr);
    sprintf(pbuff->tmpstr,"\n   Base       0x%p        end 0x%p\n",pbuff->base,pbuff->end);
    rtosPrint("%s",pbuff->tmpstr);
    sprintf(pbuff->tmpstr,"\n   in         0x%p        out 0x%p\n",pbuff->inptr,pbuff->outptr);
    rtosPrint("%s",pbuff->tmpstr);
}





#endif


#if 0 // {

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>

#include "rbuff.h"
void main(void);

//ListFunctionCall list;

void main(void)
{
    int i, ReturnStatus, done;
    void * data;
    int argc;
    char cmdstring[120] = {0};
    char tstr0[120];
    char tstr1[120];
    char tstr2[120];
    char tstr3[120];
    char prompt[80];
    int result;
    strcpy(prompt,"test>");
    cRbuffBase * foo = new cRbuffBase(&ReturnStatus,sizeof(int),10,3);

#if 0
    for( i = 0 ; i < 20 ; i++)
            foo->RBuffPut(&i); 

    for( i = 0 ; i < 20 ; i++)
    {
       data = &i;
       foo->RBuffFetch( (&data)); 
       printf("\n result %d", i);
    }

       printf("\n done");

    return;
#endif    
    
    
    
    done = 0;
    while (done == 0)
    {
        printf("\n%s",prompt);
        cmdstring[0]=0x00;  // terminate the string to start
        tstr0[0]=0x00;  // terminate the string to start
        gets(cmdstring);
        //	printf("\necho string%s",cmdstring);
        argc = sscanf(cmdstring,"%s %s %s %s",tstr0,tstr1,tstr2,tstr3);
        switch (tstr0[0]){
        case 'A':
        case 'a':
            i = atoi(tstr1);
            result = foo->RBuffPut(&i); 
            if(result == FRBUFF_FULL ) 
            {
                printf("\nrbuff full");
            }
            else if(result == FRBUFF_GETTING_FULL ) 
            {
                printf("\nrbuff filling up");
            }
            
            break;
        case 'r':
        case 'R':
//           data = &i;  // need a place 
           if(foo->RBuffFetch( (&data)) == FRBUFF_NO_DATA_AVAILABLE)
           {
               printf("\nno buffer data available");
           }
           else
           {
               printf("\n result %d", *((int *)data));
           }
            break;
        case 'D':
        case 'd':
            printf("\nelements in buffer %d",foo->RBuffAmountOfData()); 
            break;

        case 'q':
        case 'Q':
            done = 1;
            break;
        default:
            break;
        }// end switch
    }// end done
}
#endif // }
