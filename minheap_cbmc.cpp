// minheap_cbmc.cpp
// Self-contained min-heap with CBMC assertions.
// Compile: g++ -std=c++11 minheap_cbmc.cpp -o minheap
// Model-check with: cbmc minheap_cbmc.cpp --bounds-check --pointer-check --memory-leak-check

#include <cassert>
#include <cstdio>

// CBMC nondet declaration (common pattern)
extern int nondet_int();
extern bool nondet_bool();

// CBMC assume macro (__CPROVER_assume available in CBMC)
#ifndef __CPROVER_assume
  #define __CPROVER_assume(x) if(!(x)) return 0;
#endif

const int MAXN = 8; // small fixed size keeps state-space manageable for model checkers

struct MinHeap {
    int a[MAXN];
    int sz;

    MinHeap(): sz(0) {}

    bool isEmpty() const { return sz == 0; }
    bool isFull() const { return sz >= MAXN; }

    // swap helper
    void swap_i(int i, int j) {
        assert(i >= 0 && i < MAXN);
        assert(j >= 0 && j < MAXN);
        int t = a[i]; a[i] = a[j]; a[j] = t;
    }

    // heapify up (restore min-heap by moving index i up)
    void heapifyUp(int i) {
        assert(i >= 0 && i < MAXN);
        while (i > 0) {
            int p = (i - 1) / 2;
            // bounds safe: p < i < MAXN
            assert(p >= 0 && p < MAXN);
            if (a[p] <= a[i]) break;
            swap_i(p, i);
            i = p;
        }
    }

    // heapify down (restore min-heap by moving index i down)
    void heapifyDown(int i) {
        assert(i >= 0 && i < MAXN);
        while (true) {
            int l = 2*i + 1;
            int r = 2*i + 2;
            int smallest = i;
            if (l < sz) {
                assert(l >= 0 && l < MAXN);
                if (a[l] < a[smallest]) smallest = l;
            }
            if (r < sz) {
                assert(r >= 0 && r < MAXN);
                if (a[r] < a[smallest]) smallest = r;
            }
            if (smallest == i) break;
            swap_i(i, smallest);
            i = smallest;
        }
    }

    // insert value; returns false if full
    bool insert(int v) {
        if (isFull()) return false;
        a[sz] = v;
        heapifyUp(sz);
        sz++;
        return true;
    }

    // peek min (undefined if empty)
    int peek() const {
        assert(!isEmpty());
        return a[0];
    }

    // extract min; caller should ensure not empty
    int extractMin() {
        assert(!isEmpty());
        int res = a[0];
        sz--;
        if (sz > 0) {
            a[0] = a[sz];
            heapifyDown(0);
        }
        return res;
    }

    // decrease key at index i to newVal (newVal <= a[i]); returns false if i invalid or newVal > a[i]
    bool decreaseKey(int i, int newVal) {
        if (i < 0 || i >= sz) return false;
        if (newVal > a[i]) return false;
        a[i] = newVal;
        heapifyUp(i);
        return true;
    }

    // verify heap property: parent <= children for all valid nodes
    bool verifyHeapProperty() const {
        for (int i = 0; i < sz; ++i) {
            int l = 2*i + 1;
            int r = 2*i + 2;
            if (l < sz && a[i] > a[l]) return false;
            if (r < sz && a[i] > a[r]) return false;
        }
        return true;
    }
};

int main() {
    MinHeap h;

    // nondeterministic number of elements to insert (bounded)
    int n = nondet_int();
    __CPROVER_assume(n >= 0 && n <= MAXN);

    // Insert n nondet values (bounded range to reduce state-space)
    for (int i = 0; i < n; ++i) {
        int val = nondet_int();
        // bound values to a small range to keep state-space manageable
        __CPROVER_assume(val >= -10 && val <= 10);
        bool ok = h.insert(val);
        // For these first n inserts, insertion must succeed because n <= MAXN
        assert(ok);
        // after each insert, heap property must hold
        assert(h.verifyHeapProperty());
    }

    // Optionally perform some nondet operations: extract/insert/decreaseKey
    // We'll perform up to 3 nondet operations to exercise transitions
    int ops = nondet_int();
    __CPROVER_assume(ops >= 0 && ops <= 3);
    for (int t = 0; t < ops; ++t) {
        bool doExtract = nondet_bool();
        if (doExtract) {
            if (!h.isEmpty()) {
                int minv = h.extractMin();
                // extracted value must be <= any remaining element (min property)
                for (int j = 0; j < h.sz; ++j) {
                    assert(minv <= h.a[j]);
                }
            }
        } else {
            // try insert with nondet value in bounded range
            int v = nondet_int();
            __CPROVER_assume(v >= -10 && v <= 10);
            if (!h.isFull()) {
                bool ok = h.insert(v);
                assert(ok);
            } else {
                // when full, insert should return false (we check the wrapper)
                bool ok = h.insert(v);
                assert(!ok); // should not succeed
            }
        }
        // verify heap property after each operation
        assert(h.verifyHeapProperty());
    }

    // Test decreaseKey on a nondet valid index (if heap non-empty)
    if (!h.isEmpty()) {
        int idx = nondet_int();
        __CPROVER_assume(idx >= 0 && idx < h.sz);
        int newVal = nondet_int();
        // force newVal <= current value to satisfy decreaseKey precondition
        __CPROVER_assume(newVal <= h.a[idx]);
        bool dec_ok = h.decreaseKey(idx, newVal);
        assert(dec_ok);
        assert(h.verifyHeapProperty());
        // additionally, the new value must be at or above heap root
        assert(h.a[0] <= h.a[idx]);
    }

    // Finally, extract all elements and check the extracted sequence is non-decreasing
    int extracted[MAXN];
    int m = 0;
    while (!h.isEmpty()) {
        int x = h.extractMin();
        extracted[m++] = x;
        // after each extract, heap property should hold (except when empty)
        if (h.sz > 0) assert(h.verifyHeapProperty());
    }
    // check non-decreasing
    for (int i = 1; i < m; ++i) {
        assert(extracted[i-1] <= extracted[i]);
    }

    // If reached here, all assertions hold for the explored nondet choices
    return 0;
}
