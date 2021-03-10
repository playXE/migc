#include "migc.h"
#include <stdio.h>
void free1(void *object)
{
    printf("Free %p\n", object);
}
void trace1(migc_visitor visitor, void *object)
{
    printf("Trace %p\n", object);
    migc_visitor_trace(visitor, ((void **)object)[2]);
}
void trace2(migc_visitor visitor, void *object)
{
    printf("Trace #2 %p\n", object);
}

migc_rtti rtti1 = {&free1, &trace1};
migc_rtti rtti2 = {NULL, &trace2};
int main()
{
    migc_heap heap;

    migc_heap_init(&heap, &heap, 1024);
    heap.verbose = 2;
    void **obj1 = migc_malloc(&heap, 128, &rtti1);
    obj1[2] = migc_malloc(&heap, 16, &rtti2);
    migc_collect(&heap);
}