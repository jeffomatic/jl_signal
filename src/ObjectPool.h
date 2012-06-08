#pragma once
#ifndef OBJECTPOOL_H_
#define OBJECTPOOL_H_

#include <assert.h>

//#define _OBJECT_POOL_ENABLE_FREELIST_CHECK

/**
 * A family of object pool classes:
 *
 *    GenericPreallocatedObjectPool
 *    PreallocatedObjectPool (type-specific)
 *    GenericFixedObjectPool
 *    FixedObjectPool (type-specific)
 *
 * For alignment issues related to the fixed object pools, this does not derive from
 * the ScopedAllocator interface. If you need an object pool to act as a ScopedAllocator,
 * please see ObjectPoolScopedAllocator.h.
 *
 * Example:
 *    // Create a pool of 100 physics components
 *    FixedObjectPool<PhysicsComponent, 100> oPhysicsComponentPool;
 *
 *    // Allocate a physics component from the pool
 *    PhysicsComponent* pPhysics = new( oPhysicsComponentPool.Alloc() ) PhysicsComponent( pOwner );
 *    ...
 *    // When we don't need the component anymore, we can free it and return its memory back to the pool
 *    oPhysicsComponentPool.Destroy( pPhysics ); // this will call PhysicsComponent()!
 * 
 * PRO:
 *    O(1) allocate and free
 *    O(1) overhead
 * 
 * CON:
 *    No support for array-new allocation
 *    Free()/Destroy() requires knowledge of which pool a pointer was allocated from
 */

// A common implementation for our object pools.
namespace ObjectPool
{
    struct FreeNode
    {
        FreeNode* pNextFree;

        static FreeNode* Cast( unsigned char* pRaw ) { return reinterpret_cast<FreeNode*>( pRaw );    }
        static FreeNode* Cast( void* pRaw ) { return reinterpret_cast<FreeNode*>( pRaw );    }
        static const FreeNode* Cast( const unsigned char* pRaw ) { return reinterpret_cast<const FreeNode*>( pRaw ); }
        static const FreeNode* Cast( const void* pRaw ) { return reinterpret_cast<const FreeNode*>( pRaw ); }
    };

    // Return first node in the free list, and advances the free list to the next node
    inline void* Alloc( FreeNode*& pFreeListHead )
    {
        if ( pFreeListHead == NULL )
        {
            return NULL;
        }

        void* pObject = pFreeListHead;
        pFreeListHead = pFreeListHead->pNextFree;

        return pObject;
    }

    // Return object's node to the free list. Does no address validation or object destruction.
    inline void Free( void* pObject, FreeNode*& pFreeListHead )
    {
        FreeNode* pNode = FreeNode::Cast( pObject );
        pNode->pNextFree = pFreeListHead;
        pFreeListHead = pNode;
    }

    // Initializes an object buffer as a free list and returns the head of the list
    FreeNode* InitFreeList( unsigned char* pObjectBuffer, unsigned nObjectCount, unsigned nStride );

    // Returns true if an object is allocated to the given object pool
    bool IsBoundedAndAligned( const void* pObject, const unsigned char* pObjectBuffer, unsigned nObjectCount, unsigned nStride );
    bool IsFree( const void* pObject, const FreeNode* pFreeListHead );

    // Call typed destructor on an object at a generic address
    template<typename _T>
    void CastAndDestroy( void* pObject )
    {
        reinterpret_cast<_T*>( pObject )->~_T();
    }
};

/**
 * A class that manages allocations to a pre-allocated object buffer.
 * Useful for pools whose size are NOT known at compile-time.
 *
 * By default, this class will free the supplied buffer when the pool object is destroy.
 * If this behavior is enabled, make sure that the supplied buffer was created using array-new.
 */
class PreallocatedObjectPool
{
public:
    PreallocatedObjectPool();
    PreallocatedObjectPool( void* pBuffer, unsigned nObjectCount, unsigned nStride, bool bManageBuffer = true );

    ~PreallocatedObjectPool();

    // Initialize object pool with preallocated buffer. If the _ManageBuffer template parameter is set,
    // you should allocate this buffer using an array-new.
    void Init( void* pBuffer, unsigned nObjectCount, unsigned nStride, bool bManageBuffer = true );
    void Deinit();

    // Allocates memory. Does not call constructor--you should do a placement new on the returned pointer.
    void* Alloc()
    {
        assert( m_pObjectBuffer );
        void* p = ObjectPool::Alloc( m_pFreeListHead );

        if ( p )
        {
            m_nAllocations++;
        }

        return p;
    }

    // Free allocated memory, with error checking. Does NOT call destructor.
    void Free( void* pObject )
    {
        assert( m_pObjectBuffer );
        assert( ObjectPool::IsBoundedAndAligned(pObject, m_pObjectBuffer, m_nObjectCount, m_nStride) );
#ifdef _OBJECT_POOL_ENABLE_FREELIST_CHECK
        assert( ! ObjectPool::IsFree(pObject, m_pFreeListHead) );
#endif

        ObjectPool::Free( pObject, m_pFreeListHead );
        m_nAllocations--;
    }

    // Accessors
    unsigned char* GetObjectBuffer() { return m_pObjectBuffer; }
    const unsigned char* GetObjectBuffer() const { return m_pObjectBuffer; }

    unsigned GetObjectCount() const { return m_nObjectCount; }
    unsigned GetStride() const { return m_nStride; }
    unsigned CountAllocations() const { return m_nAllocations; }

    bool IsEmpty() const { return m_nAllocations == 0; }
    bool IsFull() const { return m_nAllocations == m_nObjectCount; }

    ObjectPool::FreeNode* GetFreeListHead() { return m_pFreeListHead; }
    const ObjectPool::FreeNode* GetFreeListHead() const { return m_pFreeListHead; }

private:
    void Reset();

    unsigned char* m_pObjectBuffer;
    ObjectPool::FreeNode* m_pFreeListHead;

    // The following are only necessary for:
    // - assertions
    // - Accessor queries, e.g., if you need to know the size of the pool in an external context)
    // - Managing objects, e.g., destroying the buffer and/or allocated objects upon the destruction of the pool
    // If we don't need any of those, we can safely remove these member variables;
    unsigned m_nObjectCount;
    unsigned m_nStride;
    unsigned m_nAllocations;
    bool m_bManageBuffer;
};

/**
 * An object pool with an internal buffer.
 * Useful for pools whose size are known at compile-time.
 *
 * Due to alignment issues, this is not implemented in terms of PreallocatedObjectPool.
 */
template<unsigned _ObjectStride, unsigned _ObjectCount>
class FixedObjectPool
{
public:
    FixedObjectPool()
    {
        m_pFreeListHead = ObjectPool::InitFreeList( m_pObjectBuffer, _ObjectCount, _ObjectStride );
        m_nAllocations = 0;
    }

    // Allocates memory. Does not call constructor--you should do a placement new on the returned pointer.
    void* Alloc()
    {
        void* p = ObjectPool::Alloc( m_pFreeListHead );

        if ( p )
        {
            m_nAllocations++;
        }

        return p;
    }

    // Free allocated memory, with error checking. Does NOT call destructor.
    void Free( void* pObject )
    {
        assert( ObjectPool::IsBoundedAndAligned(pObject, m_pObjectBuffer, _ObjectCount, _ObjectStride) );
#ifdef _OBJECT_POOL_ENABLE_FREELIST_CHECK
        assert( ! ObjectPool::IsFree(pObject, m_pFreeListHead) );
#endif

        ObjectPool::Free( pObject, m_pFreeListHead );
        m_nAllocations--;
    }

    // Accessors
    unsigned char* GetObjectBuffer() { return m_pObjectBuffer; }
    const unsigned char* GetObjectBuffer() const { return m_pObjectBuffer; }

    unsigned GetObjectCount() const { return _ObjectCount; }
    unsigned CountAllocations() const { return m_nAllocations; }
    unsigned GetStride() const { return _ObjectStride; }

    ObjectPool::FreeNode* GetFreeListHead() { return m_pFreeListHead; }
    const ObjectPool::FreeNode* GetFreeListHead() const { return m_pFreeListHead; }

    bool IsEmpty() const { return m_nAllocations == 0; }
    bool IsFull() const { return m_nAllocations == _ObjectCount; }

private:
    unsigned char m_pObjectBuffer[ _ObjectCount * _ObjectStride ];
    ObjectPool::FreeNode* m_pFreeListHead;
    unsigned m_nAllocations;
};

#endif // OBJECTPOOL_H_
