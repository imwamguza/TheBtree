#ifndef BTREE_A_H
#define BTREE_A_H

#include <set_a.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"{
#endif

typedef enum dtype_a{
	Char = 1,
	Short,
	Int,
	Long,
	Ulong,
	Float,
	Double,
	Other
}dtype_a;

typedef union key_a{
	char* ckey;
	short* skey;
	int* ikey;
	long* lkey;
	unsigned long* ukey;
	float* fkey;
	double* dkey;
	char* key;
}__attribute__((transparent_union)) key_a;

#define	PAGE_SIZE_1 -1
#define	PAGE_SIZE_2 0
#define	PAGE_SIZE_4 1
#define	PAGE_SIZE_8 2
#define	PAGE_SIZE_16 3
#define	PAGE_SIZE_32 4
#define	PAGE_SIZE_64 5
#define	PAGE_SIZE_128 6
#define	PAGE_SIZE_256 7
#define	PAGE_SIZE_512 8
#define	PAGE_SIZE_XXX 9
#define	PAGE_SIZE_XXX_1 9
#define	PAGE_SIZE_XXX_2 10
#define	PAGE_SIZE_XXX_4 11
#define	PAGE_SIZE_XXX_8 12
#define	PAGE_SIZE_XXX_16 13


typedef struct btree_a{
	long count, size, root_id;
	int max_items, max_iitems, item_size, iitem_size;
	int key_size, d_size, page_size, height;
	bool flags[4];
	
	dtype_a dtype;
	struct{
		int len;
		char data[128];
	}name;

	struct{
		int len;
		int data[59];
	}avail;

	int(*compare)(const void*, const void*);
	void(*copy)(void*, const void*);
	void(*print)(const void*);
}btree_a;

#define RECALL 17
#define TREE_PURPOSE PAGE_SIZE_1
#define VERBOSE true

//////////////////////////////ONLY FOR DTYPE OTHER//////////////////////////////
void btree_set_compare(btree_a* t, int(*func)(const void*, const void*));

void btree_set_copy(btree_a* t, void(*func)(void*, const void*));

void btree_set_print(btree_a* t, void(*func)(const void*));

void btree_set_d_size(btree_a* t, int size);
///////////////////////////////////////////////////////////////////////////////


bool btree_create(btree_a* t, const char* name, int key_size, dtype_a dtype, bool unique);

int btree_insert(btree_a* tree, key_a key, long value);

int btree_remove(btree_a* tree, key_a key, long value);

set_a* btree_search(btree_a* tree, key_a key, long value);


//////////////////BTREE TESTING ROUTINE////////////////////
extern void btree_test();
extern void ibtree_test();

#ifdef __cplusplus
}
#endif

#endif //BTREE_A_H
