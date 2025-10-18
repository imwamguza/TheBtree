#ifndef __BTREE_PRIVATE_A_H
#define __BTREE_PRIVATE_A_H

#include <btree_a.h>
#include <stdlib.h>

#include <file_a.h>
#include <cstring_a.h>
#include <string.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C"{
#endif

#ifndef VERBOSE
#define VERBOSE true
#endif

#ifndef TREE_PURPOSE
#define TREE_PURPOSE -1
#endif

#if TREE_PURPOSE == -1
#define PAGE_SIZE 64*16
#elif TREE_PURPOSE == 0
#define PAGE_SIZE 64*32
#elif TREE_PURPOSE == 1
#define PAGE_SIZE 64*64
#elif TREE_PURPOSE == 2
#define PAGE_SIZE 64*128
#elif TREE_PURPOSE == 3
#define PAGE_SIZE 64*256
#elif TREE_PURPOSE == 4
#define PAGE_SIZE 64*512
#elif TREE_PURPOSE == 5
#define PAGE_SIZE 64*1024
#elif TREE_PURPOSE == 6
#define PAGE_SIZE 64*2048
#elif TREE_PURPOSE == 7
#define PAGE_SIZE 64*4096
#elif TREE_PURPOSE == 8
#define PAGE_SIZE 64*8192
#elif TREE_PURPOSE == 9
#define PAGE_SIZE 1024*1024
#elif TREE_PURPOSE == 10
#define PAGE_SIZE 1024*1024*2
#elif TREE_PURPOSE == 11
#define PAGE_SIZE 1024*1024*4
#elif TREE_PURPOSE == 12
#define PAGE_SIZE 1024*1024*8
#elif TREE_PURPOSE == 13
#define PAGE_SIZE 1024*1024*16
#else
#define PAGE_SIZE 64*16
#endif

//SIZE 52 Bytes
struct btree_elt_a{
	long index, prev, next, first_child, last_child;
	int level, nitems, capacity;
	bool leaf, first, last, i;
	
	char* items;
	btree_a* tree;
}__attribute__((__packed__));

typedef struct btree_elt_a btree_elt_a;

typedef struct btree_item_a{
	long value, len;
	union{
		char ckey[0];
		short skey[0];
		int ikey[0];
		long lkey[0];
		unsigned long ukey[0];
		float fkey[0];
		double dkey[0];
		char key[0];
	};
}btree_item_a;

typedef struct{
	int node;
	bool first, last;
}btree_ends_a;

#ifndef MAX_TREE_HEIGHT
#define MAX_TREE_HEIGHT 4
#endif

typedef enum{
	STAY = 0,
	RIGHT,
	LEFT
}search_to_a;

typedef struct{
	btree_a* tree;
	btree_item_a *item;
	bool i, found;

	int nodes_len;
	int node_parent_pos[MAX_TREE_HEIGHT];
	btree_elt_a nodes[MAX_TREE_HEIGHT], other, pother;
}btree_iter_a;


/////////////////PRIVATE INTERFACE////////////////////////////
bool btree_elt_init(btree_a*t, btree_elt_a* e);

void btree_elt_destroy(btree_elt_a* e);
#define btreeeltlocal __attribute__((cleanup(btree_elt_destroy)))

///BTREE_ELT READERS
bool btree_elt_read_mini(btree_elt_a* e, file_a* idx);

bool ibtree_elt_read_mini(btree_elt_a* e, file_a* idx);

bool btree_elt_read(btree_elt_a* e);

///BTREE_ELT WRITERS
bool btree_elt_write_mini(const btree_elt_a* e, file_a* idx);

bool ibtree_elt_write_mini(const btree_elt_a* e, file_a* idx);

bool btree_elt_write(const btree_elt_a* e);


void btree_ends_write(const btree_a* t, const btree_ends_a ends, file_a* idx);

///BTREE READERS
bool btree_read_mini(btree_a* t, file_a* idx);

bool btree_read(btree_a* t);

///BTREE WRITERS
bool btree_write_mini(btree_a* t, file_a* idx);

bool btree_write(btree_a* t);


///BTREE_ELT_A UTILITIES 
void btree_copy(const btree_a* t, void* _dst, const void* _src);

void ibtree_copy(const btree_a* t, void* _dst, const void* _src);

bool btree_elt_set_capacity(btree_elt_a* e, int cap);

bool ibtree_elt_set_capacity(btree_elt_a* e, int cap);

bool btree_set_unque(btree_a* t, bool unique);

bool btree_is_unique(const btree_a* t);

long btree_item_value(const void* item);

long ibtree_item_pointer(const void* item);

void btree_item_set_value(void* item, long value);

void ibtree_item_set_pointer(void* item, long pointer);

char* item_at(btree_elt_a* e, int at);

char* iitem_at(btree_elt_a* e, int at);

bool btree_elt_create(btree_elt_a* src, btree_elt_a* dst, int start, int end);

bool ibtree_elt_create(btree_elt_a* src, btree_elt_a* dst, int start, int end);

btree_item_a* btree_item_create(const btree_a* t, key_a key, long value);

void btree_item_print(const btree_a* t, const void* item);

void ibtree_item_print(const btree_a* t, const void* item);

void btree_elt_print(btree_elt_a* e);

void btree_print(btree_a* t);

void btree_join_leaves(btree_elt_a* left, btree_elt_a* right);

void btree_join_indices(btree_elt_a* left, btree_elt_a* right);

void ibtree_join_indices(btree_elt_a* left, btree_elt_a* right);

int btree_new_index(btree_a* t);

void btree_return_index(btree_a* t, long index);

int btree_compare(const btree_a* t, const void* item1, const void* item2);

int ibtree_compare(const btree_a* t, const void* item1, const void* item2);

int btree_index_of(btree_elt_a* elt, const btree_item_a* item, bool* found);

int btree_elt_insert(btree_elt_a* elt, btree_item_a* item);

#define MOVE_UP 19

int btree_elt_remove(btree_elt_a* elt, btree_item_a* item);

int btree_elt_replace(btree_elt_a* elt, int at, btree_item_a* item);

set_a* btree_elt_search(btree_elt_a* elt, btree_item_a* item);

bool btree_move_up(btree_iter_a* it, btree_item_a* old, btree_item_a* new_item, int level, file_a* idx);

///////////DUPLICATE KEY BTREE FUNCTIONS
int ibtree_insert(btree_a* tree, key_a key, long value);

int ibtree_remove(btree_a* tree, key_a key, long value);

set_a* ibtree_search(btree_a* tree, key_a key, long value);

///////////UNIQUE KEY BTREE FUNCTIONS
int ubtree_insert(btree_a* tree, key_a key, long value);

int ubtree_remove(btree_a* tree, key_a key, long value);

set_a* ubtree_search(btree_a* tree, key_a key, long value);


////////////////END PRIVATE INTERFACE/////////////////////



#ifdef __cplusplus
}
#endif

#endif //BTREE_PRIVATE_A_H
