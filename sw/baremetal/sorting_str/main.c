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

extern int strcmp(const char* s1, const char* s2);

void swap(char** a, char** b) {
    char* t = *a;
    *a = *b;
    *b = t;
}

#if defined(ALG_BUBBLE)
#define FUNC_NAME bubble_sort
#include <stdbool.h>
void FUNC_NAME(char* a[], uint32_t n) {
    bool swapped;
    for (uint32_t i = 0; i < n - 1; i++) {
        swapped = false;
        // last i elements are already in place
        for (uint32_t j = 0; j < n - i - 1; j++) {
            if (strcmp(a[j], a[j + 1]) > 0) {
                swap(&a[j], &a[j + 1]);
                swapped = true;
            }
        }
        if (!swapped) break;
    }
}

#elif defined(ALG_INSERTION)
#define FUNC_NAME insertion_sort
void FUNC_NAME(char* a[], uint32_t n) {
    char* key;
    uint32_t i;
    int32_t j;
    for (i = 1; i < n; i++) {
        key = a[i];
        j = i - 1;
        while (j >= 0 && strcmp(a[j], key) > 0) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

#elif defined(ALG_SELECTION)
#define FUNC_NAME selection_sort
void FUNC_NAME(char* arr[], uint32_t n) {
    uint32_t i, j, min_idx;
    for (i = 0; i < n - 1; i++) {
        min_idx = i;
        for (j = i + 1; j < n; j++) {
            if (strcmp(arr[j], arr[min_idx]) < 0) {
                min_idx = j;
            }
        }
        swap(&arr[min_idx], &arr[i]);
    }
}

#elif defined(ALG_MERGE)
#define FUNC_NAME mergesort
void merge(char* arr[], uint32_t l, uint32_t m, uint32_t r) {
    uint32_t i, j, k;
    uint32_t n1 = m - l + 1;
    uint32_t n2 = r - m;
    char* L[n1], * R[n2]; // temp arrays

    // copy data to temp arrays L[] and R[]
    for (i = 0; i < n1; i++) L[i] = arr[l + i];
    for (j = 0; j < n2; j++) R[j] = arr[m + 1 + j];

    // merge the temp arrays back into arr[l..r]
    i = 0; // initial index of first subarray
    j = 0; // initial index of second subarray
    k = l; // initial index to start merging
    while (i < n1 && j < n2) {
        if (strcmp(L[i], R[j]) <= 0) {
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

void FUNC_NAME(char* arr[], uint32_t l, uint32_t r) {
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
int32_t partition(char* arr[], int32_t low, int32_t high) {
    char* pivot = arr[high];
    int32_t i = (low - 1); // index of smaller element

    for (int32_t j = low; j <= high - 1; j++) {
        // if current element is smaller than or equal to pivot
        if (strcmp(arr[j], pivot) <= 0) {
            i++; // increment index of smaller element
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

void FUNC_NAME(char* arr[], int32_t low, int32_t high) {
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
void heapify(char* arr[], int32_t n, int32_t i) {
    int32_t largest = i; // initialize largest as root
    int32_t left = 2 * i + 1;
    int32_t right = 2 * i + 2;

    // if left child is larger than root
    if (left < n && strcmp(arr[left], arr[largest]) > 0) largest = left;

    // if right child is larger than largest so far
    if (right < n && strcmp(arr[right], arr[largest]) > 0) largest = right;

    // if largest is not root
    if (largest != i) {
        swap(&arr[i], &arr[largest]);
        // recursively heapify the affected sub-tree
        heapify(arr, n, largest);
    }
}

void FUNC_NAME(char* arr[], uint32_t n) {
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
    const char *x = *(const char **)p;
    const char *y = *(const char **)q;

    return strcmp(x, y); // strcmp handles ascending/descending
}

void FUNC_NAME(char* arr[], uint32_t n) {
    qsort(arr, n, sizeof(char*), compare);
}

#else
_Static_assert(0, "No algorithm defined");
#endif

void main(void) {
    char* work_a [ARR_LEN];
    for (uint32_t i = 0; i < LOOPS; i++) {
        // copy the test array to work array
        for (uint32_t j = 0; j < ARR_LEN; j++) work_a[j] = a[j];
        uint32_t n = sizeof(work_a)/sizeof(work_a[0]);

        LOG_START;
        #if defined(ALG_MERGE) || defined(ALG_QUICK)
        FUNC_NAME(work_a, 0, n-1);
        #else
        FUNC_NAME(work_a, n);
        #endif
        LOG_STOP;

        asm(".global check");
        asm("check:");
        for (uint32_t j = 0; j < ARR_LEN; j++) {
            if (work_a[j] != ref[j]) {
                write_mismatch(0xf, 0xf, j+1); // +1 to avoid writing 0
                fail();
            }
        }
    }
    pass();
}
