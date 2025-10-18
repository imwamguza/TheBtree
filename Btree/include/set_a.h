#ifndef SET_A_H
#define SET_A_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"{
#endif

struct set_a;
typedef struct set_a set_a;

set_a* set_create();

void set_destroy(set_a** s);

#define setlocal __attribute__((cleanup(set_destroy)))

int set_size(const set_a* s);

int set_capacity(const set_a* s);

long set_at(const set_a* s, int at);

void set_set_capacity(set_a* s, int cap);

int set_insert(set_a* s, const long data);

int set_remove(set_a* s, const long data);

bool set_union(set_a* s, const set_a* other);

bool set_intersection(set_a* s, const set_a* other);

bool set_difference(set_a* s, const set_a* other);

#ifdef __cplusplus
}
#endif

#endif //SET_A_H
