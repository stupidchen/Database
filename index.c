#include "index.h"
#include "indexIO.h"


int initSystem(systemType *thisSystem, int maxBufferedIndex){
	thisSystem->maxBufferedIndex = defaultMaxBufferedIndex;
	thisSystem->indexArray = (indexType**)malloc(sizeof(indexType*) * maxBufferedIndex);
	thisSystem->exception = 0;
	if (thisSystem->indexArray == NULL) return Error_CannotAllocateEnoughSpace;
	return 0;
}

int makeIndexID(int tableID, int columnID){
	int indexID = tableID * indexHashSeed + columnID;
	return indexID;
}

nodeType *initNode(int ptrNum, int ifLeaf, nodeType *parent, nodeType *sibling){
	nodeType *newNode = (nodeType*)malloc(sizeof(nodeType));
	newNode->ifLeaf = ifLeaf;
	newNode->ptrNum = ptrNum;
	newNode->parent = parent;
	newNode->sibling = sibling;
	newNode->data = (void**)malloc(sizeof(void*) * ptrNum);
	if (ifLeaf == 1) newNode->key = (void**)malloc(sizeof(void*) * ptrNum);
	else newNode->key = (void**)malloc(sizeof(void*) * (ptrNum-1));
}

void dropIndex(systemType *thisSystem, int tableID, int columnID){
	int indexID = makeIndexID(tableID, columnID);
	int bufferID = indexID % (thisSystem->maxBufferedIndex);
	indexType **indexArray = thisSystem->indexArray;
	if (indexArray[bufferID]->indexID == indexID){
		freeIndex(indexArray[bufferID]);
		indexArray[bufferID] = NULL;
	}
	deleteIndexFile(tableID, columnID);
}

int initGlobalIndex(systemType *thisSystem, int tableID, int columnID, int keyType){
	int indexID = makeIndexID(tableID, columnID);
	int bufferID = indexID % (thisSystem->maxBufferedIndex);
	indexType **indexArray = thisSystem->indexArray;
	if (indexArray[bufferID] == NULL || indexArray[bufferID]->indexID != indexID){
		if (indexArray[bufferID] != NULL) saveBufferedIndexToDisk(indexArray[bufferID]);
		indexArray[bufferID] = (indexType*)malloc(sizeof(indexType));	
		if (indexArray[bufferID] == NULL) return Error_CannotAllocateEnoughSpace;
		return initIndex(indexArray[bufferID], tableID, columnID, keyType);
	}
	return noError;
}

int initIndex(indexType *thisIndex, int tableID, int columnID, int keyType){ 
	thisIndex->tableID = tableID;
	thisIndex->columnID = columnID;
	thisIndex->indexID = makeIndexID(tableID, columnID);

	thisIndex->firstLeaf = NULL;
	thisIndex->root = initNode(0, 1, NULL, NULL);
	thisIndex->level = defaultIndexLevel;	

	thisIndex->keyType = keyType;
	if (keyType < 2) thisIndex->keySize = 4;
	else thisIndex->keySize = keyType/2;

	switch (keyType){
		case 0:thisIndex->opFun = &opFunInt;break;
		case 1:thisIndex->opFun = &opFunFloat;break;
		default:thisIndex->opFun = &opFunVarchar;break;
	}

	return noError;
}

int opFunInt(void *x, void *y){
	int left = *((int*)x);
	int right = *((int*)y);

	if (left > right) return 1;
	if (left < right) return -1;
	return 0;
}

int opFunFloat(void *x, void *y){
	float left = *((float*)x);
	float right = *((float*)y);

	if (left > right) return 1;
	if (left < right) return -1;
	return 0;
}

int opFunVarchar(void *x, void *y){
	char *left = *((char**)x);
	char *right = *((char**)y);

	return strcmp(left, right);
}

resultType makeNar(int code){
	resultType naR;
	naR.filename[0] = '\0';
	naR.position = code;
	return naR;
}

resultType searchGlobalIndex(systemType *thisSystem, int tableID, int columnID, void *key){
	int indexID = makeIndexID(tableID, columnID);
	int bufferID = indexID % (thisSystem->maxBufferedIndex);
	indexType **indexArray = thisSystem->indexArray;
	if (indexArray[bufferID] == NULL || indexArray[bufferID]->indexID != indexID){	
		indexArray[bufferID] = getIndexFromDisk(tableID, columnID);	
		if (indexArray[bufferID] == NULL) return makeNar(Error_CannotGetIndexFromDisk);
	}
	return searchIndex(indexArray[bufferID], indexArray[bufferID]->root, key);
}

resultType searchIndex(indexType *thisIndex, nodeType *thisNode, void *key){
	void *thisKey, *thisSon;
	int i, op, ifFound = 0;

	if (thisNode->ifLeaf == 0){
		for (i = 0; i < thisNode->ptrNum - 1; i++){
			thisKey = (thisNode->key)[i];
			op = (*(thisIndex->opFun))(thisKey, key);
			if (op == 1){
				ifFound = 1;
				break;
			}
		}
		if (!ifFound) i++;
		thisSon = (thisNode->data)[i];
		return searchIndex(thisIndex, (nodeType*)thisSon, key);
	}
	else{
		for (i = 0; i < thisNode->ptrNum; i++){
			thisKey = (thisNode->key)[i];
			thisSon = (thisNode->data)[i];
			op = (*(thisIndex->opFun))(thisKey, key);
			if (op == 0) return *((resultType*)(thisSon));
		}
	}

	return makeNar(Error_TupleIsnotExist);
}

int insertGlobalIndex(systemType *thisSystem, int tableID, int columnID, char *filename, int offset, int size){
	int indexID = makeIndexID(tableID, columnID);
	int bufferID = indexID % (thisSystem->maxBufferedIndex);
	indexType **indexArray = thisSystem->indexArray;
	int result = replaceBufferedIndex(indexArray, tableID, columnID, indexID, bufferID);
	if (result != noError) return result;
	resultType *data = makeResult(filename, offset);
	void *key;
//	key = getIndexBlock(tableID, filename, offset, size);
	return insertIndex(indexArray[bufferID], indexArray[bufferID]->root, key, data);
}

resultType *makeResult(char *filename, int position){
	resultType *newResult = (resultType*)malloc(sizeof(resultType));
	strcpy(newResult->filename, filename);
	newResult->position = position;
	return newResult;
}

int insertIndex(indexType *thisIndex, nodeType *thisNode, void *key, resultType *data){
	int i, j, exception, ifInsert = 0;
	int keySize = thisIndex->keySize;
	void *thisKey, *thisSon;
	nodeType *newNode, *thisParent, *tempSon;

	if (thisNode->ifLeaf == 0){
		for (i = 0; i < thisNode->ptrNum - 1; i++){
			thisKey = (thisNode->key)[i];
			if ((*(thisIndex->opFun))(thisKey, key) == 1){
				ifInsert = 1;
				break;
			}
		}


		tempSon = (nodeType*)((thisNode->data)[i]);
		exception = insertIndex(thisIndex, tempSon, key, data);
		if (exception != 0) return exception;

		if (ifInsert) memcpy(thisNode->key[i], tempSon->sibling->key[0], keySize);
	}
	else{	
		/*
		   Insertion of the leaf node
		   
		 */
		for (i = thisNode->ptrNum; i > 0; i--){
			thisSon = (thisNode->data)[i];
			thisKey = (thisNode->key)[i];
			if ((*(thisIndex->opFun))(thisKey, key) == 1) break;
		}
		thisNode->key[thisNode->ptrNum] = (void*)malloc(keySize);
		thisNode->data[thisNode->ptrNum] = (void*)malloc(sizeof(data));
		for (j = thisNode->ptrNum; j >= i + 1; j--){
			thisNode->key[j] = thisNode->key[j - 1];
			thisNode->data[j] = thisNode->data[j - 1];
		}
		memcpy(thisNode->key[i], key, keySize);
		thisNode->data[i] = data;
		thisNode->ptrNum++;
	}

	/*
	   Adjustment after insertion to a internal node
	 */
	if (thisNode->ptrNum > thisIndex->level){
		//initiate a new node and re-assign the data and key
		newNode = initNode((thisNode->ptrNum) >> 1, thisNode->ifLeaf, thisNode->parent, thisNode->sibling);
		thisNode->sibling = newNode;
		thisNode->ptrNum = (thisNode->ptrNum) - (newNode->ptrNum);

		for (i = 0; i < newNode->ptrNum; i++) 
			newNode->data[i] = thisNode->data[i + thisNode->ptrNum];
		if (thisNode->ifLeaf == 1){
			for (i = 0; i < newNode->ptrNum; i++) 
				newNode->key[i] = thisNode->key[i + thisNode->ptrNum];
		}
		else{
			for (i = 0; i < newNode->ptrNum - 1; i++)
				newNode->key[i] = thisNode->key[i + thisNode->ptrNum];
			free(thisNode->key[thisNode->ptrNum - 1]);
		}

		//update the parent
		if (thisNode -> parent != NULL){ //if the parent isn't the root
			thisParent = thisNode -> parent;
			for (i = 0; i < thisParent->ptrNum; i++)
				if ((nodeType*)thisParent->data[i] == thisNode) break;
			thisParent->key[thisParent->ptrNum - 1] = (void*)malloc(keySize);
			thisParent->data[thisParent->ptrNum] = (void*)malloc(sizeof(void*));
			for (j = thisParent->ptrNum; j > i + 1; j--){
				if (j > i + 2) thisParent->key[j - 1] = thisParent->key[j - 2];
				thisParent->data[j] = thisParent->data[j - 1];
			}
			memcpy(thisParent->key[i], newNode->key[0], keySize);
			thisParent->data[i + 1] = (void*)newNode;
			thisParent->ptrNum++;
		}
		else{ //if the parent is root
			thisParent = initNode(2, 0, NULL, NULL);
			thisIndex->root = thisParent;

			memcpy(thisParent->key[0], thisNode->sibling->key[0], keySize);
			thisParent->data[0] = (void*)thisNode;
			thisParent->data[1] = (void*)newNode;
		}
	}
	return noError;
}

int compareResultData(resultType *left, resultType *right){
	if (strcmp(left->filename, right->filename) && (left->position == right->position)) return 0;
	return 1;
}

int deleteGlobalIndex(systemType *thisSystem, int tableID, int columnID, char *filename, int offset, int size){
	int indexID = makeIndexID(tableID, columnID);
	int bufferID = indexID % (thisSystem->maxBufferedIndex);
	indexType **indexArray = thisSystem->indexArray;
	int result = replaceBufferedIndex(indexArray, tableID, columnID, indexID, bufferID);
	if (result != noError) return result;
	resultType *data = makeResult(filename, offset);
	void *key;
//	key = getIndexBlock(tableID, filename, offset, size);
	return deleteIndex(indexArray[bufferID], indexArray[bufferID]->root, key, data);
}

int deleteIndex(indexType *thisIndex, nodeType *thisNode, void *key, resultType* data){
	int i, j, exception, ifDelete = 0;
	int keySize = thisIndex->keySize;
	void *thisKey, *thisData;
	nodeType *tempNode, *thisSon;

	if (thisNode->ifLeaf == 0){
		for (i = 0; i < thisNode->ptrNum - 1; i++){
			thisKey = (thisNode->key)[i];
			if ((*(thisIndex->opFun))(thisKey, key) == 1){
				ifDelete = 1;
				break;
			}
		}
		thisSon = (nodeType*)(thisNode->data)[i];
		exception = deleteIndex(thisIndex, thisSon, key, data);
		if (exception != 0) return exception;

		if (thisSon->ptrNum == 0){
			freeNode(thisSon, thisIndex->keyType);
			if (ifDelete){
				for (j = i; j < thisNode->ptrNum - 1; j++){
					thisNode->data[j] = thisNode->data[j + 1];
					if (j < thisNode->ptrNum - 2) thisNode->key[j - 1] = thisNode->key[j];
				}
				thisNode->ptrNum--;
			}
		}
		else{
			if (!ifDelete) memcpy(thisNode->key[i-1], thisSon->key[0], keySize);
		}
	}
	else{
		tempNode = thisNode;
		while ((tempNode != NULL) && ((*(thisIndex->opFun))(tempNode->key[0], key) != 1)){
			for (i = 0; i < tempNode->ptrNum; i++){
				thisData = (tempNode->data)[i];
				if (compareResultData(tempNode->data[i], data) == 0){
					ifDelete = 1;
					break;
				}
			}
			tempNode = tempNode->sibling;
		}
		if (!ifDelete) return Error_TupleIsnotExist;	

		free(tempNode->data[i]);
		free(tempNode->key[i]);
		for (j = i; j < tempNode->ptrNum - 1; j++){
			tempNode->data[j] = tempNode->data[j + 1];
			tempNode->key[j] = tempNode->key[j + 1];
		}
		tempNode->ptrNum--;
	}
}

