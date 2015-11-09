#include "indexIO.h"

void freeNode(nodeType *thisNode, int keyType){
	int i;

	for (i = 0; i < thisNode->ptrNum; i++){
		if (i < thisNode->ptrNum - 1){
			switch (keyType){
				case 0:free((int*)thisNode->key[i]);break;
				case 1:free((float*)thisNode->key[i]);break;
				case 2:free((char**)thisNode->key[i]);break;
			}
		}
		if (thisNode->ifLeaf == 1) free((resultType*)thisNode->data[i]);
	}

	free(thisNode->key);
	free(thisNode->data);

	free(thisNode);
}
void freeIndexNode(nodeType *thisNode, int keyType){
	if (thisNode->ifLeaf == 0){
		int i;
		for (i = 0; i <= thisNode->ptrNum; i++) 
			freeIndexNode((nodeType*)thisNode->data[i], keyType);
	}
	freeNode(thisNode, keyType);
}

void freeIndex(indexType *thisIndex){
	freeIndexNode(thisIndex->root, thisIndex->keyType);
	free(thisIndex);
}

void freeSystem(systemType *thisSystem){
	int i;
	for (i = 0; i < thisSystem->maxBufferedIndex; i++) freeIndex(thisSystem->indexArray[i]);
	free(thisSystem);
}

int replaceBufferedIndex(indexType **buffer, int tableID, int columnID, int indexID, int bufferID){
	int result;
	if (buffer[bufferID]->indexID == indexID) return noError;
	if (buffer[bufferID] != NULL){
		result = saveBufferedIndexToDisk(buffer[bufferID]);
		if (result != noError) return result;
	}
	buffer[bufferID] = getIndexFromDisk(tableID, columnID);
	if (buffer[bufferID] == NULL) return Error_CannotGetIndexFromDisk;
	return noError;
}

void reconstructIndex(indexType *thisIndex, nodeType *thisNode, int layerNum){
	if (layerNum == 1){
		thisIndex->root = thisNode;
		return;
	}

	int level = thisIndex->level;
	int nextLayerNum = layerNum / level;
	if (layerNum % level != 0) nextLayerNum++;

	nodeType *firstNode = NULL;
	nodeType *thisParent, *lastParent = NULL;
	int i, j = 0, unMatch = layerNum;
	for (i = 0; i < nextLayerNum; i++){
		thisParent = (nodeType*)malloc(sizeof(nodeType));
		if (firstNode == NULL) firstNode = thisParent;
		thisParent->ifLeaf = 0;
		thisParent->sibling = lastParent;
		lastParent = thisParent;
		if (unMatch >= (level << 1)) thisParent->ptrNum = level;
		else if (unMatch > level) thisParent->ptrNum = unMatch/2;
		unMatch = unMatch - thisParent->ptrNum;
		for (j = 0; j < thisParent->ptrNum; j++){
			thisParent->data[j] = (void*)malloc(sizeof(nodeType*));
			thisParent->data[j] = thisNode;
			if (j < thisParent->ptrNum - 1){
				thisParent->key[j] = (void*)malloc(sizeof(thisNode->key[thisNode->ptrNum]));
				thisParent->key[j] = thisNode->key[thisNode->ptrNum];
			}
			thisNode = thisNode->sibling;
		}
	}
	reconstructIndex(thisIndex, firstNode, nextLayerNum);
}

indexType *getIndexFromDisk(int tableID, int columnID){
	indexType *newIndex = (indexType*)malloc(sizeof(indexType));
	indexTypeForIO *newIndexForIO = (indexTypeForIO*)newIndex;

	char filename[maxFilenameLength];
	sprintf(filename, "%s%d%d", indexFilename, tableID, columnID);
	FILE *readFile = fopen(filename, "wb");
	if (readFile == NULL) return NULL;
	
	fread(newIndexForIO, sizeof(indexTypeForIO), 1, readFile);
	newIndex->firstLeaf = NULL;
	int keyType = newIndex->keyType;
	switch (keyType){
		case 0:newIndex->opFun = &opFunInt;break;
		case 1:newIndex->opFun = &opFunFloat;break;
		case 2:newIndex->opFun = &opFunVarchar;break;
	}

	int i, ptrNum, leafNum = 0;
	int keySize = 0;
	nodeType *newNode, *lastNode = NULL;

	while (fread(&ptrNum, sizeof(ptrNum), 1, readFile) != 0){
		leafNum++;
		newNode = (nodeType*)malloc(sizeof(nodeType));
		if (newIndex->firstLeaf == NULL) newIndex->firstLeaf = newNode;
		newNode->sibling = lastNode;
		newNode->ptrNum = ptrNum;
		newNode->ifLeaf = 1;
		lastNode = newNode;

		for (i = 0; i < newNode->ptrNum; i++){
			fread(&keySize, sizeof(keySize), 1, readFile);
			newNode->key[i] = (void*)malloc(keySize);
			fread(newNode->key[i], keySize, 1, readFile);
		}
		for (i = 0; i < newNode->ptrNum; i++){
			newNode->data[i] = (void*)malloc(sizeof(resultType));
			fread(newNode->data[i], sizeof(resultType), 1, readFile);
		}
	}
	reconstructIndex(newIndex, newIndex->firstLeaf, leafNum);
	return newIndex;
}


int saveBufferedIndexToDisk(indexType *thisIndex){
	char filename[maxFilenameLength];
	int tableID = thisIndex->tableID;
	int columnID = thisIndex->columnID;
	sprintf(filename, "%s%d%d", indexFilename, tableID, columnID);
	FILE *saveFile = fopen(filename, "wb");
	if (saveFile == NULL) return Error_IndexFileCannotOpen;

	indexTypeForIO *indexForIO = (indexTypeForIO*)thisIndex;
	fwrite(indexForIO, sizeof(indexTypeForIO*), 1, saveFile);

	int i;
	int keySize = 0;
	int keyType = thisIndex->keyType;
	if (keyType == 0) keySize = sizeof(int);
	if (keyType == 1) keySize = sizeof(float);
	
	nodeType *thisLeaf = thisIndex->firstLeaf;
	while (thisLeaf != NULL){
		fwrite(&thisLeaf->ptrNum, sizeof(thisLeaf->ptrNum), 1, saveFile);
		for (i = 0; i < thisLeaf->ptrNum; i++){
			if (keyType == 2){
				char **thisKey = (char**)thisLeaf->key[i];
				keySize = sizeof(*thisKey);
			}
			fwrite(&keySize, sizeof(keySize), 1, saveFile);
			fwrite(thisLeaf->key[i], keySize, 1, saveFile);
		}
		for (i = 0; i < thisLeaf->ptrNum; i++) 
			fwrite(thisLeaf->data[i], sizeof(resultType), 1, saveFile);
		thisLeaf = thisLeaf->sibling;
	}
	
	freeIndex(thisIndex);

	return noError;
}

int main(){
	indexType *new;
	int t = 20;
	int b = 32;
	saveBufferedIndexToDisk(new);
}
