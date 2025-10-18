#include <__btree_a.h>


typedef struct ibtree_item_a{
	long pointer;
	btree_item_a value;
}ibtree_item_a;


typedef struct{
	btree_a* tree;
	ibtree_item_a *item;
	bool i, found;

	int nodes_len;
	int node_parent_pos[MAX_TREE_HEIGHT];
	btree_elt_a nodes[MAX_TREE_HEIGHT], other, pother;
}ibtree_iter_a;


bool ibtree_elt_read_mini(btree_elt_a* e, file_a* idx)
{
	if(!e || !idx || e->index < 1 || e->leaf)
		return false;

	if(e->i)
		if(e->items)
			free(e->items);

	file_seek(idx, e->index*e->tree->page_size);
	file_read(idx, (char*)e, sizeof(btree_elt_a)-sizeof(void*)*2);

	e->items = (char*)malloc(e->tree->iitem_size*(e->nitems+sizeof(int)));
	e->i = true;

	file_read(idx, e->items, e->tree->iitem_size*e->nitems);

	return true;
}

bool ibtree_elt_write_mini(const btree_elt_a* e, file_a* idx)
{
	if(!e || !idx || e->index < 1 || !e->tree || e->leaf)
		return false;

	file_seek(idx, e->index*e->tree->page_size);
	file_write(idx, (char*)e, sizeof(btree_elt_a)-sizeof(void*)*2);
	file_write(idx, e->items, e->tree->iitem_size*e->nitems);
	return true;
}

bool btree_set_unique(btree_a* t, bool unique)
{
	if(!t)
		return false;
	t->flags[0] = unique;
	return true;
}

bool btree_is_unique(const btree_a* t)
{
	if(!t)
		return false;
	return t->flags[0];
}

void ibtree_copy(const btree_a* t, void* _dst, const void* _src)
{
	if(!t || !_dst || !_src || btree_is_unique(t))
		return;

	ibtree_item_a* dst = (ibtree_item_a*)_dst;
	ibtree_item_a* src = (ibtree_item_a*)_src;

	dst->pointer = src->pointer;
	dst->value.value = src->value.value;
	dst->value.len = src->value.len;

	int k;
	switch(t->dtype)
	{
		case Char:
			for(k = 0; k < src->value.len; k++)
				dst->value.ckey[k] = src->value.ckey[k];
			dst->value.ckey[src->value.len] = '\0';
			return;

		case Short:
			for(k = 0; k < src->value.len; k++)
				dst->value.skey[k] = src->value.skey[k];
			return;

		case Int:
			for(k = 0; k < src->value.len; k++)
				dst->value.ikey[k] = src->value.ikey[k];
			return;

		case Long:
			for(k = 0; k < src->value.len; k++)
				dst->value.lkey[k] = src->value.lkey[k];
			return;

		case Ulong:
			for(k = 0; k < src->value.len; k++)
				dst->value.ukey[k] = src->value.ukey[k];
			return;

		case Float:
			for(k = 0; k < src->value.len; k++)
				dst->value.fkey[k] = src->value.fkey[k];
			return;

		case Double:
			for(k = 0; k < src->value.len; k++)
				dst->value.dkey[k] = src->value.dkey[k];
			return;

		case Other:
			if(t->copy)
				t->copy(dst->value.key, src->value.key);
			else
			{
				magenta();
				printf("FATAL: NO COPY OPERATION DEFINED FOR BTREE.\n Exiting now...\n");
				exit(0);
			}
			return;
	}
}

bool ibtree_elt_set_capacity(btree_elt_a* e, int cap)
{
	if(!e || e->tree->item_size < 1 || cap <= e->nitems+e->capacity)
		return false;

	int item_size = e->tree->iitem_size;
	char* items = (char*)malloc(cap*item_size);
	if(!e->items && !e->nitems && !e->capacity)
	{
		e->items = items;
		e->capacity = cap;
		e->i = true;
		return true;
	}

	for(int k = 0; k < e->nitems; k++)
		ibtree_copy(e->tree, items+k*item_size, e->items+k*item_size);

	free(e->items);

	e->items = items;
	e->capacity = cap - e->nitems;

	return true;
}

long ibtree_item_pointer(const void* item)
{
	ibtree_item_a* i = (ibtree_item_a*)item;
	if(i)
		return i->pointer;
	else
		return 0;
}

void ibtree_item_set_pointer(void* item, long pointer)
{
	ibtree_item_a* i = (ibtree_item_a*)item;
	if(i)
		i->pointer = pointer;
}

char* iitem_at(btree_elt_a* e, int at)
{
	if(!e)
		return (char*)0;
	return e->items + at*e->tree->iitem_size;
}

bool ibtree_elt_create(btree_elt_a* src, btree_elt_a* dst, int start, int end)
{
	if(!src || !dst || start >= end || end > src->nitems || src->leaf)
		return false;

	btree_elt_init(src->tree, dst);
		
	if(start == 0)
		dst->first_child = src->first_child;
	else
		dst->first_child = ibtree_item_pointer(iitem_at(src, start));

	dst->last_child = ibtree_item_pointer(iitem_at(src, end-1));

	int real_start = start;
	if(start > 0)
		real_start = start + 1;

	ibtree_elt_set_capacity(dst, end-real_start+sizeof(int));
	for(int j = real_start, k = 0; j < end; j++, k++)
		ibtree_copy(src->tree, iitem_at(dst, k), iitem_at(src, j));

	dst->nitems = end-real_start;
	dst->capacity = sizeof(int);
	dst->level = src->level;

	return true;
}


ibtree_item_a* ibtree_item_create(const btree_a* t, key_a key, int value)
{
	if(!t || !key.key)
		return (ibtree_item_a*)0;

	ibtree_item_a* i = (ibtree_item_a*)malloc(t->iitem_size);
	i->pointer = 0;
	i->value.value = value;
	i->value.len = t->key_size;

	int k;
	switch(t->dtype)
	{
		case Char:
			i->value.len = strlen(key.ckey) < t->key_size-1 ? strlen(key.ckey) : t->key_size-2;
			for(k = 0; k < i->value.len; k++)
				i->value.ckey[k] = key.ckey[k];
			i->value.ckey[k+1] = '\0';
			return i;

		case Short:	
			for(k = 0; k < i->value.len; k++)
				i->value.skey[k] = key.skey[k];
			return i;

		case Int:
			for(k = 0; k < i->value.len; k++)
				i->value.ikey[k] = key.ikey[k];
			return i;

		case Long:
			for(k = 0; k < i->value.len; k++)
				i->value.lkey[k] = key.lkey[k];
			return i;

		case Ulong:
			for(k = 0; k < i->value.len; k++)
				i->value.ukey[k] = key.ukey[k];
			return i;

		case Float:
			for(k = 0; k < i->value.len; k++)
				i->value.fkey[k] = key.fkey[k];
			return i;

		case Double:
			for(k = 0; k < i->value.len; k++)
				i->value.dkey[k] = key.dkey[k];
			return i;

		case Other:
			if(t->copy)
				t->copy(i->value.key, key.key);
			else
			{
				magenta();
				printf("FATAL: NO COPY OPERATION DEFINED FOR BTREE.\n Exiting now...\n");
				exit(0);
			}
			return i;
	}
}

void ibtree_item_print(const btree_a* t, const void* item)
{
	if(!t || !item)
		return;

	ibtree_item_a* i = (ibtree_item_a*)item;
	printf("Child %ld ", i->pointer);
	btree_item_print(t, &i->value);
}

void ibtree_join_indices(btree_elt_a* left, btree_elt_a* right)
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

	ibtree_elt_set_capacity(left, left->nitems+right->nitems+3);
	
	btreeeltlocal btree_elt_a indexer;
	btree_elt_init(left->tree, &indexer);
	indexer.index = right->first_child;
	btree_elt_read(&indexer);

	while(indexer.first_child || !indexer.leaf)
	{
		indexer.index = indexer.first_child;
		btree_elt_read(&indexer);
	}

	//TODO MUSHKIL KIDOGO HAPA
	//INDEXER IS A LEAF SO USE BTREE_COPY
	btree_copy(left->tree, iitem_at(left, left->nitems)+offsetof(ibtree_item_a, value), indexer.items);
	ibtree_item_set_pointer(iitem_at(left, left->nitems), right->first_child);
	left->nitems++;

	for(int i = 0, k = left->nitems; i < right->nitems; i++, k++)
		ibtree_copy(left->tree, iitem_at(left, k), iitem_at(right, i));
	left->nitems += right->nitems;
}

int ibtree_compare(const btree_a* t, const void* item1, const void* item2)
{
	if(!t || !item1 || !item2)
	{
		printf("FATAL: CAN NOT DO A NULL COMPARISON.\n Exiting now ...\n");
		exit(0);
	}
	
	ibtree_item_a *ione = (ibtree_item_a*)item1,
								*itwo = (ibtree_item_a*)item2;
	return btree_compare(t, &ione->value, &itwo->value);
}

int ibtree_index_of(btree_elt_a* elt, const ibtree_item_a* item, bool* found)
{
	int beg = 0, end = elt->nitems, mid, cmp;
	while(beg < end)
	{
		mid = (beg+end) >> 1;
		cmp = ibtree_compare(elt->tree, item, iitem_at(elt, mid));

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

int ibtree_elt_insert(btree_elt_a* elt, ibtree_item_a* item)
{
	if(!elt || !item)
		return -1;

	bool found;
	int at = ibtree_index_of(elt, item, &found);
	if(found)
		return 1;
	
	if(!elt->capacity)
		ibtree_elt_set_capacity(elt, elt->nitems+sizeof(int));

	for(int i = elt->nitems; i > at; i--)
		ibtree_copy(elt->tree, iitem_at(elt, i), iitem_at(elt, i-1));

	ibtree_copy(elt->tree, iitem_at(elt, at), item);
	elt->capacity--;
	elt->nitems++;
	
	btree_elt_write(elt);
	
	return 0;
}

int ibtree_elt_remove(btree_elt_a* elt, ibtree_item_a* item)
{
	if(!elt || !item)
		return -1;

	bool found;
	int at = ibtree_index_of(elt, item, &found);
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
				return ibtree_elt_remove(&prev, item);
			}
		}
	}

	for(int i = at; i < elt->nitems-1; i++)
		ibtree_copy(elt->tree, iitem_at(elt, i), iitem_at(elt, i+1));

	elt->capacity++;
	elt->nitems--;

	btree_elt_write(elt);
	
	if(at == 0)
		return MOVE_UP;
	return 0;
}

int ibtree_elt_replace(btree_elt_a* elt, int at, ibtree_item_a* item)
{	
	if(at > 0 && at == elt->nitems-1 && elt->next)
	{
		if(ibtree_compare(elt->tree, iitem_at(elt, at), item) < 0)
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
			
			int cmp;
			if(n_first.leaf)
				cmp = btree_compare(elt->tree, item_at(&n_first, 0), &item->value);
			else
				cmp = ibtree_compare(elt->tree, iitem_at(&n_first, 0), item);

			if(cmp < 0)
			{
				printf("ERROR: SHOULD BE GREATER THAN ITEM\nExiting now ...\n");
				btree_elt_print(elt);
				ibtree_item_print(elt->tree, item);
				printf("Found AT node\n");
				next.index = item->value.value;
				btree_elt_read(&next);
				btree_elt_print(&next);
				btree_elt_print(&n_first);
				//btree_item_print(elt->tree, item_at(&n_first, 0));
				exit(EXIT_FAILURE);
			}
		}
	}

	ibtree_copy(elt->tree, iitem_at(elt, at), item);
	
	return 0;
}

//TODO Find some bugs in this
bool ibtree_move_up(ibtree_iter_a* it, ibtree_item_a* old, ibtree_item_a* new_item, int level, file_a* idx)
{
	if(!it || !old || !new_item || !idx)
		return false;

	bool found;
	int at;

#if false
	orange();
	ibtree_item_print(it->tree, old);
	ibtree_item_print(it->tree, new_item);
	colour_reset();
#endif
	for(int i = level; i <=it->nodes_len; i++)
	{
		ibtree_item_set_pointer(old, it->nodes[i-1].index);
		ibtree_item_set_pointer(new_item, it->nodes[i-1].index);

		at = ibtree_index_of(&it->nodes[i], old, &found);
		if(found)
		{
			ibtree_copy(it->tree, iitem_at(&it->nodes[i], at), new_item);
			ibtree_elt_write_mini(&it->nodes[i], idx);
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


static int ibtree_split_index(ibtree_iter_a* it, int level)
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
		ibtree_join_indices(node, rnode);
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
		ibtree_join_indices(node, rnode);
	}
	
	int split, at, ends_len = 0;
	bool found = false, move_up = false, split_parent = false;
	btree_ends_a ends[6];
	ibtree_item_a *ritem, *item = (ibtree_item_a*)alloca(it->tree->iitem_size);

	if(node->nitems > it->tree->max_iitems*1.75)
	{
		split = node->nitems/3;

		ibtree_elt_create(node, &left, 0, split);
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

		ibtree_elt_create(node, &mid, split, split*2);
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

		ibtree_elt_create(node, &right, split*2, node->nitems);
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

		ibtree_copy(it->tree, item, iitem_at(node, rnode_first));
		ibtree_item_set_pointer(item, rnode->index);

		at = ibtree_index_of(parent, item, &found);
		if(found)
		{
			ibtree_copy(it->tree, item, iitem_at(node, split*2));
			ibtree_item_set_pointer(item, right.index);
			ibtree_elt_replace(parent, at, item);

			ibtree_copy(it->tree, item, iitem_at(node, split));
			ibtree_item_set_pointer(item, mid.index);
			ibtree_elt_insert(parent, item);

			if(at == 0)
				move_up = true;
			if(parent->nitems == it->tree->max_iitems)
				split_parent = true;
		}
		else
		{
			printf("FATAL: TRISPLIT COULD NOT FIND AN INDEXING ELEMENT.\n Exiting now ...\n");
			printf("NODE NITEMS IS %d\n", rnode_first);
			ibtree_item_print(it->tree, item);
			btree_elt_print(node);
			btree_elt_print(rnode);
			exit(0);
		}
	}
	else if(node->nitems > it->tree->max_iitems)
	{
		split = node->nitems/2;

		ibtree_elt_create(node, &left, 0, split);
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

		ibtree_elt_create(node, &right, split, node->nitems);
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

		ibtree_copy(it->tree, item, iitem_at(node, rnode_first));
		ibtree_item_set_pointer(item, rnode->index);

		at = ibtree_index_of(parent, item, &found);
		if(found)
		{
			ibtree_copy(it->tree, item, iitem_at(node, split));
			ibtree_item_set_pointer(item, right.index);
			ibtree_elt_replace(parent, at, item);

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
			printf("THE MAXIMUM TREE HEIGHT IS REACHED.\nINCREASE MAXIMUM HEIGHT TO USE INDEXER\n");
			exit(0);
		}

		split = node->nitems/2;

		ibtree_elt_create(node, &left, 0, split);
		left.index = node->index;
		left.prev = 0;
		left.first = true;
		left.last = false;

		ends[0].node = left.last_child;
		ends[0].first = false;
		ends[0].last = true;

		ibtree_elt_create(node, &right, split, node->nitems);
		right.index= left.next = btree_new_index(it->tree);
		right.prev = left.index;
		right.next = 0;
		right.first = false;
		right.last = true;

		ends[1].node = right.first_child;
		ends[1].first = true;
		ends[1].last = false;
		ends_len = 2;

		ibtree_elt_create(node, &mid, split -1, split +1);
		ibtree_item_set_pointer(mid.items, right.index);
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
		ibtree_elt_create(node, &left, 0, node->nitems);
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

		ibtree_copy(it->tree, item, iitem_at(node, rnode_first));
		ibtree_item_set_pointer(item, rnode->index);

		at = ibtree_index_of(parent, item, &found);
		if(found)
		{
			if(!ibtree_elt_remove(parent, item))
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
	ibtree_elt_write_mini(&left, idx);
	ibtree_elt_write_mini(&mid, idx);
	ibtree_elt_write_mini(&right, idx);

	for(int i = 0; i < ends_len; i++)
		btree_ends_write(it->tree, ends[i], idx);

	if(move_up)
	{
		ritem = (ibtree_item_a*)alloca(it->tree->iitem_size);
		ibtree_copy(it->tree, ritem, rnode->items);
		ibtree_item_set_pointer(ritem, parent->index);
		ibtree_move_up(it, ritem, item, parent->level+1, idx);
	}

	if(split_parent)
	{
		file_destroy(&idx);
		btree_elt_destroy(&it->other);
		btree_elt_destroy(&it->pother);
		btree_elt_destroy(&left);
		btree_elt_destroy(&mid);
		btree_elt_destroy(&right);
		return ibtree_split_index(it, level+1);
	}

	if(parent)
		ibtree_elt_write_mini(parent, idx);

	btree_write_mini(it->tree, idx);
#if VERBOSE
	//if(!parent)
		btree_print(it->tree);
#endif
	file_destroy(&idx);

	return RECALL;
}

static int ibtree_split_leaf(ibtree_iter_a* it, int level)
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
	ibtree_item_a *ritem, *item = (ibtree_item_a*)alloca(it->tree->iitem_size);

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

		btree_copy(it->tree, &item->value, rnode->items);
		ibtree_item_set_pointer(item, rnode->index);

		at = ibtree_index_of(parent, item, &found);
		if(found)
		{
			btree_copy(it->tree, &item->value, right.items);
			ibtree_item_set_pointer(item, right.index);
			ibtree_elt_replace(parent, at, item);

			btree_copy(it->tree, &item->value, mid.items);
			ibtree_item_set_pointer(item, mid.index);
			ibtree_elt_insert(parent, item);

			if(at == 0 && it->tree->height > 1)
				move_up = true;
			if(parent->nitems == it->tree->max_iitems)
				split_parent = true;
		}
		else
		{
			btree_elt_print(parent);
			btree_elt_print(node);
			btree_elt_print(rnode);
			ibtree_item_print(node->tree, item);
			printf("The at variable got %d\n", at);
			printf("FATAL: COULD NOT FIND A LEAFY TRISPLIT INDEXING ELEMENT.\n Exiting now ...\n");
			abort();
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

		btree_copy(it->tree, &item->value, rnode->items);
		ibtree_item_set_pointer(item, rnode->index);

		at = ibtree_index_of(parent, item, &found);
		if(found)
		{
			btree_copy(it->tree, &item->value, right.items);
			ibtree_item_set_pointer(item, right.index);
			ibtree_elt_replace(parent, at, item);

			if(at == 0 && it->tree->height > 1)
				move_up = true;
		}
		else
		{
			btree_elt_print(parent);
			btree_elt_print(node);
			btree_elt_print(rnode);
			ibtree_item_print(node->tree, item);
			printf("The at variable got %d\n", at);
			printf("FATAL: COULD NOT FIND A LEAFY BISPLIT INDEXING ELEMENT.\n Exiting now ...\n");
			abort();
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

		//FIXME Everything is assigned manually so can't
		//be relied upon
		btree_copy(it->tree, &item->value, right.items);
		ibtree_item_set_pointer(item, right.index);
		mid.items = (char*)item;
		mid.nitems = 1;
		mid.tree = it->tree;
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

		btree_copy(it->tree, &item->value, rnode->items);
		ibtree_item_set_pointer(item, rnode->index);

		at = ibtree_index_of(parent, item, &found);
		if(found)
		{
			if(!ibtree_elt_remove(parent, item))
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
	if(!parent)	
		ibtree_elt_write_mini(&mid, idx);
	else
		btree_elt_write_mini(&mid, idx);
	
	btree_elt_write_mini(&right, idx);
	
	///FIXME WHICH COMES FIRST, MOVE UP OR WRITES
	if(move_up)
	{
		ritem = (ibtree_item_a*)alloca(it->tree->iitem_size);
		btree_copy(it->tree, &ritem->value, rnode->items);
		ibtree_item_set_pointer(ritem, parent->index);
		ibtree_move_up(it, ritem, item, parent->level+1, idx);
	}

	if(parent)
		ibtree_elt_write_mini(parent, idx);

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
		return ibtree_split_index(it, level+1);
	}

	return RECALL;
}

void ibtree_iter_destroy(ibtree_iter_a* iter)
{
	ibtree_iter_a it = *iter;
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

static bool ibtree_iter_init(ibtree_iter_a* it, btree_a* t, key_a key, long value)
{
	if(!it || !t || !key.key)
		return false;

	it->tree = t;
	it->other.i = it->pother.i = false;
	it->nodes_len = t->height;
	it->item = ibtree_item_create(t, key, value);
	it->i = true;

	int at, level = t->height, curr_index = t->root_id;
	bool found = false;
	it->node_parent_pos[level] = -2;

	while(level)
	{
		btree_elt_init(t, &it->nodes[level]);
		it->nodes[level].index = curr_index;
		btree_elt_read(&it->nodes[level]);

		at = ibtree_index_of(&it->nodes[level], it->item, &found);
		
		if(found)
		{
			it->node_parent_pos[level-1] = at;
			curr_index = ibtree_item_pointer(iitem_at(&it->nodes[level], at));
		}
		else
		{
			if(at)
			{
				it->node_parent_pos[level-1] = at-1;
				curr_index = ibtree_item_pointer(iitem_at(&it->nodes[level], at-1));
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
	btree_elt_init(t, &it->nodes[level]);
	it->nodes[level].index = curr_index;
	btree_elt_read(&it->nodes[level]);

	return true;
}

static int ibtree_iter_lookup(ibtree_iter_a* it)
{
	if(it->item->value.value == -1)
		return 0;

	btree_elt_a* curr = &it->nodes[0];
	if(!curr->leaf)
		return -1;
	
	bool found;
	int at = btree_index_of(curr, &it->item->value, &found);
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
		at = btree_index_of(curr, &it->item->value, &found);

		if(found)
			return 1;

		if(at)
		{
			if(curr->nitems < it->tree->max_items)
				return 0;
			return ibtree_split_leaf(it, 0);
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
		at = btree_index_of(&next, &it->item->value, &found);

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
				return ibtree_split_leaf(it, 0);
			}
		}
	}
}

static void ibtree_iter_print(ibtree_iter_a* it)
{
	if(!it)
		return;
	
	cyan();
	for(int i = 0; i < 50; i++)
		printf("$");
	printf("\n");
	
	printf("IITEM : ");
	ibtree_item_print(it->tree, it->item);

	for(int i = 0; i <= it->nodes_len; i++)
		btree_elt_print(&it->nodes[i]);
	
	cyan();
	for(int i = 0; i < 50; i++)
		printf("$");
	printf("\n");
	colour_reset();
}

#define ibtreeiterlocal __attribute__((cleanup(ibtree_iter_destroy)))


int ibtree_insert(btree_a* tree, key_a key, long value)
{
	ibtreeiterlocal ibtree_iter_a it;
	ibtree_iter_init(&it, tree, key, value);
	int lookup = ibtree_iter_lookup(&it);
	if(lookup)
		return lookup;

	btree_elt_a* curr = &it.nodes[0];
	if(curr->leaf)
	{
		if(curr->nitems == tree->max_items)
			return ibtree_split_leaf(&it, 0);

		return btree_elt_insert(curr, &it.item->value);
	}

	printf("FATAL: Cannot insert in a non leaf.\n Exiting now ...\n");
	exit(0);

	return -1;
}

int ibtree_remove(btree_a* tree, key_a key, long value)
{
	ibtreeiterlocal ibtree_iter_a it;
	ibtree_iter_init(&it, tree, key, value);

	btree_elt_a* curr = &it.nodes[0];
	if(curr->leaf)
	{
		if(curr->nitems < tree->max_items/3 && curr->index != tree->root_id)
			return ibtree_split_leaf(&it, 0);

		int ret = btree_elt_remove(curr, &it.item->value);
		if(ret == MOVE_UP)
		{
			ibtree_item_a* new_item = (ibtree_item_a*)alloca(tree->iitem_size);
			btree_copy(tree, &new_item->value, item_at(curr, 0));

			file_a* idx = file_create(tree->name.data, "rb+");
			if(!idx)
				exit(0);

			bool ans = ibtree_move_up(&it, it.item, new_item, 1, idx);
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

set_a* ibtree_search(btree_a* tree, key_a key, long value)
{
	ibtreeiterlocal ibtree_iter_a it;
	ibtree_iter_init(&it, tree, key, value);

	btree_elt_a* curr = &it.nodes[0];
	if(curr->leaf)
		return btree_elt_search(curr, &it.item->value);

	printf("FATAL: Cannot search from a non leaf.\n Exiting now ...\n");
	exit(0);

	return (set_a*)0;
}
