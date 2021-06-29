#include "./DynamicArray.h"

#include <stdlib.h>
#include <string.h>

void* DynamicArrayCreate_(u64 capacity, u64 stride) {
    u64 headerSize = DynamicArrayField_Count * sizeof(u64);
    u64 arraySize = capacity * stride;
    void* header = malloc(headerSize + arraySize);
    void* array = (cast(u64*) header) + DynamicArrayField_Count;
    DynamicArrayCapacity(array) = capacity;
    DynamicArrayLength(array) = 0;
    DynamicArrayStride(array) = stride;
    return array;
}

void DynamicArrayDestroy_(void* array) {
    free((cast(u64*) array) - DynamicArrayField_Count);
}

void* DynamicArrayPush_(void* array, const void* valuePtr) {
    if (DynamicArrayLength(array) >= DynamicArrayCapacity(array)) {
        void* newArray = DynamicArrayCreate_(DynamicArrayCapacity(array) != 0 ? DynamicArrayCapacity(array) * 2 : 1, DynamicArrayStride(array));
        memcpy(newArray, array, DynamicArraySize(array));
        DynamicArrayLength(newArray) = DynamicArrayLength(array);

        DynamicArrayDestroy(array);
        array = newArray;
    }

    memcpy(&(cast(u8*) array)[DynamicArrayLength(array) * DynamicArrayStride(array)], valuePtr, DynamicArrayStride(array));
    DynamicArrayLength(array)++;
    return array;
}

void* DynamicArrayPop_(void* array, void* dest) {
    DynamicArrayLength(array)--;
    if (dest) {
        memcpy(dest, &(cast(u8*) array)[DynamicArrayLength(array) * DynamicArrayStride(array)], DynamicArrayStride(array));
    }
    return array;
}

void* DynamicArrayInsert_(void* array, u64 index, const void* valuePtr) {
    if (index > DynamicArrayLength(array)) {
        ASSERT(FALSE);
        return array;
    }

    if (index == DynamicArrayLength(array)) {
        return DynamicArrayPush_(array, valuePtr);
    }

    if (DynamicArrayLength(array) >= DynamicArrayCapacity(array)) {
        void* newArray = DynamicArrayCreate_(DynamicArrayCapacity(array) != 0 ? DynamicArrayCapacity(array) * 2 : 1, DynamicArrayStride(array));
        memcpy(newArray, array, DynamicArraySize(array));
        DynamicArrayLength(newArray) = DynamicArrayLength(array);

        DynamicArrayDestroy(array);
        array = newArray;
    }

    memmove(&(cast(u8*) array)[(index + 1) * DynamicArrayStride(array)], &(cast(u8*) array)[index * DynamicArrayStride(array)], (DynamicArrayLength(array) - index) * DynamicArrayStride(array));
    memcpy(&(cast(u8*) array)[index * DynamicArrayStride(array)], valuePtr, DynamicArrayStride(array));
    DynamicArrayLength(array)++;
    return array;
}

void* DynamicArrayPopAt_(void* array, u64 index, void* dest) {
    if (index >= DynamicArrayLength(array)) {
        ASSERT(FALSE);
        return array;
    }

    if (index == (DynamicArrayLength(array) - 1)) {
        return DynamicArrayPop_(array, dest);
    }

    if (dest) {
        memcpy(dest, &(cast(u8*) array)[index * DynamicArrayStride(array)], DynamicArrayStride(array));
    }
    memmove(&(cast(u8*) array)[index * DynamicArrayStride(array)], &(cast(u8*) array)[(index + 1) * DynamicArrayStride(array)], (DynamicArrayLength(array) - index) * DynamicArrayStride(array));
    DynamicArrayLength(array)--;
    return array;
}
