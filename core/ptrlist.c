#include "ptrlist.h"
#include <stdlib.h>

static struct node* new_node(void)
{
	return (struct node*)malloc(sizeof(struct node));
}

ptrlist_t list_new(void)
{
	ptrlist_t list;
	list.head=NULL;
	list.tail=NULL;
	return list;
}

void list_add_node(ptrlist_t* l, void* item)
{
	if(!l->head)
	{
		l->head=new_node();
		l->tail=l->head;
	}
	else l->tail->next=new_node();
	l->tail->item=item;
	l->tail->next=NULL;
}

static void free_node(struct node* n)
{
	if(n->next) free_node(n->next);
	free(n);
}

void list_free(ptrlist_t* l)
{
	if(l->head)
	{
		free_node(l->head);
	}
}
