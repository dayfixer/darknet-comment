#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "option_list.h"
#include "utils.h"
#include "data.h"

/*
**  读取数据集配置文件（obj.data文件），包含数据集所在的路径、名称，其中包含的物体类别数等等
**  如下(位于cfg/xxx.data):
**
**    classes= 80
**    train  = /home/pjreddie/data/coco/trainvalno5k.txt
**    #valid  = coco_testdev
**    valid = data/coco_val_5k.list
**    names = data/coco.names
**    backup = /home/pjreddie/backup/
**    eval=coco
** 
**  返回：list指针，包含所有数据信息。函数中会创建options变量，并返回其指针（若文件打开失败，将直接退出程序，不会返空指针）
*/
list *read_data_cfg(char *filename)
{
    FILE *file = fopen(filename, "r");
    if(file == 0) file_error(filename);
    char *line;
    int nu = 0;
    list *options = make_list();
    while((line=fgetl(file)) != 0){
        ++nu;
        // 删除line中的空白符
        strip(line);  // jump
        switch(line[0]){
            // 以下面三种字符开头的都是无效行，直接跳过（如注释等）
            case '\0':
            case '#':
            case ';':
                free(line);
                break;
            default:
                if(!read_option(line, options)){
                    fprintf(stderr, "Config file error line %d, could parse: %s\n", nu, line);
                    free(line);
                }
                break;
        }
    }
    fclose(file);
    return options;
}

metadata get_metadata(char *file)
{
    metadata m = { 0 };
    list *options = read_data_cfg(file);

    char *name_list = option_find_str(options, "names", 0);
    if (!name_list) name_list = option_find_str(options, "labels", 0);
    if (!name_list) {
        fprintf(stderr, "No names or labels found\n");
    }
    else {
        m.names = get_labels(name_list);
    }
    m.classes = option_find_int(options, "classes", 2);
    free_list(options);
    if(name_list) {
        printf("Loaded - names_list: %s, classes = %d \n", name_list, m.classes);
    }
    return m;
}

/*
**  解析一行数据的内容，为options赋值，主要调用option_insert()
**  输入：s        从文件读入的某行字符数组指针
**       options    实际输出，解析出的数据将为该变量赋值
**  返回：int类型数据，1表示成功读取有效数据，0表示未能读取有效数据（说明文件中数据格式有问题）
**  流程：从配置（.data或者.cfg，不管是数据配置文件还是神经网络结构数据文件，其读取都需要调用这个函数）
**      文件中读入的每行数据包括两部分，第一部分为变量名称，如learning_rate，
**      第二部分为值，如0.01，两部分由=隔开，因此，要分别读入两部分的值，首先识别出
**      等号，获取等号所在的指针，并将等号替换为terminating null-characteristic '\0'，
**      这样第一部分会自动识别到'\0'停止，而第二部分则从等号下一个地方开始
*/
int read_option(char *s, list *options)
{
    size_t i;
    size_t len = strlen(s);
    char *val = 0;
    for(i = 0; i < len; ++i){
        if(s[i] == '='){
            s[i] = '\0';
            val = s+i+1;
            break;
        }
    }
    if(i == len-1) return 0;
    char *key = s;
    option_insert(options, key, val);  // jump
    return 1;
}

/*
**  将输入key和val赋值给kvp结构体对象，最终调用list_insert()将kvp赋值给list对象l，
**  完成最后的赋值（此函数之后，文件中某行的数据真正读入进list变量）
**  说明： 这个函数有点类似C++中按键值插入元素值的功能
**  输入：l        输出，最终被赋值的list变量
**       key      变量的名称，C风格字符数组
**       value    变量的值，C风格字符数组（还未转换成float或者double数据类型）
*/
void option_insert(list *l, char *key, char *val)
{
    kvp* p = (kvp*)xmalloc(sizeof(kvp));
    p->key = key;
    p->val = val;
    p->used = 0;
    list_insert(l, p);  // jump
}

void option_unused(list *l)
{
    node *n = l->front;
    while(n){
        kvp *p = (kvp *)n->val;
        if(!p->used){
            fprintf(stderr, "Unused field: '%s = %s'\n", p->key, p->val);
        }
        n = n->next;
    }
}

/*
**  在l中查找并返回指定键的值（返回类型统一为C风格字符数组指针，若l中没有该键，则返回0指针）
**  输入：l        链表
**       key      指定键（C风格字符数组指针）
**  返回：char*    l中对应键值key的值（C风格字符数组指针）
**  说明：该函数实现了类似C++中的map数据按键值查找的功能；返回值统一为C风格字符数组，可以进一步转为int,float等
*/
char *option_find(list *l, char *key)
{
    // 获取l中的第一个节点
    // （一个节点的值包含一条信息：键与值
    // 比如: classes=80，键classes，值80，表示该数据集共包含80类物体）
    node *n = l->front;

    // 遍历链表l
    while(n){
        kvp *p = (kvp *)n->val;
        // 比较字符串
        if(strcmp(p->key, key) == 0){
            p->used = 1;
            return p->val;
        }
        n = n->next;
    }
    return 0;
}

// 查找数据集配置文件中的字符串
char *option_find_str(list *l, char *key, char *def)
{
    char *v = option_find(l, key);
    if(v) return v;
    if(def) fprintf(stderr, "%s: Using default '%s'\n", key, def);
    return def;
}

char *option_find_str_quiet(list *l, char *key, char *def)
{
    char *v = option_find(l, key);
    if (v) return v;
    return def;
}

/*
**  按给定键值key从l中查找对应的参数值，主要调用option_find()函数。
**  功能和上一个函数option_find_str()基本一样，只不过多了一步处理，将输出转为了int之后再输出。
**  输入：l    list指针，实际为section结构体中的options元素，包含该层神经网络的所有配置参数
**       key  键值，即参数名称，比如卷积核尺寸size，卷积核个数filters，跨度stride等等
**       def  默认值，如果没有找到对应键值的参数值，则使用def作为默认值
**  输出：int类型（该函数专门用来识别整数数值），即参数值，比如filters的值为96，size的值为11等
*/
int option_find_int(list *l, char *key, int def)
{
    char *v = option_find(l, key);  // 跳转-->option_find()
    if(v) return atoi(v);
    fprintf(stderr, "%s: Using default '%d'\n", key, def);
    return def;
}

int option_find_int_quiet(list *l, char *key, int def)
{
    char *v = option_find(l, key);
    if(v) return atoi(v);
    return def;
}

float option_find_float_quiet(list *l, char *key, float def)
{
    char *v = option_find(l, key);
    if(v) return atof(v);
    return def;
}

float option_find_float(list *l, char *key, float def)
{
    char *v = option_find(l, key);
    if(v) return atof(v);
    fprintf(stderr, "%s: Using default '%lf'\n", key, def);
    return def;
}
