#include "config.h"

static int item_compare(const void* a, const void* b)
{
	return strncmp(a,b,ITEM_MAXLEN);
}
