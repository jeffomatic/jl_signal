#include <stdio.h>
#include <stdlib.h>

#include "ArrayUtils.h"
#include "ObjectPoolScopedAllocator.h"

// This is a unit test of the various object pool classes.
namespace
{
    class TestObject
    {
    public:
        TestObject() {}

        TestObject( const char* pString )
        {
            m_pString = pString;
            printf( "%s constructed!\n", m_pString );
        }

        ~TestObject()
        {
            printf( "%s Freeed!\n", m_pString );
        }

    private:
        const char* m_pString;
    };

    struct TestItem
    {
        const char* pText;
        TestObject* pObject;
    };

    TestItem g_ppTests[] = {
        { "First", NULL },
        { "Second", NULL },
        { "Third", NULL },
        { "Fourth", NULL },
        { "Fifth", NULL },
        { "Sixth", NULL },
    };

    const char* g_ppReplacements[] =
    {
        "Replacement 1",
        "Replacement 2",
        "Replacement 3",
        "Replacement 4",
        "Replacement 5",
        "Replacement 6",
    };

    // Allocator for PreallocatedObjectPool
    template<unsigned _Stride, unsigned _ObjectCount, bool _ManageBuffer>
    class PreallocatedPoolFactory
    {
    public:
        static ScopedAllocator* Create()
        {
            void* pBuffer = new unsigned char[ _Stride * _ObjectCount ];
            PreallocatedObjectPoolAllocator* pAllocator = new PreallocatedObjectPoolAllocator();
            pAllocator->Init( pBuffer, _ObjectCount, _Stride, _ManageBuffer );
            return pAllocator;
        }
    };

    // Allocator for FixedObjectPool
    template<unsigned _Stride, unsigned _ObjectCount>
    class FixedPoolFactory
    {
    public:
        static ScopedAllocator* Create()
        {
            return new FixedObjectPoolAllocator<_Stride, _ObjectCount>();
        }
    };

    template<typename _TPoolFactory>
    void PoolTest()
    {
        ScopedAllocator* pAllocator;

        // Test allocations only
        printf( "\nCreating pool...\n");
        pAllocator = _TPoolFactory::Create();

        printf( "Allocating to pool...\n" );
        for ( unsigned i = 0; i < ARRAY_SIZE(g_ppTests); ++i )
        {
            g_ppTests[i].pObject = new( pAllocator->Alloc(sizeof(TestObject)) ) TestObject( g_ppTests[i].pText );
        }

        printf( "Deleting pool...\n" );
        delete pAllocator;

        // Test allocations, plus deallocations
        printf( "\nCreating pool...\n");
        pAllocator = _TPoolFactory::Create();

        printf( "Allocating to pool...\n" );
        for ( unsigned i = 0; i < ARRAY_SIZE(g_ppTests); ++i )
        {
            g_ppTests[i].pObject = new( pAllocator->Alloc(sizeof(TestObject)) ) TestObject( g_ppTests[i].pText );
        }

        printf( "Deleting elements from pool...\n" );
        for ( unsigned i = 0; i < ARRAY_SIZE(g_ppTests); ++i )
        {
            pAllocator->Free( g_ppTests[i].pObject );
        }

        printf( "Deleting pool...\n" );
        delete pAllocator;

        // Test allocations, plus random deallocation/reallocation
        printf( "\nCreating pool...\n");
        pAllocator = _TPoolFactory::Create();

        printf( "Allocating to pool...\n" );
        for ( unsigned i = 0; i < ARRAY_SIZE(g_ppTests); ++i )
        {
            g_ppTests[i].pObject = new( pAllocator->Alloc(sizeof(TestObject)) ) TestObject( g_ppTests[i].pText );
        }

        printf( "random element destruction/creation...\n");
        for ( unsigned i = 0; i < ARRAY_SIZE(g_ppReplacements); ++i )
        {
            printf( "Deleting randomly and replacing...\n" );
            unsigned n = rand() % ARRAY_SIZE(g_ppTests);
            pAllocator->Free( g_ppTests[n].pObject );
            g_ppTests[n].pObject = new( pAllocator->Alloc(sizeof(TestObject)) ) TestObject( g_ppReplacements[i] );
        }

        printf( "Deleting pool...\n" );
        delete pAllocator;

        // Test allocations, plus random deallocation/reallocation, plus deallocation
        printf( "\nCreating pool...\n");
        pAllocator = _TPoolFactory::Create();

        printf( "Allocating to pool...\n" );
        for ( unsigned i = 0; i < ARRAY_SIZE(g_ppTests); ++i )
        {
            g_ppTests[i].pObject = new( pAllocator->Alloc(sizeof(TestObject)) ) TestObject( g_ppTests[i].pText );
        }

        printf( "random element destruction/creation...\n");
        for ( unsigned i = 0; i < ARRAY_SIZE(g_ppReplacements); ++i )
        {
            printf( "Deleting randomly and replacing...\n" );
            unsigned n = rand() % ARRAY_SIZE(g_ppTests);
            pAllocator->Free( g_ppTests[n].pObject );
            g_ppTests[n].pObject = new( pAllocator->Alloc(sizeof(TestObject)) ) TestObject( g_ppReplacements[i] );
        }

        printf( "Deleting elements from pool...\n" );
        for ( unsigned i = 0; i < ARRAY_SIZE(g_ppTests); ++i )
        {
            pAllocator->Free( g_ppTests[i].pObject );
        }

        printf( "Deleting pool...\n" );
        delete pAllocator;

        // Test random allocations
        for ( unsigned i = 0; i < 10; ++i )
        {
            printf( "\nCreating pool...\n");
            pAllocator = _TPoolFactory::Create();

            const unsigned nTimes = ( rand() % ARRAY_SIZE(g_ppTests) ) + 1;
            printf( "Allocating randomly to pool %d times...\n", nTimes );
            for ( unsigned i = 0; i < nTimes; ++i )
            {
                unsigned n = rand() % ARRAY_SIZE(g_ppTests);
                g_ppTests[i].pObject = new( pAllocator->Alloc(sizeof(TestObject)) ) TestObject( g_ppTests[n].pText );
            }

            printf( "Deleting pool...\n" );
            delete pAllocator;
        }
    }
}

void ObjectPoolUnitTest()
{
    enum { eTestObjectCount = ARRAY_SIZE(g_ppTests) };

    typedef PreallocatedPoolFactory<sizeof(TestObject), eTestObjectCount, true> PreallocatedPoolFactory;
    printf( "\n=== MANAGED PREALLOCATED POOL TestObjectS ===\n" );
    PoolTest<PreallocatedPoolFactory>();

    typedef FixedPoolFactory<sizeof(TestObject), eTestObjectCount> FixedPoolFactory;
    printf( "\n=== MANAGED FIXED POOL TestObjectS ===\n" );
    PoolTest<FixedPoolFactory>();
}
