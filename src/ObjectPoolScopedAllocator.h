#pragma once
#ifndef OBJECT_POOL_SCOPED_ALLOCATOR_H_
#define OBJECT_POOL_SCOPED_ALLOCATOR_H_

#include "ScopedAllocator.h"
#include "ObjectPool.h"

/**
 * Object pool wrappers that provide ScopedAllocator interfaces
 */

class PreallocatedObjectPoolAllocator : public ScopedAllocator
{
public:
    // Initialize object pool with preallocated buffer. If the _ManageBuffer template parameter is set,
    // you should allocate this buffer using an array-new.
    void Init( void* pBuffer, unsigned nObjectCount, unsigned nStride, bool bManageBuffer = true )
    {
        m_oPool.Init( pBuffer, nObjectCount, nStride, bManageBuffer );
    }

    void Deinit()
    {
        m_oPool.Deinit();
    }

    unsigned CountAllocations() const
    {
        return m_oPool.CountAllocations();
    }

    // Virtual overrides
    void* Alloc( size_t nBytes )
    {
        assert( nBytes <= m_oPool.GetStride() );
        return m_oPool.Alloc();
    }

    void Free( void* pObject )
    {
        m_oPool.Free( pObject );
    }

private:
    PreallocatedObjectPool m_oPool;
};

template<unsigned _ObjectStride, unsigned _ObjectCount>
class FixedObjectPoolAllocator : public ScopedAllocator
{
public:
    typedef FixedObjectPool<_ObjectStride, _ObjectCount> InternalObjectPool;

    unsigned CountAllocations() const
    {
        return m_oPool.CountAllocations();
    }

    // Virtual overrides
    void* Alloc( size_t nBytes )
    {
        assert( nBytes <= m_oPool.GetStride() );
        return m_oPool.Alloc();
    }

    void Free( void* pObject )
    {
        m_oPool.Free( pObject );
    }

private:
    InternalObjectPool m_oPool;
};

#endif
