#pragma once
#ifndef SCOPED_ALLOCATOR_H_
#define SCOPED_ALLOCATOR_H_

#include <new> // Allow for placement new

/**
 * An interface for very basic, stateful allocators. No array allocation.
 */
class ScopedAllocator
{
public:
    virtual ~ScopedAllocator() {};
    virtual void* Alloc( size_t nBytes ) = 0;
    virtual void Free( void* pObject ) = 0;
};

#endif // ! defined( SCOPED_ALLOCATOR_H_ )
