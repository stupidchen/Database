#ifndef _INDEX_H
#define _INDEX_H
#include<stdio.h>
#include<stdlib.h>

#define defaultMaxBufferedIndex 100
#define saveFileName indexSave

#define Error_CannotGetIndexFromDisk -1
#define Error_TupleIsnotExist -2
#define Error_CannotAllocateEnoughSpace -3
#define Error_InvalidKeyType -4
#define Error_InvalidBufferedIndexCode -5

/*
   struct result stores the index result, as well as which the datafield of the leaf node of the B+tree points to. 
   filename are the name of the file which specific tuple was stored in, and position is the position where it stays.
 */
typedef struct result{
	char *filename;
	int position;
}resultType;

/*
   struct node is the node of the B+tree index.
   int ifLeaf is a sign that distinguish a node whether it is leaf:0 -> internal node; 1 -> leaf node.
   void **key is the array of the key.Due to the uncertain type of the key, here use the void pointer.
   void **data is the array of the data field. If a node is leaf, a data pointer points to the result field. Otherwise, it points to its son.
   int ptrNum is the number of the pointers of key/data. 
 */
typedef struct node{ 
	struct node *parent, *root, *sibling;
	int ifLeaf;
	void **key;
	void **data;
	int ptrNum;
}nodeType;

/*
   struct index stores the information of a B+tree index.
   The type of key value:0 for int; 1 for float; 2 for varchar.
   Key number limit: level
   Pointer number limit: level+1
   But the last pointer points to its sibling.
 */

typedef struct index{ 
	int tableID, columnID, indexID;
	nodeType *root, *firstLeaf;	
	int level;
	int (*opFun)(void *x, void *y);
	int keyType;		
}indexType;

typedef struct indexSystem{
	indexType **indexArray; //the global pointer array of buffered Index
	int exception;
	int maxBufferedIndex;
}systemType;

char *getExceptionName(int code){
}

//three compare operator of each types, if leftside's value bigger than rightside's, then return 1
int opFunInt(void *x, void *y);

int opFunFloat(void *x, void *y);

int opFunVarchar(void *x, void *y);

int makeIndexID(int tableID, int columnID);

int initSystem(systemType *thisSystem, int maxBufferedIndex);
/*Initialize the index and put it into the buffer
return value:1 for exist; -1 for failure; 0 for success
*/
int initGlobalIndex(systemType *thisSystem, int tableID, int columnID, int keyType);

int initIndex(indexType *thisIndex, int tableID, int columnID, int keyType);	 

/*
   Make a resultType which means the result doesn't exist.
   Nar = Not a result
 */
resultType makeNar(void);

resultType searchGlobalIndex(systemType *thisSystem, int tableID, int columnID, void *key);

resultType searchIndex(indexType *thisIndex, nodeType *thisNode, void *key);

/*
   Major operation of the index B+tree:insert and delete
   Still building...
 */
int insertGlobalIndex(systemType *thisSystem, int tableID, int columnID, void *key, int keyType){
	int indexID = makeIndexID(tableID, columnID);
	int bufferID = indexID % (thisSystem->maxBufferedIndex);
	indexType **indexArray = thisSystem->indexArray;
	if (indexArray == NULL || indexArray[bufferID]->indexID != indexID){	
		indexArray[bufferID] = getIndexFromDisk(tableID, columnID);
		if (indexArray[bufferID] == NULL) return Error_CannotGetIndexFromDisk;
	}
	if (thisSystem->keyType != keyType) return Error_InvalidKeyType;
	return insertIndex(indexArray[bufferID], indexArray[bufferID]->root, key);
}

int insertIndex(indexType *thisIndex, nodeType *thisNode, void *key){
	int i, exception;
	void *thisSon;

	if (thisNode->ifLeaf == 0){
		for (i = 0;i < thisNode->ptrNum; i++){
			thisSon = (thisNode->data)[i];
			if ((*(thisIndex->opFun))(thisKey,key)){
				exception = insertIndex(thisIndex, thisSon, key);
				if (exception != 0) return exception;
				else break;
			}
		}

	}
	else{
		if ()
	}
}


/*
   IO operation.
   getIndexFromMemory is the procedure that get the index from disk to the buffer.
   saveBufferdIndexToDisk is the procedure that save the buffered index to the disk.
   Still building...
   */
indexType *getIndexFromDisk(int tableID, int columnID);

int saveBufferedIndexToDisk(int tableID, int columnID);
#endif
