#include <stddef.h>

#include "ObjectPool.h"

// Some helper routines
namespace
{
    bool IsBounded( const void* pObject, const unsigned char* pObjectBuffer, unsigned nObjectCount, unsigned nStride )
    {
        const unsigned char* const pFirst = pObjectBuffer;
        const unsigned char* const pLast = pObjectBuffer + nStride * (nObjectCount - 1);
        return pFirst <= pObject && pObject <= pLast;
    }

    bool IsAligned( const void* pObject, const unsigned char* pObjectBuffer, unsigned nStride )
    {
        const ptrdiff_t nDiff = reinterpret_cast<const unsigned char*>(pObject) - pObjectBuffer;
        return ( nDiff % nStride == 0 );
    }

    // Populate a sorted array with pointers to every free node, and return the number of free nodes.
    unsigned GetSortedFreeNodeList( ObjectPool::FreeNode* ppSortedFreeNodes[], ObjectPool::FreeNode* pFreeListHead )
    {
        unsigned nFreeCount = 0;

        for ( ObjectPool::FreeNode* n = pFreeListHead; n != NULL; n = n->pNextFree )
        {
            ppSortedFreeNodes[ nFreeCount ] = n;
            nFreeCount += 1;
        }

        // Insertion sort
        for ( unsigned i = 1; i < nFreeCount; ++i )
        {
            ObjectPool::FreeNode* pCurrent = ppSortedFreeNodes[ i ];
            unsigned j = i;

            // Insert pCurrent into the appropriate spot in the span [0, i]
            while ( j > 0 && ppSortedFreeNodes[j - 1] > pCurrent )
            {
                ppSortedFreeNodes[ j ] = ppSortedFreeNodes[ j - 1 ];
                --j;
            }

            ppSortedFreeNodes[ j ] = pCurrent;
        }

        return nFreeCount;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Initializes an object buffer as a free list and returns the head of the list
ObjectPool::FreeNode* ObjectPool::InitFreeList( unsigned char* pObjectBuffer, unsigned nObjectCount, unsigned nStride )
{
    // Setup free list links
    unsigned char* const pLast = pObjectBuffer + nStride * (nObjectCount - 1);

    for ( unsigned char* pCurrent = pObjectBuffer; pCurrent < pLast; pCurrent += nStride )
    {
        FreeNode::Cast( pCurrent )->pNextFree = FreeNode::Cast( pCurrent + nStride );
    }

    // End free list
    FreeNode::Cast( pLast )->pNextFree = NULL;

    // Return start of free list
    return FreeNode::Cast( pObjectBuffer );
}

bool ObjectPool::IsBoundedAndAligned( const void* pObject, const unsigned char* pObjectBuffer, unsigned nObjectCount, unsigned nStride )
{
    return IsBounded( pObject, pObjectBuffer, nObjectCount, nStride )
        && IsAligned( pObject, pObjectBuffer, nStride );
}

bool ObjectPool::IsFree( const void* pObject, const FreeNode* pFreeListHead )
{
    // Scrub through free list and make sure this object hasn't been freed already
    for ( const ObjectPool::FreeNode* n = pFreeListHead; n != NULL; n = n->pNextFree )
    {
        if ( n == pObject )
        {
            return true;
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

PreallocatedObjectPool::PreallocatedObjectPool()
{
    Reset();
}

PreallocatedObjectPool::PreallocatedObjectPool( void* pBuffer, unsigned nObjectCount, unsigned nStride, bool bManageBuffer /*= true */ )
{
    m_pObjectBuffer = NULL; // prevent assert in Init()
    Init( pBuffer, nObjectCount, nStride, bManageBuffer );
}

PreallocatedObjectPool::~PreallocatedObjectPool()
{
    if ( m_bManageBuffer )
    {
        delete[] m_pObjectBuffer;
    }
}

void PreallocatedObjectPool::Init( void* pBuffer, unsigned nObjectCount, unsigned nStride, bool bManageBuffer /*= true */ )
{
    assert( m_pObjectBuffer == NULL );

    m_pObjectBuffer = (unsigned char*)pBuffer;
    m_pFreeListHead = ObjectPool::InitFreeList( m_pObjectBuffer, nObjectCount, nStride );

    m_nObjectCount = nObjectCount;
    m_nStride = nStride;
    m_bManageBuffer = bManageBuffer;
}

void PreallocatedObjectPool::Deinit()
{
    if ( m_bManageBuffer )
    {
        delete[] m_pObjectBuffer;
    }

    Reset();
}

void PreallocatedObjectPool::Reset()
{
    m_pObjectBuffer = NULL;
    m_pFreeListHead = NULL;

    m_nObjectCount = 0;
    m_nAllocations = 0;
    m_nStride = 0;
    m_bManageBuffer = false;
}
