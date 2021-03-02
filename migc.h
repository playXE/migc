#pragma once
#ifndef CVECTOR_H_
#define CVECTOR_H_

#include <assert.h> /* for assert */
#include <stdlib.h> /* for malloc/realloc/free */

/**
 * @brief cvector_vector_type - The vector type used in this library
 */
#define cvector_vector_type(type) type *

/**
 * @brief cvector_set_capacity - For internal use, sets the capacity variable of the vector
 * @param vec - the vector
 * @param size - the new capacity to set
 * @return void
 */
#define cvector_set_capacity(vec, size)     \
    do                                      \
    {                                       \
        if (vec)                            \
        {                                   \
            ((size_t *)(vec))[-1] = (size); \
        }                                   \
    } while (0)

/**
 * @brief cvector_set_size - For internal use, sets the size variable of the vector
 * @param vec - the vector
 * @param size - the new capacity to set
 * @return void
 */
#define cvector_set_size(vec, size)         \
    do                                      \
    {                                       \
        if (vec)                            \
        {                                   \
            ((size_t *)(vec))[-2] = (size); \
        }                                   \
    } while (0)

/**
 * @brief cvector_capacity - gets the current capacity of the vector
 * @param vec - the vector
 * @return the capacity as a size_t
 */
#define cvector_capacity(vec) \
    ((vec) ? ((size_t *)(vec))[-1] : (size_t)0)

/**
 * @brief cvector_size - gets the current size of the vector
 * @param vec - the vector
 * @return the size as a size_t
 */
#define cvector_size(vec) \
    ((vec) ? ((size_t *)(vec))[-2] : (size_t)0)

/**
 * @brief cvector_empty - returns non-zero if the vector is empty
 * @param vec - the vector
 * @return non-zero if empty, zero if non-empty
 */
#define cvector_empty(vec) \
    (cvector_size(vec) == 0)

/**
 * @brief cvector_grow - For internal use, ensures that the vector is at least <count> elements big
 * @param vec - the vector
 * @param count - the new capacity to set
 * @return void
 */
#define cvector_grow(vec, count)                                              \
    do                                                                        \
    {                                                                         \
        const size_t cv_sz = (count) * sizeof(*(vec)) + (sizeof(size_t) * 2); \
        if (!(vec))                                                           \
        {                                                                     \
            size_t *cv_p = malloc(cv_sz);                                     \
            assert(cv_p);                                                     \
            (vec) = (void *)(&cv_p[2]);                                       \
            cvector_set_capacity((vec), (count));                             \
            cvector_set_size((vec), 0);                                       \
        }                                                                     \
        else                                                                  \
        {                                                                     \
            size_t *cv_p1 = &((size_t *)(vec))[-2];                           \
            size_t *cv_p2 = realloc(cv_p1, (cv_sz));                          \
            assert(cv_p2);                                                    \
            (vec) = (void *)(&cv_p2[2]);                                      \
            cvector_set_capacity((vec), (count));                             \
        }                                                                     \
    } while (0)

/**
 * @brief cvector_pop_back - removes the last element from the vector
 * @param vec - the vector
 * @return void
 */
#define cvector_pop_back(vec)                           \
    do                                                  \
    {                                                   \
        cvector_set_size((vec), cvector_size(vec) - 1); \
    } while (0)

/**
 * @brief cvector_erase - removes the element at index i from the vector
 * @param vec - the vector
 * @param i - index of element to remove
 * @return void
 */
#define cvector_erase(vec, i)                                \
    do                                                       \
    {                                                        \
        if (vec)                                             \
        {                                                    \
            const size_t cv_sz = cvector_size(vec);          \
            if ((i) < cv_sz)                                 \
            {                                                \
                cvector_set_size((vec), cv_sz - 1);          \
                size_t cv_x;                                 \
                for (cv_x = (i); cv_x < (cv_sz - 1); ++cv_x) \
                {                                            \
                    (vec)[cv_x] = (vec)[cv_x + 1];           \
                }                                            \
            }                                                \
        }                                                    \
    } while (0)

/**
 * @brief cvector_free - frees all memory associated with the vector
 * @param vec - the vector
 * @return void
 */
#define cvector_free(vec)                        \
    do                                           \
    {                                            \
        if (vec)                                 \
        {                                        \
            size_t *p1 = &((size_t *)(vec))[-2]; \
            free(p1);                            \
        }                                        \
    } while (0)

/**
 * @brief cvector_begin - returns an iterator to first element of the vector
 * @param vec - the vector
 * @return a pointer to the first element (or NULL)
 */
#define cvector_begin(vec) \
    (vec)

/**
 * @brief cvector_end - returns an iterator to one past the last element of the vector
 * @param vec - the vector
 * @return a pointer to one past the last element (or NULL)
 */
#define cvector_end(vec) \
    ((vec) ? &((vec)[cvector_size(vec)]) : NULL)

/* user request to use logarithmic growth algorithm */
#ifdef CVECTOR_LOGARITHMIC_GROWTH

/**
 * @brief cvector_push_back - adds an element to the end of the vector
 * @param vec - the vector
 * @param value - the value to add
 * @return void
 */
#define cvector_push_back(vec, value)                               \
    do                                                              \
    {                                                               \
        size_t cv_cap = cvector_capacity(vec);                      \
        if (cv_cap <= cvector_size(vec))                            \
        {                                                           \
            cvector_grow((vec), !cv_cap ? cv_cap + 1 : cv_cap * 2); \
        }                                                           \
        vec[cvector_size(vec)] = (value);                           \
        cvector_set_size((vec), cvector_size(vec) + 1);             \
    } while (0)

#else

/**
 * @brief cvector_push_back - adds an element to the end of the vector
 * @param vec - the vector
 * @param value - the value to add
 * @return void
 */
#define cvector_push_back(vec, value)                   \
    do                                                  \
    {                                                   \
        size_t cv_cap = cvector_capacity(vec);          \
        if (cv_cap <= cvector_size(vec))                \
        {                                               \
            cvector_grow((vec), cv_cap + 1);            \
        }                                               \
        vec[cvector_size(vec)] = (value);               \
        cvector_set_size((vec), cvector_size(vec) + 1); \
    } while (0)

#endif /* CVECTOR_LOGARITHMIC_GROWTH */

/**
 * @brief cvector_copy - copy a vector
 * @param from - the original vector
 * @param to - destination to which the function copy to
 * @return void
 */
#define cvector_copy(from, to)                          \
    do                                                  \
    {                                                   \
        for (size_t i = 0; i < cvector_size(from); i++) \
        {                                               \
            cvector_push_back(to, from[i]);             \
        }                                               \
    } while (0)

#endif /* CVECTOR_H_ */

#include "mimalloc/include/mimalloc.h"
#include <stdint.h>
#include <stddef.h>

typedef struct conservative_roots_s
{
    struct conservative_roots_s *next;

    void *from;
    void *to;
} conservaive_roots;
typedef struct mi_gc_header_s
{
    uint64_t live : 1;
    uint64_t mark : 1;
    uint64_t finalizer_fn : 56;
} mi_gc_header;

typedef struct migc_heap_s
{
    int verbose;
    /// Allocated bytes in this GC heap
    size_t allocated;
    /// Maximum bytes to allocate until GC cycle. This value is dynamically changed after each GC cycle if needed.
    size_t max_heap_size;
    /// mimalloc heap that is used for allocation inside this GC heap.
    mi_heap_t *mi_heap;

    conservaive_roots *roots;
    cvector_vector_type(mi_gc_header *) mark_stack;
    void *sp;
    void *current_sp;

} migc_heap;

/// Frees `ptr` and invokes finalizer if it exists.
void migc_free(migc_heap *heap, void *ptr);
/// Allocate `size` bytes in GC heap.
void *migc_malloc(migc_heap *heap, size_t size);
/// Reallocate `pointer` to `newSize` bytes.
void *migc_realloc(migc_heap *heap, void *pointer, size_t newSize);
/// Initialize MIGC heap. `sp` should point to any on-stack variable for scanning for roots and `initial_heap_size` is GC threshold.
void migc_heap_init(migc_heap *heap, void *sp, size_t initial_heap_size);
/// Destroy MIGC heap.
void migc_heap_destroy(migc_heap *heap);
/// Collect garbage.
void migc_collect(migc_heap *heap);
/// Collect garbage if necessary.
int migc_collect_if_necessary(migc_heap *heap);
/// Add pointer range for scanning for roots.
void migc_add_roots(migc_heap *heap, void *from, void *to);
/// Delete pointer range from potential roots.
int migc_delete_roots(migc_heap *heap, void *from, void *to);

typedef void (*finalizer)(void *addr);
/// Register `finalizer` function for `ptr`. If `ptr` is not alive in next GC cycle then its finalizer will be
/// invoked, this is highly useful when you have to free memory from some external heap or just close IO handles.
///
/// NOTE: FInalization order is not guaranteed, no GC allocation or GC cycles should be performed inside finalizer.
///
void migc_register_finalizer(void *ptr, finalizer fin);

/// Function that can be used to obtain current stack pointer. Internally it does not use any inline assembly but just returns
/// pointer to local variable.
void *__attribute__((noinline)) current_stack_pointer()
{
    void *sp = (void *)&sp;
    return sp;
}
void __keep_on_stack(void *addr);
#define keep_on_stack(var) __keep_on_stack(&var);