
#ifndef __EV_HASH__H
#define __EV_HASH__H

typedef struct hash  hash_t;
typedef unsigned int (*hashfunc_t)(unsigned int, void *);
extern hash_t *global_cid_hash;

hash_t *hash_alloc(unsigned int buckets, hashfunc_t hash_func);

void * hash_lookup_entry(hash_t *hash, void *key, unsigned int key_size);

void hash_add_entry(hash_t *hash, void *key, unsigned int key_size, void *value, unsigned int value_size);

void hash_free_entry(hash_t *hash, void *key, unsigned int key_size);

unsigned int hash_func(unsigned int buckets, void *key);

void hash_foreach_entry(hash_t *hash, void call_back(void *value, void *arg), void *arg);

#endif // ev_hash.h


