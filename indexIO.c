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
	if (thisNode == NULL) return;
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

void deleteIndexFile(int tableID, int columnID){
	char filename[maxFilenameLength];
	sprintf(filename, "%s%d%d", indexFilename, tableID, columnID);
	remove(filename);
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
	if (layerNum == 0){
		thisIndex->root = NULL;
		return;
	}
	if (layerNum == 1){
		thisIndex->root = thisNode;
		return;
	}

	int keySize = thisIndex->keySize;
	int level = thisIndex->level;
	int nextLayerNum = layerNum / level;
	if (layerNum % level != 0) nextLayerNum++;

	nodeType *firstNode = NULL;
	nodeType *thisParent, *lastParent = NULL;
	int i, j = 0, unMatch = layerNum, ptrNum;
	for (i = 0; i < nextLayerNum; i++){
		if (unMatch >= (level << 1)) ptrNum = level;
		else{
			if (unMatch > level) ptrNum = unMatch >> 1;
		}
		unMatch = unMatch - ptrNum;

		thisParent = initNode(ptrNum, 0, NULL, lastParent);
		if (firstNode == NULL) firstNode = thisParent;
		lastParent = thisParent;

		for (j = 0; j < thisParent->ptrNum; j++){
			thisParent->data[j] = (void*)malloc(sizeof(nodeType*));
			thisParent->data[j] = thisNode;
			if (j < thisParent->ptrNum - 1){
				thisParent->key[j] = (void*)malloc(keySize);
				memcpy(thisParent->key[j], thisNode->sibling->key[0], keySize);
			}
			thisNode->parent = thisParent;
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
	FILE *readFile = fopen(filename, "rb");
	if (readFile == NULL) return NULL;
	
	fread(newIndexForIO, sizeof(indexTypeForIO), 1, readFile);
	newIndex->firstLeaf = NULL;
	if (newIndex->keySize < 2) newIndex->keySize = 4;
	else newIndex->keySize = newIndex->keyType/2;

	int keyType = newIndex->keyType;
	int keySize = newIndex->keySize;
	switch (keyType){
		case 0:newIndex->opFun = &opFunInt;break;
		case 1:newIndex->opFun = &opFunFloat;break;
		case 2:newIndex->opFun = &opFunVarchar;break;
	}

	int i, ptrNum, leafNum = 0;
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
			newNode->key[i] = (void*)malloc(keySize);
			fread(newNode->key[i], keySize, 1, readFile);
		}
		for (i = 0; i < newNode->ptrNum; i++){
			newNode->data[i] = (void*)malloc(sizeof(resultType));
			fread(newNode->data[i], sizeof(resultType), 1, readFile);
		}
	}
	fclose(readFile);

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
	fwrite(indexForIO, sizeof(indexTypeForIO), 1, saveFile);

	int i;
	int keySize = thisIndex->keySize;
	int keyType = thisIndex->keyType;
	
	nodeType *thisLeaf = thisIndex->firstLeaf;
	while (thisLeaf != NULL){
		fwrite(&thisLeaf->ptrNum, sizeof(thisLeaf->ptrNum), 1, saveFile);
		for (i = 0; i < thisLeaf->ptrNum; i++) fwrite(thisLeaf->key[i], keySize, 1, saveFile);
		for (i = 0; i < thisLeaf->ptrNum; i++) 
			fwrite(thisLeaf->data[i], sizeof(resultType), 1, saveFile);
		thisLeaf = thisLeaf->sibling;
	}
	fclose(saveFile);

	freeIndex(thisIndex);
	return noError;
}

int main(){
	indexType *new;
	new = (indexType*)malloc(sizeof(indexType));
	new->tableID = 0;
	new->columnID = 2;
	new->indexID = 2;
	new->level = 3;
	new->keyType = 0;
	new->keySize = 4;
	new->root = NULL;
	new->firstLeaf = NULL;
	nodeType *rootNode = (nodeType*)malloc(sizeof(nodeType));
	rootNode->ptrNum = 2;

	saveBufferedIndexToDisk(new);

	indexType *new1 = getIndexFromDisk(0, 2);
	new1->tableID++;
	saveBufferedIndexToDisk(new1);
}
