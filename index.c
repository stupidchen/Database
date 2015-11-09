#include "index.h"
#include "indexIO.h"


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

int initNode(nodeType *newNode){
	newNode = (nodeType*)malloc(sizeof(nodeType));
	newNode->parent = NULL;
	newNode->sibling = NULL;
	newNode->pSibling = NULL;
	newNode->ptrNum = 0;
}

int initGlobalIndex(systemType *thisSystem, int tableID, int columnID, int keyType){
	int indexID = makeIndexID(tableID, columnID);
	int bufferID = indexID % (thisSystem->maxBufferedIndex);
	indexType **indexArray = thisSystem->indexArray;
	if (indexArray[bufferID] == NULL || indexArray[bufferID]->indexID != indexID){
		indexArray[bufferID] = (indexType*)malloc(sizeof(indexType));	
		if (indexArray[bufferID] == NULL) return Error_CannotAllocateEnoughSpace;
		return initIndex(indexArray[bufferID], tableID, columnID, keyType);
	}
	return 0;
}

int initIndex(indexType *thisIndex, int tableID, int columnID, int keyType){	 //1:exist -1:failure 0:success
	thisIndex->tableID = tableID;
	thisIndex->columnID = columnID;
	thisIndex->indexID = makeIndexID(tableID, columnID);

	thisIndex->firstLeaf = NULL;
	thisIndex->root = NULL;
	thisIndex->level = 2;	

	thisIndex->keyType = keyType;
	switch (keyType){
		case 0:thisIndex->opFun = &opFunInt;break;
		case 1:thisIndex->opFun = &opFunFloat;break;
		case 2:thisIndex->opFun = &opFunVarchar;break;
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
	if (indexArray[bufferID] == NULL || indexArray[bufferID]->indexID != indexID){	
		indexArray[bufferID] = getIndexFromDisk(tableID, columnID);	
		if (indexArray[bufferID] == NULL) return makeNar(Error_CannotGetIndexFromDisk);
	}
	return searchIndex(indexArray[bufferID], indexArray[bufferID]->root, key);
}

resultType searchIndex(indexType *thisIndex, nodeType *thisNode, void *key){
	void *thisKey, *thisSon;
	int i, op;

	if (thisNode->ifLeaf == 0){
		for (i = 0; i < thisNode->ptrNum; i++){
			thisKey = (thisNode->key)[i];
			op = (*(thisIndex->opFun))(thisKey, key);
			if (op == 1) break;
		}
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

int insertGlobalIndex(systemType *thisSystem, int tableID, int columnID, void *key, void *data){
	int indexID = makeIndexID(tableID, columnID);
	int bufferID = indexID % (thisSystem->maxBufferedIndex);
	indexType **indexArray = thisSystem->indexArray;
	int result = replaceBufferedIndex(indexArray, tableID, columnID, indexID, bufferID);
	if (result != noError) return result;
	return insertIndex(indexArray[bufferID], indexArray[bufferID]->root, key, data);
}

int insertIndex(indexType *thisIndex, nodeType *thisNode, void *key, void *data){
	int i, j, exception, ifInsert = 0;
	void *thisKey, *thisSon;
	nodeType *newNode, *thisParent, *tempSon;

	if (thisNode->ifLeaf == 0){
		for (i = 0; i < thisNode->ptrNum; i++){
			thisKey = (thisNode->key)[i];
			if ((*(thisIndex->opFun))(thisKey, key)){
				ifInsert = 1;
				break;
			}
		}

		tempSon = (nodeType*)((thisNode->data)[i]);
		exception = insertIndex(thisIndex, tempSon, key, data);
		if (exception != 0) return exception;

		if (ifInsert) thisNode->key[i] = (tempSon->key)[tempSon->ptrNum];
	}
	else{	
		/*
		   Insertion of the leaf node
		   
		 */
		for (i = thisNode->ptrNum; i > 0; i--){
			thisSon = (thisNode->data)[i];
			thisKey = (thisNode->key)[i];
			if ((*(thisIndex->opFun))(thisKey, key)) break;
		}
		thisNode->ptrNum++;
		thisNode->key[thisNode->ptrNum] = (void*)malloc(sizeof(key));
		thisNode->data[thisNode->ptrNum] = (void*)malloc(sizeof(data));
		for (j = thisNode->ptrNum; j > i + 1; j--){
			thisNode->key[j] = thisNode->key[j - 1];
			thisNode->data[j] = thisNode->data[j - 1];
		}
		thisNode->key[j] = key;
		thisNode->data[j] = data;
	}

	/*
	   Adjustment after insertion to a internal node
	 */
	if (thisNode->ptrNum == thisIndex->level - 1){
		//initiate a new node and re-assign the data and key
		initNode(newNode);
		newNode->parent = thisNode->parent;	
		newNode->sibling = thisNode->sibling;	
		newNode->pSibling = thisNode;
		newNode->ptrNum = (thisNode->ptrNum) / 2;
		newNode->ifLeaf = thisNode->ifLeaf;
		thisNode->sibling = newNode;
		thisNode->ptrNum = (thisNode->ptrNum) - (newNode->ptrNum);
		for (i = 0; i < newNode->ptrNum; i++){
			newNode->key[i] = (void*)malloc(sizeof(key));
			newNode->key[i] = thisNode->key[i + thisNode->ptrNum];
			newNode->data[i] = (void*)malloc(sizeof(thisNode->data[i]));
			newNode->data[i] = thisNode->data[i + thisNode->ptrNum];
			thisNode->key[i + thisNode->ptrNum] = NULL;
			thisNode->data[i + thisNode->ptrNum] = NULL;
		}

		//update the parent
		if (thisNode -> parent != NULL){ //if the parent isn't the root
			thisParent = thisNode -> parent;
			for (i = 0; i < thisParent->ptrNum; i++)
				if ((nodeType*)thisParent->data[i] == thisNode) break;
			thisParent->ptrNum++;
			thisParent->key[thisParent->ptrNum] = (void*)malloc(sizeof(key));
			thisParent->data[thisParent->ptrNum] = (void*)malloc(sizeof(nodeType*));
			for (j = thisParent->ptrNum; j > i+1; j--){
				thisParent->key[j] = thisParent->key[j - 1];
				thisParent->data[j] = thisParent->data[j - 1];
			}
			thisParent->key[i + 1] = newNode->key[newNode->ptrNum];
			thisParent->data[i + 1] = (void*)newNode;
		}
		else{ //if the parent is root
			initNode(thisParent);
			thisIndex->root = thisParent;
			thisParent->ptrNum = 1;
			thisParent->ifLeaf = 0;
			thisParent->key[0] = (void*)malloc(sizeof(key));
			thisParent->key[0] = thisNode->key[thisNode->ptrNum];

			thisParent->data[0] = (void*)malloc(sizeof(thisNode));
			thisParent->data[0] = (void*)thisNode;
			thisParent->data[1] = (void*)malloc(sizeof(newNode));
			thisParent->data[1] = (void*)newNode;
		}
	}
	return 0;
}

int compareResultData(resultType *left, resultType *right){
	if (strcmp(left->filename, right->filename) && (left->position == right->position)) return 0;
	return 1;
}

int deleteGlobalIndex(systemType *thisSystem, int tableID, int columnID, void *key, resultType* data){
	int indexID = makeIndexID(tableID, columnID);
	int bufferID = indexID % (thisSystem->maxBufferedIndex);
	indexType **indexArray = thisSystem->indexArray;
	int result = replaceBufferedIndex(indexArray, tableID, columnID, indexID, bufferID);
	if (result != noError) return result;
	return deleteIndex(indexArray[bufferID], indexArray[bufferID]->root, key, data);
}

int deleteIndex(indexType *thisIndex, nodeType *thisNode, void *key, resultType* data){
	int i, j, exception, ifDelete = 0;
	void *thisKey, *thisSon, *thisData;
	nodeType *newNode, *thisParent;

	if (thisNode->ifLeaf == 0){
		for (i = 0; i < thisNode->ptrNum; i++){
			thisSon = (thisNode->data)[i];
			thisKey = (thisNode->key)[i];
			if ((*(thisIndex->opFun))(thisKey, key)){
				exception = deleteIndex(thisIndex, thisSon, key, data);
				if (exception != 0) return exception;
				else{
					ifDelete = 1;
					break;
				}
			}
		}
		if (!ifDelete){ 
			thisSon = (thisNode->data)[i];
			if (thisSon != NULL){
				exception = deleteIndex(thisIndex, thisSon, key, data);
				if (exception != 0) return exception;
			}
		}
	}
	else{
		for (i = 0; i < thisNode->ptrNum; i++){
			thisData = (thisNode->data)[i];
			if (compareResultData(thisData, data) == 0){
				ifDelete = 1;
				break;
			}
		}
		if (!ifDelete) return Error_TupleIsnotExist;	
	}

	//merge node after deletion if possible 
}


int main(){
}
