#ifndef _flist_h
#define _flist_h

#define LST_SUCCESS 0
#define LST_LIST_IS_EMPTY 1 
#define LST_DATA_NOT_FOUND  2

// Reference; https://www.learn-c.org/en/Linked_lists

typedef struct my_data_struct {
    int64_t  d;
    char     s[64];
} my_data_t;


// all these  void * are  actually a my_data_t *)
void    DataPrintMy(void * pdata);   
void    DataCopyMy(void * Src, void * Dst);
int64_t DataCompareMy_d(void * A, void * B);
int64_t DataCompareMy_s(void * A, void * B);

//  for now, f_list has a global lock for when node_t = NULL;  
//     if node_t != null, then l_lock should point to a mutex lock
//     which is list dependent.   
//     the global mutex should be used when createing or deleting completely 
//     a list 

pthread_mutex_t g_lock;

/* list data Structure */
typedef struct node {
    void *        pd;     // void * > pointer to my_data_t
    pthread_mutex_t *pl_lock; // mutex for locking
    struct node * next;
} node_t;

// Add data to the list
int64_t ListPushHead(node_t **head, void* addData,int addData_sz,void (*CopyFunction)(void* src, void* dst));
int64_t ListPushEnd(node_t **head, void* addData,int addData_sz, void (*CopyFunction)(void* src, void* dst)) ;

// Removes the first/last Item in the list
//  returns 0 if data present
//          LST_LIST_IS_EMPTY if no data present
int64_t ListPopHead(node_t **head, void * removedData , void (*CopyFunction)(void * src, void * dst));
int64_t ListPopEnd(node_t **head, void * removedData , void (*CopyFunction)(void * src, void * dst));

// Remove the Matching Data
//     deletes the first match  of "data" from the list.
int64_t ListPopMatch(node_t **head,void *ptr_data, int64_t (*CompareFunction)(void * A, void* B),void (*CopyFunction)(void * src, void * dst) );



///////////////////////////////////////////////////////////
//   ptr_data is storage structure.
//      if a match is found, the data structure in the list
//      is copied to the ptr_data Struct.
//
//returns 0 if the value exists.
//        1 if the value does not
int64_t ListMatch(node_t *head, void *ptr_data,
                    int64_t (*CompareFunction)(void * A, void * B),
                    void (*CopyFunction)(void * src, void * dst)  );

// How many records in the list  match "Data", 
int64_t ListNumberOfMatch(node_t *head,void *ptr_data, int64_t (*CompareFunction)(void * A, void* B) );

// print the contents of the List 
void   ListPrint(node_t *head ,void (*PrintFunction)( void * pd)) ;

// How many elements/records in the list",
int64_t ListNumberOfElements(node_t *head);


#endif



