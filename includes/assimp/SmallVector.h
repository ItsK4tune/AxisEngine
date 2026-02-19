



#pragma once
#ifndef AI_SMALLVECTOR_H_INC
#define AI_SMALLVECTOR_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

namespace Assimp {








template<typename T, unsigned int Capacity>
class SmallVector {
public:
    
    SmallVector() :
            mStorage(mInplaceStorage),
            mSize(0),
            mCapacity(Capacity) {
        
    }

    
    ~SmallVector() {
        if (mStorage != mInplaceStorage) {
            delete [] mStorage;
        }
    }

    
    
    void push_back(const T& item) {
        if (mSize < mCapacity) {
            mStorage[mSize++] = item;
            return;
        }

        push_back_and_grow(item);
    }

    
    
    void resize(size_t newSize) {
        if (newSize > mCapacity) {
            grow(newSize);
        }
        mSize = newSize;
    }

    
    
    size_t size() const {
        return mSize;
    }

    
    
    T* begin() {
        return mStorage;
    }

    
    
    T* end() {
        return &mStorage[mSize];
    }

    
    
    T* begin() const {
        return mStorage;
    }

    
    
    T* end() const {
        return &mStorage[mSize];
    }

    SmallVector(const SmallVector &) = delete;
    SmallVector(SmallVector &&) = delete;
    SmallVector &operator = (const SmallVector &) = delete;
    SmallVector &operator = (SmallVector &&) = delete;

private:
    void grow( size_t newCapacity) {
        T* oldStorage = mStorage;
        T* newStorage = new T[newCapacity];

        std::memcpy(newStorage, oldStorage, mSize * sizeof(T));

        mStorage = newStorage;
        mCapacity = newCapacity;

        if (oldStorage != mInplaceStorage) {
            delete [] oldStorage;
        }
    }

    void push_back_and_grow(const T& item) {
        grow(mCapacity + Capacity);

        mStorage[mSize++] = item;
    }

    T* mStorage;
    size_t mSize;
    size_t mCapacity;
    T mInplaceStorage[Capacity];
};

} 

#endif 
