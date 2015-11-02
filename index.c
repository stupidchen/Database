#ifndef _INDEX_H
#define _INDEX_H
#include<stdio.h>

struct node{
	int level;
	struct node* parent;
	int keyType;
}
struct index{
	int databaseID, tableID, columnID;

};
