// minheap_cbmc.cpp
// Self-contained min-heap with CBMC assertions.
// Compile: g++ -std=c++11 minheap_cbmc.cpp -o minheap
// Model-check with: cbmc minheap_cbmc.cpp --bounds-check --pointer-check --memory-leak-check

#include <assert.h>
#include <stddef.h>

// CBMC nondet declaration (common pattern)
extern int nondet_int();
extern bool nondet_bool();

// CBMC assume macro (__CPROVER_assume available in CBMC)
#ifndef __CPROVER_assume
  #define __CPROVER_assume(x) if(!(x)) return 0;
#endif

// A simple min-heap implementation without STL dependencies

#define MAX_SIZE 10

typedef struct {
    int data[MAX_SIZE];
    int size;
} MinHeap;

void heap_init(MinHeap *h) {
    h->size = 0;
}

void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void heapify_up(MinHeap *h, int i) {
    if (i == 0) return;
    int parent = (i - 1) / 2;
    if (h->data[parent] > h->data[i]) {
        swap(&h->data[parent], &h->data[i]);
        heapify_up(h, parent);
    }
}

void heap_insert(MinHeap *h, int val) {
    assert(h->size < MAX_SIZE);
    h->data[h->size] = val;
    heapify_up(h, h->size);
    h->size++;
}

int heap_min(MinHeap *h) {
    assert(h->size > 0);
    return h->data[0];
}

// CBMC harness
int main() {
    MinHeap h;
    heap_init(&h);

    // nondet inputs (CBMC will explore all possibilities)
    int x, y, z;
    __CPROVER_assume(x >= 0 && x <= 100);
    __CPROVER_assume(y >= 0 && y <= 100);
    __CPROVER_assume(z >= 0 && z <= 100);

    heap_insert(&h, x);
    heap_insert(&h, y);
    heap_insert(&h, z);

    int m = heap_min(&h);

    // property: min should be <= all inserted values
    assert(m <= x);
    assert(m <= y);
    assert(m <= z);

    return 0;
}
