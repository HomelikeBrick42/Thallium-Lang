#pragma once

#include "./Typedefs.h"

enum DynamicArrayField {
    DynamicArrayField_Capacity,
    DynamicArrayField_Length,
    DynamicArrayField_Stride,
    
    DynamicArrayField_Count,
};

void* DynamicArrayCreate_(u64 capacity, u64 stride);
void DynamicArrayDestroy_(void* array);

void* DynamicArrayPush_(void* array, const void* valuePtr);
void* DynamicArrayPop_(void* array, void* dest);

void* DynamicArrayInsert_(void* array, u64 index, const void* valuePtr);
void* DynamicArrayPopAt_(void* array, u64 index, void* dest);

#define DynamicArrayCreate(type) \
    (cast(type*) DynamicArrayCreate_(1, sizeof(type)))

#define DynamicArrayDestroy(array) \
    (cast(void) (DynamicArrayDestroy_((array)), (array) = NULL))

#define DynamicArrayPush(array, value) \
    do { \
        __typeof__(*(array)) temp = value; \
        (array) = DynamicArrayPush_((array), &(temp)); \
    } while (0)

#define DynamicArrayPop(array, dest) \
    do { \
        (array) = DynamicArrayPop_((array), (dest)); \
    } while (0)

#define DynamicArrayInsert(array, index, value) \
    do { \
        __typeof__(*(array)) temp = (value); \
        (array) = DynamicArrayInsert_((array), (index), &(temp)); \
    } while (0)

#define DynamicArrayPopAt(array, index, dest) \
    do { \
        (array) = DynamicArrayPopAt_((array), (index), (dest)); \
    } while (0)

#define DynamicArrayGetField(array, field) \
    (*((cast(u64*) (array)) - DynamicArrayField_Count + field))

#define DynamicArrayCapacity(array) \
    DynamicArrayGetField((array), DynamicArrayField_Capacity)

#define DynamicArrayLength(array) \
    DynamicArrayGetField((array), DynamicArrayField_Length)

#define DynamicArrayStride(array) \
    DynamicArrayGetField((array), DynamicArrayField_Stride)

#define DynamicArraySize(array) \
    (DynamicArrayLength((array)) * DynamicArrayStride((array)))
