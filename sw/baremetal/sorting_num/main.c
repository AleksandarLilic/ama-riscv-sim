#include "common.h"

#if defined(LEN_TINY)
#include "test_arrays_tiny.h"
#elif defined(LEN_SMALL)
#include "test_arrays_small.h"
#elif defined(LEN_MEDIUM)
#include "test_arrays_medium.h"
#elif defined(LEN_LARGE)
#include "test_arrays_large.h"
#else
_Static_assert(0, "No array length defined or unsupported length specified");
#endif

#ifndef LOOPS
#define LOOPS 1u
#endif

void swap(NF_IN* a, NF_IN* b) {
    NF_IN t = *a;
    *a = *b;
    *b = t;
}

#if defined(ALG_BUBBLE)
#include <stdbool.h>
#define FUNC_NAME bubble_sort
void FUNC_NAME(NF_IN a[], uint32_t n) {
    bool swapped;
    for (uint32_t i = 0; i < n-1; i++) {
        swapped = false;
        // last i elements are already in place
        for (uint32_t j = 0; j < n-i-1; j++) {
            if (a[j] > a[j+1]) {
                swap(&a[j], &a[j+1]);
                swapped = true;
            }
        }
        if (!swapped) break;
    }
}

#elif defined(ALG_INSERTION)
#define FUNC_NAME insertion_sort
void FUNC_NAME(NF_IN a[], uint32_t n) {
    NF_IN key;
    uint32_t i;
    int32_t j;
    for (i = 1; i < n; i++) {
        key = a[i];
        j = i - 1;
        while (j >= 0 && a[j] > key) {
            a[j+1] = a[j];
            j--;
        }
        a[j+1] = key;
    }
}

#elif defined(ALG_SELECTION)
#define FUNC_NAME selection_sort
void FUNC_NAME(NF_IN arr[], uint32_t n) {
    uint32_t i, j, min_idx;
    for (i = 0; i < n-1; i++) {
        min_idx = i;
        for (j = i+1; j < n; j++) {
            if (arr[j] < arr[min_idx]) min_idx = j;
        }
        swap(&arr[min_idx], &arr[i]);
    }
}

#elif defined(ALG_MERGE)
#define FUNC_NAME mergesort
void merge(NF_IN arr[], uint32_t l, uint32_t m, uint32_t r) {
    uint32_t i, j, k;
    uint32_t n1 = m - l + 1;
    uint32_t n2 = r - m;
    NF_IN L[n1], R[n2]; // temp arrays

    // copy data to temp arrays L[] and R[]
    for (i = 0; i < n1; i++) L[i] = arr[l + i];
    for (j = 0; j < n2; j++) R[j] = arr[m + 1 + j];

    // merge the temp arrays back into arr[l..r]
    i = 0; // initial index of first subarray
    j = 0; // initial index of second subarray
    k = l; // initial index to start merging
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    // copy the remaining elements of L[], if any
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    // copy the remaining elements of R[], if any
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void FUNC_NAME(NF_IN arr[], uint32_t l, uint32_t r) {
    if (l < r) {
        // find the middle point
        uint32_t m = l + (r - l) / 2;
        // sort first and second halves
        FUNC_NAME(arr, l, m);
        FUNC_NAME(arr, m + 1, r);
        // merge the sorted halves
        merge(arr, l, m, r);
    }
}

#elif defined(ALG_QUICK)
#define FUNC_NAME quicksort
int32_t partition(NF_IN arr[], int32_t low, int32_t high) {
    NF_IN pivot = arr[high];
    int32_t i = (low - 1); // index of smaller element

    for (int32_t j = low; j <= high - 1; j++) {
        // if current element is smaller than or equal to pivot
        if (arr[j] <= pivot) {
            i++; // increment index of smaller element
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

void FUNC_NAME(NF_IN arr[], int32_t low, int32_t high) {
    if (low < high) {
        // partitioning index, arr[pi] is now at right place
        int32_t pi = partition(arr, low, high);

        // separately sort elements before partition and after partition
        FUNC_NAME(arr, low, pi - 1);
        FUNC_NAME(arr, pi + 1, high);
    }
}

#elif defined(ALG_HEAP)
#define FUNC_NAME heapsort
// heapify a subtree rooted with node i which is an index in arr[]
void heapify(NF_IN arr[], int32_t n, int32_t i) {
    int32_t largest = i; // initialize largest as root
    int32_t left = 2 * i + 1;
    int32_t right = 2 * i + 2;

    // if left child is larger than root
    if (left < n && arr[left] > arr[largest]) largest = left;

    // if right child is larger than largest so far
    if (right < n && arr[right] > arr[largest]) largest = right;

    // if largest is not root
    if (largest != i) {
        swap(&arr[i], &arr[largest]);
        // recursively heapify the affected sub-tree
        heapify(arr, n, largest);
    }
}

void FUNC_NAME(NF_IN arr[], uint32_t n) {
    // build heap (rearrange array)
    for (int32_t i = n / 2 - 1; i >= 0; i--) heapify(arr, n, i);

    // one by one extract an element from heap
    for (int32_t i = n - 1; i > 0; i--) {
        // move current root to end
        swap(&arr[0], &arr[i]);
        // call max heapify on the reduced heap
        heapify(arr, i, 0);
    }
}

#elif defined(ALG_QUICKER)
#define FUNC_NAME quickersort
#include <stdlib.h>

int compare(const void *p, const void *q) {
    NF_IN x = *(const NF_IN *)p;
    NF_IN y = *(const NF_IN *)q;

    if (x < y) return -1; // return -1 for ascending, 1 for descending
    else if (x > y) return 1; // return 1 for ascending, -1 for descending
    return 0;
}

void FUNC_NAME(NF_IN arr[], uint32_t n) {
    qsort(arr, n, sizeof(NF_IN), compare);
}

#else
_Static_assert(0, "No algorithm defined");
#endif

void main(void) {
    NF_IN work_a [ARR_LEN];
    for (uint32_t i = 0; i < LOOPS; i++) {
        // copy the test array to work array
        for (uint32_t j = 0; j < ARR_LEN; j++) work_a[j] = a[j];
        uint32_t n = sizeof(work_a)/sizeof(work_a[0]);

        PROF_START;
        #if defined(ALG_MERGE) || defined(ALG_QUICK)
        FUNC_NAME(work_a, 0, n-1);
        #else
        FUNC_NAME(work_a, n);
        #endif
        PROF_STOP;

        asm(".global check");
        asm("check:");
        for (uint32_t j = 0; j < ARR_LEN; j++) {
            if (work_a[j] != ref[j]) {
                write_mismatch(a[j], ref[j], j+1); // +1 to avoid writing 0
                fail();
            }
        }
    }
    pass();
}
