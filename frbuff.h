/**********************************************************************|
|                                                                      |
| File:      rbuff.H                                                   |
| Copyright (c) 1999  Natural MicroSystems Corp.  All rights reserved.
|
| All rights reserved.                                                 |
|                                                                      |
|----------------------------------------------------------------------|
-  Revision Information:

  Date      Name              Description
  x/xx/xx   F Smith
|**********************************************************************/
#ifndef _frbuf_h
#define _frbuf_h


#define rtosPrint printf

// ---------------------------- #defines  -----------------------------
#ifndef NO_ERROR
#define NO_ERROR 0
#endif
#define FRBUFF_NO_DATA_AVAILABLE            1
#define FRBUFF_BAD_PARAMETERS               2
#define FRBUFF_UNABLE_TO_ALLOCATE_BUFFER    3
#define FRBUFF_UNABLE_TO_ALLOCATE_HANDLE    4
#define FRBUFF_FULL                         5
#define FRBUFF_GETTING_FULL                 6

//////////////////////////////////////////////////////////////////
//  ring buffer
//////////////////////////////////////////////////////////////////



#ifdef F_CPP

class cRbuffBase{
public:
    //cRbuffBase();                                                  // Constructor 
    cRbuffBase(int *result,int ElementSize,int Elements,int Pad);  //  
    virtual      ~cRbuffBase(void);                                // Distructor
    
    // 
    // MGR Functions
    //

    int  m_ArraySize;
    int  m_ElementSize;
    int  m_Pad;
    int  m_size;
    void * m_base;
    void * m_inptr;
    void * m_outptr;
    void * m_end;
    
    inline void RBuffFlush(void) {m_outptr = m_inptr; }  // this flushes the buffer

    inline int RBuffData(void) { return((int)m_inptr - (int)m_outptr); }    // not zero if data in the buffer
    
    
    int Initialize (int ElementSize,int Elements,int Pad);  //  
    
    int RBuffPut(void * data );  /* put value int ring buffer */
    
     /* ----------------------------------------------------------------------
     put data into ring buffer and allows the buffer to wrap and lose data
     - use this function when you want to document something and want to
     save only the last n pieces of data
     0 success
    -----------------------------------------------------------------------*/
    int SWBuffPut(void * c);    /* put value int ring buffer */
    
    /* ----------------------------------------------------------------------
    fetch data from the ring buffer
    -1 buffer full
    0 success
    -----------------------------------------------------------------------*/
    int RBuffFetch(void ** data); /* fetch value out of ring buffer */
    
    
    /* ----------------------------------------------------------------------
    calculates the number of bytes available in the ring buffer
    0 empty
    number of bytes else
    -----------------------------------------------------------------------*/
    int RBuffAmountOfData(void);
    
    
    /* ----------------------------------------------------------------------
    fills in data with the byte which the out ptr is pointing to.
    also calculates the number of bytes available in the ring buffer
    if data = 0x03 and function returns 0  - ignore the data;
    else the data is good and what will be returned when fetch is called
    0 empty
    number of bytes else
    -----------------------------------------------------------------------*/
    int RBuffPeek(void ** data);
    

    // will print current Input and output Pointers
    void RBuffPrintPointers(char * String);

    // will print current Input and output Pointers
    void RBuffPrintMembers(char * String);

};



#else
//  c 


typedef struct {
    int  ArraySize;
    int  ElementSize;
    int  Pad;
    int  size;
    void * base;
    void * inptr;
    void * outptr;
    void * end;
    void * tmpptrF;
    void * tmpptrP;

    int allocatedStruct;
    int allocatedBuffer;
    int dif;
    char tmpstr[128];

} frbuff;


    static inline void RBuffFlush(frbuff * pbuff) { pbuff->outptr = pbuff->inptr; }    // not zero if data in the buffer

    static inline int RBuffData(frbuff * pbuff) { return((uint64_t)pbuff->inptr - (uint64_t)pbuff->outptr); }    // not zero if data in the buffer
    

    // to Initialize do one of the following
    //           pass the address of frbuff ptr.  Make sure the value of the ptr is NULL.   the Init will do the rest.
    //      or   Pass the Init function a pointer to a valid frbuff. If frbuff->base is NULL, the Init will allocate
    //      or   Pass the Init function a pointer to a vaild frbuff. and have frbuff->base, and frbuff->size initialized          
    
    int RbuffInitialize (frbuff ** pbuff, int *result, int ElementSize,int Elements,int Pad);  //  
    int RbuffClose (frbuff * pbuff);  //  
    
    int RBuffPut(frbuff * pbuff, void * data );  /* put value int ring buffer */
    
     /* ----------------------------------------------------------------------
     put data into ring buffer and allows the buffer to wrap and lose data
     - use this function when you want to document something and want to
     save only the last n pieces of data
     0 success
    -----------------------------------------------------------------------*/
    int RBuffSWBuffPut(frbuff * pbuff, void * c);    /* put value int ring buffer */
    
    /* ----------------------------------------------------------------------
    fetch data from the ring buffer
    -1 buffer full
    0 success
    -----------------------------------------------------------------------*/
    int RBuffFetch(frbuff * pbuff, void ** data); /* fetch value out of ring buffer */
    
    /* ----------------------------------------------------------------------
    calculates the number of bytes available in the ring buffer
    0 empty
    number of bytes else
    -----------------------------------------------------------------------*/
    int RBuffAmountOfData(frbuff * pbuff );
    
    /* ----------------------------------------------------------------------
    fills in data with the byte which the out ptr is pointing to.
    also calculates the number of bytes available in the ring buffer
    if data = 0x03 and function returns 0  - ignore the data;
    else the data is good and what will be returned when fetch is called
    0 empty
    number of bytes else
    -----------------------------------------------------------------------*/
    int RBuffPeek(frbuff * pbuff, void ** data);   

    // will print current Input and output Pointers
    void RBuffPrintPointers(frbuff * pbuff, char * String);

    // will print current Input and output Pointers
    void RBuffPrintMembers(frbuff * pbuff, char * String);

#endif

#endif
