#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "memory_study.h"

// Example of padding in a struct
typedef struct  {
    char    a;      // 1 byte
    int     b;      // 4 bytes (padding likely after 'a')
    char    c;      // 1 byte (more padding may follow)
} PaddingExample;

// rodata section (read-only data)
const char* message = "Hello, rodata!";

// data section (initialized global variable)
int global_initialized = 0x2A;

// bss section (uninitialized global variable)
int global_uninitialized;

// text section (function code)
static void print_addresses() {
    int local_var = 0x0A;             // stack (local variable)
    static int static_var = 0x63;     // data section (static local variable)

    printf("=== Memory Segment Addresses ===\n");
    printf("Function (text):           %p\n", (void*)print_addresses);
    printf("Const string (rodata):     %p\n", (void*)message);
    printf("Initialized global (data): %p\n", (void*)&global_initialized);
    printf("Uninitialized global (bss):%p\n", (void*)&global_uninitialized);
    printf("Static local (data):       %p\n", (void*)&static_var);
    printf("Local variable (stack):    %p\n", (void*)&local_var);
}

static void pass_by_pointer(int* array, PaddingExample copy_ex, PaddingExample* ptr_ex) 
{
    for (int i = 0; i < 5; i++) {
        printf("array[%d] = 0x%x\n", i, array[i]);
    }

    for (int i = 0; i < 5; i++) {
        array[i] += 0x10;
    }

    printf("Address of copy_ex.a:   %p\n", (void*)&copy_ex.a);
    printf("Address of copy_ex.b:   %p\n", (void*)&copy_ex.b);
    printf("Address of copy_ex.c:   %p\n", (void*)&copy_ex.c);

    printf("Address of ptr_ex->a:   %p\n", (void*)&ptr_ex->a);
    printf("Address of ptr_ex->b:   %p\n", (void*)&ptr_ex->b);
    printf("Address of ptr_ex->c:   %p\n", (void*)&ptr_ex->c);

    ptr_ex->a = 'A';
    ptr_ex->b = 0xFFFF;
    ptr_ex->c = 'Z';

    return;
};
int memmory_study() {
    print_addresses();

    static const PaddingExample const_ex = {'X', 123, 'Y'};
    PaddingExample auto_ex = {'X', 123, 'Y'};

    printf("\n=== Padding and Pointers ===\n");
    printf("Size of struct: %zu bytes\n", sizeof(PaddingExample));
    printf("Address of const_ex.a:   %p\n", (void*)&const_ex.a);
    printf("Address of const_ex.b:   %p\n", (void*)&const_ex.b);
    printf("Address of const_ex.c:   %p\n", (void*)&const_ex.c);

    printf("Address of auto_ex.a:   %p\n", (void*)&auto_ex.a);
    printf("Address of auto_ex.b:   %p\n", (void*)&auto_ex.b);
    printf("Address of auto_ex.c:   %p\n", (void*)&auto_ex.c);

    // Pointer and heap allocation example
    uint32_t* ptr = malloc(sizeof(uint32_t));
    *ptr = 0x11223344;
    printf("\nHeap memory:    %p → Value: B:0x%x, W:0x%x, DW:0x%x\n", 
        (void*)ptr, *(uint8_t*)ptr, *(uint16_t*)ptr, *(uint32_t*)ptr);
    free(ptr);

    static int array[5] = {0x1, 0x2, 0x3, 0x4, 0x5};
    printf("\nArray base address: %p\n", (void*)array);

//  pass_by_pointer(array, (PaddingExample)const_ex, (PaddingExample*)&const_ex);
    pass_by_pointer(array, (PaddingExample)auto_ex, (PaddingExample*)&auto_ex);

    for (int i = 0; i < 5; i++) {
        printf("array[%d] = 0x%x\n", i, array[i]);
    }


//  printf("After function call, const_ex.a = %c, const_ex.b = 0x%x, const_ex.c = %c\n", const_ex.a, const_ex.b, const_ex.c);
    printf("After function call, auto_ex.a = %c, auto_ex.b = 0x%x, auto_ex.c = %c\n", auto_ex.a, auto_ex.b, auto_ex.c);

    return 0;
}
