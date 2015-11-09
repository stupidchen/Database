#ifndef _INDEX_H
#define _INDEX_H
#include<stdio.h>
#include<stdlib.h>

/*
	defaultMaxBufferedIndex 默认的缓存索引数
	dbFilenameLength	DBFile的文件名长度
	defaultIndexLevel	默认的B+树阶数	
*/

#define defaultMaxBufferedIndex 100
#define dbFilenameLength 20
#define defaultIndexLevel 5


/*
	以下均为错误返回值
*/

#define Error_CannotGetIndexFromDisk -1
#define Error_TupleIsnotExist -2
#define Error_CannotAllocateEnoughSpace -3
#define Error_InvalidKeyType -4
#define Error_InvalidBufferedIndexCode -5
#define Error_IndexFileCannotOpen -6
#define noError 0

/*
	resultType为索引查询结果结构，同时也是B+树叶子节点指向的数据结构
	filename为索引对应的行所在的数据库文件名
	position为索引对应的行所在的数据库文件中的位置
 */

typedef struct result{
	int position;
	char filename[dbFilenameLength];
}resultType;

/*
	nodeType为B+树的结点的结构体。
	ptrNum为此结点的儿子个数；对于叶子结点，儿子个数=关键字个数；对于非叶子结点，儿子个数=关键字个数-1
	key为关键字指针数组
	data为儿子指针数组；对于叶子结点，指针指向数据域（resultType）；对于非叶子结点，指针指向其它结点
	ifLeaf为叶子结点标识
	parent为父结点指针，sibling为右兄弟指针，如果不存在则为NULL
 */

typedef struct node{ 
	int ptrNum;
	void **key;
	void **data;
	int ifLeaf;
	struct node *parent, *sibling;
}nodeType;

/*
	indexType为B+树的结构体。
	tableID，columnID为此B+树索引基于的表ID与列ID，indexID为索引ID（方便内部模块调用，为表ID与列ID的唯一映射）
	level为B+树的阶数
	keyType为索引关键字的类型，0代表int，1代表float，2代表varchar（即char[]）
	root为B+树的根结点指针，firstLeaf为B+树的第一个叶子结点的指针（方便线性扫描）
	opFun为此索引的关键字的比较函数
 */

typedef struct index{ 
	int tableID, columnID, indexID;
	int level;
	int keyType;		
	nodeType *root, *firstLeaf;	
	int (*opFun)(void *x, void *y);
}indexType;

/*
	systemType为索引模块结构体。
	indexArray为缓存的索引的指针数组
	maxBufferedIndex为系统缓存的最大索引数
*/

typedef struct indexSystem{
	indexType **indexArray;
	int maxBufferedIndex;
}systemType;

/*	
	三个关键字比较函数，模块内用
*/
int opFunInt(void *x, void *y);

int opFunFloat(void *x, void *y);

int opFunVarchar(void *x, void *y);

/*
	indexID的映射函数，模块内用
*/

int makeIndexID(int tableID, int columnID);

/*
	结点初始化函数，模块内用
*/	

int initNode(nodeType *newNode);

/*
	系统初始化的外部接口，最大缓存索引数为maxBufferedIndex
*/

int initSystem(systemType *thisSystem, int maxBufferedIndex);

/*
	索引初始化的外部接口，新建基于表ID和列ID为tableID和columnID的索引，关键字类型为keyType
*/
int initGlobalIndex(systemType *thisSystem, int tableID, int columnID, int keyType);

/*
	索引初始化的内部接口，模块内用
*/

int initIndex(indexType *thisIndex, int tableID, int columnID, int keyType);	 

/*
	异常返回函数，模块内用
 */
resultType makeNar(int code);

/*
	索引搜索的外部接口，搜索基于表ID和列ID为tableID和columnID的索引，关键字为key，返回一个resultType的结构
*/

resultType searchGlobalIndex(systemType *thisSystem, int tableID, int columnID, void *key);

/*
	索引搜索的内部接口，模块内用
*/

resultType searchIndex(indexType *thisIndex, nodeType *thisNode, void *key);

/*
	新建一个resultType结构体，文件名为filename，位置为position，并返回其指针

*/
resultType *makeResult(char *filename, int position);


/*
	insertGlobalIndex为索引插入的外部接口，插入表ID和列ID为tableID和columnID的索引，关键字为key，插入的数据为data
 */

int insertGlobalIndex(systemType *thisSystem, int tableID, int columnID, void *key, void *data);

/*
	索引插入的内部接口
*/

int insertIndex(indexType *thisIndex, nodeType *thisNode, void *key, void *data);

int compareResultData(resultType *left, resultType *right);

/*
	deleteGlobalIndex为索引删除的外部接口，删除表ID和列ID为tableID和columnID的索引，关键字为key，要删除的数据为data
 */

int deleteGlobalIndex(systemType *thisSystem, int tableID, int columnID, void *key, resultType *data);

/*
	索引删除的内部接口
*/

int deleteIndex(indexType *thisIndex, nodeType *thisNode, void *key, resultType *data);

#endif
