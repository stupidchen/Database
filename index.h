#ifndef _INDEX_H
#define _INDEX_H
#include<stdio.h>
#include<stdlib.h>

#define maxTypeNumber 5
#define maxBufferedIndex 100

typedef struct node{
	struct node *parent, *root;
	int nodeType;
	void **key;
	void **data;
	int ptrNum;
}nodeType;

typedef struct index{
	int tableID, columnID, indexID;
	nodeType *root, *firstLeaf;	
	int level;
	void *opFun;
	int keyType;			//the type of key value:0 for int; 1 for float; 2 for varchar
}indexType;

indexType **glbIndexArray; //the global pointer array of buffered Index

//three compare operator of each types, if leftside's value bigger than rightside's, then return 1
int opFunInt(void *x, void *y);

int opFunFloat(void *x, void *y);

int opFunVarchar(void *x, void *y);

int initIndex(int tableID, int columnID, int keyType);	 
/*Initialize the index and put it into the buffer
return value:1 for exist; -1 for failure; 0 for success
*/



#endif
