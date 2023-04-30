#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <pthread.h>    // for mutex.
#include "flist.h"


// Reference; https://www.learn-c.org/en/Linked_lists

/*  *** REFERENCE ONLY.  ******
typedef struct node {
    char string[64] ;  // this could be a pointer to data instead of storing the data
    struct node * next;
} node_t;


This version stores a ptr to a struct in the list structure.   
 Sruct functoins required:
      DataPrintMy() - knows how to print the structure
      DataCopyMy()  - knows how to copy the from src ptr to dst ptr.
      DataCompareMy_x() - knows how to compare structure passed in
                          against Items in the list. Must return  
                          0 on match  !0 on no match 


 On Push  - The code itself mallocs space and ccopies the data structure 
            into the allocated buffer.   
 On Pop   - If a pointer to a structure is passed is passed in, the 
            removed list item will be copied into the data structure.
 On Match - Returns 0 on the first "dat_ptr/DataCompareMy_x() found in 
            the list data.  If a DataCopyMy() is supplied, all data
            stored in the list element is copied to the location point 
            to by the dat_ptr.   
 On PopMatch - Does everything of the "Match" data above, and removes
            the element from the list.
  
 On NumberOfMatch - scans the list against the ptr_data/DataCompareMy_x()
            and returns how many matching elements are found in the 
            list.  
 On Print - Prints each piece of data in the list. 
 
 On NumberOfElements = returns the total number of elements stored in the
            list;

*/

/******************************************************************
*  support functions
*******************************************************************/

void DataPrintMy(void * pdata)
{
   my_data_t *p =(my_data_t *) pdata;
   printf("        %p   d: %ld   s: %s\n ",p,p->d,p->s);
}

void DataCopyMy(void * Dst, void * Src)
{
   memcpy(Dst,Src,sizeof(my_data_t)); 
}

int64_t DataCompareMy_s(void * A, void * B)
{
   return strcmp( ((my_data_t *)A)->s , ((my_data_t *)B)->s);
}

int64_t DataCompareMy_d(void * A, void * B)
{
   return ! ( ((my_data_t *)A)->d == ((my_data_t *)B)->d );
}

   
/******************************************************************
*  List functions 
*******************************************************************/
pthread_mutex_t g_lock;
#define GLOBAL_FLIST_MUTEX_LOCK()    pthread_mutex_lock(&g_lock); 
#define GLOBAL_FLIST_MUTEX_UN_LOCK() pthread_mutex_unlock(&g_lock);


#define WAI()   printf("%d %s:%s\n",__LINE__,__FILE__,__FUNCTION__)

int g_verbose_flist=0;

#define VT(level) if( level <= g_verbose_flist ) 



int64_t ListPushHead(node_t **head, void* addData,int addData_sz, void (*CopyFunction)(void* src, void* dst))
{
    pthread_mutex_t *plock = NULL;
    node_t * new_node = *head;
    
//   if *head = NULL, there is no list and this is the first element
    VT(2) WAI();
    // check if list is empty.
    if ( new_node  == NULL )
    {
        GLOBAL_FLIST_MUTEX_LOCK();
        new_node = (node_t *) malloc(sizeof(node_t));
        if ( new_node  == NULL) {
            GLOBAL_FLIST_MUTEX_UN_LOCK();
            return -2;
        }
       // allocate a list lock
        new_node->pl_lock = (pthread_mutex_t  *) malloc(sizeof(pthread_mutex_t));
        if ( new_node->pl_lock  == NULL) {
            GLOBAL_FLIST_MUTEX_UN_LOCK();
            free (new_node);
            return -3;
        }

// <Data>
        new_node->pd = (void *) malloc(addData_sz);
        CopyFunction( new_node->pd , addData );
// </Data>
        new_node->next = NULL;        
        *head = new_node;
        GLOBAL_FLIST_MUTEX_UN_LOCK();
        return 0;
    }  

    //  fish out the lock structure
    plock=(*head)->pl_lock;

    // allocate new list struct
    pthread_mutex_lock( plock);

    new_node = (node_t *) malloc(sizeof(node_t));
    if (new_node == 0)
    {
       pthread_mutex_unlock( plock);
       return -1;
    } 
// <Data>
    // alllocate memory for data, and transfer.
    new_node->pd = (void *) malloc(addData_sz);
    CopyFunction ( new_node->pd , addData );
// </Data>

    // move the lock to the new structure
    new_node->next = *head;
    // move the head forward 
    *head = new_node;
    // unlock this list lock
    pthread_mutex_unlock(plock);

    return 0;
}

int64_t ListPushEnd(node_t **head, void * addData,int addData_sz,void (*CopyFunction)(void * src, void * dst))
{
   pthread_mutex_t *plock = NULL;
   node_t * p_node = NULL;
   VT(2) WAI();
   // special case - no item in the list
   if (*head == NULL) 
   {
       GLOBAL_FLIST_MUTEX_LOCK();
       p_node = (node_t *) malloc(sizeof(node_t));
       if ( p_node  == NULL) {
           GLOBAL_FLIST_MUTEX_UN_LOCK();
           return -3;
       }
       // allocate a list lock in the new structure
       p_node->pl_lock = (pthread_mutex_t  *) malloc(sizeof(pthread_mutex_t));
       if ( p_node->pl_lock  == NULL) {
           GLOBAL_FLIST_MUTEX_UN_LOCK();
           printf( "[%s] Should never get here \n",__FUNCTION__);
           free (*head);
            return -3;
        }

// <Data>
       p_node->pd = (void *) malloc(addData_sz);
       CopyFunction ( p_node->pd, addData );
// </Data>
      p_node->next=NULL;
      *head = p_node;        
       GLOBAL_FLIST_MUTEX_UN_LOCK();
       return 0;
    } // end *head == NULL 
 
   p_node=*head;
   plock=(*head)->pl_lock ;

   // fine the end of the list
   while ( p_node->next != NULL) 
   {
       VT(3)  printf(" %p  %p  %p\n",p_node,p_node->next,p_node->pd );
       p_node = p_node->next;
   }
   VT(3) printf(" %p  %p  %p\n",p_node,p_node->next,p_node->pd );

   // lock the list
   pthread_mutex_lock(plock);

   // allocate memory for new item
   p_node->next = (node_t *) malloc(sizeof(node_t));
   if (p_node->next == NULL) 
   {
        return -1;  // failed to add data
   }
   p_node=p_node->next;
   p_node->next=NULL;
   // move the lock forward
   p_node->pl_lock=plock;

// <Data>
   p_node->pd = (void *) malloc(addData_sz);
   CopyFunction ( p_node->pd , addData );
// </Data>
   pthread_mutex_unlock(plock);
   return 0;
}

// Removes the first Item in the array
int64_t  ListPopHead(node_t **head, void * removedData ,  void (*CopyFunction)(void * src, void * dst))
{
    pthread_mutex_t *plock = NULL;
    node_t * next_node = NULL;
    VT(2) WAI();
    if (*head == NULL) {
        return LST_LIST_IS_EMPTY;   // return 1 if list is empty
    }
    plock = (*head)->pl_lock ; 
    pthread_mutex_lock( plock );
    next_node = (*head)->next;

// <Data>
    if(( removedData != NULL) && (CopyFunction != NULL))
    { 
        CopyFunction( removedData, (*head)->pd ); /* need to copy function to xfer the 
                                                     entire data struct to the data_ptr */
    }

    free ( (*head)->pd ); // free the stored data pointer 
// </Data>

    free(*head);  // free the list struct
    *head = next_node;
    pthread_mutex_unlock(plock);
    return 0;   
}

int64_t ListPopEnd(node_t **head, void * removedData,  void (*CopyFunction)(void * src, void * dst))
{
   VT(2) WAI();
   pthread_mutex_t *plock = NULL;
   node_t * current_node=NULL;
   node_t * prev_node=NULL;

   // Special Case no elements in the list
   if ( *head == NULL )
   {
       return LST_LIST_IS_EMPTY ;   // 1 because it is empty;
   }
   plock = (*head)->pl_lock ;

   // Special case, one element in the list
   if ( (*head)->next == NULL)
   {
      // use the global lock here because we will be setting Head prt to NULL;
      GLOBAL_FLIST_MUTEX_LOCK()
// <Data>
      if(( removedData != NULL) && (CopyFunction != NULL))
      { 
         CopyFunction(removedData, (*head)->pd ); /* need to copy function 
                          to xfer the entire data struct to the data_ptr */
      }
      free ( (*head)->pd ); // free the stored data pointer 
      free ( (*head)->pl_lock ); // free the stored data pointer 
// </Data>
      free (*head);
      *head= NULL;
      GLOBAL_FLIST_MUTEX_UN_LOCK();
      return 0;
   }    
   prev_node=*head;
   current_node=(*head)->next;

   while ( current_node->next != NULL) 
   {
       VT(3) printf(" %p  %p  %p\n",current_node,current_node->next,current_node->pd );
       prev_node=current_node;
       current_node = current_node->next;
   }
   VT(3) printf(" %p  %p  %p\n",current_node,current_node->next,current_node->pd );

 
    pthread_mutex_lock(plock);
// <Data>
    if(( removedData != NULL) && (CopyFunction != NULL))
    { 
        CopyFunction(removedData, current_node->pd ); /* need to copy function to 
                            xfer the entire data struct to the data_ptr */
    }
    free ( current_node->pd ); // free the stored data pointer 
// </Data>
   prev_node->next=NULL;
   free (current_node);
   pthread_mutex_unlock( plock);
   return 0;
} 


// prints the contents of the list;
void ListPrint(node_t *head ,void (*PrintFunction)(void * pd)) 
{
   int i = 0;
   VT(2) WAI();
   printf(" ");
   while ( head != NULL) 
   {
      if ( g_verbose_flist  == 0)
      {      
          printf("  %3d)  ",i );
          PrintFunction(head->pd) ;
      }
      else
      { 
          printf("  %3d) %p  %p ",i,head,head->next );
          PrintFunction(head->pd) ;
      }
      i++;
      head = head->next;
   }
   printf("\n");
}

///////////////////////////////////////////////////////////
//   ptr_data is storage structure.  
//      if a match is found, the data structure in the list
//      is copied to the ptr_data Struct. 
//
//returns 0 if the value exists.
//        1 if the value does not
int64_t ListMatch(node_t *head, void *ptr_data, int64_t (*CompareFunction)(void * A, void * B), void (*CopyFunction)(void * src, void * dst)  )
{
   VT(2) WAI();
   while ( head != NULL)
   { 
      if ( CompareFunction( head->pd, ptr_data) == 0)
      {
          if ( CopyFunction != NULL)
          { 
              CopyFunction(ptr_data,head->pd); /* need to copy function to
                             xfer the entire data struct to the data_ptr */
          }
          return 0;
      }
      head = head->next;
   }
   return 1;
}

// How many records in the list match "Data",
int64_t ListNumberOfMatch(node_t *head,void * ptr_data, int64_t (*CompareFunction)(void * A, void * B))
{
   int i = 0 ;
   VT(2) WAI();
   while ( head != NULL)
   { 
      if ( CompareFunction( head->pd, ptr_data) == 0)
      {
          i++ ;         
      }
      head = head->next;
   }
   return i;
}

// How many records in the list",
int64_t ListNumberOfElements(node_t *head)
{
   int i = 1 ;
   VT(2) WAI();

   /*special case, passed in an empty list */
   if ( head == NULL ) return 0;
   
   while ( head->next != NULL)
   { 
      i++ ;   // one more elementt       
      head = head->next;
   }
   return i;
}





// deletes the first match  of "data" from the list.
int64_t ListPopMatch(node_t **head,void *ptr_data, int64_t (*CompareFunction)(void * A, void* B),void (*CopyFunction)(void * src, void * dst) )
{
   VT(2) WAI();
   node_t * current_node=NULL;
   node_t * prev_node=NULL;

   // Special Case no elements in the list
   if ( *head == NULL )
   {
       return LST_LIST_IS_EMPTY ;   // 1 because it is empty;
   }

   // Special case, match on first element !
   if (CompareFunction (ptr_data, (*head)->pd) == 0) 
   {
       // use the global lock here because we could be setting Head prt to NULL;
       GLOBAL_FLIST_MUTEX_LOCK();
// <Data>
       if(( ptr_data != NULL) && (CopyFunction != NULL))
       { 
           CopyFunction(ptr_data, (*head)->pd ); /* need to copy function 
                        to xfer the entire data struct to the data_ptr */
       }
       free ((*head)->pd); // free the stored data.
// </Data>

       if ( (*head)->next != NULL) // there are more  
       {
           prev_node = (*head)->next; // need a holder   
           free (*head);
           *head = prev_node ;
           GLOBAL_FLIST_MUTEX_UN_LOCK();
           return LST_SUCCESS;
       } 
       else  // nope, only 1 element
       {
           free ((*head)->pl_lock);   // free the list lock
           free (*head);              // free the first list node
           *head= NULL;               // set the calling ptr to NULL;
           GLOBAL_FLIST_MUTEX_UN_LOCK();
           return LST_SUCCESS;
       }   
   }    
 
   prev_node=*head;
   current_node=(*head)->next;

   while ( current_node->next != NULL) 
   {
       if (CompareFunction (ptr_data, current_node->pd) == 0) // I have one, and there is a record after this one.
       {
// <Data>
          if(( ptr_data != NULL) && (CopyFunction != NULL))
          { 
              CopyFunction( ptr_data, current_node->pd); /* need to copy function 
                            to xfer the entire data struct to the data_ptr */
          }
          free ( current_node->pd );
// </Data>
           pthread_mutex_lock((*head)->pl_lock); 
           prev_node->next = current_node->next ; // fix up the pointers
           free (current_node) ;                  // free this record
           pthread_mutex_unlock((*head)->pl_lock); 
           return LST_SUCCESS;
       }
       VT(3) printf(" %p  %p  %p\n",current_node,current_node->next,current_node->pd );
       pthread_mutex_lock((*head)->pl_lock); 
       prev_node=current_node;
       current_node = current_node->next;
       pthread_mutex_unlock((*head)->pl_lock); 
   }
   VT(3) printf(" %p  %p  %p\n",current_node,current_node->next,current_node->pd );
 
   // check the last record

   if (CompareFunction (ptr_data, current_node->pd) == 0) // I have one, and the last record.
   {
// <Data>
          if(( ptr_data != NULL) && (CopyFunction != NULL))
          { 
              CopyFunction(ptr_data , current_node->pd ); /* need to copy function 
                          to xfer the entire data struct to the data_ptr */
          }
          free ( current_node->pd );
// </Data>
       
       pthread_mutex_lock((*head)->pl_lock); 
       prev_node->next=NULL;
       free (current_node) ;
       pthread_mutex_unlock((*head)->pl_lock); 
       return LST_SUCCESS;
   }
   return LST_DATA_NOT_FOUND ;

} 




//#define FLIST_STANDALONE
#ifdef  FLIST_STANDALONE

/*
char stringA[]={"Kilroy was here"};
char stringB[]={"mama told me not to come"};
char stringC[]={"1234567890"};
char stringD[]={"Need I say more #4"};
*/

my_data_t dataA = {.d=1 , .s="Kilroy was here"};
my_data_t dataB = {.d=2 , .s="mama told me not to come" };
my_data_t dataC = {.d=3 , .s="1234567890"               };
my_data_t dataD = {.d=4 , .s="Need I say more #4"       };





int test1(void);
int test1(void)
{
    node_t * head = NULL;
//    char     ReadData[64];
//    char  *  pData=&(ReadData[0]);
    int64_t  result;
    
    my_data_t myReadData;
    my_data_t *p_myData = &myReadData;

    printf("\n\n\n");
    printf(" ----------------------------------------\n");
    printf(" ----- Test ListPushHead(),  ListPushEnd() \n");
    printf(" -----      ListPopHead(),   ListPopEnd()  \n");
    printf(" ----------------------------------------\n");




    printf("1st ListPushHead dataA : %ld %s\n",dataA.d,dataA.s);
    result =ListPushHead( &head,  &dataA,sizeof(my_data_t), DataCopyMy);
    if ( result != 0)
        printf (" ListPushHead returned error %ld  \n",result );
    VT(4) ListPrint(head ,DataPrintMy  );

    printf("2nd ListPushHead dataB : %ld %s\n",dataB.d,dataB.s);
    result =ListPushHead( &head,  &dataB,sizeof(my_data_t), DataCopyMy);
    if ( result != 0)
        printf (" ListPushHead returned error %ld  \n",result);
    VT(4) ListPrint(head ,DataPrintMy  );

    printf("3rd ListPushHead dataC : %ld %s\n",dataC.d,dataC.s);
    result =ListPushHead( &head,  &dataC,sizeof(my_data_t), DataCopyMy);
    if ( result != 0)
        printf (" ListPushHead returned error %ld  \n",result);
    VT(4) ListPrint(head ,DataPrintMy  );

    printf("4th ListPushEnd  dataD : %ld %s\n",dataD.d,dataD.s);
    result =ListPushEnd( &head,  &dataD,sizeof(my_data_t), DataCopyMy);
    if ( result != 0)
        printf (" ListPushEnd returned  error %ld  \n",result );

    printf("5th ListPrint \n");
    ListPrint(head ,DataPrintMy  );

    printf("6th  ListPopEnd \n");
    result =ListPopEnd( &head, (void*) p_myData,DataCopyMy);
    if ( result == 0 )
    {
        printf(" ListPopEnd returned:   \n");
        DataPrintMy( (void*) p_myData);
    }
    else
        printf (" ListPopEnd returned error %ld \n",result);

    printf(" print ListPrint\n");
    ListPrint(head ,DataPrintMy  );

    printf("\n  Cleanup: (via ListPopHead)\n");
    do {
       result =ListPopHead( &head , (void *) p_myData ,DataCopyMy);
       if ( result == 0 )
       {
            DataPrintMy( (void*) p_myData);
       }
    } while ( result == 0 );

    printf("\n");

    printf(" DONE!");
    return 0;
}


// check pop end with no items and one item in the list
int test2(void);
int test2(void)
{
    node_t * head = NULL;
    int64_t  result;
    my_data_t myReadData;
    my_data_t *p_myData = &myReadData;

    printf("\n\n\n");
    printf(" ----------------------------------------\n");
    printf(" ----- Test ListPop() & ListPopEnd       \n");
    printf(" ----------------------------------------\n");

//////////////////////////////////////////////
    printf("  Pop end with no Items in the list  \n");

    result =ListPopEnd( &head, (void *) p_myData,DataCopyMy);
    if ( result == 0 )
    {
        printf(" ListPopEnd returned: %ld   \n",result);
        DataPrintMy ( p_myData);
    }
    else if ( result == 1 )
        printf (" ListPopEnd returned empty list:  %ld \n",result);
    else
        printf (" ListPopEnd returned error:  %ld \n",result);

    printf(" print the contents of the list\n");
    ListPrint(head ,DataPrintMy  );

//////////////////////////////////////////////
    printf("\n\n ----------------------------------------\n");
    printf("    push one item in the list then pop end \n");


    printf(" Push 1 string on the list:  %ld %s \n",dataA.d,dataA.s);
    result =ListPushHead( &head,  &dataA,sizeof(my_data_t), DataCopyMy);
    VT(4) ListPrint(head ,DataPrintMy  );

    printf(" ListPopEnd\n");
    result =ListPopEnd( &head, (void *) p_myData ,sizeof(my_data_t), DataCopyMy);
    if ( result == 0 )
    {
        printf(" ListPopEnd returned: %ld :   \n",result);
        DataPrintMy ( p_myData);
    }
    else if ( result == 1 )
        printf (" ListPopEnd returned empty list:  %ld \n",result);
    else
        printf (" ListPopEnd returned error:  %ld \n",result);

    printf(" print the contents of the list\n");
    ListPrint(head ,DataPrintMy  );

    printf(" ListPopEnd\n");
    result =ListPopEnd( &head, (void *) p_myData, DataCopyMy);
    if ( result == 0 )
    {
        printf(" ListPopEnd returned: %ld :   \n",result);
        DataPrintMy ( p_myData);
    }
    else if ( result == 1 )
        printf (" ListPopEnd returned empty list:  %ld \n",result);
    else
        printf (" ListPopEnd returned error:  %ld \n",result);

    printf(" print the contents of the list\n");
    ListPrint(head ,DataPrintMy  );

    printf("\n");
    printf(" DONE!");
    return 0;
}




int test3(void);
int test3(void)
{
    node_t * head    = NULL;
    int64_t  result;
    my_data_t myReadData;
    my_data_t *p_myData = &myReadData;


    printf("\n\n\n");
    printf(" ----------------------------------------\n");
    printf(" ----- Test compare functions            \n");
    printf(" DataCompare_s()   DataCompare_d()       \n");
    printf(" ----------------------------------------\n");
    printf("\n    push 4 item in, find if an entry exists \n");

    ListPushHead( &head,  &dataA,sizeof(my_data_t),DataCopyMy);
    ListPushHead( &head,  &dataB,sizeof(my_data_t),DataCopyMy);
    ListPushHead( &head,  &dataC,sizeof(my_data_t),DataCopyMy);
    ListPushHead( &head,  &dataD,sizeof(my_data_t),DataCopyMy);
    
    printf("\n  Load: \n");
    ListPrint(head ,DataPrintMy  );

    printf("  Find the Data (int): %ld \n",dataC.d);
    // zero out the myReadData Structure
    memset ( &myReadData,0,sizeof(my_data_t) );
    // initialize only the int part.
    myReadData.d=dataC.d;   
    // find the date. 
    result = ListMatch( head, &myReadData , DataCompareMy_d , DataCopyMy );
    if( result == 0 )
    {
           printf(" found: %ld in a record   \n ",myReadData.d);
           DataPrintMy ( &myReadData );
    }
    else
       printf("  Data intnot found : %ld\n",result);

    printf("\n  Find the Data String: %s \n",dataB.s);
    // zero out the myReadData Structure
    memset ( &myReadData,0,sizeof(my_data_t) );
    // initialize only the int part.
    strcpy( myReadData.s,dataB.s ) ;   
    // find the date. 
    result = ListMatch( head, &myReadData , DataCompareMy_s , DataCopyMy);
    if( result == 0 )
    { 
           printf(" found: %s in a record   \n ",myReadData.s);
           DataPrintMy ( &myReadData );
    }
    else
       printf("  Data String not found : %ld\n",result);

    printf("\n  Cleanup: \n");
    do {
       result =ListPopHead( &head , (void*) p_myData , DataCopyMy );
       if ( result == 0 )
       {
            DataPrintMy ( p_myData );
       }
    } while ( result == 0 );

    printf("\n");
    printf(" DONE!");
    return 0;
}


int test4(void);
int test4(void)
{
    node_t * head = NULL;
    int64_t  result;
    my_data_t myReadData;
    my_data_t *p_myData = &myReadData;

    printf("\n\n\n");
    printf(" ----------------------------------------\n");
    printf(" ----- Test ListPopMatch()               \n");
    printf(" ----------------------------------------\n");

    printf("    push 4 item in, delete c,d,a,b \n");

    ListPushEnd( &head,  &dataA,sizeof(my_data_t), DataCopyMy);
    ListPushEnd( &head,  &dataB,sizeof(my_data_t), DataCopyMy);
    ListPushEnd( &head,  &dataC,sizeof(my_data_t), DataCopyMy);
    ListPushEnd( &head,  &dataD,sizeof(my_data_t), DataCopyMy);
    printf("\n  Load: \n");
    ListPrint(head ,DataPrintMy  );

    printf("\n   Delete: (int)  %ld \n", dataC.d );
    // zero out the myReadData Structure
    memset ( &myReadData,0,sizeof(my_data_t) );
    // initialize only the int part.
    myReadData.d=dataC.d;   
    // find the data. 
    result =ListPopMatch( &head, (void *) &myReadData, DataCompareMy_d , DataCopyMy);
    if ( result == LST_SUCCESS )
    {
        printf(" ListPopMatch Successfully deleted: %ld  \n",result);
        DataPrintMy( (void *)&myReadData);
    }
    else if ( result == LST_LIST_IS_EMPTY ) 
        printf (" ListPopMatch LST_LIST_IS_EMPTY %ld \n",result);
    else
        printf (" ListPopMatch returned error %ld \n",result);
    printf(" print the contents of the list\n");
    ListPrint(head ,DataPrintMy  );


    printf("\n   Delete: (int)  %ld \n", dataD.d);
    // zero out the myReadData Structure
    memset ( &myReadData,0,sizeof(my_data_t) );
    // initialize only the int part.
    myReadData.d=dataD.d;   
    // find the data. 
    result =ListPopMatch( &head, (void *) p_myData, DataCompareMy_d , DataCopyMy);
    if ( result == LST_SUCCESS )
    {
        printf(" ListPopMatch Successfully deleted: %ld  \n",result);
        DataPrintMy( (void *)p_myData);
    }
    else if ( result == LST_LIST_IS_EMPTY ) 
        printf (" ListPopMatch LST_LIST_IS_EMPTY %ld \n",result);
    else
        printf (" ListPopMatch returned error %ld \n",result);
    printf(" print the contents of the list\n");
    ListPrint(head ,DataPrintMy  );


    printf("\n   Delete:  (int) %ld \n", dataA.d);
    // zero out the myReadData Structure
    memset ( &myReadData,0,sizeof(my_data_t) );
    // initialize only the int part.
    myReadData.d=dataA.d;   
    // find the data. 
    result =ListPopMatch( &head, (void *) p_myData, DataCompareMy_d , DataCopyMy);
    if ( result == LST_SUCCESS )
    {
        printf(" ListPopMatch Successfully deleted: %ld  \n",result);
        DataPrintMy( (void *)p_myData);
    }
    else if ( result == LST_LIST_IS_EMPTY ) 
        printf (" ListPopMatch LST_LIST_IS_EMPTY %ld \n",result);
    else
        printf (" ListPopMatch returned error %ld \n",result);
    printf(" print the contents of the list\n");
    ListPrint(head ,DataPrintMy  );


    printf("\n   Delete:  (int) %ld \n", dataB.d);
    // zero out the myReadData Structure
    memset ( &myReadData,0,sizeof(my_data_t) );
    // initialize only the int part.
    myReadData.d=dataB.d;   
    // find the data. 
    result =ListPopMatch( &head, (void *) p_myData, DataCompareMy_d , DataCopyMy);
    if ( result == LST_SUCCESS )
    {
        printf(" ListPopMatch Successfully deleted: %ld  \n",result);
        DataPrintMy( (void *)p_myData);
    }
    else if ( result == LST_LIST_IS_EMPTY ) 
        printf (" ListPopMatch LST_LIST_IS_EMPTY %ld \n",result);
    else
        printf (" ListPopMatch returned error %ld \n",result);
    printf(" print the contents of the list\n");
    ListPrint(head ,DataPrintMy  );


    printf("\n   Delete:  %s \n", dataD.s);
    // zero out the myReadData Structure
    memset ( &myReadData,0,sizeof(my_data_t) );
    // initialize only the int part.
    myReadData.d=dataD.d;   
    // find the data. 
 
    result =ListPopMatch( &head, (void *) p_myData, DataCompareMy_d , DataCopyMy);
    if ( result == LST_SUCCESS )
    {
        printf(" ListPopMatch Successfully deleted: %ld  \n",result);
        DataPrintMy( (void *)p_myData);
    }
    else if ( result == LST_LIST_IS_EMPTY ) 
        printf (" ListPopMatch LST_LIST_IS_EMPTY %ld \n",result);
    else
        printf (" ListPopMatch returned error %ld \n",result);
    printf(" print the contents of the list\n");
    ListPrint(head ,DataPrintMy  );

    printf("\n");
    printf(" DONE!");
    return 0;
}

int test5(void);
int test5(void)
{
    node_t * head = NULL;
    int64_t  result;
    my_data_t myReadData;
    my_data_t *p_myData = &myReadData;

    printf("\n\n\n");
    printf(" ----------------------------------------\n");
    printf(" ----- Test ListNumberOfElements()        \n");
    printf(" -----      ListNumberOfMatches()        \n");
    printf(" ----------------------------------------\n");

    printf("Count multiple entries.  \n");
    ListPushHead( &head,  (void*) &dataA,sizeof(my_data_t), DataCopyMy);
    ListPushHead( &head,  (void*) &dataB,sizeof(my_data_t), DataCopyMy);
    ListPushHead( &head,  (void*) &dataB,sizeof(my_data_t), DataCopyMy);
    ListPushHead( &head,  (void*) &dataC,sizeof(my_data_t), DataCopyMy);
    ListPushHead( &head,  (void*) &dataC,sizeof(my_data_t), DataCopyMy);
    ListPushHead( &head,  (void*) &dataC,sizeof(my_data_t), DataCopyMy);
    ListPushEnd( &head, (void*) &dataD,sizeof(my_data_t), DataCopyMy);
    ListPushEnd( &head, (void*) &dataD,sizeof(my_data_t), DataCopyMy);
    ListPushEnd( &head, (void*) &dataD,sizeof(my_data_t), DataCopyMy);
    ListPushEnd( &head, (void*) &dataD,sizeof(my_data_t), DataCopyMy);
    ListPushEnd( &head, (void*) &dataD,sizeof(my_data_t), DataCopyMy);
    printf("\n  Load: \n");
    ListPrint(head ,DataPrintMy  );

    result = ListNumberOfElements( head);
    printf(" Total nmber of elemenst:  %ld \n",  result);
    result = ListNumberOfMatch( head, (void *) &dataA , DataCompareMy_d );
    printf("   count of \"%s\" %ld \n", dataA.s, result);
    result = ListNumberOfMatch( head, (void *) &dataB , DataCompareMy_d );
    printf("   count of \"%s\" %ld \n", dataB.s, result);
    result = ListNumberOfMatch( head, (void *) &dataC , DataCompareMy_d );
    printf("   count of \"%s\" %ld \n", dataC.s, result);
    result = ListNumberOfMatch( head, (void *) &dataD , DataCompareMy_d );
    printf("   count of \"%s\" %ld \n", dataD.s, result);

    ListPrint(head ,DataPrintMy  );

    printf("\n  Cleanup: \n");
    do {
       result =ListPopHead( &head , (void*)p_myData, DataCopyMy );
       if ( result == 0 )
       {
            DataPrintMy( (void *)p_myData);
       }
    } while ( result == 0 );

    printf("\n");

    printf(" DONE!");
    return 0;
}


int main(int argc, char ** argv);
int main(int argc, char ** argv)
{

    test1();
    test2();
    test3();
    test4();
    test5();
    printf("\n");
    printf(" DONE!");
    return 0;
}

#endif
