#include <stdio.h>
#include <assert.h>

#include "Signal.h"
#include "SignalConnectionPools.h"

using namespace jl;

namespace
{
    enum { eSignalMaxArity = 8 };    
    
    class TestObserver : public SignalObserver
    {
    private:
        static int s_id;      
        static unsigned s_pCallsByArity[ eSignalMaxArity + 1 ];
        
        int m_nId;

    public:    
        static void ResetCallsByArity()
        {
            for ( unsigned i = 0; i < JL_ARRAY_SIZE(s_pCallsByArity); ++i )
            {
                s_pCallsByArity[i] = 0;
            }
        }
        
        static unsigned CountCallsByArity( unsigned arity )
        {
            return s_pCallsByArity[arity];
        }
        
        TestObserver()
        {
            m_nId = s_id++;
        }
        
        void M0()
        {
            s_pCallsByArity[0] += 1;
            printf( "TestObserver %d: method 0!\n", m_nId );
        }
        
        void M1( int p1 )
        {
            s_pCallsByArity[1] += 1;
            printf( "TestObserver %d: method 1! %d\n", m_nId, p1 );
        }
        
        void M2( int p1, float p2 )
        {
            s_pCallsByArity[2] += 1;
            printf( "TestObserver %d: method 2! %d %f\n", m_nId, p1, p2 );
        }
        
        void M3( int p1, float p2, char p3 )
        {
            s_pCallsByArity[3] += 1;            
            printf( "TestObserver %d: method 3! %d %f %c\n", m_nId, p1, p2, p3 );
        }
        
        void M4( int p1, float p2, char p3, const char* p4 )
        {
            s_pCallsByArity[4] += 1;
            printf( "TestObserver %d: method 4! %d %f %c %s\n", m_nId, p1, p2, p3, p4 );
        }
        
        void M5( int p1, float p2, char p3, const char* p4, int p5 )
        {
            s_pCallsByArity[5] += 1;
            printf( "TestObserver %d: method 5! %d %f %c %s %d\n", m_nId, p1, p2, p3, p4, p5 );
        }
        
        void M6( int p1, float p2, char p3, const char* p4, int p5, float p6 )
        {
            s_pCallsByArity[6] += 1;
            printf( "TestObserver %d: method 6! %d %f %c %s %d %f\n", m_nId, p1, p2, p3, p4, p5, p6 );
        }
        
        void M7( int p1, float p2, char p3, const char* p4, int p5, float p6, char p7 )
        {
            s_pCallsByArity[7] += 1;
            printf( "TestObserver %d: method 7! %d %f %c %s %d %f %c\n", m_nId, p1, p2, p3, p4, p5, p6, p7 );
        }
        
        void M8( int p1, float p2, char p3, const char* p4, int p5, float p6, char p7, const char* p8 )
        {
            s_pCallsByArity[8] += 1;
            printf( "TestObserver %d: method 8! %d %f %c %s %d %f %c %s\n", m_nId, p1, p2, p3, p4, p5, p6, p7, p8 );
        }
        
        void CM0() const
        {
            s_pCallsByArity[0] += 1;
            printf( "TestObserver %d: const method 0!\n", m_nId );
        }
        
        void CM1( int p1 ) const
        {
            s_pCallsByArity[1] += 1;
            printf( "TestObserver %d: const method 1! %d\n", m_nId, p1 );
        }
        
        void CM2( int p1, float p2 ) const
        {
            s_pCallsByArity[2] += 1;
            printf( "TestObserver %d: const method 2! %d %f\n", m_nId, p1, p2 );
        }
        
        void CM3( int p1, float p2, char p3 ) const
        {
            s_pCallsByArity[3] += 1;
            printf( "TestObserver %d: const method 3! %d %f %c\n", m_nId, p1, p2, p3 );
        }
        
        void CM4( int p1, float p2, char p3, const char* p4 ) const
        {
            s_pCallsByArity[4] += 1;
            printf( "TestObserver %d: const method 4! %d %f %c %s\n", m_nId, p1, p2, p3, p4 );
        }
        
        void CM5( int p1, float p2, char p3, const char* p4, int p5 ) const
        {
            s_pCallsByArity[5] += 1;
            printf( "TestObserver %d: const method 5! %d %f %c %s %d\n", m_nId, p1, p2, p3, p4, p5 );
        }
        
        void CM6( int p1, float p2, char p3, const char* p4, int p5, float p6 ) const
        {
            s_pCallsByArity[6] += 1;
            printf( "TestObserver %d: const method 6! %d %f %c %s %d %f\n", m_nId, p1, p2, p3, p4, p5, p6 );
        }
        
        void CM7( int p1, float p2, char p3, const char* p4, int p5, float p6, char p7 ) const
        {
            s_pCallsByArity[7] += 1;
            printf( "TestObserver %d: const method 7! %d %f %c %s %d %f %c\n", m_nId, p1, p2, p3, p4, p5, p6, p7 );
        }
        
        void CM8( int p1, float p2, char p3, const char* p4, int p5, float p6, char p7, const char* p8 ) const
        {
            s_pCallsByArity[8] += 1;
            printf( "TestObserver %d: const method 8! %d %f %c %s %d %f %c %s\n", m_nId, p1, p2, p3, p4, p5, p6, p7, p8 );
        }
    };
        
    int TestObserver::s_id = 1;
    unsigned TestObserver::s_pCallsByArity[ eSignalMaxArity + 1 ];
} // anonymous namespace

void SignalTest()
{
    // Allocators
    enum { eMaxConnections = 500, eSignalMaxArgs = 8 };
    StaticSignalConnectionPool< eMaxConnections > oSignalPool;
    StaticObserverConnectionPool< eMaxConnections > oSlotPool;
    
    // Signals
    JL_SIGNAL() Sig0( & oSignalPool );
    JL_SIGNAL( int ) Sig1( & oSignalPool );
    JL_SIGNAL( int, float ) Sig2( & oSignalPool );
    JL_SIGNAL( int, float, char ) Sig3( & oSignalPool );
    JL_SIGNAL( int, float, char, const char* ) Sig4( & oSignalPool );
    JL_SIGNAL( int, float, char, const char*, int ) Sig5( & oSignalPool );
    JL_SIGNAL( int, float, char, const char*, int, float ) Sig6( & oSignalPool );
    JL_SIGNAL( int, float, char, const char*, int, float, char ) Sig7( & oSignalPool );
    JL_SIGNAL( int, float, char, const char*, int, float, char, const char* ) Sig8( & oSignalPool );
    
    const SignalBase* const ppSignalsByArity[] = {
        & Sig0, & Sig1, & Sig2, & Sig3, & Sig4, & Sig5, & Sig6, & Sig7, & Sig8
    };
    
    // Observers
    TestObserver pObservers[ 16 ];
    
    // Test connections
    printf( "Connection observer methods to signals...\n" );
    
    for ( int i = 0; i < JL_ARRAY_SIZE(pObservers); ++i )
    {
        pObservers[i].SetSlotConnectionAllocator( & oSlotPool );
        
        Sig0.Connect( & pObservers[i], & TestObserver::M0 );
        Sig1.Connect( & pObservers[i], & TestObserver::M1 );
        
        const int nIndex = i + 1;
        
        if ( nIndex % 2 == 0 )
        {
            Sig2.Connect( & pObservers[i], & TestObserver::M2 );
        }
        
        if ( nIndex % 3 == 0 )
        {
            Sig3.Connect( & pObservers[i], & TestObserver::M3 );
        }
        
        if ( nIndex % 4 == 0 )
        {
            Sig4.Connect( & pObservers[i], & TestObserver::M4 );
        }
        
        if ( nIndex % 5 == 0 )
        {
            Sig5.Connect( & pObservers[i], & TestObserver::M5 );
        }
        
        if ( nIndex % 6 == 0 )
        {
            Sig6.Connect( & pObservers[i], & TestObserver::M6 );
        }
        
        if ( nIndex % 7 == 0 )
        {
            Sig7.Connect( & pObservers[i], & TestObserver::M7 );
        }
        
        if ( nIndex % 8 == 0 )
        {
            Sig8.Connect( & pObservers[i], & TestObserver::M8 );
        }
    }
    
    printf( "Firing signals...\n" );
    
    // Zero the received call count
    TestObserver::ResetCallsByArity();
    
    // Emit signals
    Sig0();
    Sig1( 1 );
    Sig2( 1, 2.0f );
    Sig3( 1, 2.0f, 'T' );
    Sig4( 1, 2.0f, 'T', "Four" );
    Sig5( 1, 2.0f, 'T', "Four", 5 );
    Sig6( 1, 2.0f, 'T', "Four", 5, 6.0f );
    Sig7( 1, 2.0f, 'T', "Four", 5, 6.0f, 'S' );
    Sig8( 1, 2.0f, 'T', "Four", 5, 6.0f, 'S', "Eight" );
    
    // Verify that the observer count is equal to the received call count
    for ( unsigned i = 0; i < JL_ARRAY_SIZE(ppSignalsByArity); ++i )
    {
        const unsigned nObservers = ppSignalsByArity[i]->CountObservers();
        const unsigned nCalls = TestObserver::CountCallsByArity( i );
        printf( "Arity %d, Observers: %d, Calls: %d\n", i, nObservers, nCalls);
        assert( ppSignalsByArity[i]->CountObservers() == TestObserver::CountCallsByArity(i) );
    }  
    
    // Test const connections
    printf( "Connecting const observer methods to signals...\n" );
    
    for ( int i = 0; i < JL_ARRAY_SIZE(pObservers); ++i )
    {
        Sig0.Connect( & pObservers[i], & TestObserver::CM0 );
        Sig1.Connect( & pObservers[i], & TestObserver::CM1 );
        
        const int nIndex = (JL_ARRAY_SIZE(pObservers) + 1 - i);
        
        if ( nIndex % 2 == 0 )
        {           
            Sig2.Connect( & pObservers[i], & TestObserver::CM2 );
        }
        
        if ( nIndex % 3 == 0 )
        {
            Sig3.Connect( & pObservers[i], & TestObserver::CM3 );
        }
        
        if ( nIndex % 4 == 0 )
        {
            Sig4.Connect( & pObservers[i], & TestObserver::CM4 );
        }
        
        if ( nIndex % 5 == 0 )
        {      
            Sig5.Connect( & pObservers[i], & TestObserver::CM5 );
        }
        
        if ( nIndex % 6 == 0 )
        {      
            Sig6.Connect( & pObservers[i], & TestObserver::CM6 );
        }
        
        if ( nIndex % 7 == 0 )
        {      
            Sig7.Connect( & pObservers[i], & TestObserver::CM7 );
        }
        
        if ( nIndex % 8 == 0 )
        {         
            Sig8.Connect( & pObservers[i], & TestObserver::CM8 );
        }
    }
    
    printf( "Firing signals...\n" );
    
    // Zero the received call count
    TestObserver::ResetCallsByArity();
    
    // Emit signals
    Sig0();
    Sig1( 1 );
    Sig2( 1, 2.0f );
    Sig3( 1, 2.0f, 'T' );
    Sig4( 1, 2.0f, 'T', "Four" );
    Sig5( 1, 2.0f, 'T', "Four", 5 );
    Sig6( 1, 2.0f, 'T', "Four", 5, 6.0f );
    Sig7( 1, 2.0f, 'T', "Four", 5, 6.0f, 'S' );
    Sig8( 1, 2.0f, 'T', "Four", 5, 6.0f, 'S', "Eight" );
    
    // Verify that the observer count is equal to the received call count
    for ( unsigned i = 0; i < JL_ARRAY_SIZE(ppSignalsByArity); ++i )
    {
        const unsigned nObservers = ppSignalsByArity[i]->CountObservers();
        const unsigned nCalls = TestObserver::CountCallsByArity( i );
        printf( "Arity %d, Observers: %d, Calls: %d\n", i, nObservers, nCalls);
        assert( ppSignalsByArity[i]->CountObservers() == TestObserver::CountCallsByArity(i) );
    }
    
    // Test disconnections
    printf( "Disconnecting observer methods to signals...\n" );
    
    // NOTE: Signal::Disconnect() will fully disconnect an observer from a signal.
    // If an observer is connected to a signal more than once, each connection will
    // we broken.
    for ( int i = 0; i < JL_ARRAY_SIZE(pObservers); ++i )
    {
        Sig0.Disconnect( & pObservers[i] );
        Sig1.Disconnect( & pObservers[i] );
        
        const int nIndex = i + 1;
        
        if ( nIndex % 2 == 0 )
        {
            Sig2.Disconnect( & pObservers[i] );
        }
        
        if ( nIndex % 3 == 0 )
        {
            Sig3.Disconnect( & pObservers[i] );
        }
        
        if ( nIndex % 4 == 0 )
        {
            Sig4.Disconnect( & pObservers[i] );
        }
        
        if ( nIndex % 5 == 0 )
        {
            Sig5.Disconnect( & pObservers[i] );
        }
        
        if ( nIndex % 6 == 0 )
        {
            Sig6.Disconnect( & pObservers[i] );
        }
        
        if ( nIndex % 7 == 0 )
        {
            Sig7.Disconnect( & pObservers[i] );
        }
        
        if ( nIndex % 8 == 0 )
        {
            Sig8.Disconnect( & pObservers[i] );
        }
    }
    
    printf( "Firing signals...\n" );
    
    // Zero the received call count
    TestObserver::ResetCallsByArity();    
    
    // Emit signals
    Sig0();
    Sig1( 1 );
    Sig2( 1, 2.0f );
    Sig3( 1, 2.0f, 'T' );
    Sig4( 1, 2.0f, 'T', "Four" );
    Sig5( 1, 2.0f, 'T', "Four", 5 );
    Sig6( 1, 2.0f, 'T', "Four", 5, 6.0f );
    Sig7( 1, 2.0f, 'T', "Four", 5, 6.0f, 'S' );
    Sig8( 1, 2.0f, 'T', "Four", 5, 6.0f, 'S', "Eight" );
    
    // Verify that the observer count is equal to the received call count
    for ( unsigned i = 0; i < JL_ARRAY_SIZE(ppSignalsByArity); ++i )
    {
        const unsigned nObservers = ppSignalsByArity[i]->CountObservers();
        const unsigned nCalls = TestObserver::CountCallsByArity( i );
        printf( "Arity %d, Observers: %d, Calls: %d\n", i, nObservers, nCalls);
        assert( ppSignalsByArity[i]->CountObservers() == TestObserver::CountCallsByArity(i) );
    }      
    
    printf( "Disconnecting const observer methods to signals...\n" );
    
    for ( int i = 0; i < JL_ARRAY_SIZE(pObservers); ++i )
    {
        Sig0.Disconnect( & pObservers[i] );
        Sig1.Disconnect( & pObservers[i] );
        
        const int nIndex = (JL_ARRAY_SIZE(pObservers) + 1 - i);
        
        if ( nIndex % 2 == 0 )
        {
            Sig2.Disconnect( & pObservers[i] );
        }
        
        if ( nIndex % 3 == 0 )
        {
            Sig3.Disconnect( & pObservers[i] );
        }
        
        if ( nIndex % 4 == 0 )
        {
            Sig4.Disconnect( & pObservers[i] );
        }
        
        if ( nIndex % 5 == 0 )
        {
            Sig5.Disconnect( & pObservers[i] );
        }
        
        if ( nIndex % 6 == 0 )
        {
            Sig6.Disconnect( & pObservers[i] );
        }
        
        if ( nIndex % 7 == 0 )
        {
            Sig7.Disconnect( & pObservers[i] );
        }
        
        if ( nIndex % 8 == 0 )
        {
            Sig8.Disconnect( & pObservers[i] );
        }
    }
    
    printf( "Firing signals...\n" );

    // Zero the received call count
    TestObserver::ResetCallsByArity();      
    
    // Emit signals
    Sig0();
    Sig1( 1 );
    Sig2( 1, 2.0f );
    Sig3( 1, 2.0f, 'T' );
    Sig4( 1, 2.0f, 'T', "Four" );
    Sig5( 1, 2.0f, 'T', "Four", 5 );
    Sig6( 1, 2.0f, 'T', "Four", 5, 6.0f );
    Sig7( 1, 2.0f, 'T', "Four", 5, 6.0f, 'S' );
    Sig8( 1, 2.0f, 'T', "Four", 5, 6.0f, 'S', "Eight" );
    
    // Verify that the observer count is equal to the received call count
    for ( unsigned i = 0; i < JL_ARRAY_SIZE(ppSignalsByArity); ++i )
    {
        const unsigned nObservers = ppSignalsByArity[i]->CountObservers();
        const unsigned nCalls = TestObserver::CountCallsByArity( i );
        printf( "Arity %d, Observers: %d, Calls: %d\n", i, nObservers, nCalls);
        assert( ppSignalsByArity[i]->CountObservers() == TestObserver::CountCallsByArity(i) );
    }    
}