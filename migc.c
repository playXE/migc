#include "migc.h"
#include <stdbool.h>
#include <setjmp.h>
void migc_heap_init(migc_heap *heap, void *sp, size_t initial_heap_size)
{
    heap->sp = sp;
    heap->roots = NULL;
    heap->allocated = 0;
    heap->max_heap_size = initial_heap_size;
    heap->mi_heap = mi_heap_new();
}

void migc_heap_destroy(migc_heap *heap)
{
    mi_heap_destroy(heap->mi_heap);
}
void migc_add_roots(migc_heap *heap, void *from, void *to)
{
    conservaive_roots *memory = malloc(sizeof(conservaive_roots));

    memory->next = NULL;
    memory->from = from;
    memory->to = to;
    memory->next = heap->roots;
    heap->roots = memory;
}

int migc_delete_roots(migc_heap *heap, void *from, void *to)
{
    conservaive_roots *prev = heap->roots;
    if (prev == NULL)
    {
        return -1;
    }
    conservaive_roots *current = prev->next;
    while (current != NULL)
    {
        if (current->from == from && current->to == to)
        {
            break;
        }
        prev = current;
        current = current->next;
    }
    if (current == NULL)
    {
        return 0;
    }
    prev->next = current->next;
    free(current);
    return 1;
}

static void mark_conservative(migc_heap *heap, void *from_, void *to_)
{
    void **from = from_;
    void **to = to_;
    if (heap->verbose)
        printf("-mark %p->%p (%zu words)\n", from, to, (size_t)(to - from));
    while (from < to)
    {
        void *ptr = *from;
        if (!ptr)
            goto end;
        void *real_ptr = ptr - sizeof(mi_gc_header);

        if (mi_heap_check_owned(heap->mi_heap, real_ptr))
            if (mi_heap_contains_block(heap->mi_heap, ptr))
            {

                mi_gc_header *hdr = real_ptr;
                if (hdr->live)
                {
                    if (hdr->mark == 0)
                    {
                        if (heap->verbose == 2)
                            printf("--mark  %p at %p\n", real_ptr, from);
                        hdr->mark = 1;
                        cvector_push_back(heap->mark_stack, real_ptr);
                    }
                }
            }
    end:
        from++;
    }
}

static bool migc_sweep_block(const mi_heap_t *heap, const mi_heap_area_t *area, void *block, size_t block_size, void *arg)
{
    if (block == NULL)
    {
        return 1;
    }
    migc_heap *theap = arg;
    mi_gc_header *hdr = (mi_gc_header *)block;
    if (hdr->mark)
    {
        hdr->mark = 0;
        theap->allocated += block_size;
    }
    else
    {
        if (hdr->finalizer_fn)
        {
            finalizer fin = (finalizer)hdr->finalizer_fn;
            (fin)(((void *)hdr) + sizeof(mi_gc_header));
        }
        hdr->mark = 0;
        hdr->live = 0;
        mi_free(block);
    }
    return 1;
}
static void __attribute__((noinline)) migc_collect_internal(migc_heap *heap)
{
    if (heap->verbose)
        printf("GC after %zu bytes allocated (max heap size %zu)\n", heap->allocated, heap->max_heap_size);
    jmp_buf buf;
    setjmp(&buf);
    void *sp = current_stack_pointer();
    heap->current_sp = sp;
    void *from = sp;
    void *to = heap->sp;
    if (from > to)
    {
        void *tmp = from;
        from = to;
        to = tmp;
    }
    heap->mark_stack = NULL;
    mark_conservative(heap, from, to);
    mark_conservative(heap, &buf, (void **)&buf + (sizeof(jmp_buf) / sizeof(void *) - 1));

    conservaive_roots *root = heap->roots;
    while (root)
    {
        mark_conservative(heap, root->from, root->to);
        root = root->next;
    }

    while (cvector_size(heap->mark_stack))
    {
        mi_gc_header *hdr = cvector_begin(heap->mark_stack)[cvector_size(heap->mark_stack) - 1];
        cvector_pop_back(heap->mark_stack);
        void *from = ((void *)hdr) + 8;
        void *to = ((void *)hdr) + mi_usable_size(hdr);
        mark_conservative(heap, from, to);
    }
    heap->allocated = 0;
    mi_heap_visit_blocks(heap->mi_heap, 1, migc_sweep_block, heap);

    if (heap->allocated > heap->max_heap_size)
    {
        size_t prev = heap->max_heap_size;
        heap->max_heap_size = (size_t)((double)heap->allocated * 1.5);
        if (heap->verbose)
            printf("Increased GC threshold %zu->%zu", prev, heap->max_heap_size);
    }
    if (heap->verbose)
    {
        printf("GC cycle finished with %.3fKB memory in use\n", (double)(heap->allocated) / 1024.0);
    }
    cvector_free(heap->mark_stack);
}
int migc_collect_if_necessary(migc_heap *heap)
{
    if (heap->allocated > heap->max_heap_size)
    {

        migc_collect_internal(heap);
        return 1;
    }
    return 0;
}

void migc_collect(migc_heap *heap)
{
    migc_collect_internal(heap);
}
void migc_free(migc_heap *heap, void *ptr)
{
    mi_gc_header *hdr = ptr - 8;
    hdr->live = 0;
    if (hdr->finalizer_fn)
    {
        finalizer fin = (finalizer)(size_t)hdr->finalizer_fn;
        (fin)(ptr);
    }
    mi_free(hdr);
}

void *migc_realloc(migc_heap *heap, void *pointer, size_t newSize)
{
    if (newSize)
    {
        migc_collect_if_necessary(heap);
        size_t real_size = sizeof(mi_gc_header) + mi_usable_size(newSize);
        void *newHdr = mi_heap_realloc(heap->mi_heap, pointer, real_size);
        return newHdr + 8;
    }
    else
    {
        migc_free(heap, pointer);
        return NULL;
    }
}
void *migc_malloc(migc_heap *heap, size_t size)
{
    migc_collect_if_necessary(heap);
    mi_gc_header *hdr;
    size_t real_size = mi_good_size(size + sizeof(mi_gc_header));
    if (real_size <= MI_SMALL_SIZE_MAX)
    {
        hdr = mi_heap_malloc_small(heap->mi_heap, real_size);
    }
    else
    {
        hdr = mi_heap_malloc_aligned(heap->mi_heap, real_size, 16);
    }
    heap->allocated += real_size;
    hdr->live = 1;
    hdr->mark = 0;
    hdr->finalizer_fn = 0;
    return ((void *)hdr) + sizeof(mi_gc_header);
}

void migc_register_finalizer(void *ptr, finalizer fin)
{
    ((mi_gc_header *)(ptr - 8))->finalizer_fn = (uint64_t)(size_t)fin;
}
#include <stdio.h>

static volatile void *SINK = 0;
void __keep_on_stack(void *var)
{
    SINK = var;
}
