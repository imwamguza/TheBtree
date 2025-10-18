#include <malloc.h>
#include <stdbool.h>

#include <set_a.h>

struct set_a{
	int size, capacity;
	long* set;
};

typedef struct set_a set_a;

set_a* set_create()
{
	set_a* s = (set_a*)malloc(sizeof(set_a));
	s->size = s->capacity = 0;
	s->set = (long*)0;
	return s;
}

void set_destroy(set_a** s)
{
	set_a* st = *s;
	if(st)
	{
		if(st->set)
			free(st->set);

		free(st);
		st = (set_a*)0;
	}
}

int set_size(const set_a* s)
{
	if(!s)
		return -1;
	return s->size;
}

int set_capacity(const set_a* s)
{
	if(!s)
		return -1;
	return s->capacity;
}

long set_at(const set_a* s, int at)
{
	return s->set[at];
}

void set_set_capacity(set_a* s, int cap)
{
	if(!s)
		return;
		
	if(cap < s->size + s->capacity)
		return;

	long* set = (long*)malloc(cap*sizeof(long));
	if(!s->set && !s->size && !s->capacity)
	{
		s->set = set;
		s->capacity = cap;
		return;
	}

	for(int k = 0; k < s->size; k++)
		set[k] = s->set[k];

	free(s->set);
	s->set = set;
	s->capacity = cap - s->size;
}

static int set_index_of(const set_a* s, const long data, bool* found)
{
	if(!s->set || !s->size)
	{
		*found = false;
		return 0;
	}

	int beg = 0, end = s->size, mid, cmp;

	while(beg < end)
	{
		mid = (beg + end) >> 1;
		cmp = data - s->set[mid];

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

int set_insert(set_a* s, const long data)
{
	if(!s)
		return -1;

	bool found = false;
	int at = set_index_of(s, data, &found);
	if(found)
		return 1;

	if(!s->capacity)
	{
		if(s->size)
			set_set_capacity(s, s->size*2);
		else
			set_set_capacity(s, sizeof(void*));
	}

	for(int i = s->size; i > at; i--)
		s->set[i] = s->set[i-1];
	
	s->set[at] = data;
	s->capacity--;
	s->size++;

	return 0;
}

int set_remove(set_a* s, const long data)
{
	if(!s)
		return -1;

	bool found = false;
	int at = set_index_of(s, data, &found);
	if(!found)
		return 1;

	for(int i = at; i < s->size; i++)
		s->set[i] = s->set[i+1];

	s->capacity++;
	s->size--;

	return 0;
}

bool set_union(set_a* ret, const set_a* other)
{
	if(!ret || !other || !other->set || !other->size)
		return false;

	for(int i = 0; i < other->size; i++)
		set_insert(ret, other->set[i]);

	return true;
}

bool set_intersection(set_a* s, const set_a* other)
{
	if(!s || !other || !other->set || !other->size)
		return false;
	
	if(!s->size && !s->set)
		return set_union(s, other);

	bool found = false;
	for(int i = 0; i < s->size; i++)
	{
		set_index_of(other, s->set[i], &found);
		if(!found)
			set_remove(s, s->set[i]);
	}

	return true;
}

bool set_difference(set_a* s, const set_a* other)
{
	if(!s || !other || !other->set || !other->size)
		return false;

	if(!s->size && !s->set)
		return set_union(s, other);

	for(int i = 0; i < other->size; i++)
		set_remove(s, other->set[i]);

	return true;
}
