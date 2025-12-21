
#ifndef DD_ARRAY_H
#define DD_ARRAY_H

#include <stdlib.h>
#include <string.h>

#define AR_MAX_ALLOCATORS 9
#define ConstArraySize(array) ((int)(sizeof(array)/sizeof(array[0])))

#define arrheader(arr)                  (*(_arrheader*) (((char*)(arr)) - sizeof(_arrheader)))
#define arrcount(arr)                   arrheader(arr).count
#define arrcap(arr)                     arrheader(arr).capacity
#define arrtypesize(arr)                arrheader(arr).typesize
#define arrallocator(arr)               arrheader(arr).allocator
#define arrlast(arr)                    arr[arrcount(arr)-1]
#define arrisempty(arr)                 (arrcount(arr) == 0)
#define arrget(arr, i)                  ((void*) (((char*) arr) + i * arrtypesize(array)))
#define arradd(arr, instance)           _arrAdd((void*&)arr, (void*)&instance)
#define arrconcat(arr1, arr2)           _arrConcat((void*&)arr1, (void*&)arr2)
#define arrallocate(type, capacity, allocatorIndex) (type*) AR_Allocate(&AR_Allocators[allocatorIndex], sizeof(type), capacity)
#define arralloc(type, capacity)        arrallocate(type, capacity, 0)
#define arrfree(arr)                    _arrFree((void*&)arr)
#define arrclear(arr)                   _arrClear((void*&)arr)
#define arrcopy(arr1, arr2)             _arrCopy((void*&)arr1, (void*&)arr2)

// TODO: arrclone not tested!
#define arrclone(arr1)                  AR_Clone(arr)
#define arrslice(arr, first, last)      _arrSlice((void*&)arr, first, last)
#define arrremove(arr, index)           _arrRemove((void*&)arr, index)
#define arrremovematch(arr, instance)   _arrRemoveMatch((void*&)arr, (void*)&instance)
#define arrremovemany(arr, firstIndex, removeCount) _arrRemoveMany((void*&)(arr), firstIndex, removeCount)
#define arrfind(arr, instance)          _arrFind((void*&)arr, (void*)&instance)
#define arrsort(arr, preceds)           AR_QuickSort((void*&) arr, preceds)

#define arralloc0(type, capacity)       arrallocate(type, capacity, 0)
#define arralloc1(type, capacity)       arrallocate(type, capacity, 1)
#define arralloc2(type, capacity)       arrallocate(type, capacity, 2)
#define arralloc3(type, capacity)       arrallocate(type, capacity, 3)
#define arralloc4(type, capacity)       arrallocate(type, capacity, 4)
#define arralloc5(type, capacity)       arrallocate(type, capacity, 5)
#define arralloc6(type, capacity)       arrallocate(type, capacity, 6)
#define arralloc7(type, capacity)       arrallocate(type, capacity, 7)
#define arralloc8(type, capacity)       arrallocate(type, capacity, 8)

struct AR_Allocator {
    void* (*malloc)(void* data, unsigned long long amount);
    void  (*free)(void* data, void* pointer);
    void* data;
};

void* AR_callocCaller(void* data, unsigned long long amount) {
    return calloc(1, amount);
}

void AR_freeCaller(void* data, void* pointer) {
    return free(pointer);
}

AR_Allocator AR_DefaultAllocator = {AR_callocCaller, AR_freeCaller};

AR_Allocator AR_Allocators[AR_MAX_ALLOCATORS] = {
    AR_DefaultAllocator
};

struct _arrheader {
    AR_Allocator* allocator;
    int typesize;
    int capacity;
    int count;
};

void AR_RegisterAllocator(AR_Allocator allocator, int index = -1) {
    if (index == -1) {
        AR_Allocator* a = AR_Allocators;
        while (a->malloc) {
            ++a;
        }
        *a = allocator;
    } else {
        AR_Allocators[index] = allocator;
    }
}

void* AR_Allocate(AR_Allocator* allocator, int typesize, int capacity) {
    int grossSize = sizeof(_arrheader) + typesize*capacity;
    _arrheader* header = (_arrheader*) allocator->malloc(allocator->data, grossSize);
    memset(header, 0, grossSize);
    header->allocator = allocator;
    header->typesize = typesize;
    header->capacity = capacity;
    header->count = 0;
    
    return (void*) (header+1);
}

void* AR_Allocate(int typesize, int capacity, int allocatorIndex) {
    return AR_Allocate(&AR_Allocators[allocatorIndex], typesize, capacity);
}

void* AR_Clone(void*& arr) {
    void* result = AR_Allocate(arrtypesize(arr), arrcount(arr), 0);
    memcpy(result, arr, arrtypesize(arr)*arrcount(arr));
    arrcount(result) = arrcount(arr);
    return result;
}

void _arrFree(void*& arr) {
    AR_Allocator* allocator = arrallocator(arr);
    allocator->free(allocator->data, ((char*)(arr)) - sizeof(_arrheader));
    arr = 0;
}

void _arrAdd(void*& array, void* instance) {
    if (arrcount(array) + 1 > arrcap(array)) {
        void* newarray = AR_Allocate(arrallocator(array), arrtypesize(array), arrcount(array) + 1);
        memcpy(newarray, array, arrcount(array) * arrtypesize(array));
        arrcount(newarray) = arrcount(array);
        arrallocator(newarray) = arrallocator(array);
        _arrFree(array);
        array = newarray;
    }
    char* dest = ((char*)(array)) + arrcount(array)*arrtypesize(array);
    memcpy(dest, instance, arrtypesize(array));
    ++arrcount(array);
}

void _arrConcat(void*& array1, void*& array2) {
    for (int i = 0; i < arrcount(array2); ++i) {
        _arrAdd(array1, ((char*) array2) + i*arrtypesize(array2));
    }
}

void _arrCopy(void*& array1, void*& array2) {
    if (arrcap(array1) < arrcount(array2)) {
        arrfree(array1);
        array1 = AR_Allocate(arrallocator(array2), arrtypesize(array2), arrcount(array2));
    }
    
    memcpy(array1, array2, arrtypesize(array2) * arrcount(array2));
    arrcount(array1) = arrcount(array2);
    arrallocator(array1) = arrallocator(array2);
}

void* _arrSlice(void*& array, int first, int last) {
    int copyCount = last - first + 1;
    void* result = AR_Allocate(arrallocator(array), arrtypesize(array), copyCount);
    memcpy(result, ((char*) array) + first*arrtypesize(array), copyCount * arrtypesize(array));
    arrcount(result) = copyCount;
    arrallocator(result) = arrallocator(array);
    return result;
}

void _arrRemoveMany(void*& array, int firstIndex, int removeCount) {
    for (int i = firstIndex+removeCount; i < arrcount(array); ++i) {
        char* dest   = ((char*)array) + (i-removeCount)*arrtypesize(array);
        char* source = ((char*)array) + i*arrtypesize(array);
        memcpy(dest, source, arrtypesize(array));
    }
    arrcount(array) -= removeCount;
}

void _arrRemove(void*& array, int index) {
    for (int i = index+1; i < arrcount(array); ++i) {
        char* dest   = ((char*)array) + (i-1)*arrtypesize(array);
        char* source = ((char*)array) + i*arrtypesize(array);
        memcpy(dest, source, arrtypesize(array));
    }
    --arrcount(array);
}

int _arrFind(void*& array, void* instance) {
    for (int i = 0; i < arrcount(array); ++i) {
        char* testItem = ((char*)array) + i*arrtypesize(array);
        if (memcmp(testItem, instance, arrtypesize(array)) == 0) {
            return i;
        }
    }
    return -1;
}

void _arrRemoveMatch(void*& array, void* instance) {
    int index = _arrFind(array, instance);
    if (index >= 0) {
        _arrRemove(array, index);
    }
}

void _arrClear(void*& array) {
    void* newArray = AR_Allocate(arrallocator(array), arrtypesize(array), 0);
    _arrFree(array);
    array = newArray;
}

// Quicksort
//------------
int AR_QuickSortPartition(void*& array, int lo, int hi, bool (*preceds)(void*, void*), void* aux) {
    void* pivot = arrget(array, hi);
    int i = lo;
    for (int j = lo; j <= hi; ++j) {
        if (preceds(arrget(array, j), pivot)) {
            memcpy(aux, arrget(array, i), arrtypesize(array));
            memcpy(arrget(array, i), arrget(array, j), arrtypesize(array));
            memcpy(arrget(array, j), aux, arrtypesize(array));
            ++i;
        }
        
    }
    
    memcpy(aux, arrget(array, i), arrtypesize(array));
    memcpy(arrget(array, i), arrget(array, hi), arrtypesize(array));
    memcpy(arrget(array, hi), aux, arrtypesize(array));
    
    return i;
}

void AR_QuickSort(void*& array, int lo, int hi, bool (*preceds)(void*, void*), void* aux) {
    if (lo < hi) {
        int p = AR_QuickSortPartition(array, lo, hi, preceds, aux);
        AR_QuickSort(array, lo, p-1, preceds, aux);
        AR_QuickSort(array, p+1, hi, preceds, aux);
    }
}

void AR_QuickSort(void*& array, bool (*preceds)(void*, void*)) {
    void* aux = malloc(arrtypesize(array));
    AR_QuickSort(array, 0, arrcount(array)-1, preceds, aux);
    free(aux);
}

// String helpers
//-----------------
char* AR_AllocString(AR_Allocator* allocator, int size) {
    char* string = (char*) AR_Allocate(allocator, sizeof(char), size+1);
    string[size] = 0;
    return string;
}

char* AR_AllocString(AR_Allocator* allocator, const char* c_str) {
    int length = 0;
    char* c = (char*) c_str;
    while (*c != 0) {
        c++; 
        length++;
    }
    
    char* string = AR_AllocString(allocator, length);
    memcpy(string, c_str, length * sizeof(char));
    arrcount(string) = length;
    return string;
}

char* AR_AllocString(AR_Allocator* allocator, const char* string, int size) {
    char* result = AR_AllocString(allocator, size);
    memcpy(result, string, size);
    arrcount(result) = size;
    return result;
}

char* arrstringalloc(int size, int allocatorIndex) {return AR_AllocString(&AR_Allocators[allocatorIndex], size);}
char* arrstringalloc(const char* c_str, int allocatorIndex) {return AR_AllocString(&AR_Allocators[allocatorIndex], c_str);}
char* arrstringalloc(const char* string, int size, int allocatorIndex) {return AR_AllocString(&AR_Allocators[allocatorIndex], string, size);}

char* arrstring(int size) {return AR_AllocString(&AR_Allocators[0], size);}
char* arrstring(const char* c_str) {return AR_AllocString(&AR_Allocators[0], c_str);}
char* arrstring(const char* string, int size) {return AR_AllocString(&AR_Allocators[0], string, size);}

char* arrstring0(int size) {return AR_AllocString(&AR_Allocators[0], size);}
char* arrstring0(const char* c_str) {return AR_AllocString(&AR_Allocators[0], c_str);}
char* arrstring0(const char* string, int size) {return AR_AllocString(&AR_Allocators[0], string, size);}

char* arrstring1(int size) {return AR_AllocString(&AR_Allocators[1], size);}
char* arrstring1(const char* c_str) {return AR_AllocString(&AR_Allocators[1], c_str);}
char* arrstring1(const char* string, int size) {return AR_AllocString(&AR_Allocators[1], string, size);}

char* arrstring2(int size) {return AR_AllocString(&AR_Allocators[2], size);}
char* arrstring2(const char* c_str) {return AR_AllocString(&AR_Allocators[2], c_str);}
char* arrstring2(const char* string, int size) {return AR_AllocString(&AR_Allocators[2], string, size);}

char* arrstring3(int size) {    return AR_AllocString(&AR_Allocators[3], size);}
char* arrstring3(const char* c_str) {return AR_AllocString(&AR_Allocators[3], c_str);}
char* arrstring3(const char* string, int size) {return AR_AllocString(&AR_Allocators[3], string, size);}

char* arrstring4(int size) {return AR_AllocString(&AR_Allocators[4], size);}
char* arrstring4(const char* c_str) {return AR_AllocString(&AR_Allocators[4], c_str);}
char* arrstring4(const char* string, int size) {return AR_AllocString(&AR_Allocators[4], string, size);}

char* arrstring5(int size) {return AR_AllocString(&AR_Allocators[5], size);}
char* arrstring5(const char* c_str) {return AR_AllocString(&AR_Allocators[5], c_str);}
char* arrstring5(const char* string, int size) {return AR_AllocString(&AR_Allocators[5], string, size);}

char* arrstring6(int size) {return AR_AllocString(&AR_Allocators[6], size);}
char* arrstring6(const char* c_str) {return AR_AllocString(&AR_Allocators[6], c_str);}
char* arrstring6(const char* string, int size) {return AR_AllocString(&AR_Allocators[6], string, size);}

char* arrstring7(int size) {return AR_AllocString(&AR_Allocators[7], size);}
char* arrstring7(const char* c_str) {return AR_AllocString(&AR_Allocators[7], c_str);}
char* arrstring7(const char* string, int size) {return AR_AllocString(&AR_Allocators[7], string, size);}

char* arrstring8(int size) {return AR_AllocString(&AR_Allocators[8], size);}
char* arrstring8(const char* c_str) {return AR_AllocString(&AR_Allocators[8], c_str);}
char* arrstring8(const char* string, int size) {return AR_AllocString(&AR_Allocators[8], string, size);}

#endif








