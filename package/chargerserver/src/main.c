
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define   JSON_OBJECT   1
#define   JSON_ARRAY    2

struct  Arr {
      char       vtype;
      char       *name;
      struct Arr *next;  
};

struct OBJ {
        char    *name;
        char    obj_flag;
        char    val_flag;
        char    *svalue;
        int     ivalue;
        struct OBJ  *next;
        struct OBJ  *obj_head;
        struct Arr   *arr_head;
        float fvalue;
};

struct JSON {
        char  type; // falg决定有数组还是对象
        struct ARR  *arr_head; 
        struct OBJ  *obj_head;
        struct JSON *next;
};
char *skip(const char *str)
{
        while (*str == ' ' || *str == '\r' || *str == '\n')
                str++;
        return str;
}
char *look_for(const char *str, char ch)
{
        while (*str++ != ch)
        return str;
}

struct OBJ *json_object(const char *json, int *offset)
{

}

struct JSON *json_step1(const char *json)
{
        char *str_head, *str_tail, *str;
        struct JSON  *json, json_head = NULL;
        char empty = 0, quotes = 0;
        char tmp[100] = {0};
        int offset;

        str_head = skip(json);
        if (*str_head != '[' && *str_head != '{') // 数组
            return NULL;
        str = skip(str_head + 1);
        if ( *str == '}' || *str == ']')
        {
             json = (struct JSON *)malloc(sizeof(struct JSON));
             if (*str == '}') // 对象
             {
                    json->type = JSON_OBJECT;
             } else {  // 数组
                    json->type = JSON_ARRAY;
             }
             return json;
        }
        
        if (*str_head == '{') // 对象
        {
               json = (struct JSON *)malloc(sizeof(struct JSON));
               if (json_head == NULL)
               {
                       json_head =json;
               } else
               {
                       json->next = json_head;
                       json_head = json;
               }
               json->type = JSON_OBJECT;
               json->obj_head = json_object(str_head, &offest);
               return json_head;
        } else {  // 数组
               while (str_head)
               {
                        json->type = JSON_ARRAY;
                        if ( (str_tail = look_for(str_head, ',')))
                        {
                                json = (struct JSON *)malloc(sizeof(struct JSON));
                                str = str_head;
                                while (str < str_tail)
                                {
                                        if ( *str == '\"')
                                        {
                                            quotes = 1;
                                            continue;
                                        }
                                        *ptmp++ = str++;
                                }
                                if (json_head == NULL)
                                {
                                        json_head =json;
                                } else
                                {
                                        json->next = json_head;
                                        json_head = json;
                                }
                                str = skip(str_tail + 1);
                                if (*str == '{')
                                {
                                         json_object(str, &offset);
                                        str_head += offset;
                                        continue;
                                }

                        } else
                        {
                        }
                       
               }

        }


        }
}

struct  Element
{
        char *name;
        int   flag;
        int   ival;
        char *sval;
        struct Element *next;
        struct Element *element_next;
        float fval;
};
struct Item
{
        int  cid;
        int  vol;
        int  cur;
        int  pow;
        struct Element *head;
};

struct Element *json_element(const char *json)
{
        char *element_head, *element_tail, *ptmp, *value;
        char *key, status = 0, return_status = 0;
        char tmp[100] = {0};
        struct Element  *object, *head = NULL;

        element_head = json;

        while(element_head)
        {
                // 递归
                if ( (element_tail = strchr(element_head, ':')) && *(element_tail + 1) == '{')
                {
                        object = (struct Element *)malloc(sizeof(struct Element));
                        ptmp = tmp;
                        key = element_head + 1;
                        while (key < element_tail)
                        {
                                if (*key == '\"')
                                {
                                        key++;
                                        continue;
                                }
                                *ptmp++ = *key++; 
                        }
                        *ptmp = 0;
//                        printf("tmp:%s\n", tmp);
                        object->name = (char *)malloc(strlen(tmp) + 1);
                        object->next = NULL;
                        strcpy(object->name, tmp);
                        if (head == NULL)
                        {
                                head = object;
                        } else
                        {
                                object->next = head;
                                 head = object;
                        }
//                        printf("%s\n", element_tail + 2);
                        object->element_next = json_element(element_tail + 2);
//                        printf("element_next->name:%s, value:%s\n", object->element_next->name, object->element_next->sval);
                        if ( (key = strchr(element_head, '}')))
                        {
                                if ( *(key + 1) == ',')
                                {
                                        element_head = key + 2;
                                } if ( *(key + 1) == '}')
                                {
                                        return head;
                                }
                        }
                        continue;
                }
                if ( !(element_tail = strchr(element_head, ','))) 
                {
                        if (( element_tail = strchr(element_head, '}')) && *(element_tail + 1) == ',')
                            return_status = 1;
                        if (( element_tail = strchr(element_head, '}')) && *(element_tail + 1) == ']')
                            return_status = 1;
                } else
                {
                   if ( *(--element_tail) == '}')
                    {
                         return_status = 1;
                    }
                }

                key = strchr(element_head, ':');
                if (*(key + 1) == '\"')
                     status = 1;
                key = element_head;
                ptmp = tmp;
//                printf("key:%s\n", key);
                while (key < element_tail)
                {
                     if (*key == '\"')
                     {
                            key++;
                            continue;
                     }
                     *ptmp++ = *key++; 
                }
                *ptmp = 0;
                // 处理
                key = strchr(tmp, ':');
                *key = 0;
                value = key + 1;
                key = tmp;
//                printf("key:%s, value:%s\n", key, value);
                object = (struct Element *)malloc(sizeof(struct Element));
                object->name = (char *)malloc(strlen(key) + 1);
                strcpy(object->name, key);
                object->flag = status;
                object->next = NULL;

                // 字符串
                if (status == 1)
                {
                        object->sval = (char *)malloc(strlen(value) + 1);
                        strcpy(object->sval, value);
                }
                if (head == NULL)
                {
                        head = object;
                } else
                {
                        object->next = head;
                        head = object;
                }
                if (return_status == 1)
                    return  head;
                element_head = element_tail + 2;
//                printf("end--->%s\n", element_head);
        }

}

int  json_item_example(struct Item  *item, char *json)
{
        char *item_head, *p2, *key, *value, *item_tail;
        char demilter1 = '{', demilter2 = ':', demilter3 = ',';
        int  i1, i2;
        char *str, *str1;

        str = (char *)malloc(strlen(json) + 1);
        str1 = str;

        while (*json)
        {
                if (*json == ' ' || *json == '\r')
                {
                        json++;
                        continue;
                }
                *str1++ = *json++;
        }
        *str1 = 0;
        if (*str == '{') // 对象
        {
        
        } else if (*str == '[') // 数组
        {
        
        }
        printf("str1=%s\n", str);

        if ( (item_head = strchr(str, demilter3)) && *(item_head + 1) == demilter1)
        {
                item_head += 2;
                item_tail = strchr(str, ']');
                item->head = json_element(item_head);
        }
        

}

int main(int argc, char **argv)
{

char *string = "[1,{\"people\":{\"firstName\":\"Brett\",\"lastName\":\"McLaughlin\",\"email\":\"aaaa\"}, \
            {\"firstName\":\"Jason\",\"lastName\":\"Hunter\",\"email\":\"bbbb\"},\
            {\"firstName\":\"Elliotte\",\"lastName\":\"Harold\",\"email\":\"cccc\"}}]";

        char *str = "[\"123\",{\"cid\":\"54001\",\"series\":{\"id\":\"2001\"},\"target\":{\"onee\":\"yes\",\"ip\":{\"addr\":\"192.168.6.1\"},\"\"two\":\"no\"},\"model\":\"EVG-32N\"}]";
        struct Item  item;
        struct Element *ele;
        struct Element *tmp;

        printf("string:%s\n\n\n", string);
        json_item_example(&item, string);
        ele = item.head;

        while (ele)
        {
               printf("name:%s, value:%s\n", ele->name, ele->sval);
               if (strcmp(ele->name, "series") == 0)
               {
                        printf("      name:%s, value:%s\n", ele->element_next->name, ele->element_next->sval);
               }
               if (strcmp(ele->name, "target") == 0)
               {
                        tmp = ele->element_next;
                        while(tmp)
                        {
                                printf("     name:%s, value:%s\n", tmp->name, tmp->sval);
                                tmp = tmp->next;
                        }
               }
              
               ele = ele->next;
        }

        return 0;
}



