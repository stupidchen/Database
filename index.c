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

int initSystem(systemType *thisSystem, int maxBufferedIndex){
	thisSystem->maxBufferedIndex = maxBufferedIndex;
	thisSystem->indexArray = (indexType**)malloc(sizeof(indexType*) * maxBufferedIndex);
	thisSystem->exception = 0;
	if (thisSystem->indexArray == NULL) return Error_CannotAllocateEnoughSpace;
	return 0;
}

int makeIndexID(int tableID, int columnID){
	int indexID = tableID * columnID;
	return indexID;
}

int initGlobalIndex(systemType *thisSystem, int tableID, int columnID, int keyType){
	int indexID = makeIndexID(tableID, columnID);
	int bufferID = indexID % (thisSystem->maxBufferedIndex);
	indexType **indexArray = thisSystem->indexArray;
	if (indexArray == NULL || indexArray[bufferID]->indexID != indexID){
		indexArray[bufferID] = (indexType*)malloc(sizeof(indexType));	
		if (indexArray[bufferID] == NULL) return Error_CannotAllocateEnoughSpace;
		return initIndex(indexArray[bufferID], tableID, columnID, keyType);
	}
	return 0;
}

int initIndex(indexType *thisIndex, int tableID, int columnID, int keyType){	 //1:exist -1:failure 0:success
	thisIndex -> tableID = tableID;
	thisIndex -> columnID = columnID;
	thisIndex -> indexID = makeIndexID(tableID, columnID);

	thisIndex -> firstLeaf = NULL;
	thisIndex -> root = NULL;
	thisIndex -> level = 2;	

	thisIndex -> keyType = keyType;
	switch (keyType){
		case 0:thisIndex -> opFun = &opFunInt;break;
		case 1:thisIndex -> opFun = &opFunFloat;break;
		case 2:thisIndex -> opFun = &opFunVarchar;break;
		default:return Error_InvalidKeyType;
	}

	return 0;
}

char *getExceptionName(int code){
}

resultType makeNar(int code){
	resultType naR;
	naR.filename = NULL;
	naR.position = code;
	return naR;
}

resultType searchGlobalIndex(systemType *thisSystem, int tableID, int columnID, void *key){
	int indexID = makeIndexID(tableID, columnID);
	int bufferID = indexID % (thisSystem->maxBufferedIndex);
	indexType **indexArray = thisSystem->indexArray;
	if (indexArray == NULL || indexArray[bufferID]->indexID != indexID){	
		indexArray[bufferID] = getIndexFromDisk(thisSystem, tableID, columnID);	
		if (indexArray[bufferID] == NULL) return makeNar(Error_CannotGetIndexFromDisk);
	}
	return searchIndex(indexArray[bufferID], indexArray[bufferID]->root, key);
}

resultType searchIndex(indexType *thisIndex, nodeType *thisNode, void *key){
	void *thisKey, *thisSon;
	int i;
	for (i = 0; i < thisNode->ptrNum; i++){
		thisKey = (thisNode->key)[i];
		if ((*(thisIndex->opFun))(thisKey, key)){
			thisSon = (thisNode->data)[i];
			if (thisNode->ifLeaf == 0) return searchIndex(thisIndex, (nodeType*)thisSon, key);
			else return *((resultType*)(thisSon));
		}
	}
	return makeNar(Error_TupleIsnotExist);
}

int insertGlobalIndex(systemType *thisSystem, int tableID, int columnID, void *key, int keyType){
	int indexID = makeIndexID(tableID, columnID);
	int bufferID = indexID % (thisSystem->maxBufferedIndex);
	int result;
	indexType **indexArray = thisSystem->indexArray;
	if (indexArray == NULL || indexArray[bufferID]->indexID != indexID){	
		result = saveBufferedIndexToDisk(thisSystem, indexArray[bufferID]->tableID, indexArray[bufferID]->columnID);
		if (result != 0) return result;
		indexArray[bufferID] = getIndexFromDisk(thisSystem, tableID, columnID);
		if (indexArray[bufferID] == NULL) return Error_CannotGetIndexFromDisk;
	}
	if (indexArray[bufferID]->keyType != keyType) return Error_InvalidKeyType;
	return insertIndex(indexArray[bufferID], indexArray[bufferID]->root, key);
}

int insertIndex(indexType *thisIndex, nodeType *thisNode, void *key){
	int i, exception;
	void *thisKey, *thisSon;

	if (thisNode->ifLeaf == 0){
		for (i = 0;i < thisNode->ptrNum; i++){
			thisSon = (thisNode->data)[i];
			thisKey = (thisNode->key)[i];
			if ((*(thisIndex->opFun))(thisKey,key)){
				exception = insertIndex(thisIndex, thisSon, key);
				if (exception != 0) return exception;
				else break;
			}
		}
		/*
		   Adjustment after insertion to a internal node
		 */
	}
	else{
		/*
		   Insertion of the leaf node
		   
		 */
	}
}

indexType *getIndexFromDisk(systemType *thisSystem, int tableID, int columnID){
	return NULL;
}

int saveBufferedIndexToDisk(systemType *thisSystem, int tableID, int columnID){
	int indexID = makeIndexID(tableID, columnID);
	int bufferID = indexID % (thisSystem->maxBufferedIndex);
	indexType **indexArray = thisSystem->indexArray;
	if (indexArray == NULL || indexArray[bufferID]->indexID != indexID) return Error_InvalidBufferedIndexCode;
/*   
     Building...
     
 */
	return 0;
}

int main(){
}
