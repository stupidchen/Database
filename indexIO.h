#ifndef _INDEX_IO_H
#define _INDEX_IO_H

#include "index.h"

#define indexFilename "index"
#define indexLogFilename "indexLog"
#define indexFilehead "Index file"
#define maxFilenameLength 30

typedef struct indexForIO{ 
	int tableID, columnID, indexID;
	int level;
	int keyType;
}indexTypeForIO;

/*
   IO operation.
   getIndexFromMemory is the procedure that get the index from disk to the buffer.
   saveBufferdIndexToDisk is the procedure that save the buffered index to the disk.
   */

void freeNode(nodeType *thisNode, int keyType);

void freeIndexNode(nodeType *thisNode, int keyType);

void freeIndex(indexType *thisIndex);

void freeSystem(systemType *thisSystem);

void deleteIndexFile(int tableID, int columnID);

int replaceBufferedIndex(indexType **buffer, int tableID, int columnID, int indexID, int bufferID);

void reconstructIndex(indexType *thisIndex, nodeType *thisNode, int layerNum);

indexType *getIndexFromDisk(int tableID, int columnID);

int saveBufferedIndexToDisk(indexType *thisIndex);

#endif
