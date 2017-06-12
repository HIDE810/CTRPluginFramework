#include "ctrulib/allocator/newlibHeap.h"

#include <malloc.h>    // for mallinfo() 
#include <unistd.h>    // for sbrk() 

extern u8 *fake_heap_end;   // current heap start 
extern u8 *fake_heap_start;   // current heap end 

u8* getHeapStart() {
    return fake_heap_start;
}

u8* getHeapEnd() {
    return (u8*)sbrk(0);
}

u8* getHeapLimit() {
    return fake_heap_end;
}

int getMemUsed() { // returns the amount of used memory in bytes 
    struct mallinfo mi = mallinfo();
    return mi.uordblks;
}

int getMemFree() { // returns the amount of free memory in bytes 
    struct mallinfo mi = mallinfo();
    return mi.fordblks + (getHeapLimit() - getHeapEnd());
}