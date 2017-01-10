
#include  <jansson.h>
#include "include/ev_jansson_charger.h"

static int ev_get_value(json_t *element, const char *key, struct json_value *store_value);

// json字符串解析
json_t *ev_decode_jansson(const char *str, size_t flag)
{
        json_t  *root;
        json_error_t   error;

        root = json_loads(str, flag, &error);
        if ( !root)
                fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
        return root;
}

const char *ev_get_cmd_from_jansson(json_t *root, int cmd_index)
{
        return (json_string_value(json_array_get(root, cmd_index)));
}

// 从对象中找子对象
json_t *ev_get_subobject_from_object(json_t *object, const char *pk)
{
        const char *key;
        json_t *value, *tvalue;

       if ( !json_is_object(object))
                return (json_t *)0;
        json_object_foreach(object, key, value){
                if ( !strcmp(key, pk) && json_is_object(value))
                      return value;
                // 递归
                if ( json_is_object(value))
                { 
                        tvalue = ev_get_subobject_from_object(value, pk);
                        if (tvalue)
                              return tvalue;
                }
        }
        return (json_t *)0;
}

// 从数组中找对象
json_t *ev_get_object_from_array(const json_t *element)
{
        size_t size, i;
        json_t *value;

        size = json_array_size(element);
        for ( i = 0; i < size; i++)
        {
                value = json_array_get(element, i);
                if ( json_is_object(value) )
                    return value;
        }

        return (json_t *)0;
}
// 从数组中找元素
json_t *ev_get_element_from_array(const json_t *element, size_t index)
{
        return json_array_get(element, index);
}

// 从对象中找到数组, 可以遍历大的对象包含多个小对象
json_t *ev_get_array_from_object(json_t *object, const char *key)
{
        json_t *value, *tvalue;
        char    *str, *str1;
        const char    *key1;

        if ( !(str = (char *)malloc(strlen(key) + 1)) )
            return (json_t *)0;
        strcpy(str, key);
        if ( (str1 = strchr(str, '.')) )
             *str1++ = 0;
//        printf("val1:%s, val2:%s\n", str, str1);
        json_object_foreach(object, key1, value){
                if ( !strncmp( key1, str, strlen(str)) && json_is_array(value))
                {
                        free(str);
                        return value;
                }
                if ( !strncmp(key1, str, strlen(str)) && json_is_object(value))
                {
                         tvalue = ev_get_array_from_object(value, str1);
                         free(str);
                         return tvalue;
                }
        }
        free(str);
        return (json_t *)0;
}

// 没有实现递归
int ev_get_value_from_array(const json_t *array, size_t index, struct json_value *store_value)
{
        json_t *element;

        if ( !json_is_array(array))
                goto error;
        if ( !(element = ev_get_element_from_array(array, index)) )
                goto error;
        if ( json_is_object(element) || json_is_array(element) )
                goto error;
        return ev_get_value(element, NULL, store_value);

error:
       store_value = NULL;
       return -1;

}

int ev_get_value_from_object(const json_t *object, const char *key, struct json_value *store_value)
{
        json_t *value;
        char    *str, *str1;
        
        if ( !(str = (char *)malloc(strlen(key) + 1)) ){
            store_value = NULL;
            return -1;
        }
        strcpy(str, key);
        if ( (str1 = strchr(str, '.')) )
             *str1++ = 0;
        if ( !json_is_object(object))
             goto error;
        if ( !(value = json_object_get(object, str)) )
            goto error;
        if ( json_is_object(value) )
        {
                if ( ev_get_value(value, str1, store_value) < 0)
                    goto error;
        }
        else
        {
                if ( ev_get_value(value, str, store_value) < 0)
                    goto error;
        }
        free(str);
        return 0;
error:
        free(str);
        store_value = NULL;
        return -1;
}


static int ev_get_value(json_t *element, const char *key, struct json_value *store_value)
{

       if        ( json_is_object(element))
       {
               return  ev_get_value_from_object(element, key, store_value);
       } else if (json_is_array(element))
       {
       
       } else if ( json_is_string(element))
       {
                store_value->u.sdata = json_string_value(element);
                store_value->type = EV_JSON_STRING;
                store_value->element = element;
                return 0;

       } else if (json_is_integer(element))
       {
                store_value->u.idata = json_integer_value(element);
                store_value->type = EV_JSON_INTEGER;
                store_value->element = element;
                return 0;
       } else if (json_is_real(element) )
       {
                store_value->u.ddata = json_real_value(element);
                store_value->type = EV_JSON_DOUBLE; 
                store_value->element = element;
                return 0;
       } else if (json_is_true(element))
       {
                store_value->u.bdata = true;
                store_value->type = EV_JSON_BOOL;
                store_value->element = element;
                return 0;
       } else if (json_is_false(element))
       {
                store_value->u.bdata = false;
                store_value->type = EV_JSON_BOOL;
                store_value->element = element;
                return 0;
       } else if (json_is_null(element))
       {
                store_value->type = EV_JSON_NULL;
                store_value->element = element;
                return 0;
       }
       fprintf(stderr, "json error: %s\n", "there is no json-type");
       return -1;
}


#if 0
int main(int argc, char **argv)
{

        json_t *ROOT, *root, *object, *sub_object;
        json_auto_t *array;
        struct json_value   value;
        char    *str = "[\"name\",1, \"age\",24, {\"id\":\"10011\", \"idtag\":{\"name1\":\"12345\", \"people\":[1,2,3,4,5]}}]";
        char buff[20] = "idtag.name1";
        char  buf[10] = {'t', 'e', 's', 't'};
        
        root = json_pack("[ssi]", "foo","bar",  4);
        printf("%s\n", json_dumps(root, 0));
        printf("%s\n", json_dumps(root, 0));
       // 测试对象插入
       root = json_object();
       json_t *string = json_string("1");
       json_object_set_new(root, "num1", string);
               string = json_string("2");
       json_object_set_new(root, "num2", string);
       json_t *integer = json_integer(3);
       json_object_set_new(root, "num3", integer);
              array = json_array();
              json_t    *jstr1;
              json_t    *jstr2;
              json_t    *jstr3;
              json_array_append_new(array, (jstr1 = json_string("guiyang")));
              json_array_append_new(array, jstr2 = json_string("shenzhen"));
              json_array_append_new(array, jstr3 = json_string("cengong"));
       printf("array:%s\n", ROOT = json_dumps(array, 0));
       json_decref(jstr1);
       json_decref(jstr1);
       json_decref(jstr1);
       json_decref(jstr2);
       json_decref(jstr3);
       json_decref(array);
       printf("Root:%s\n", ROOT);
//       free(jstr);
       printf("aaa\n");
//       free(array);json_auto_t
       json_object_set_new(root, "citys", array);
       printf("ROOT:%s\n\n\n", ROOT = json_dumps(root, 0));
       free(ROOT);
        root = ev_decode_jansson(str, 0);
        printf("%s\n", str);
        if (root)
        {
                 object = ev_get_object_from_array(root);
                 if ( !object)
                        return -1;
                 printf("found [object] is in array\n");
                 if (!(sub_object = ev_get_subobject_from_object(object, "idtag")) )
                        return -1;
                 printf("found [sub object] is object\n");
                if ( !(array = ev_get_array_from_object(object, "idtag.people")) )
                        return -1;
                printf("found array is in object\n");
                if ( (ev_get_value_from_array(array, 2, &value)) < 0)
                        return -1;
                printf("get value from array-->value:%d\n", value.u.idata);
                if ( (ev_get_value_from_array(root, 1, &value)) < 0)
                        return -1;
                if (ev_get_value_from_object(object, "idtag.name1", &value) < 0)
                      return -1;
                printf("get value from object\n");
                printf("value:%s\n", value.u.sdata);
                 
        }
        return 0;
}
#endif
