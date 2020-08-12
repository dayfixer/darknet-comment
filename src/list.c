#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "utils.h"
#include "option_list.h"

list *make_list()
{
    list* l = (list*)xmalloc(sizeof(list));
    l->size = 0;
    l->front = 0;
    l->back = 0;
    return l;
}

/*
void transfer_node(list *s, list *d, node *n)
{
    node *prev, *next;
    prev = n->prev;
    next = n->next;
    if(prev) prev->next = next;
    if(next) next->prev = prev;
    --s->size;
    if(s->front == n) s->front = next;
    if(s->back == n) s->back = prev;
}
*/

void *list_pop(list *l){
    if(!l->back) return 0;
    node *b = l->back;
    void *val = b->val;
    l->back = b->prev;
    if(l->back) l->back->next = 0;
    free(b);
    --l->size;

    return val;
}

/*
**	将val指针插入链表l中，这里相当于是用C实现了C++中的list的元素插入功能
**	流程：	list中并不直接含有void*类型指针，但list中含有的node指针含有void*类型指针，
**		  因此，将首先创建一个node指针new，而后将val传给new，最后再把new插入list指针l中
**	说明： void*指针可以接收不同数据类型的指针，因此第二个参数具体是什么类型的指针得视情况而定
**	调用： 该函数在众多地方调用，很多从文件中读取信息存入到list变量中，都会调用此函数，
**		  注意此函数类似C++的insert()插入方式；而在option_list.h中的opion_insert()函数，
**		  有点类似C++ map数据结构中的按值插入方式，比如map[key]=value，两个函数操作对象都是list变量，
**		  只是操作方式略有不同。
*/
void list_insert(list *l, void *val)
{
    // 定义一个node指针并动态分配内存
    node* newnode = (node*)xmalloc(sizeof(node));
    // 将输入的val指针赋值给new中的val元素，注意，这是指针复制，共享地址，二者都是void*类型指针
    newnode->val = val;
    newnode->next = 0;

    if(!l->back){
        l->front = newnode;
        newnode->prev = 0;
    }else{
        l->back->next = newnode;
        newnode->prev = l->back;
    }
    l->back = newnode;
    ++l->size;
}

void free_node(node *n)
{
    node *next;
    while(n) {
        next = n->next;
        free(n);
        n = next;
    }
}

void free_list_val(list *l)
{
    node *n = l->front;
    node *next;
    while (n) {
        next = n->next;
        free(n->val);
        n = next;
    }
}

void free_list(list *l)
{
    free_node(l->front);
    free(l);
}

void free_list_contents(list *l)
{
    node *n = l->front;
    while(n){
        free(n->val);
        n = n->next;
    }
}

void free_list_contents_kvp(list *l)
{
    node *n = l->front;
    while (n) {
        kvp* p = (kvp*)n->val;
        free(p->key);
        free(n->val);
        n = n->next;
    }
}

void **list_to_array(list *l)
{
    void** a = (void**)xcalloc(l->size, sizeof(void*));
    int count = 0;
    node *n = l->front;
    while(n){
        a[count++] = n->val;
        n = n->next;
    }
    return a;
}
