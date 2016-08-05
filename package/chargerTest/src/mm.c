
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
int num;
void *show_pthread(void *arg)
{
//    pthread_detach(pthread_self());
    printf("pid = %d, tid = %d, num = %d\n", getpid(), pthread_self(), num++);

    usleep(1);
}

int 
main(void)
{
    pthread_t   tid;
    pthread_attr_t  attr;

    pthread_attr_init(&attr);
    

//    while(1)
//    {
//        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
//        pthread_create(&tid, NULL, show_pthread, NULL);
//        usleep(1);
//    }


    return 0;
}

#define     JSON_MAX    (300 * 24)

char    *json_to_string(void)
{
    // 获取数据库表
    static  char    json_str[JSON_MAX];
    char            tab_name[8][10] = {0};
    char            tab_info[8][300] = {0};
    char            tmp[20] = {0};
    char            *str;
    int             i, j, cnt;
    if ( (str = find_uci_tables(TAB_POS)) == NULL)
        return NULL;

    // 提取各表名称
    i = 0; j = 0; cnt = 0;
    while(*str)
    {
        if (*str == ',')
        {
            i++;
            cnt++;
            j = 0;
            str++;
            continue;
        }
        tab_name[i][j++] = *str++;
    }
    free(str);
    cnt += 1;
    //根据表查找
    for (i = 0; i < cnt; i++)
    {
        if (ev_uci_data_get_val(tmp, 20, "chargerinfo.%s.CID", tab_name[i]) < 0)
            sprintf(tab_info[i], "chargerID=failure");
        sprintf(tab_info[i], "chargerID=%s&", tmp);

        if (ev_uci_data_get_val(tmp, 20, "chargerinfo.%s.MAC", tab_name[i]) < 0)
            sprintf(tab_info[i] + strlen(tab_info[i]), "macaddr=failure");
        sprintf(tab_info[i] + strlen(tab_info[i]), "macaddr=%s&", tmp);
        
        if (ev_uci_data_get_val(tmp, 20, "chargerinfo.%s.cycleID", tab_name[i]) < 0)
            sprintf(tab_info[i] + strlen(tab_info[i]), "cycleID=failure");
        sprintf(tab_info[i] + strlen(tab_info[i]), "cycleID=%s&", tmp);
        
        if (ev_uci_data_get_val(tmp, 20, "chargerinfo.%s.charging_type", tab_name[i]) < 0)
            sprintf(tab_info[i] + strlen(tab_info[i]), "charging_type=failure");
        sprintf(tab_info[i] + strlen(tab_info[i]), "charging_type=%s&", tmp);
        
        if (ev_uci_data_get_val(tmp, 20, "chargerinfo.%s.Module", tab_name[i]) < 0)
            sprintf(tab_info[i] + strlen(tab_info[i]), "Module=failure");
        sprintf(tab_info[i] + strlen(tab_info[i]), "Module=%s&", tmp);

        if (ev_uci_data_get_val(tmp, 20, "chargerinfo.%s.privateID", tab_name[i]) < 0)
            sprintf(tab_info[i] + strlen(tab_info[i]), "privateID=failure");
        sprintf(tab_info[i] + strlen(tab_info[i]), "privateID=%s&", tmp);
        
        if (ev_uci_data_get_val(tmp, 20, "chargerinfo.%s.power", tab_name[i]) < 0)
            sprintf(tab_info[i] + strlen(tab_info[i]), "power=failure");
        sprintf(tab_info[i] + strlen(tab_info[i]), "power=%s&", tmp);
        
        if (ev_uci_data_get_val(tmp, 20, "chargerinfo.%s.start_time", tab_name[i]) < 0)
            sprintf(tab_info[i] + strlen(tab_info[i]), "start_time=failure");
        sprintf(tab_info[i] + strlen(tab_info[i]), "start_time=%s&", tmp);
        
        if (ev_uci_data_get_val(tmp, 20, "chargerinfo.%s.end_time", tab_name[i]) < 0)
            sprintf(tab_info[i] + strlen(tab_info[i]), "end_time=failure");
        sprintf(tab_info[i] + strlen(tab_info[i]), "end_time=%s&", tmp);

        if (ev_uci_data_get_val(tmp, 20, "chargerinfo.%s.status", tab_name[i]) < 0)
            sprintf(tab_info[i] + strlen(tab_info[i]), "status=failure");
        sprintf(tab_info[i] + strlen(tab_info[i]), "status=%s&", tmp);
       
        if (ev_uci_data_get_val(tmp, 20, "chargerinfo.%s.charging_recorde", tab_name[i]) < 0)
            sprintf(tab_info[i] + strlen(tab_info[i]), "charging_recorde=failure");
        sprintf(tab_info[i] + strlen(tab_info[i]), "charging_recode=%s&", tmp);
    }
    // 拼接
    memset(json_str, 0, sizeof(json_str));
    for (i = 0; i < cnt; i++)
    {
        strcat(json_str + strlen(json_str), tab_info[i]);
    }
    json_str[strlen(json_str) - 1] = '\0';
    return json_str;
}


