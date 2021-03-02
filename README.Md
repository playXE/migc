# migc
Small and simple library that implements conservative GC using mimalloc API. 

# Features
- Small and tiny. `libmigc.so` is just 20KB when linked with mimalloc. 
- Has API that is needed by most users of conservative GCs in C and can replace BDWGC in simple cases (e.g when precise marking is not needed)
    There is: `migc_add_roots` and `migc_delete_roots` to add or delete range of pointers to scan for potential pointers, `migc_register_finalizer` to invoke finalizers
    on dead objects. 
- Fast allocation. When there is multiple instances of migc heap running in multiple threads this library could get much much faster than BDWGC, in single threaded
  situations it should be pretty fast too since mimalloc is fast in single threaded apps too.
- Small overhead per allocation: 8 bytes. These 8 bytes is used to store mark,live bit and finalizer pointer (if exists).

# Building
```sh
git clone https://github.com/playxe/migc
cd migc
make submodules
make
```
This should produce `libmigc.so` which can be dynamically linked with your programs.


# Examples

```c
#include "migc.h"
// NOTE: current_stack_pointer is provided by migc.h

int main() {
  migc_heap heap;
  migc_heap_init(&heap,current_stack_pointer() /*stack start for conservative scanning */,1024/* GC threshold */);
  int* mptr = migc_malloc(&heap,sizeof(int)*4);
  keep_on_stack(mptr); // <- optional call, this forces compiler to put variable on stack rather than registers so GC can definitely see it
  mptr[0] = 42;
  printf("%i\n",mptr);
  migc_collect(&heap);
}


```
