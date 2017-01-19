
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "include/ev_hash.h"


hash_t  *global_cid_hash;

typedef struct hash_node 
{
        void *key;
        void *value;
        struct hash_node *prev;
        struct hash_node *next;
}hash_node_t;

struct hash 
{
        unsigned int buckets;   //  桶的个数
        hashfunc_t hash_func;
        hash_node_t **nodes;
};

// 得到桶号
hash_node_t ** hash_get_bucket(hash_t *hash, void *key)
{
        unsigned int bucket = hash->hash_func(hash->buckets, key);
        if (bucket >= hash->buckets)
        {
                fprintf(stderr, "bad bucket lookup\n");
                exit(EXIT_FAILURE);
        }
        return &(hash->nodes[bucket]);
}

hash_node_t * hash_get_node_by_key(hash_t *hash, void *key, unsigned int key_size)
{
        hash_node_t **bucket = hash_get_bucket(hash, key);
        hash_node_t *node = *bucket;
        if (node == NULL)
                return NULL;
        while (node != NULL && memcmp(node->key, key, key_size) != 0)
                node = node->next;
        return node;
}

hash_t *hash_alloc(unsigned int buckets, hashfunc_t hash_func)
{
        hash_t *hash = (hash_t *)malloc(sizeof(hash_t));
        
        assert(hash != NULL);
        hash->buckets = buckets; // 256
        hash->hash_func = hash_func;

        int size = buckets * sizeof(hash_node_t *);
        hash->nodes = (hash_node_t **)malloc(size);
        assert(hash->nodes != NULL);
        memset(hash->nodes, 0, size);

        return hash;
}

void * hash_lookup_entry(hash_t *hash, void *key, unsigned int key_size) 
{
        hash_node_t *node = hash_get_node_by_key(hash, key, key_size);

        if (node == NULL)
            return NULL;

        return node->value;
}
void hash_add_entry(hash_t *hash, void *key, unsigned int key_size, 
        void *value, unsigned int value_size)
{
        if (hash_lookup_entry(hash, key, key_size))
        {
                fprintf(stderr, "duplicate hash key");
                return ;
        }
        hash_node_t *node = (hash_node_t *)malloc(sizeof(hash_node_t));
        node->prev = NULL;
        node->next = NULL;

        node->key = malloc(key_size);
        memcpy(node->key, key, key_size);

        node->value = malloc(value_size);
        memcpy(node->value, value, value_size);

        hash_node_t **bucket = hash_get_bucket(hash, key);
        if (*bucket == NULL)
              *bucket = node;
        else 
        {
                // 头插法，将节点插入链表头部
                node->next = *bucket;
                (*bucket)->prev = node;
                *bucket = node;
        }
}

void call_back(void *value, unsigned int arg)
{
        printf("value= %d\n", *(int *)value);
}

void hash_foreach_entry(hash_t *hash, void call_back(void *value, void *arg), void *arg)
{
        unsigned int i;
        hash_node_t *node;

        for (i = 0; i < hash->buckets; i++)
        {
                if ( (node = hash->nodes[i]) == NULL)
                      continue;
                while (node)
                {
                      call_back(node->value, arg);
                      node = node->next;
                }
        }
}

void hash_free_entry(hash_t *hash, void *key, unsigned int key_size)
{
        hash_node_t *node = hash_get_node_by_key(hash, key, key_size);

        if (node == NULL)
                return ;
        free(node->key);
        free(node->value);

        // 判断是否头节点
        if (node->prev == NULL)
        {
                hash_node_t  **head = hash_get_bucket(hash, key);
                *head = node->next;
                free(node);
                return ;
        }
        // 判断是否尾节点
        if (node->next == NULL)
        {
                node->prev->next = NULL;
                free(node);
                return ;
        }
        // 中间节点
        node->prev->next = node->next;
        node->next->prev = node->prev;
        free(node);

        return ;
}

unsigned int hash_func(unsigned int buckets, void *key)
{
        unsigned int *number = (unsigned int *)key;

        return (*number) % buckets;
}


#if 0
static hash_t *s_ip_hash;
int main(int argc, char **argv)
{
        unsigned int key, value;

        s_ip_hash = hash_alloc(256, hash_func); 
        key = 0;
        value = 1000;
        hash_add_entry(s_ip_hash, &key, sizeof(key), &value, sizeof(value));
        key = 1;
        value = 1001;
        hash_add_entry(s_ip_hash, &key, sizeof(key), &value, sizeof(value));
        key = 256;
        value = 1002;
        hash_add_entry(s_ip_hash, &key, sizeof(key), &value, sizeof(value));
        key = 257;
        value = 1003;
        hash_add_entry(s_ip_hash, &key, sizeof(key), &value, sizeof(value));
        
        hash_foreach_entry(s_ip_hash, call_back, key);
        unsigned int *val;
        key = 1;
        val = hash_lookup_entry(s_ip_hash, &key, sizeof(key));
        printf("key = %d, value = %d\n", key, *val);
        key = 257;
        val = hash_lookup_entry(s_ip_hash, &key, sizeof(key));
        printf("key = %d, value = %d\n", key, *val);
        key = 0;
        val = hash_lookup_entry(s_ip_hash, &key, sizeof(key)); 
        printf("key = %d, value = %d\n", key, *val);
       
        *val = 1010;
        key = 0;
        val = hash_lookup_entry(s_ip_hash, &key, sizeof(key));
        printf("key = %d, value = %d\n", key, *val);
        
        key = 3;
        hash_free_entry(s_ip_hash, &key, sizeof(key));
        key = 3;
        val = hash_lookup_entry(s_ip_hash, &key, sizeof(key));
        if (val)
                printf("key = %d, value = %d\n", key, *val);
        else 
                printf("alread free ...\n");
}

#endif


