#pragma once
#ifndef _JL_OBJECT_POOL_SCOPED_ALLOCATOR_H_
#define _JL_OBJECT_POOL_SCOPED_ALLOCATOR_H_

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
    void Init( void* pBuffer, unsigned nCapacity, unsigned nStride, bool bManageBuffer = true )
    {
        m_oPool.Init( pBuffer, nCapacity, nStride, bManageBuffer );
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

template<unsigned _Stride, unsigned _Capacity>
class StaticObjectPoolAllocator : public ScopedAllocator
{
public:
    typedef StaticObjectPool<_Stride, _Capacity> InternalObjectPool;

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

#endif // _JL_OBJECT_POOL_SCOPED_ALLOCATOR_H_
