
#ifndef  __EV_JANSSON_H
#define  __EV_JANSSON_H

#include  <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define         EV_JSON_STRING          0x03
#define         EV_JSON_INTEGER         0x04
#define         EV_JSON_DOUBLE          0x05
#define         EV_JSON_BOOL            0x06
#define         EV_JSON_NULL            0x07


json_t *ev_decode_jansson(const char *str, size_t flag);
const char *ev_get_cmd_from_jansson(json_t *root, int cmd_index);

struct json_value {
        int     type;
        json_t  *element;
        union 
        {
                bool            bdata;
                const char      *sdata;
                int             idata;
                double          ddata;
        } u;
};


// 从对象中找子对象
json_t *ev_get_subobject_from_object(json_t *object, const char *pk);

// 从数组中找对象
json_t *ev_get_object_from_array(const json_t *element);

// 从数组中找元素
json_t *ev_get_element_from_array(const json_t *element, size_t index);

// 从对象中找到数组, 可以遍历大的对象包含多个小对象
json_t *ev_get_array_from_object(json_t *object, const char *key);

// 没有实现递归
int ev_get_value_from_array(const json_t *array, size_t index, struct json_value *store_value);

int ev_get_value_from_object(const json_t *object, const char *key, struct json_value *store_value);

#endif 

