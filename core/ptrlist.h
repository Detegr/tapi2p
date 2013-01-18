#ifndef TAPI2P_PTRLIST_H
/* Very simple linkedlist to store pointers */

struct node
{
	void* item;
	struct node* next;
};

typedef struct list
{
	struct node* head;
	struct node* tail;
} ptrlist_t;

ptrlist_t list_new(void);
void list_add_node(ptrlist_t* l, void* item);
void list_free(ptrlist_t* l);
#endif
