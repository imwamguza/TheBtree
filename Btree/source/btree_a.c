#include <__btree_a.h>

bool btree_elt_init(btree_a* t, btree_elt_a* e)
{
	if(!t || !e)
		return false;

	e->index = e->nitems = e->capacity = e->level = 0;
	e->prev = e->next = e->first_child = e->last_child = 0;
	e->leaf = e->first = e->last = e->i = false;

	e->items = (char*)0;
	e->tree = t;

	return true;
}

void btree_elt_destroy(btree_elt_a* e)
{
	if(e->i)
		if(e->items)
			free(e->items);

	e->i = false;
	e->items = (char*)0;
}

#define btreeeltlocal __attribute__((cleanup(btree_elt_destroy)))

bool btree_elt_read_mini(btree_elt_a* e, file_a* idx)
{
	if(!e || !idx || e->index < 1)
		return false;

	if(e->i)
		if(e->items)
			free(e->items);

	file_seek(idx, e->index*e->tree->page_size);
	file_read(idx, (char*)e, sizeof(btree_elt_a)-sizeof(void*)*2);
	
	e->items = (char*)malloc(e->tree->item_size*(e->nitems+sizeof(int)));
	e->i = true;

	file_read(idx, e->items, e->tree->item_size*e->nitems);

	return true;
}

bool btree_elt_read(btree_elt_a* e)
{
	if(!e || e->index < 1)
		return false;

	file_a* idx = file_create(e->tree->name.data, "rb+");
	if(!idx)
		return false;
	
	bool ret;
	file_lock(idx);

	if(btree_is_unique(e->tree))
		ret = btree_elt_read_mini(e, idx);
	else
		ret = ibtree_elt_read_mini(e, idx);
	
	file_destroy(&idx);
	return ret;
}

bool btree_elt_write_mini(const btree_elt_a* e, file_a* idx)
{
	if(!e || !idx || e->index < 1 || !e->tree)
		return false;

	file_seek(idx, e->index*e->tree->page_size);
	file_write(idx, (char*)e, sizeof(btree_elt_a)-sizeof(void*)*2);
	file_write(idx, e->items, e->tree->item_size*e->nitems);
	return true;
}

bool btree_elt_write(const btree_elt_a* e)
{
	if(!e || e->index < 1)
		return false;

	file_a* idx = file_create(e->tree->name.data, "rb+");
	if(!idx)
		return false;
	
	bool ret;
	file_lock(idx);

	if(btree_is_unique(e->tree))
		ret = btree_elt_write_mini(e, idx);
	else
		ret = ibtree_elt_write_mini(e, idx);
	
	file_destroy(&idx);
	return ret;
}

void btree_ends_write(const btree_a* t, const btree_ends_a ends, file_a* idx)
{
	if(!t || !idx || ends.node < 1)
		return;

	off_t off = ends.node * t->page_size + offsetof(btree_elt_a, first);
	file_seek(idx, off);

	file_write(idx, (char*)&ends.first, sizeof(bool));
	file_write(idx, (char*)&ends.last, sizeof(bool));
}

bool btree_read_mini(btree_a* t, file_a* idx)
{
	if(!t || !idx)
		return false;

	file_seek(idx, 0);
	file_read(idx, (char*)t, sizeof(btree_a));

	return true;
}

bool btree_read(btree_a* t)
{
	if(!t || !t->name.len)
		return false;

	file_a* idx = file_create(t->name.data, "rb+");
	if(!idx)
		return false;

	file_lock(idx);
	bool ret = btree_read_mini(t, idx);
	file_destroy(&idx);
	return ret;
}

bool btree_write_mini(btree_a* t, file_a* idx)
{
	if(!t || !idx)
		return false;

	file_seek(idx, 0);
	file_write(idx, (char*)t, sizeof(btree_a));

	return true;
}

bool btree_write(btree_a* t)
{
	if(!t || !t->name.len)
		return false;

	file_a* idx = file_create(t->name.data, "rb+");
	if(!idx)
		return false;

	file_lock(idx);
	bool ret = btree_write_mini(t, idx);
	file_destroy(&idx);
	return ret;
}

void btree_copy(const btree_a* t, void* _dst, const void* _src)
{
	if(!t || !_dst || !_src)
		return;

	btree_item_a* dst = (btree_item_a*)_dst;
	btree_item_a* src = (btree_item_a*)_src;

	dst->value = src->value;
	dst->len = src->len;

	int k;
	switch(t->dtype)
	{
		case Char:
			for(k = 0; k < src->len; k++)
				dst->ckey[k] = src->ckey[k];
			dst->ckey[src->len] = '\0';
			return;

		case Short:
			for(k = 0; k < src->len; k++)
				dst->skey[k] = src->skey[k];
			return;

		case Int:
			for(k = 0; k < src->len; k++)
				dst->ikey[k] = src->ikey[k];
			return;

		case Long:
			for(k = 0; k < src->len; k++)
				dst->lkey[k] = src->lkey[k];
			return;

		case Ulong:
			for(k = 0; k < src->len; k++)
				dst->ukey[k] = src->ukey[k];
			return;

		case Float:
			for(k = 0; k < src->len; k++)
				dst->fkey[k] = src->fkey[k];
			return;

		case Double:
			for(k = 0; k < src->len; k++)
				dst->dkey[k] = src->dkey[k];
			return;

		case Other:
			if(t->copy)
				t->copy(dst->key, src->key);
			else
			{
				magenta();
				printf("FATAL: NO COPY OPERATION DEFINED FOR BTREE.\n Exiting now...\n");
				exit(0);
			}
			return;
	}
}

bool btree_elt_set_capacity(btree_elt_a* e, int cap)
{
	if(!e || e->tree->item_size < 1 || cap <= e->nitems+e->capacity)
		return false;

	int item_size = e->tree->item_size;
	char* items = (char*)malloc(cap*item_size);
	if(!e->items && !e->nitems && !e->capacity)
	{
		e->items = items;
		e->capacity = cap;
		e->i = true;
		return true;
	}

	for(int k = 0; k < e->nitems; k++)
		btree_copy(e->tree, items+k*item_size, e->items+k*item_size);

	free(e->items);

	e->items = items;
	e->capacity = cap - e->nitems;

	return true;
}

long btree_item_value(const void* item)
{
	btree_item_a* i = (btree_item_a*)item;
	if(i)
		return i->value;
	else
		return 0;
}

void btree_item_set_value(void* item, long value)
{
	btree_item_a* i = (btree_item_a*)item;
	if(i)
		i->value = value;
}

char* item_at(btree_elt_a* e, int at)
{
	if(!e)
		return (char*)0;
	return e->items + at*e->tree->item_size;
}

bool btree_elt_create(btree_elt_a* src, btree_elt_a* dst, int start, int end)
{
	if(!src || !dst || start >= end || end > src->nitems)
		return false;

	btree_elt_init(src->tree, dst);
	if(!src->leaf)
	{
		if(start == 0)
			dst->first_child = src->first_child;
		else
			dst->first_child = btree_item_value(item_at(src, start));

		dst->last_child = btree_item_value(item_at(src, end-1));
	}
	else
		dst->leaf = src->leaf;

	int real_start = start;
	if(!src->leaf && start > 0)
		real_start = start + 1;

	btree_elt_set_capacity(dst, end-real_start+sizeof(int));
	for(int j = real_start, k = 0; j < end; j++, k++)
		btree_copy(src->tree, item_at(dst, k), item_at(src, j));

	dst->nitems = end-real_start;
	dst->capacity = sizeof(int);
	dst->level = src->level;

	return true;
}

void btree_set_compare(btree_a* t, int(*func)(const void*, const void*))
{
	if(!t)
		return;
	t->compare = func;
}

void btree_set_copy(btree_a* t, void(*func)(void*, const void*))
{
	if(!t)
		return;
	t->copy = func;
}

void btree_set_print(btree_a* t, void(*func)(const void*))
{
	if(!t)
		return;
	t->print = func;
}

void btree_set_d_size(btree_a* t, int size)
{
	if(!t)
		return;
	t->d_size = size;
}

btree_item_a* btree_item_create(const btree_a* t, key_a key, long value)
{
	if(!t || !key.key)
		return (btree_item_a*)0;

	btree_item_a* i = (btree_item_a*)malloc(t->item_size);
	i->value = value;
	i->len = t->key_size;

	int k;
	switch(t->dtype)
	{
		case Char:
			i->len = strlen(key.ckey) < t->key_size-1 ? strlen(key.ckey) : t->key_size-2;
			for(k = 0; k < i->len; k++)
				i->ckey[k] = key.ckey[k];
			i->ckey[k+1] = '\0';
			return i;

		case Short:	
			for(k = 0; k < i->len; k++)
				i->skey[k] = key.skey[k];
			return i;

		case Int:
			for(k = 0; k < i->len; k++)
				i->ikey[k] = key.ikey[k];
			return i;

		case Long:
			for(k = 0; k < i->len; k++)
				i->lkey[k] = key.lkey[k];
			return i;

		case Ulong:
			for(k = 0; k < i->len; k++)
				i->ukey[k] = key.ukey[k];
			return i;

		case Float:
			for(k = 0; k < i->len; k++)
				i->fkey[k] = key.fkey[k];
			return i;

		case Double:
			for(k = 0; k < i->len; k++)
				i->dkey[k] = key.dkey[k];
			return i;

		case Other:
			if(t->copy)
				t->copy(i->key, key.key);
			else
			{
				magenta();
				printf("FATAL: NO COPY OPERATION DEFINED FOR BTREE.\n Exiting now...\n");
				exit(0);
			}
			return i;
	}
}

void btree_item_print(const btree_a* t, const void* item)
{
	if(!t || !item)
		return;

	btree_item_a* i = (btree_item_a*)item;
	printf("Value %ld Size %ld Key ", i->value, i->len);

	int k;
	switch(t->dtype)
	{
		case Char:
			printf("%s\n", i->ckey);
			return;

		case Short:
			printf("{");
			for(k = 0; k < i->len-1; k++)
				printf("%hd, ", i->skey[k]);
			printf("%hd}\n", i->skey[i->len-1]);
			return;

		case Int:
			printf("{");
			for(k = 0; k < i->len-1; k++)
				printf("%d, ", i->ikey[k]);
			printf("%d}\n", i->ikey[i->len-1]);
			return;

		case Long:
			printf("{");
			for(k = 0; k < i->len-1; k++)
				printf("%ld, ", i->lkey[k]);
			printf("%ld}\n", i->lkey[i->len-1]);
			return;

		case Ulong:
			printf("{");
			for(k = 0; k < i->len-1; k++)
				printf("%zul, ", i->ukey[k]);
			printf("%zu}\n", i->ukey[i->len-1]);
			return;

		case Float:
			printf("{");
			for(k = 0; k < i->len-1; k++)
				printf("%f, ", i->fkey[k]);
			printf("%f}\n", i->fkey[i->len-1]);
			return;

		case Double:
			printf("{");
			for(k = 0; k < i->len-1; k++)
				printf("%lf, ", i->dkey[k]);
			printf("%lf}\n", i->dkey[i->len-1]);
			return;

		case Other:
			if(t->print)
				t->print(i->key);
			else
			{
				magenta();
				printf("FATAL: NO PRINT OPERATION DEFINED FOR BTREE.\n Exiting now...\n");
				exit(0);
			}
	}
}

void btree_join_leaves(btree_elt_a* left, btree_elt_a* right)
{
	if(left->next != right->index && right->prev != left->index)
	{
		printf("FATAL : NON LINKED LEAVES CAN NOT BE JOINED.\n Exiting now ...\n");
		exit(0);
	}

	if(left->last || right->first)
	{
		printf("FATAL : LEAFY LEFT CAN NOT BE LAST AND RIGHT CAN NOT BE FIRST.\nExiting now ...\n");
		exit(0);
	}

	btree_elt_set_capacity(left, left->tree->max_items*2);

	for(int i = 0, k = left->nitems; i < right->nitems; i++, k++)
		btree_copy(left->tree, item_at(left, k), item_at(right, i));
	left->nitems += right->nitems;
}

void btree_join_indices(btree_elt_a* left, btree_elt_a* right)
{
	if(left->next != right->index && right->prev != left->index)
	{
		printf("FATAL : NON LINKED INDICES CAN NOT BE JOINED.\n Exiting now ...\n");
		exit(0);
	}

	if(left->last || right->first)
	{
		printf("FATAL : NON-LEAFY LEFT CAN NOT BE LAST AND RIGHT CAN NOT BE FIRST.\nExiting now ...\n");
		exit(0);
	}

	btree_elt_set_capacity(left, left->tree->max_items*2+1);
	
	btreeeltlocal btree_elt_a indexer;
	btree_elt_init(left->tree, &indexer);
	indexer.index = right->first_child;
	btree_elt_read(&indexer);

	while(indexer.first_child || !indexer.leaf)
	{
		indexer.index = indexer.first_child;
		btree_elt_read(&indexer);
	}

	btree_copy(left->tree, item_at(left, left->nitems), indexer.items);
	btree_item_set_value(item_at(left, left->nitems), right->first_child);
	left->nitems++;

	for(int i = 0, k = left->nitems; i < right->nitems; i++, k++)
		btree_copy(left->tree, item_at(left, k), item_at(right, i));
	left->nitems += right->nitems;
}

#define LEAF "LEAF"
#define NON_LEAF "NON_LEAF"
#define IS_TRUE "TRUE"
#define IS_FALSE "FALSE"

void btree_elt_print(btree_elt_a* e)
{
	if(!e)
		return;

	orange();
	for(int i = 0; i < 50; i++)
		printf("-");
	printf("\n");

	printf("{Prev : %ld, Index : %ld, Next : %ld, Num Items : %d}\n",
		e->prev, e->index, e->next, e->nitems);
	printf("{First Child : %ld, Last Child : %ld, Level : %d}\n",
		e->first_child, e->last_child, e->level);
	printf("{First : %s, Last : %s, Type : %s}\n",
		e->first ? IS_TRUE : IS_FALSE, e->last ? IS_TRUE : IS_FALSE, e->leaf ? LEAF : NON_LEAF);
	
	if(btree_is_unique(e->tree) || e->leaf)
	{
		for(int i = 0; i < e->nitems && i < e->tree->max_items*2; i++)
			btree_item_print(e->tree, item_at(e, i));
	}
	else
	{
		for(int i = 0; i < e->nitems && i < e->tree->max_items*2; i++)
			ibtree_item_print(e->tree, iitem_at(e, i));
	}

	for(int i = 0; i < 50; i++)
		printf("-");
	printf("\n");
	colour_reset();
}

void btree_print(btree_a* t)
{
	if(!t)
		return;

	green();
	for(int i = 0; i < 57; i++)
		printf("#");
	printf("\n");

	printf("{Tree Name : %s, Size : %ld, Count : %ld, Root ID : %ld}\n",
		t->name.data, t->size, t->count, t->root_id);
	printf("{Max Items : %d, Item Size : %d, Key Size : %d, Unique : %s}\n",
		t->max_items, t->item_size, t->key_size, btree_is_unique(t) ? IS_TRUE : IS_FALSE);
	printf("{Height : %d, Page Size : %d, Data Size : %d, Max IItems : %d}\n",
		t->height, t->page_size, t->d_size, t->max_iitems);

	btreeeltlocal btree_elt_a root;
	btree_elt_init(t, &root);
	root.index = t->root_id;
	btree_elt_read(&root);
	btree_elt_print(&root);
	
	green();
	for(int i = 0; i < 57; i++)
		printf("#");
	printf("\n");
	colour_reset();
}

bool btree_create(btree_a* t, const char* name, int key_size, dtype_a dtype, bool unique)
{
	if(!t || !name || key_size < 1 || strlen(name) >= sizeof(t->name.data)-sizeof(double))
		return false;

	memset(&t->name, 0, sizeof(t->name));
	strcpy(t->name.data, name);
	strcat(t->name.data, ".idx");
	t->name.len = strlen(t->name.data);

	if(btree_read(t))
	{
		if(t->key_size != key_size || t->dtype != dtype)
			return false;

		btree_print(t);
		return true;
	}
	else
	{
		t->size = t->count = t->height = 0;
		t->key_size = key_size;
		t->dtype = dtype;
		switch(t->dtype)
		{
			case Char:
				t->d_size = sizeof(char);
				break;
			case Short:
				t->d_size = sizeof(short);
				break;
			case Int:
				t->d_size = sizeof(int);
				break;
			case Long:
				t->d_size = sizeof(long);
				break;
			case Ulong:
				t->d_size = sizeof(unsigned long);
				break;
			case Float:
				t->d_size = sizeof(float);
				break;
			case Double:
				t->d_size = sizeof(double);
				break;
			case Other:
				t->d_size = 0;
				break;
		}

		t->item_size = t->key_size*t->d_size + sizeof(btree_item_a);
		t->iitem_size = t->item_size+sizeof(long);
		t->page_size = PAGE_SIZE;
		t->flags[0] = unique;
		t->max_items = (t->page_size - sizeof(btree_elt_a)) / t->item_size;
		t->max_iitems = (t->page_size - sizeof(btree_elt_a)) / t->iitem_size;
		memset(&t->avail, 0, sizeof(t->avail));

		btree_elt_a root;
		btree_elt_init(t, &root);
		root.first = root.last = root.leaf = true;
		t->size = t->root_id = root.index = 1;

		file_a* idx = file_create(t->name.data, "wb+");
		if(!idx)
			return false;

		file_lock(idx);
		btree_write_mini(t, idx);
		btree_elt_write_mini(&root, idx);
		file_destroy(&idx);
	}

	t->copy = NULL;
	t->compare = NULL;
	t->print  = NULL;

	return true;
}

int btree_new_index(btree_a* t)
{
	if(!t)
		return 0;

	if(t->avail.len)
	{
		int ret = t->avail.data[t->avail.len];
		t->avail.len--;
		return ret;
	}

	return ++t->size;
}

void btree_return_index(btree_a* t, long index)
{
	if(!t)
		return;

	t->avail.data[t->avail.len] = index;
	t->avail.len++;
	t->size--;
}

int btree_compare(const btree_a* t, const void* item1, const void* item2)
{
	if(!t || !item1 || !item2)
	{
		printf("FATAL: CAN NOT DO A NULL COMPARISON.\n Exiting now ...\n");
		exit(0);
	}

	btree_item_a* one = (btree_item_a*)item1;
	btree_item_a* two = (btree_item_a*)item2;

	int k, other_cmp = 0, min = one->len < two->len ? one->len : two->len;
	double real_cmp = 0.0;

	switch(t->dtype)
	{
		case Char:
			for(k = 0; k < min; k++)
				if(one->ckey[k] - two->ckey[k])
					return one->ckey[k] - two->ckey[k];
			break;

		case Short:
			for(k = 0; k < min; k++)
				if(one->skey[k] - two->skey[k])
					return one->skey[k] - two->skey[k];
			break;

		case Int:
			for(k = 0; k < min; k++)
				if(one->ikey[k] - two->ikey[k])
					return one->ikey[k] - two->ikey[k];
			break;

		case Long:
			for(k = 0; k < min; k++)
				if(one->lkey[k] - two->lkey[k])
					return one->lkey[k] - two->lkey[k];
			break;

		case Ulong:
			for(k = 0; k < min; k++)
				if(one->ukey[k] - two->ukey[k])
					return one->ukey[k] - two->ukey[k];
			break;

		case Float:
			for(k = 0; k < min; k++)
				if(one->fkey[k] - two->fkey[k])
				{
					real_cmp = one->fkey[k] - two->fkey[k];
					if(real_cmp < 0)
						return -1;
					else
						return 1;
				}
			break;

		case Double:
			for(k = 0; k < min; k++)
				if(one->dkey[k] - two->dkey[k])
				{
					real_cmp = one->dkey[k] - two->dkey[k];
					if(real_cmp < 0)
						return -1;
					else
						return 1;
				}
			break;

		case Other:
			if(t->compare)
			{
				other_cmp = t->compare(one->key, two->key);
				if(other_cmp)
					return other_cmp;
			}
			else
			{
				magenta();
				printf("FATAL: NO COMPARE OPERATION DEFINED FOR BTREE.\n Exiting now...\n");
				exit(0);
			}
	}

	if(one->len - two->len)
		return one->len - two->len;
	
	if(btree_is_unique(t))
		return 0;

	if(one->value < 0 || two->value < 0)
		return 0;
	
	return one->value - two->value;
}

int btree_index_of(btree_elt_a* elt, const btree_item_a* item, bool* found)
{
	int beg = 0, end = elt->nitems, mid, cmp;
	while(beg < end)
	{
		mid = (beg+end) >> 1;
		cmp = btree_compare(elt->tree, item, item_at(elt, mid));

		if(cmp == 0)
		{
			*found = true;
			return mid;
		}
		else if(cmp < 0)
			end = mid;
		else
			beg = mid+1;
	}

	*found = false;
	return beg;
}

int btree_elt_insert(btree_elt_a* elt, btree_item_a* item)
{
	if(!elt || !item)
		return -1;

	bool found;
	int at = btree_index_of(elt, item, &found);
	if(found)
		return 1;
	
	if(!elt->capacity)
		btree_elt_set_capacity(elt, elt->nitems+sizeof(int));

	for(int i = elt->nitems; i > at; i--)
		btree_copy(elt->tree, item_at(elt, i), item_at(elt, i-1));

	btree_copy(elt->tree, item_at(elt, at), item);
	elt->capacity--;
	elt->nitems++;
	
	file_a* idx = file_create(elt->tree->name.data, "rb+");
	if(!idx)
		return -1;

	file_lock(idx);
	btree_elt_write_mini(elt, idx);
	
	if(elt->leaf)
	{
		elt->tree->count++;
		btree_write_mini(elt->tree, idx);
	}
	file_destroy(&idx);

	return 0;
}

int btree_elt_remove(btree_elt_a* elt, btree_item_a* item)
{
	if(!elt || !item)
		return -1;

	bool found;
	int at = btree_index_of(elt, item, &found);
	if(!found)
	{
		if(at)
			return 1;
		else
		{
			if(elt->prev)
			{
				btreeeltlocal btree_elt_a prev;
				btree_elt_init(elt->tree, &prev);
				prev.index = elt->prev;
				btree_elt_read(&prev);
				return btree_elt_remove(&prev, item);
			}
		}
	}

	for(int i = at; i < elt->nitems-1; i++)
		btree_copy(elt->tree, item_at(elt, i), item_at(elt, i+1));

	elt->capacity++;
	elt->nitems--;

	file_a* idx = file_create(elt->tree->name.data, "rb+");
	if(!idx)
		return -1;

	file_lock(idx);
	btree_elt_write_mini(elt, idx);
	if(elt->leaf)
	{
		elt->tree->count--;
		btree_write_mini(elt->tree, idx);
	}
	file_destroy(&idx);

	if(at == 0)
		return MOVE_UP;
	return 0;
}

int btree_elt_replace(btree_elt_a* elt, int at, btree_item_a* item)
{	
	if(at > 0 && at == elt->nitems-1 && elt->next)
	{
		if(btree_compare(elt->tree, item_at(elt, at), item) < 0)
		{
			btreeeltlocal btree_elt_a next, n_first;
			btree_elt_init(elt->tree, &next);
			next.index = elt->next;
			btree_elt_read(&next);
			if(next.leaf || !next.first_child)
			{
				printf("ERROR: SHOULD BE AN INDEX NODE\nExiting now ...\n");
				exit(EXIT_FAILURE);
			}

			btree_elt_init(elt->tree, &n_first);
			n_first.index = next.first_child;
			btree_elt_read(&n_first);
			
			if(btree_compare(elt->tree, item_at(&n_first, 0), item) < 0)
			{
				printf("ERROR: SHOULD BE GREATER THAN ITEM\nExiting now ...\n");
				btree_elt_print(elt);
				btree_item_print(elt->tree, item);
				printf("Found AT node\n");
				next.index = item->value;
				btree_elt_read(&next);
				btree_elt_print(&next);
				btree_elt_print(&n_first);
				btree_item_print(elt->tree, item_at(&n_first, 0));
				exit(EXIT_FAILURE);
			}
		}
	}

	btree_copy(elt->tree, item_at(elt, at), item);
	
	return 0;
}

set_a* btree_elt_search(btree_elt_a* elt, btree_item_a* item)
{
	if(!elt || !item)
		return (set_a*)0;

	bool found;
	search_to_a left = STAY, right = STAY;
	btreeeltlocal btree_elt_a lnode, rnode;
	lnode.i = rnode.i = false;

	int at = btree_index_of(elt, item, &found);
	if(!found)
	{
		if(at || !elt->prev)
			return (set_a*)0;
		else
		{
			btreeeltlocal btree_elt_a prev;
			btree_elt_init(elt->tree, &prev);
			prev.index = elt->prev;
			btree_elt_read(&prev);
			return btree_elt_search(&prev, item);
		}
	}

	set_a* results = set_create();
	set_insert(results, btree_item_value(item_at(elt, at)));

	for(int i = at+1; i < elt->nitems; i++)
	{
		if(!btree_compare(elt->tree, item_at(elt, i), item))
			set_insert(results, btree_item_value(item_at(elt, i)));
		else
			break;
		
		if(i == elt->nitems-1)
			right = RIGHT;
	}

	for(int i = at-1; i >= 0; i--)
	{
		if(!btree_compare(elt->tree, item_at(elt, i), item))
			set_insert(results, btree_item_value(item_at(elt, i)));
		else
			break;
		
		if(i == 0)
			left = LEFT;
	}

	if(right && elt->next)
	{
		int next = elt->next;
		btree_elt_init(elt->tree, &rnode);
		while(right)
		{
			rnode.index = next;
			btree_elt_read(&rnode);

			for(int i = 0; i < rnode.nitems; i++)
			{
				if(!btree_compare(elt->tree, item_at(&rnode, i), item))
					set_insert(results, btree_item_value(item_at(&rnode, i)));
				else
				{
					right = STAY;
					break;
				}

				if(i == rnode.nitems-1 && rnode.next)
					next = rnode.next;
			}
			btree_elt_destroy(&rnode);
		}
	}

	if(left && elt->prev)
	{
		int prev = elt->prev;
		btree_elt_init(elt->tree, &lnode);
		while(left)
		{
			lnode.index = prev;
			btree_elt_read(&lnode);

			for(int i = lnode.nitems-1; i > -1; i--)
			{
				if(!btree_compare(elt->tree, item_at(&lnode, i), item))
					set_insert(results, btree_item_value(item_at(&lnode, i)));
				else
				{
					left = STAY;
					break;
				}

				if(i == 0 && lnode.prev)
					prev = lnode.prev;
			}
			btree_elt_destroy(&lnode);
		}
	}

	return results;
}

bool btree_move_up(btree_iter_a* it, btree_item_a* old, btree_item_a* new_item, int level, file_a* idx)
{
	if(!it || !old || !new_item || !idx)
		return false;

	bool found;
	int at;

#if false
	orange();
	btree_item_print(it->tree, old);
	btree_item_print(it->tree, new_item);
	colour_reset();
#endif
	for(int i = level; i <=it->nodes_len; i++)
	{
		btree_item_set_value(old, it->nodes[i-1].index);
		btree_item_set_value(new_item, it->nodes[i-1].index);

		at = btree_index_of(&it->nodes[i], old, &found);
		if(found)
		{
			btree_copy(it->tree, item_at(&it->nodes[i], at), new_item);
			btree_elt_write_mini(&it->nodes[i], idx);
			if(at)
				return true;
		}
		else
		{
			if(at)
				return false; /*NOT FOUND*/
			else
				return true; /*FIRST CHILD*/
		}
	}

	return true;
}

static int btree_split_index(btree_iter_a* it, int level)
{
	if(!it)
		return -1;

	btree_elt_a *node, *rnode, *parent;
	node = rnode = parent = (btree_elt_a*)0;

	btreeeltlocal btree_elt_a left, mid, right;
	left.index = mid.index = right.index = 0;
	left.i = mid.i = right.i = false;
	int rnode_first;

	if(it->nodes[level].index == it->tree->root_id && it->nodes[level].first && it->nodes[level].last)
		node = &it->nodes[level];
	else if(it->nodes[level].last)
	{
		rnode = &it->nodes[level];
		node = &it->other;
		btree_elt_init(it->tree, node);
		node->index = rnode->prev;
		btree_elt_read(node);
		rnode_first = node->nitems;
		parent = &it->nodes[level+1];
		btree_join_indices(node, rnode);
	}
	else
	{
		node = &it->nodes[level];
		rnode_first = node->nitems;
		rnode = &it->other;
		btree_elt_init(it->tree, rnode);
		rnode->index = node->next;
		btree_elt_read(rnode);
		parent = &it->nodes[level+1];
		btree_join_indices(node, rnode);
	}
	
	int split, at, ends_len = 0;
	bool found = false, move_up = false, split_parent = false;
	btree_ends_a ends[6];
	btree_item_a *ritem, *item = (btree_item_a*)alloca(it->tree->item_size);

	if(node->nitems > it->tree->max_items*1.75)
	{
		split = node->nitems/3;

		btree_elt_create(node, &left, 0, split);
		left.index = node->index;
		left.prev = node->prev;
		left.first = node->first;
		left.last = false;

		ends[0].node = node->last_child;
		ends[0].first = false;
		ends[0].last = false;

		ends[1].node = left.last_child;
		ends[1].first = false;
		ends[1].last = true;

		btree_elt_create(node, &mid, split, split*2);
		left.next = mid.index = btree_new_index(it->tree);
		mid.prev = node->index;
		mid.next = node->next;
		mid.first = mid.last = false;

		ends[2].node = mid.first_child;
		ends[2].first = true;
		ends[2].last = false;

		ends[3].node = mid.last_child;
		ends[3].first = false;
		ends[3].last = true;

		btree_elt_create(node, &right, split*2, node->nitems);
		right.index = node->next;
		right.next = rnode->next;
		right.prev = mid.index;
		right.first = false;
		right.last = rnode->last;

		ends[4].node = rnode->first_child;
		ends[4].first = false;
		ends[4].last = false;

		ends[5].node = right.first_child;
		ends[5].first = true;
		ends[5].last = false;
		ends_len = 6;

		btree_copy(it->tree, item, item_at(node, rnode_first));
		//btree_copy(it->tree, item, rnode->items);
		btree_item_set_value(item, rnode->index);

		at = btree_index_of(parent, item, &found);
		if(found)
		{
			btree_copy(it->tree, item, item_at(node, split*2));
			//btree_copy(it->tree, item, right.items);
			btree_item_set_value(item, right.index);
			btree_elt_replace(parent, at, item);

			btree_copy(it->tree, item, item_at(node, split));
			//btree_copy(it->tree, item, mid.items);
			btree_item_set_value(item, mid.index);
			btree_elt_insert(parent, item);

			if(at == 0)
				move_up = true;
			if(parent->nitems == it->tree->max_items)
				split_parent = true;
		}
		else
		{
			printf("FATAL: TRISPLIT COULD NOT FIND AN INDEXING ELEMENT.\n Exiting now ...\n");
			printf("NODE NITEMS IS %d\n", rnode_first);
			btree_item_print(it->tree, item);
			btree_elt_print(node);
			btree_elt_print(rnode);
			exit(0);
		}
	}
	else if(node->nitems > it->tree->max_items)
	{
		split = node->nitems/2;

		btree_elt_create(node, &left, 0, split);
		left.index = node->index;
		left.prev = node->prev;
		left.next = node->next;
		left.first = node->first;
		left.last = false;

		ends[0].node = node->last_child;
		ends[0].first = false;
		ends[0].last = false;

		ends[1].node = left.last_child;
		ends[1].first = false;
		ends[1].last = true;

		btree_elt_create(node, &right, split, node->nitems);
		right.index = node->next;
		right.next = rnode->next;
		right.prev = node->index;
		right.first = false;
		right.last = rnode->last;

		ends[2].node = rnode->first_child;
		ends[2].first = false;
		ends[2].last = false;

		ends[3].node = right.first_child;
		ends[3].first = true;
		ends[3].last = false;
		ends_len = 4;

		//TODO Fetch first of rnode
		btree_copy(it->tree, item, item_at(node, rnode_first));
		//btree_copy(it->tree, item, rnode->items);
		btree_item_set_value(item, rnode->index);

		at = btree_index_of(parent, item, &found);
		if(found)
		{
			//TODO Use item at(node, split)
			btree_copy(it->tree, item, item_at(node, split));
			//btree_copy(it->tree, item, right.items);
			btree_item_set_value(item, right.index);
			btree_elt_replace(parent, at, item);

			if(at == 0)
				move_up = true;
		}
		else
		{
			printf("FATAL: BISPLIT COULD NOT FIND AN INDEXING ELEMENT.\n Exiting now ...\n");
			exit(0);
		}
	}
	else if(!parent)
	{
		if(node->level == MAX_TREE_HEIGHT-1)
		{
			printf("THE MAXIMUM TREE HEIGHT IS REACED.\nINCREASE MAXIMUM HEIGHT TO USE INDEXER\n");
			exit(0);
		}
		split = node->nitems/2;

		btree_elt_create(node, &left, 0, split);
		left.index = node->index;
		left.prev = 0;
		left.first = true;
		left.last = false;

		ends[0].node = left.last_child;
		ends[0].first = false;
		ends[0].last = true;

		btree_elt_create(node, &right, split, node->nitems);
		right.index= left.next = btree_new_index(it->tree);
		right.prev = left.index;
		right.next = 0;
		right.first = false;
		right.last = true;

		ends[1].node = right.first_child;
		ends[1].first = true;
		ends[1].last = false;
		ends_len = 2;

		btree_elt_create(node, &mid, split -1, split +1);
		//btree_elt_create(node, &mid, split, split+2);
		btree_item_set_value(mid.items, right.index);
		mid.leaf = false;
		mid.index = btree_new_index(it->tree);
		mid.first_child = left.index;
		mid.last_child = right.index;
		mid.level = node->level+1;
		mid.prev = mid.next = 0;
		mid.first = mid.last = true;

		it->tree->root_id = mid.index;
		it->tree->height++;
	}
	else
	{
		btree_elt_create(node, &left, 0, node->nitems);
		left.index = node->index;
		left.next = rnode->next;
		left.prev = node->prev;
		left.first = node->first;
		left.last = rnode->last;

		ends[0].node = node->last_child;
		ends[0].first = false;
		ends[0].last = false;

		ends[1].node = rnode->first_child;
		ends[1].first = false;
		ends[1].last = false;
		ends_len = 2;

		btree_copy(it->tree, item, item_at(node, rnode_first));
		//btree_copy(it->tree, item, rnode->items);
		btree_item_set_value(item, rnode->index);

		at = btree_index_of(parent, item, &found);
		if(found)
		{
			if(!btree_elt_remove(parent, item))
			{
				btree_return_index(it->tree, rnode->index);
				if(!parent->nitems)
				{
					btree_return_index(node->tree, parent->index);
					it->tree->size -= 2;
					it->tree->height--;
					it->tree->root_id = left.index;
					left.first = left.last = true;
					left.prev = left.next = 0;
					left.level = parent->level-1;
				}
			}
		}
		else
		{
			printf("FATAL: UNISPLIT COULD NOT FIND AN INDEXING ELEMENT.\n Exiting now ...\n");
			exit(0);
		}
	}

	file_a* idx = file_create(it->tree->name.data, "rb+");
	if(!idx)
		return -1;

	file_lock(idx);
	btree_elt_write_mini(&left, idx);
	btree_elt_write_mini(&mid, idx);
	btree_elt_write_mini(&right, idx);

	for(int i = 0; i < ends_len; i++)
		btree_ends_write(it->tree, ends[i], idx);

	if(move_up)
	{
		ritem = (btree_item_a*)alloca(it->tree->item_size);
		btree_copy(it->tree, ritem, rnode->items);
		btree_item_set_value(ritem, parent->index);
		btree_move_up(it, ritem, item, parent->level+1, idx);
	}

	if(split_parent)
	{
		file_destroy(&idx);
		btree_elt_destroy(&it->other);
		btree_elt_destroy(&it->pother);
		btree_elt_destroy(&left);
		btree_elt_destroy(&mid);
		btree_elt_destroy(&right);
		return btree_split_index(it, level+1);
	}

	if(parent)
		btree_elt_write_mini(parent, idx);

	btree_write_mini(it->tree, idx);
#if VERBOSE
	//if(!parent)
		btree_print(it->tree);
#endif
	file_destroy(&idx);

	return RECALL;
}

static int btree_split_leaf(btree_iter_a* it, int level)
{
	if(!it)
		return -1;

	btree_elt_a *node, *rnode, *parent;
	node = rnode = parent = (btree_elt_a*)0;

	btreeeltlocal btree_elt_a left, mid, right;
	left.index = mid.index = right.index = 0;
	left.i = mid.i = right.i = false;

	if(it->nodes[level].index == it->tree->root_id && it->nodes[level].first && it->nodes[level].last)
		node = &it->nodes[level];
	else if(it->nodes[level].last)
	{
		rnode = &it->nodes[level];
		node = &it->other;
		btree_elt_init(it->tree, node);
		node->index = rnode->prev;
		btree_elt_read(node);
		parent = &it->nodes[level+1];
		btree_join_leaves(node, rnode);
	}
	else//First or mid
	{
		node = &it->nodes[level];
		rnode = &it->other;
		btree_elt_init(it->tree, rnode);
		rnode->index = node->next;
		btree_elt_read(rnode);
		parent = &it->nodes[level+1];
		btree_join_leaves(node, rnode);
	}
	
	int split, at;
	bool found = false, move_up = false, split_parent = false;
	btree_item_a *ritem, *item = (btree_item_a*)alloca(it->tree->item_size);

	if(node->nitems > it->tree->max_items*1.75)
	{
		split = node->nitems/3;

		btree_elt_create(node, &left, 0, split);
		left.index = node->index;
		left.prev = node->prev;
		left.first = node->first;
		left.last = false;

		btree_elt_create(node, &mid, split, split*2);
		left.next = mid.index = btree_new_index(it->tree);
		mid.prev = node->index;
		mid.next = node->next;
		mid.first = mid.last = false;

		btree_elt_create(node, &right, split*2, node->nitems);
		right.index = node->next;
		right.next = rnode->next;
		right.prev = mid.index;
		right.first = false;
		right.last = rnode->last;

		btree_copy(it->tree, item, rnode->items);
		btree_item_set_value(item, rnode->index);

		at = btree_index_of(parent, item, &found);
		if(found)
		{
			btree_copy(it->tree, item, right.items);
			btree_item_set_value(item, right.index);
			btree_elt_replace(parent, at, item);

			btree_copy(it->tree, item, mid.items);
			btree_item_set_value(item, mid.index);
			btree_elt_insert(parent, item);

			if(at == 0 && it->tree->height > 1)
				move_up = true;
			if(parent->nitems == it->tree->max_items)
				split_parent = true;
		}
		else
		{
			btree_elt_print(parent);
			btree_elt_print(node);
			btree_elt_print(rnode);
			btree_item_print(node->tree, item);
			printf("The at variable got %d\n", at);
			printf("FATAL: COULD NOT FIND A LEAFY TRISPLIT INDEXING ELEMENT.\n Exiting now ...\n");
			exit(0);
		}
	}
	else if(node->nitems > it->tree->max_items)
	{
		split = node->nitems/2;

		btree_elt_create(node, &left, 0, split);
		left.index = node->index;
		left.prev = node->prev;
		left.next = node->next;
		left.first = node->first;
		left.last = false;

		btree_elt_create(node, &right, split, node->nitems);
		right.index = node->next;
		right.next = rnode->next;
		right.prev = node->index;
		right.first = false;
		right.last = rnode->last;

		btree_copy(it->tree, item, rnode->items);
		btree_item_set_value(item, rnode->index);

		at = btree_index_of(parent, item, &found);
		if(found)
		{
			btree_copy(it->tree, item, right.items);
			btree_item_set_value(item, right.index);
			btree_elt_replace(parent, at, item);

			if(at == 0 && it->tree->height > 1)
				move_up = true;
		}
		else
		{
			btree_elt_print(parent);
			btree_elt_print(node);
			btree_elt_print(rnode);
			btree_item_print(node->tree, item);
			printf("The at variable got %d\n", at);
			printf("FATAL: COULD NOT FIND A LEAFY BISPLIT INDEXING ELEMENT.\n Exiting now ...\n");
			exit(0);
		}
	}
	else if(!parent)
	{
		split = node->nitems/2;

		btree_elt_create(node, &left, 0, split);
		left.index = node->index;
		left.prev = 0;
		left.first = true;
		left.last = false;

		btree_elt_create(node, &right, split, node->nitems);
		right.index= left.next = btree_new_index(it->tree);
		right.prev = left.index;
		right.next = 0;
		right.first = false;
		right.last = true;

		btree_elt_create(node, &mid, split, split+1);
		btree_item_set_value(mid.items, right.index);
		mid.leaf = false;
		mid.index = btree_new_index(it->tree);
		mid.first_child = left.index;
		mid.last_child = right.index;
		mid.level = node->level+1;
		mid.prev = mid.next = 0;
		mid.first = mid.last = true;

		it->tree->root_id = mid.index;
		it->tree->height++;
	}
	else
	{
		btree_elt_create(node, &left, 0, node->nitems);
		left.index = node->index;
		left.next = rnode->next;
		left.prev = node->prev;
		left.first = node->first;
		left.last = rnode->last;

		btree_copy(it->tree, item, rnode->items);
		btree_item_set_value(item, rnode->index);

		at = btree_index_of(parent, item, &found);
		if(found)
		{
			if(!btree_elt_remove(parent, item))
			{
				btree_return_index(it->tree, rnode->index);
				if(!parent->nitems)
				{
					it->tree->size = it->tree->root_id = left.index = 1;
					left.first = left.last = left.leaf = true;
					it->tree->avail.len = left.prev = left.next = left.level = 0;
				}
			}
		}
		else
		{
			printf("FATAL: COULD NOT FIND A LEAFY UNISPLIT INDEXING ELEMENT.\n Exiting now ...\n");
			exit(0);
		}
	}

	file_a* idx = file_create(it->tree->name.data, "rb+");
	if(!idx)
		return -1;

	file_lock(idx);
	
	btree_elt_write_mini(&left, idx);
	btree_elt_write_mini(&mid, idx);
	btree_elt_write_mini(&right, idx);
	
	///FIXME WHICH COMES FIRST, MOVE UP OR WRITES
	if(move_up)
	{
		ritem = (btree_item_a*)alloca(it->tree->item_size);
		btree_copy(it->tree, ritem, rnode->items);
		btree_item_set_value(ritem, parent->index);
		btree_move_up(it, ritem, item, parent->level+1, idx);
	}

	if(parent)
		btree_elt_write_mini(parent, idx);

	btree_write_mini(it->tree, idx);
#if VERBOSE
	btree_print(it->tree);
#endif
	file_destroy(&idx);

	if(split_parent)
	{
		btree_elt_destroy(&it->other);
		btree_elt_destroy(&it->pother);
		btree_elt_destroy(&left);
		btree_elt_destroy(&mid);
		btree_elt_destroy(&right);
		return btree_split_index(it, level+1);
	}

	return RECALL;
}

static void btree_iter_destroy(btree_iter_a* iter)
{
	btree_iter_a it = *iter;
	if(it.i)
		if(it.item)
			free(it.item);

	for(int i = 0; i <= it.nodes_len; i++)
		if(it.nodes[i].i)
			if(it.nodes[i].items)
				free(it.nodes[i].items);

	for(int i = 0; i <= it.nodes_len; i++)
		it.nodes[i].i = false;

	if(it.other.i)
		if(it.other.items)
			free(it.other.items);

	if(it.pother.i)
		if(it.pother.items)
			free(it.pother.items);

	it.i = it.other.i = it.pother.i = false;
}

static bool btree_iter_init(btree_iter_a* it, btree_a* t, key_a key, long value)
{
	if(!it || !t || !key.key)
		return false;

	it->tree = t;
	it->other.i = it->pother.i = false;
	it->nodes_len = t->height;
	it->item = btree_item_create(t, key, value);
	it->i = true;

	int at, level = t->height, curr_index = t->root_id, value_holder = value;
	bool found = false;
	it->item->value = -1;
	it->node_parent_pos[level] = -2;

	while(level)
	{
		btree_elt_init(t, &it->nodes[level]);
		it->nodes[level].index = curr_index;
		btree_elt_read(&it->nodes[level]);

		at = btree_index_of(&it->nodes[level], it->item, &found);
		if(found)
		{
			it->node_parent_pos[level-1] = at;
			curr_index = btree_item_value(item_at(&it->nodes[level], at));
		}
		else
		{
			if(at)
			{
				it->node_parent_pos[level-1] = at-1;
				curr_index = btree_item_value(item_at(&it->nodes[level], at-1));
			}
			else
			{
				it->node_parent_pos[level-1] = -1;
				curr_index = it->nodes[level].first_child;
			}
		}

		level--;
	}

	it->found = found;
	it->item->value = value_holder;
	btree_elt_init(t, &it->nodes[level]);
	it->nodes[level].index = curr_index;
	btree_elt_read(&it->nodes[level]);

	return true;
}

static int btree_iter_lookup(btree_iter_a* it)
{
	if(it->item->value == -1)
		return 0;

	btree_elt_a* curr = &it->nodes[0];
	if(!curr->leaf)
		return -1;
	
	bool found;
	int at = btree_index_of(curr, it->item, &found);
	if(found)
		return 1;

	if(at && at < curr->nitems)
		return 0;

	while(at == 0)
	{//TODO WHAT IF CURR IS ALREADY A FIRST CHILD
		if(!curr->prev)
			return 0;
		curr->index = curr->prev;
		btree_elt_read(curr);
		at = btree_index_of(curr, it->item, &found);

		if(found)
			return 1;

		if(at)
		{
			if(curr->nitems < it->tree->max_items)
				return 0;
			return btree_split_leaf(it, 0);
		}
	}
	
	if(!it->found)
		return 0;

	btreeeltlocal btree_elt_a next;
	btree_elt_init(it->tree, &next);
	while(at == curr->nitems)
	{
		if(!curr->next)
			return 0;

		next.index = curr->next;
		btree_elt_read(&next);
		at = btree_index_of(&next, it->item, &found);

		if(found)
			return 1;

		if(at == 0)
			return 0;

		//WORRY NOT ABOUT ME	
		//TODO WHAT IF CURR IS ALREADY A LAST CHILD *add f and l bools to iter
		if(at)
		{
			curr->index = next.index;
			btree_elt_read(curr);
			if(at < next.nitems)
			{
				if(curr->nitems < it->tree->max_items)
					return 0;
				return btree_split_leaf(it, 0);
			}
		}
	}
}

static void btree_iter_print(btree_iter_a* it)
{
	if(!it)
		return;
	
	cyan();
	for(int i = 0; i < 50; i++)
		printf("$");
	printf("\n");
	
	printf("ITEM : ");
	btree_item_print(it->tree, it->item);

	for(int i = 0; i <= it->nodes_len; i++)
		btree_elt_print(&it->nodes[i]);
	
	cyan();
	for(int i = 0; i < 50; i++)
		printf("$");
	printf("\n");
	colour_reset();
}

#define btreeiterlocal __attribute__((cleanup(btree_iter_destroy)))

int ubtree_insert(btree_a* tree, key_a key, long value)
{
	btreeiterlocal btree_iter_a it;
	btree_iter_init(&it, tree, key, value);
	int lookup = btree_iter_lookup(&it);
	if(lookup)
		return lookup;

	btree_elt_a* curr = &it.nodes[0];
	if(curr->leaf)
	{
		if(curr->nitems == tree->max_items)
			return btree_split_leaf(&it, 0);

		return btree_elt_insert(curr, it.item);
	}

	printf("FATAL: Cannot insert in a non leaf.\n Exiting now ...\n");
	exit(0);

	return -1;
}

int ubtree_remove(btree_a* tree, key_a key, long value)
{
	btreeiterlocal btree_iter_a it;
	btree_iter_init(&it, tree, key, value);

	btree_elt_a* curr = &it.nodes[0];
	if(curr->leaf)
	{
		if(curr->nitems < tree->max_items/3 && curr->index != tree->root_id)
			return btree_split_leaf(&it, 0);

		int ret = btree_elt_remove(curr, it.item);
		if(ret == MOVE_UP)
		{
			btree_item_a* new_item = (btree_item_a*)alloca(tree->item_size);
			btree_copy(tree, new_item, item_at(curr, 0));

			file_a* idx = file_create(tree->name.data, "rb+");
			if(!idx)
				exit(0);

			bool ans = btree_move_up(&it, it.item, new_item, 1, idx);
			file_destroy(&idx);
			if(ans)
				return 0;
			else
				return -1;
		}

		return ret;
	}

	printf("FATAL: Cannot remove from a non leaf.\n Exiting now ...\n");
	exit(0);

	return -1;
}

set_a* ubtree_search(btree_a* tree, key_a key, long value)
{
	btreeiterlocal btree_iter_a it;
	btree_iter_init(&it, tree, key, value);

	btree_elt_a* curr = &it.nodes[0];
	if(curr->leaf)
		return btree_elt_search(curr, it.item);

	printf("FATAL: Cannot search from a non leaf.\n Exiting now ...\n");
	exit(0);

	return (set_a*)0;
}


int btree_insert(btree_a* tree, key_a key, long value)
{
	if(btree_is_unique(tree))
		return ubtree_insert(tree, key, value);
	return ibtree_insert(tree, key, value);
}

int btree_remove(btree_a* tree, key_a key, long value)
{
	if(btree_is_unique(tree))
		return ubtree_remove(tree, key, value);
	return ibtree_remove(tree, key, value);
}

set_a* btree_search(btree_a* tree, key_a key, long value)
{
	if(btree_is_unique(tree))
		return ubtree_search(tree, key, value);
	return ibtree_search(tree, key, value);
}





void btree_test()
{
	btree_a tree;
	if(!btree_create(&tree, "text", 18, Char, true))
		return;
	
	char buffer[80];
	
	//file_a* input = file_create("data/rockyou.txt", "r");
	file_a* input = file_create("data/dns-big.txt", "r");
	if(!input)
		return;
	
	cyan();
	file_lock(input);
	for(int i = 1;; i++)
	{
		if(!file_get_delim(input, buffer, 80, '\n'))
			if(!file_get_delim(input, buffer, 80, '\n'))
				break;

		if(btree_insert(&tree, buffer, i) == RECALL)
			btree_insert(&tree, buffer, i);

		if(i%1719 == 0)
		{
			i++;
			for(int j = 0; j < 12; j++, i++)
				if(btree_insert(&tree, buffer, i) == RECALL)
					btree_insert(&tree, buffer, i);
		}
	}
	file_destroy(&input);
	return;
}

#define REPEATED "ibtree_a_tester0001"

void ibtree_test()
{
	btree_a tree;
	if(!btree_create(&tree, "text", 16, Char, true))
		return;
	
	for(int i = 1;; i++)
	{
		if(btree_insert(&tree, REPEATED, i) == RECALL)
			btree_insert(&tree, REPEATED, i);
	}
	return;

}
