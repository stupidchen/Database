#include "index.h"

int opFunInt(void *x, void *y){
	int left = *((int*)x);
	int right = *((int*)y);

	if (left > right) return 1;
	return 0;
}

int opFunFloat(void *x, void *y){
	double left = *((double*)x);
	double right = *((double*)y);

	if (left > right) return 1;
	return 0;}

int opFunVarchar(void *x, void *y){
	char *left = *((char**)x);
	char *right = *((char**)y);

	if (strcmp(left, right) == 1) return 1;
	return 0;
}

int initIndex(int tableID, int columnID, int keyType){	 //1:exist -1:failure 0:success
	indexType *thisIndex;
	int indexID = tableID * columnID;
	int bufferID = indexID % maxBufferedIndex;
	if (glbIndexArray[bufferID] -> indexID == indexID) return 1;

	glbIndexArray[bufferID] = (indexType*)malloc(sizeof(indexType));	
	thisIndex = glbIndexArray[bufferID];

	thisIndex -> tableID = tableID;
	thisIndex -> columnID = columnID;
	thisIndex -> indexID = indexID;

	thisIndex -> firstLeaf = NULL;
	thisIndex -> root = NULL;
	thisIndex -> level = 2;	

	thisIndex -> keyType = keyType;
	switch (keyType){
		case 0:thisIndex -> opFun = &opFunInt;break;
		case 1:thisIndex -> opFun = &opFunFloat;break;
		case 2:thisIndex -> opFun = &opFunVarchar;break;
		default:return -1;
	}

	return 0;
}

resultType searchIndex(indexType *thisIndex, nodeType *thisNode, void *key){
	void *thisKey, *thisSon;
	int i;
	for (i = 0; i < thisNode->ptrNum; i++){
		thisKey = (thisNode->key)[i];
		if (!((*(thisIndex->opFun))(key, thisKey))){
			thisSon = (thisNode->data)[i];
			if (thisNode->ifLeaf == 0) return searchIndex(thisIndex, (nodeType*)thisSon, key);
			else return *((resultType*)(thisSon));
		}
	}
	
	resultType naR;
	naR.position = -1;
	return naR;
}

int main(){
}
