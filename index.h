#ifndef _INDEX_H
#define _INDEX_H
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define defaultMaxBufferedIndex 100
#define dbFilenameLength 20
#define defaultIndexLevel 5
#define indexHashSeed 1331

#define Error_CannotGetIndexFromDisk -1
#define Error_TupleIsnotExist -2
#define Error_CannotAllocateEnoughSpace -3
#define Error_InvalidKeyType -4
#define Error_InvalidBufferedIndexCode -5
#define Error_IndexFileCannotOpen -6
#define noError 0

/*
   struct result stores the index result, as well as which the datafield of the leaf node of the B+tree points to. 
   filename are the name of the file which specific tuple was stored in, and position is the position where it stays.
 */
typedef struct result{
	int position;
	char filename[dbFilenameLength];
}resultType;
/*
   struct node is the node of the B+tree index.
   int ifLeaf is a sign that distinguish a node whether it is leaf:0 -> internal node; 1 -> leaf node.
   void **key is the array of the key.Due to the uncertain type of the key, here use the void pointer.
   void **data is the array of the data field. If a node is leaf, a data pointer points to the result field. Otherwise, it points to its son.
   int ptrNum is the number of the pointers of key/data. 
 */
typedef struct node{ 
	int ptrNum;
	void **key;
	void **data;
	int ifLeaf;
	struct node *parent, *sibling;
}nodeType;

/*
   struct index stores the information of a B+tree index.
   The type of key value:0 for int; 1 for float; 2 for varchar.
   Key number limit: level-1
   Pointer number limit: level
   But the last pointer points to its sibling.
 */

typedef struct index{ 
	int tableID, columnID, indexID;
	int level;
	int keyType, keySize;
	nodeType *root, *firstLeaf;	
	int (*opFun)(void *x, void *y);
}indexType;

typedef struct indexSystem{
	indexType **indexArray; //the global pointer array of buffered Index
	int exception;
	int maxBufferedIndex;
}systemType;

//three compare operator of each types, if leftside's value bigger than rightside's, then return 1
int opFunInt(void *x, void *y);

int opFunFloat(void *x, void *y);

int opFunVarchar(void *x, void *y);

int makeIndexID(int tableID, int columnID);

nodeType *initNode(int ptrNum, int ifLeaf, nodeType *parent, nodeType *sibling);

int initSystem(systemType *thisSystem, int maxBufferedIndex);

void dropIndex(systemType *thisSystem, int tableID, int columnID);

/*Initialize the index and put it into the buffer
return value:1 for exist; -1 for failure; 0 for success
*/

int initGlobalIndex(systemType *thisSystem, int tableID, int columnID, int keyType);

int initIndex(indexType *thisIndex, int tableID, int columnID, int keyType);	 
/*
   Make a resultType which means the result doesn't exist.
   Nar = Not a result
 */

resultType makeNar(int code);

resultType searchGlobalIndex(systemType *thisSystem, int tableID, int columnID, void *key);

resultType searchIndex(indexType *thisIndex, nodeType *thisNode, void *key);

/*
   Major operation of the index B+tree:insert and delete
   Insertion finished.
   But deletion is still building. Too complexible...
 */
resultType *makeResult(char *filename, int position);

int insertGlobalIndex(systemType *thisSystem, int tableID, int columnID, char *filename, int offset, int size);

int insertIndex(indexType *thisIndex, nodeType *thisNode, void *key, resultType *data);

int compareResultData(resultType *left, resultType *right);

int deleteGlobalIndex(systemType *thisSystem, int tableID, int columnID, char *filename, int offset, int size);

int deleteIndex(indexType *thisIndex, nodeType *thisNode, void *key, resultType *data);

#endif
