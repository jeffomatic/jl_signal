#ifndef _JL_SIGNAL_DEFINITIONS_H_
#define _JL_SIGNAL_DEFINITIONS_H_

#include "FastDelegate.h"
#include "Utils.h"
#include "SignalBase.h"

/**
 * The content of the following classes (Signal0 -> Signal8) is identical,
 * except for the number of parameters specified in the Connect() and Emit()
 * functions.
 */

#ifdef JL_SIGNAL_ENABLE_LOGSPAM
#include <stdio.h>
#define JL_SIGNAL_LOG( ... ) printf( __VA_ARGS__ )
#else
#define JL_SIGNAL_LOG( ... )
#endif

#if defined( JL_SIGNAL_ASSERT_ON_DOUBLE_CONNECT )
#define JL_SIGNAL_DOUBLE_CONNECTED_FUNCTION_ASSERT( _function ) JL_ASSERT( ! IsConnected(_function) )
#define JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( _obj, _method ) JL_ASSERT( ! IsConnected(_obj, _method) )
#else
#define JL_SIGNAL_DOUBLE_CONNECTED_FUNCTION_ASSERT( _function )
#define JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( _obj, _method )
#endif

namespace jl {

#ifdef FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX
// Forward-declare a variable-signature template
template<typename _Signature>
class Signal;
#endif

/**
 * Signal0: signals with 0 arguments
 */
template< typename _NoParam = void >
class Signal0 : public SignalBase
{
public:
    typedef fastdelegate::FastDelegate0< void > Delegate;
    
    struct Connection
    {
        Delegate d;
        SignalObserver* pObserver;
    };
    
    typedef DoublyLinkedList<Connection> ConnectionList;
    
    enum { eAllocationSize = sizeof(typename ConnectionList::Node) };
    
private:
    typedef typename ConnectionList::iterator ConnectionIter;
    typedef typename ConnectionList::const_iterator ConnectionConstIter;
    
    ConnectionList m_oConnections;
    
public:
    Signal0() { SetAllocator( s_pCommonAllocator ); }
    Signal0( ScopedAllocator* pAllocator ) { SetAllocator( pAllocator ); }
    
    virtual ~Signal0()
    {
        JL_SIGNAL_LOG( "Destroying signal %p\n", this );
        DisconnectAll();
    }
    
    void SetAllocator( ScopedAllocator* pAllocator ) { m_oConnections.Init( pAllocator ); }    
    unsigned CountConnections() const { return m_oConnections.Count(); }
    
    // Connects non-instance functions.
    template< typename Unused >
    void Connect( Unused (*fpFunction)(void) )
    {
        JL_SIGNAL_DOUBLE_CONNECTED_FUNCTION_ASSERT( fpFunction );
        JL_SIGNAL_LOG( "Signal %p connection to non-instance function %p", this, BruteForceCast<void*>(fpFunction) );
        
        Connection c = { Delegate(fpFunction), NULL };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
    }    
    
    // Connects instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(void) )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Connects const instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(void) const )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Returns true if the given observer and non-instance function are connected to this signal.
    template< typename Unused >
    bool IsConnected( Unused (*fpFunction)(void) ) const
    {
        return IsConnected( Delegate(fpFunction) );
    }    
    
    // Returns true if the given observer and instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(void) ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    // Returns true if the given observer and const instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(void) const ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    void Emit( void ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            (*i).d();
        }
    }
    
    void operator()( void ) const { Emit(); }
    
    // Disconnects a non-instance method.
    template< typename Unused >
    void Disconnect( Unused (*fpFunction)(void) )
    {
        Delegate test( fpFunction );
        
        JL_SIGNAL_LOG( "Signal %p removing connections to non-instance method %p\n", this, BruteForceCast<void*>(fpFunction) );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).d == test )
            {
                JL_ASSERT( (*i).pObserver == NULL );
                JL_SIGNAL_LOG( "\tRemoving connection to non-instance method\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }    
    
    // Disconnects all connected instance methods from a single observer    
    void Disconnect( SignalObserver* pObserver )
    {
        if ( ! pObserver )
        {
            return;
        }
        
        JL_SIGNAL_LOG( "Signal %p removing connections to Observer %p\n", this, pObserver );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\tRemoving connection to observer\n" );
                
                m_oConnections.Remove( i );
                NotifyObserverDisconnect( pObserver );
            }
            else
            {
                ++i;
            }
        }
    }
    
    void DisconnectAll()
    {
        JL_SIGNAL_LOG( "Signal %p disconnecting all observers\n", this );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); ++i )
        {
            SignalObserver* pObserver = (*i).pObserver;
            if ( pObserver )
            {
                NotifyObserverDisconnect( pObserver );
            }
        }
        
        m_oConnections.Clear();
    }
    
private:
    bool IsConnected( const Delegate& d ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            if ( (*i).d == d )
            {
                return true;
            }
        }
        
        return false;        
    }    
    
    void OnObserverDisconnect( SignalObserver* pObserver )
    {
        JL_SIGNAL_LOG( "\tSignal %p received disconnect message from observer %p\n", this, pObserver );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\t\tRemoving connection to observer\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }
};

#ifdef FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX

/**
 * Signal<[0 arguments]>: wrapper class for Signal0
 * This wrapper template, in conjunction with the "Signal" macro, will allow you to ignore the argument count in the signal typename.
 */
template< typename TUnused >
class Signal< TUnused(void) > : public Signal0< void >
{
public:
    typedef Signal0< void > TParent;
    Signal() {}
    Signal( ScopedAllocator* pNodeAllocator ) : TParent(pNodeAllocator) {}
    void operator=( const TParent& other ) { *static_cast<TParent*>(this) = other; }
};

#endif // defined( FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX )

/**
 * Signal1: signals with 1 argument
 */
template< typename _P1 >
class Signal1 : public SignalBase
{
public:
    typedef fastdelegate::FastDelegate1< _P1 > Delegate;
    
    struct Connection
    {
        Delegate d;
        SignalObserver* pObserver;
    };
    
    typedef DoublyLinkedList<Connection> ConnectionList;
    
    enum { eAllocationSize = sizeof(typename ConnectionList::Node) };
    
private:
    typedef typename ConnectionList::iterator ConnectionIter;
    typedef typename ConnectionList::const_iterator ConnectionConstIter;
    
    ConnectionList m_oConnections;
    
public:
    Signal1() { SetAllocator( s_pCommonAllocator ); }
    Signal1( ScopedAllocator* pAllocator ) { SetAllocator( pAllocator ); }
    
    virtual ~Signal1()
    {
        JL_SIGNAL_LOG( "Destroying signal %p\n", this );
        DisconnectAll();
    }
    
    void SetAllocator( ScopedAllocator* pAllocator ) { m_oConnections.Init( pAllocator ); }    
    unsigned CountConnections() const { return m_oConnections.Count(); }
    
    // Connects non-instance functions.
    template< typename Unused >
    void Connect( Unused (*fpFunction)(_P1) )
    {
        JL_SIGNAL_DOUBLE_CONNECTED_FUNCTION_ASSERT( fpFunction );
        JL_SIGNAL_LOG( "Signal %p connection to non-instance function %p", this, BruteForceCast<void*>(fpFunction) );
        
        Connection c = { Delegate(fpFunction), NULL };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
    }    
    
    // Connects instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1) )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Connects const instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1) const )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Returns true if the given observer and non-instance function are connected to this signal.
    template< typename Unused >
    bool IsConnected( Unused (*fpFunction)(_P1) ) const
    {
        return IsConnected( Delegate(fpFunction) );
    }    
    
    // Returns true if the given observer and instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1) ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    // Returns true if the given observer and const instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1) const ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    void Emit( _P1 p1 ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            (*i).d( p1 );
        }
    }
    
    void operator()( _P1 p1 ) const { Emit( p1 ); }
    
    // Disconnects a non-instance method.
    template< typename Unused >
    void Disconnect( Unused (*fpFunction)(_P1) )
    {
        Delegate test( fpFunction );
        
        JL_SIGNAL_LOG( "Signal %p removing connections to non-instance method %p\n", this, BruteForceCast<void*>(fpFunction) );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).d == test )
            {
                JL_ASSERT( (*i).pObserver == NULL );
                JL_SIGNAL_LOG( "\tRemoving connection to non-instance method\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }    
    
    // Disconnects all connected instance methods from a single observer    
    void Disconnect( SignalObserver* pObserver )
    {
        if ( ! pObserver )
        {
            return;
        }
        
        JL_SIGNAL_LOG( "Signal %p removing connections to Observer %p\n", this, pObserver );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\tRemoving connection to observer\n" );
                
                m_oConnections.Remove( i );
                NotifyObserverDisconnect( pObserver );
            }
            else
            {
                ++i;
            }
        }
    }
    
    void DisconnectAll()
    {
        JL_SIGNAL_LOG( "Signal %p disconnecting all observers\n", this );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); ++i )
        {
            SignalObserver* pObserver = (*i).pObserver;
            if ( pObserver )
            {
                NotifyObserverDisconnect( pObserver );
            }
        }
        
        m_oConnections.Clear();
    }
    
private:
    bool IsConnected( const Delegate& d ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            if ( (*i).d == d )
            {
                return true;
            }
        }
        
        return false;        
    }    
    
    void OnObserverDisconnect( SignalObserver* pObserver )
    {
        JL_SIGNAL_LOG( "\tSignal %p received disconnect message from observer %p\n", this, pObserver );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\t\tRemoving connection to observer\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }
};

#ifdef FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX

/**
 * Signal<[1 argument]>: wrapper class for Signal1
 * This wrapper template, in conjunction with the "Signal" macro, will allow you to ignore the argument count in the signal typename.
 */
template< typename TUnused, typename _P1 >
class Signal< TUnused(_P1) > : public Signal1< _P1 >
{
public:
    typedef Signal1< _P1 > TParent;
    Signal() {}
    Signal( ScopedAllocator* pNodeAllocator ) : TParent(pNodeAllocator) {}
    void operator=( const TParent& other ) { *static_cast<TParent*>(this) = other; }
};

#endif // defined( FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX )

/**
 * Signal2: signals with 2 arguments
 */
template< typename _P1, typename _P2 >
class Signal2 : public SignalBase
{
public:
    typedef fastdelegate::FastDelegate2< _P1, _P2 > Delegate;
    
    struct Connection
    {
        Delegate d;
        SignalObserver* pObserver;
    };
    
    typedef DoublyLinkedList<Connection> ConnectionList;
    
    enum { eAllocationSize = sizeof(typename ConnectionList::Node) };
    
private:
    typedef typename ConnectionList::iterator ConnectionIter;
    typedef typename ConnectionList::const_iterator ConnectionConstIter;
    
    ConnectionList m_oConnections;
    
public:
    Signal2() { SetAllocator( s_pCommonAllocator ); }
    Signal2( ScopedAllocator* pAllocator ) { SetAllocator( pAllocator ); }
    
    virtual ~Signal2()
    {
        JL_SIGNAL_LOG( "Destroying signal %p\n", this );
        DisconnectAll();
    }
    
    void SetAllocator( ScopedAllocator* pAllocator ) { m_oConnections.Init( pAllocator ); }    
    unsigned CountConnections() const { return m_oConnections.Count(); }
    
    // Connects non-instance functions.
    template< typename Unused >
    void Connect( Unused (*fpFunction)(_P1, _P2) )
    {
        JL_SIGNAL_DOUBLE_CONNECTED_FUNCTION_ASSERT( fpFunction );
        JL_SIGNAL_LOG( "Signal %p connection to non-instance function %p", this, BruteForceCast<void*>(fpFunction) );
        
        Connection c = { Delegate(fpFunction), NULL };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
    }    
    
    // Connects instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1, _P2) )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Connects const instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1, _P2) const )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Returns true if the given observer and non-instance function are connected to this signal.
    template< typename Unused >
    bool IsConnected( Unused (*fpFunction)(_P1, _P2) ) const
    {
        return IsConnected( Delegate(fpFunction) );
    }    
    
    // Returns true if the given observer and instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1, _P2) ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    // Returns true if the given observer and const instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1, _P2) const ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    void Emit( _P1 p1, _P2 p2 ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            (*i).d( p1, p2 );
        }
    }
    
    void operator()( _P1 p1, _P2 p2 ) const { Emit( p1, p2 ); }
    
    // Disconnects a non-instance method.
    template< typename Unused >
    void Disconnect( Unused (*fpFunction)(_P1, _P2) )
    {
        Delegate test( fpFunction );
        
        JL_SIGNAL_LOG( "Signal %p removing connections to non-instance method %p\n", this, BruteForceCast<void*>(fpFunction) );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).d == test )
            {
                JL_ASSERT( (*i).pObserver == NULL );
                JL_SIGNAL_LOG( "\tRemoving connection to non-instance method\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }    
    
    // Disconnects all connected instance methods from a single observer    
    void Disconnect( SignalObserver* pObserver )
    {
        if ( ! pObserver )
        {
            return;
        }
        
        JL_SIGNAL_LOG( "Signal %p removing connections to Observer %p\n", this, pObserver );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\tRemoving connection to observer\n" );
                
                m_oConnections.Remove( i );
                NotifyObserverDisconnect( pObserver );
            }
            else
            {
                ++i;
            }
        }
    }
    
    void DisconnectAll()
    {
        JL_SIGNAL_LOG( "Signal %p disconnecting all observers\n", this );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); ++i )
        {
            SignalObserver* pObserver = (*i).pObserver;
            if ( pObserver )
            {
                NotifyObserverDisconnect( pObserver );
            }
        }
        
        m_oConnections.Clear();
    }
    
private:
    bool IsConnected( const Delegate& d ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            if ( (*i).d == d )
            {
                return true;
            }
        }
        
        return false;        
    }    
    
    void OnObserverDisconnect( SignalObserver* pObserver )
    {
        JL_SIGNAL_LOG( "\tSignal %p received disconnect message from observer %p\n", this, pObserver );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\t\tRemoving connection to observer\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }
};

#ifdef FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX

/**
 * Signal<[2 arguments]>: wrapper class for Signal2
 * This wrapper template, in conjunction with the "Signal" macro, will allow you to ignore the argument count in the signal typename.
 */
template< typename TUnused, typename _P1, typename _P2 >
class Signal< TUnused(_P1, _P2) > : public Signal2< _P1, _P2 >
{
public:
    typedef Signal2< _P1, _P2 > TParent;
    Signal() {}
    Signal( ScopedAllocator* pNodeAllocator ) : TParent(pNodeAllocator) {}
    void operator=( const TParent& other ) { *static_cast<TParent*>(this) = other; }
};

#endif // defined( FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX )

/**
 * Signal3: signals with 3 arguments
 */
template< typename _P1, typename _P2, typename _P3 >
class Signal3 : public SignalBase
{
public:
    typedef fastdelegate::FastDelegate3< _P1, _P2, _P3 > Delegate;
    
    struct Connection
    {
        Delegate d;
        SignalObserver* pObserver;
    };
    
    typedef DoublyLinkedList<Connection> ConnectionList;
    
    enum { eAllocationSize = sizeof(typename ConnectionList::Node) };
    
private:
    typedef typename ConnectionList::iterator ConnectionIter;
    typedef typename ConnectionList::const_iterator ConnectionConstIter;
    
    ConnectionList m_oConnections;
    
public:
    Signal3() { SetAllocator( s_pCommonAllocator ); }
    Signal3( ScopedAllocator* pAllocator ) { SetAllocator( pAllocator ); }
    
    virtual ~Signal3()
    {
        JL_SIGNAL_LOG( "Destroying signal %p\n", this );
        DisconnectAll();
    }
    
    void SetAllocator( ScopedAllocator* pAllocator ) { m_oConnections.Init( pAllocator ); }    
    unsigned CountConnections() const { return m_oConnections.Count(); }
    
    // Connects non-instance functions.
    template< typename Unused >
    void Connect( Unused (*fpFunction)(_P1, _P2, _P3) )
    {
        JL_SIGNAL_DOUBLE_CONNECTED_FUNCTION_ASSERT( fpFunction );
        JL_SIGNAL_LOG( "Signal %p connection to non-instance function %p", this, BruteForceCast<void*>(fpFunction) );
        
        Connection c = { Delegate(fpFunction), NULL };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
    }    
    
    // Connects instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3) )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Connects const instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3) const )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Returns true if the given observer and non-instance function are connected to this signal.
    template< typename Unused >
    bool IsConnected( Unused (*fpFunction)(_P1, _P2, _P3) ) const
    {
        return IsConnected( Delegate(fpFunction) );
    }    
    
    // Returns true if the given observer and instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3) ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    // Returns true if the given observer and const instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3) const ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    void Emit( _P1 p1, _P2 p2, _P3 p3 ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            (*i).d( p1, p2, p3 );
        }
    }
    
    void operator()( _P1 p1, _P2 p2, _P3 p3 ) const { Emit( p1, p2, p3 ); }
    
    // Disconnects a non-instance method.
    template< typename Unused >
    void Disconnect( Unused (*fpFunction)(_P1, _P2, _P3) )
    {
        Delegate test( fpFunction );
        
        JL_SIGNAL_LOG( "Signal %p removing connections to non-instance method %p\n", this, BruteForceCast<void*>(fpFunction) );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).d == test )
            {
                JL_ASSERT( (*i).pObserver == NULL );
                JL_SIGNAL_LOG( "\tRemoving connection to non-instance method\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }    
    
    // Disconnects all connected instance methods from a single observer    
    void Disconnect( SignalObserver* pObserver )
    {
        if ( ! pObserver )
        {
            return;
        }
        
        JL_SIGNAL_LOG( "Signal %p removing connections to Observer %p\n", this, pObserver );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\tRemoving connection to observer\n" );
                
                m_oConnections.Remove( i );
                NotifyObserverDisconnect( pObserver );
            }
            else
            {
                ++i;
            }
        }
    }
    
    void DisconnectAll()
    {
        JL_SIGNAL_LOG( "Signal %p disconnecting all observers\n", this );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); ++i )
        {
            SignalObserver* pObserver = (*i).pObserver;
            if ( pObserver )
            {
                NotifyObserverDisconnect( pObserver );
            }
        }
        
        m_oConnections.Clear();
    }
    
private:
    bool IsConnected( const Delegate& d ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            if ( (*i).d == d )
            {
                return true;
            }
        }
        
        return false;        
    }    
    
    void OnObserverDisconnect( SignalObserver* pObserver )
    {
        JL_SIGNAL_LOG( "\tSignal %p received disconnect message from observer %p\n", this, pObserver );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\t\tRemoving connection to observer\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }
};

#ifdef FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX

/**
 * Signal<[3 arguments]>: wrapper class for Signal3
 * This wrapper template, in conjunction with the "Signal" macro, will allow you to ignore the argument count in the signal typename.
 */
template< typename TUnused, typename _P1, typename _P2, typename _P3 >
class Signal< TUnused(_P1, _P2, _P3) > : public Signal3< _P1, _P2, _P3 >
{
public:
    typedef Signal3< _P1, _P2, _P3 > TParent;
    Signal() {}
    Signal( ScopedAllocator* pNodeAllocator ) : TParent(pNodeAllocator) {}
    void operator=( const TParent& other ) { *static_cast<TParent*>(this) = other; }
};

#endif // defined( FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX )

/**
 * Signal4: signals with 4 arguments
 */
template< typename _P1, typename _P2, typename _P3, typename _P4 >
class Signal4 : public SignalBase
{
public:
    typedef fastdelegate::FastDelegate4< _P1, _P2, _P3, _P4 > Delegate;
    
    struct Connection
    {
        Delegate d;
        SignalObserver* pObserver;
    };
    
    typedef DoublyLinkedList<Connection> ConnectionList;
    
    enum { eAllocationSize = sizeof(typename ConnectionList::Node) };
    
private:
    typedef typename ConnectionList::iterator ConnectionIter;
    typedef typename ConnectionList::const_iterator ConnectionConstIter;
    
    ConnectionList m_oConnections;
    
public:
    Signal4() { SetAllocator( s_pCommonAllocator ); }
    Signal4( ScopedAllocator* pAllocator ) { SetAllocator( pAllocator ); }
    
    virtual ~Signal4()
    {
        JL_SIGNAL_LOG( "Destroying signal %p\n", this );
        DisconnectAll();
    }
    
    void SetAllocator( ScopedAllocator* pAllocator ) { m_oConnections.Init( pAllocator ); }    
    unsigned CountConnections() const { return m_oConnections.Count(); }
    
    // Connects non-instance functions.
    template< typename Unused >
    void Connect( Unused (*fpFunction)(_P1, _P2, _P3, _P4) )
    {
        JL_SIGNAL_DOUBLE_CONNECTED_FUNCTION_ASSERT( fpFunction );
        JL_SIGNAL_LOG( "Signal %p connection to non-instance function %p", this, BruteForceCast<void*>(fpFunction) );
        
        Connection c = { Delegate(fpFunction), NULL };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
    }    
    
    // Connects instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4) )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Connects const instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4) const )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Returns true if the given observer and non-instance function are connected to this signal.
    template< typename Unused >
    bool IsConnected( Unused (*fpFunction)(_P1, _P2, _P3, _P4) ) const
    {
        return IsConnected( Delegate(fpFunction) );
    }    
    
    // Returns true if the given observer and instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4) ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    // Returns true if the given observer and const instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4) const ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    void Emit( _P1 p1, _P2 p2, _P3 p3, _P4 p4 ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            (*i).d( p1, p2, p3, p4 );
        }
    }
    
    void operator()( _P1 p1, _P2 p2, _P3 p3, _P4 p4 ) const { Emit( p1, p2, p3, p4 ); }
    
    // Disconnects a non-instance method.
    template< typename Unused >
    void Disconnect( Unused (*fpFunction)(_P1, _P2, _P3, _P4) )
    {
        Delegate test( fpFunction );
        
        JL_SIGNAL_LOG( "Signal %p removing connections to non-instance method %p\n", this, BruteForceCast<void*>(fpFunction) );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).d == test )
            {
                JL_ASSERT( (*i).pObserver == NULL );
                JL_SIGNAL_LOG( "\tRemoving connection to non-instance method\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }    
    
    // Disconnects all connected instance methods from a single observer    
    void Disconnect( SignalObserver* pObserver )
    {
        if ( ! pObserver )
        {
            return;
        }
        
        JL_SIGNAL_LOG( "Signal %p removing connections to Observer %p\n", this, pObserver );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\tRemoving connection to observer\n" );
                
                m_oConnections.Remove( i );
                NotifyObserverDisconnect( pObserver );
            }
            else
            {
                ++i;
            }
        }
    }
    
    void DisconnectAll()
    {
        JL_SIGNAL_LOG( "Signal %p disconnecting all observers\n", this );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); ++i )
        {
            SignalObserver* pObserver = (*i).pObserver;
            if ( pObserver )
            {
                NotifyObserverDisconnect( pObserver );
            }
        }
        
        m_oConnections.Clear();
    }
    
private:
    bool IsConnected( const Delegate& d ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            if ( (*i).d == d )
            {
                return true;
            }
        }
        
        return false;        
    }    
    
    void OnObserverDisconnect( SignalObserver* pObserver )
    {
        JL_SIGNAL_LOG( "\tSignal %p received disconnect message from observer %p\n", this, pObserver );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\t\tRemoving connection to observer\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }
};

#ifdef FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX

/**
 * Signal<[4 arguments]>: wrapper class for Signal4
 * This wrapper template, in conjunction with the "Signal" macro, will allow you to ignore the argument count in the signal typename.
 */
template< typename TUnused, typename _P1, typename _P2, typename _P3, typename _P4 >
class Signal< TUnused(_P1, _P2, _P3, _P4) > : public Signal4< _P1, _P2, _P3, _P4 >
{
public:
    typedef Signal4< _P1, _P2, _P3, _P4 > TParent;
    Signal() {}
    Signal( ScopedAllocator* pNodeAllocator ) : TParent(pNodeAllocator) {}
    void operator=( const TParent& other ) { *static_cast<TParent*>(this) = other; }
};

#endif // defined( FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX )

/**
 * Signal5: signals with 5 arguments
 */
template< typename _P1, typename _P2, typename _P3, typename _P4, typename _P5 >
class Signal5 : public SignalBase
{
public:
    typedef fastdelegate::FastDelegate5< _P1, _P2, _P3, _P4, _P5 > Delegate;
    
    struct Connection
    {
        Delegate d;
        SignalObserver* pObserver;
    };
    
    typedef DoublyLinkedList<Connection> ConnectionList;
    
    enum { eAllocationSize = sizeof(typename ConnectionList::Node) };
    
private:
    typedef typename ConnectionList::iterator ConnectionIter;
    typedef typename ConnectionList::const_iterator ConnectionConstIter;
    
    ConnectionList m_oConnections;
    
public:
    Signal5() { SetAllocator( s_pCommonAllocator ); }
    Signal5( ScopedAllocator* pAllocator ) { SetAllocator( pAllocator ); }
    
    virtual ~Signal5()
    {
        JL_SIGNAL_LOG( "Destroying signal %p\n", this );
        DisconnectAll();
    }
    
    void SetAllocator( ScopedAllocator* pAllocator ) { m_oConnections.Init( pAllocator ); }    
    unsigned CountConnections() const { return m_oConnections.Count(); }
    
    // Connects non-instance functions.
    template< typename Unused >
    void Connect( Unused (*fpFunction)(_P1, _P2, _P3, _P4, _P5) )
    {
        JL_SIGNAL_DOUBLE_CONNECTED_FUNCTION_ASSERT( fpFunction );
        JL_SIGNAL_LOG( "Signal %p connection to non-instance function %p", this, BruteForceCast<void*>(fpFunction) );
        
        Connection c = { Delegate(fpFunction), NULL };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
    }    
    
    // Connects instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5) )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Connects const instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5) const )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Returns true if the given observer and non-instance function are connected to this signal.
    template< typename Unused >
    bool IsConnected( Unused (*fpFunction)(_P1, _P2, _P3, _P4, _P5) ) const
    {
        return IsConnected( Delegate(fpFunction) );
    }    
    
    // Returns true if the given observer and instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5) ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    // Returns true if the given observer and const instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5) const ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    void Emit( _P1 p1, _P2 p2, _P3 p3, _P4 p4, _P5 p5 ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            (*i).d( p1, p2, p3, p4, p5 );
        }
    }
    
    void operator()( _P1 p1, _P2 p2, _P3 p3, _P4 p4, _P5 p5 ) const { Emit( p1, p2, p3, p4, p5 ); }
    
    // Disconnects a non-instance method.
    template< typename Unused >
    void Disconnect( Unused (*fpFunction)(_P1, _P2, _P3, _P4, _P5) )
    {
        Delegate test( fpFunction );
        
        JL_SIGNAL_LOG( "Signal %p removing connections to non-instance method %p\n", this, BruteForceCast<void*>(fpFunction) );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).d == test )
            {
                JL_ASSERT( (*i).pObserver == NULL );
                JL_SIGNAL_LOG( "\tRemoving connection to non-instance method\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }    
    
    // Disconnects all connected instance methods from a single observer    
    void Disconnect( SignalObserver* pObserver )
    {
        if ( ! pObserver )
        {
            return;
        }
        
        JL_SIGNAL_LOG( "Signal %p removing connections to Observer %p\n", this, pObserver );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\tRemoving connection to observer\n" );
                
                m_oConnections.Remove( i );
                NotifyObserverDisconnect( pObserver );
            }
            else
            {
                ++i;
            }
        }
    }
    
    void DisconnectAll()
    {
        JL_SIGNAL_LOG( "Signal %p disconnecting all observers\n", this );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); ++i )
        {
            SignalObserver* pObserver = (*i).pObserver;
            if ( pObserver )
            {
                NotifyObserverDisconnect( pObserver );
            }
        }
        
        m_oConnections.Clear();
    }
    
private:
    bool IsConnected( const Delegate& d ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            if ( (*i).d == d )
            {
                return true;
            }
        }
        
        return false;        
    }    
    
    void OnObserverDisconnect( SignalObserver* pObserver )
    {
        JL_SIGNAL_LOG( "\tSignal %p received disconnect message from observer %p\n", this, pObserver );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\t\tRemoving connection to observer\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }
};

#ifdef FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX

/**
 * Signal<[5 arguments]>: wrapper class for Signal5
 * This wrapper template, in conjunction with the "Signal" macro, will allow you to ignore the argument count in the signal typename.
 */
template< typename TUnused, typename _P1, typename _P2, typename _P3, typename _P4, typename _P5 >
class Signal< TUnused(_P1, _P2, _P3, _P4, _P5) > : public Signal5< _P1, _P2, _P3, _P4, _P5 >
{
public:
    typedef Signal5< _P1, _P2, _P3, _P4, _P5 > TParent;
    Signal() {}
    Signal( ScopedAllocator* pNodeAllocator ) : TParent(pNodeAllocator) {}
    void operator=( const TParent& other ) { *static_cast<TParent*>(this) = other; }
};

#endif // defined( FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX )

/**
 * Signal6: signals with 6 arguments
 */
template< typename _P1, typename _P2, typename _P3, typename _P4, typename _P5, typename _P6 >
class Signal6 : public SignalBase
{
public:
    typedef fastdelegate::FastDelegate6< _P1, _P2, _P3, _P4, _P5, _P6 > Delegate;
    
    struct Connection
    {
        Delegate d;
        SignalObserver* pObserver;
    };
    
    typedef DoublyLinkedList<Connection> ConnectionList;
    
    enum { eAllocationSize = sizeof(typename ConnectionList::Node) };
    
private:
    typedef typename ConnectionList::iterator ConnectionIter;
    typedef typename ConnectionList::const_iterator ConnectionConstIter;
    
    ConnectionList m_oConnections;
    
public:
    Signal6() { SetAllocator( s_pCommonAllocator ); }
    Signal6( ScopedAllocator* pAllocator ) { SetAllocator( pAllocator ); }
    
    virtual ~Signal6()
    {
        JL_SIGNAL_LOG( "Destroying signal %p\n", this );
        DisconnectAll();
    }
    
    void SetAllocator( ScopedAllocator* pAllocator ) { m_oConnections.Init( pAllocator ); }    
    unsigned CountConnections() const { return m_oConnections.Count(); }
    
    // Connects non-instance functions.
    template< typename Unused >
    void Connect( Unused (*fpFunction)(_P1, _P2, _P3, _P4, _P5, _P6) )
    {
        JL_SIGNAL_DOUBLE_CONNECTED_FUNCTION_ASSERT( fpFunction );
        JL_SIGNAL_LOG( "Signal %p connection to non-instance function %p", this, BruteForceCast<void*>(fpFunction) );
        
        Connection c = { Delegate(fpFunction), NULL };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
    }    
    
    // Connects instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5, _P6) )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Connects const instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5, _P6) const )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Returns true if the given observer and non-instance function are connected to this signal.
    template< typename Unused >
    bool IsConnected( Unused (*fpFunction)(_P1, _P2, _P3, _P4, _P5, _P6) ) const
    {
        return IsConnected( Delegate(fpFunction) );
    }    
    
    // Returns true if the given observer and instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5, _P6) ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    // Returns true if the given observer and const instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5, _P6) const ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    void Emit( _P1 p1, _P2 p2, _P3 p3, _P4 p4, _P5 p5, _P6 p6 ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            (*i).d( p1, p2, p3, p4, p5, p6 );
        }
    }
    
    void operator()( _P1 p1, _P2 p2, _P3 p3, _P4 p4, _P5 p5, _P6 p6 ) const { Emit( p1, p2, p3, p4, p5, p6 ); }
    
    // Disconnects a non-instance method.
    template< typename Unused >
    void Disconnect( Unused (*fpFunction)(_P1, _P2, _P3, _P4, _P5, _P6) )
    {
        Delegate test( fpFunction );
        
        JL_SIGNAL_LOG( "Signal %p removing connections to non-instance method %p\n", this, BruteForceCast<void*>(fpFunction) );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).d == test )
            {
                JL_ASSERT( (*i).pObserver == NULL );
                JL_SIGNAL_LOG( "\tRemoving connection to non-instance method\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }    
    
    // Disconnects all connected instance methods from a single observer    
    void Disconnect( SignalObserver* pObserver )
    {
        if ( ! pObserver )
        {
            return;
        }
        
        JL_SIGNAL_LOG( "Signal %p removing connections to Observer %p\n", this, pObserver );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\tRemoving connection to observer\n" );
                
                m_oConnections.Remove( i );
                NotifyObserverDisconnect( pObserver );
            }
            else
            {
                ++i;
            }
        }
    }
    
    void DisconnectAll()
    {
        JL_SIGNAL_LOG( "Signal %p disconnecting all observers\n", this );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); ++i )
        {
            SignalObserver* pObserver = (*i).pObserver;
            if ( pObserver )
            {
                NotifyObserverDisconnect( pObserver );
            }
        }
        
        m_oConnections.Clear();
    }
    
private:
    bool IsConnected( const Delegate& d ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            if ( (*i).d == d )
            {
                return true;
            }
        }
        
        return false;        
    }    
    
    void OnObserverDisconnect( SignalObserver* pObserver )
    {
        JL_SIGNAL_LOG( "\tSignal %p received disconnect message from observer %p\n", this, pObserver );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\t\tRemoving connection to observer\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }
};

#ifdef FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX

/**
 * Signal<[6 arguments]>: wrapper class for Signal6
 * This wrapper template, in conjunction with the "Signal" macro, will allow you to ignore the argument count in the signal typename.
 */
template< typename TUnused, typename _P1, typename _P2, typename _P3, typename _P4, typename _P5, typename _P6 >
class Signal< TUnused(_P1, _P2, _P3, _P4, _P5, _P6) > : public Signal6< _P1, _P2, _P3, _P4, _P5, _P6 >
{
public:
    typedef Signal6< _P1, _P2, _P3, _P4, _P5, _P6 > TParent;
    Signal() {}
    Signal( ScopedAllocator* pNodeAllocator ) : TParent(pNodeAllocator) {}
    void operator=( const TParent& other ) { *static_cast<TParent*>(this) = other; }
};

#endif // defined( FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX )

/**
 * Signal7: signals with 7 arguments
 */
template< typename _P1, typename _P2, typename _P3, typename _P4, typename _P5, typename _P6, typename _P7 >
class Signal7 : public SignalBase
{
public:
    typedef fastdelegate::FastDelegate7< _P1, _P2, _P3, _P4, _P5, _P6, _P7 > Delegate;
    
    struct Connection
    {
        Delegate d;
        SignalObserver* pObserver;
    };
    
    typedef DoublyLinkedList<Connection> ConnectionList;
    
    enum { eAllocationSize = sizeof(typename ConnectionList::Node) };
    
private:
    typedef typename ConnectionList::iterator ConnectionIter;
    typedef typename ConnectionList::const_iterator ConnectionConstIter;
    
    ConnectionList m_oConnections;
    
public:
    Signal7() { SetAllocator( s_pCommonAllocator ); }
    Signal7( ScopedAllocator* pAllocator ) { SetAllocator( pAllocator ); }
    
    virtual ~Signal7()
    {
        JL_SIGNAL_LOG( "Destroying signal %p\n", this );
        DisconnectAll();
    }
    
    void SetAllocator( ScopedAllocator* pAllocator ) { m_oConnections.Init( pAllocator ); }    
    unsigned CountConnections() const { return m_oConnections.Count(); }
    
    // Connects non-instance functions.
    template< typename Unused >
    void Connect( Unused (*fpFunction)(_P1, _P2, _P3, _P4, _P5, _P6, _P7) )
    {
        JL_SIGNAL_DOUBLE_CONNECTED_FUNCTION_ASSERT( fpFunction );
        JL_SIGNAL_LOG( "Signal %p connection to non-instance function %p", this, BruteForceCast<void*>(fpFunction) );
        
        Connection c = { Delegate(fpFunction), NULL };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
    }    
    
    // Connects instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5, _P6, _P7) )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Connects const instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5, _P6, _P7) const )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Returns true if the given observer and non-instance function are connected to this signal.
    template< typename Unused >
    bool IsConnected( Unused (*fpFunction)(_P1, _P2, _P3, _P4, _P5, _P6, _P7) ) const
    {
        return IsConnected( Delegate(fpFunction) );
    }    
    
    // Returns true if the given observer and instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5, _P6, _P7) ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    // Returns true if the given observer and const instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5, _P6, _P7) const ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    void Emit( _P1 p1, _P2 p2, _P3 p3, _P4 p4, _P5 p5, _P6 p6, _P7 p7 ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            (*i).d( p1, p2, p3, p4, p5, p6, p7 );
        }
    }
    
    void operator()( _P1 p1, _P2 p2, _P3 p3, _P4 p4, _P5 p5, _P6 p6, _P7 p7 ) const { Emit( p1, p2, p3, p4, p5, p6, p7 ); }
    
    // Disconnects a non-instance method.
    template< typename Unused >
    void Disconnect( Unused (*fpFunction)(_P1, _P2, _P3, _P4, _P5, _P6, _P7) )
    {
        Delegate test( fpFunction );
        
        JL_SIGNAL_LOG( "Signal %p removing connections to non-instance method %p\n", this, BruteForceCast<void*>(fpFunction) );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).d == test )
            {
                JL_ASSERT( (*i).pObserver == NULL );
                JL_SIGNAL_LOG( "\tRemoving connection to non-instance method\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }    
    
    // Disconnects all connected instance methods from a single observer    
    void Disconnect( SignalObserver* pObserver )
    {
        if ( ! pObserver )
        {
            return;
        }
        
        JL_SIGNAL_LOG( "Signal %p removing connections to Observer %p\n", this, pObserver );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\tRemoving connection to observer\n" );
                
                m_oConnections.Remove( i );
                NotifyObserverDisconnect( pObserver );
            }
            else
            {
                ++i;
            }
        }
    }
    
    void DisconnectAll()
    {
        JL_SIGNAL_LOG( "Signal %p disconnecting all observers\n", this );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); ++i )
        {
            SignalObserver* pObserver = (*i).pObserver;
            if ( pObserver )
            {
                NotifyObserverDisconnect( pObserver );
            }
        }
        
        m_oConnections.Clear();
    }
    
private:
    bool IsConnected( const Delegate& d ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            if ( (*i).d == d )
            {
                return true;
            }
        }
        
        return false;        
    }    
    
    void OnObserverDisconnect( SignalObserver* pObserver )
    {
        JL_SIGNAL_LOG( "\tSignal %p received disconnect message from observer %p\n", this, pObserver );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\t\tRemoving connection to observer\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }
};

#ifdef FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX

/**
 * Signal<[7 arguments]>: wrapper class for Signal7
 * This wrapper template, in conjunction with the "Signal" macro, will allow you to ignore the argument count in the signal typename.
 */
template< typename TUnused, typename _P1, typename _P2, typename _P3, typename _P4, typename _P5, typename _P6, typename _P7 >
class Signal< TUnused(_P1, _P2, _P3, _P4, _P5, _P6, _P7) > : public Signal7< _P1, _P2, _P3, _P4, _P5, _P6, _P7 >
{
public:
    typedef Signal7< _P1, _P2, _P3, _P4, _P5, _P6, _P7 > TParent;
    Signal() {}
    Signal( ScopedAllocator* pNodeAllocator ) : TParent(pNodeAllocator) {}
    void operator=( const TParent& other ) { *static_cast<TParent*>(this) = other; }
};

#endif // defined( FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX )

/**
 * Signal8: signals with 8 arguments
 */
template< typename _P1, typename _P2, typename _P3, typename _P4, typename _P5, typename _P6, typename _P7, typename _P8 >
class Signal8 : public SignalBase
{
public:
    typedef fastdelegate::FastDelegate8< _P1, _P2, _P3, _P4, _P5, _P6, _P7, _P8 > Delegate;
    
    struct Connection
    {
        Delegate d;
        SignalObserver* pObserver;
    };
    
    typedef DoublyLinkedList<Connection> ConnectionList;
    
    enum { eAllocationSize = sizeof(typename ConnectionList::Node) };
    
private:
    typedef typename ConnectionList::iterator ConnectionIter;
    typedef typename ConnectionList::const_iterator ConnectionConstIter;
    
    ConnectionList m_oConnections;
    
public:
    Signal8() { SetAllocator( s_pCommonAllocator ); }
    Signal8( ScopedAllocator* pAllocator ) { SetAllocator( pAllocator ); }
    
    virtual ~Signal8()
    {
        JL_SIGNAL_LOG( "Destroying signal %p\n", this );
        DisconnectAll();
    }
    
    void SetAllocator( ScopedAllocator* pAllocator ) { m_oConnections.Init( pAllocator ); }    
    unsigned CountConnections() const { return m_oConnections.Count(); }
    
    // Connects non-instance functions.
    template< typename Unused >
    void Connect( Unused (*fpFunction)(_P1, _P2, _P3, _P4, _P5, _P6, _P7, _P8) )
    {
        JL_SIGNAL_DOUBLE_CONNECTED_FUNCTION_ASSERT( fpFunction );
        JL_SIGNAL_LOG( "Signal %p connection to non-instance function %p", this, BruteForceCast<void*>(fpFunction) );
        
        Connection c = { Delegate(fpFunction), NULL };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
    }    
    
    // Connects instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5, _P6, _P7, _P8) )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Connects const instance methods. Class X should be equal to Y, or an ancestor type.
    template< class X, class Y, typename Unused >
    void Connect( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5, _P6, _P7, _P8) const )
    {
        if ( ! pObject )
        {
            return;
        }
        
        JL_SIGNAL_DOUBLE_CONNECTED_INSTANCE_METHOD_ASSERT( pObject, fpMethod );
        SignalObserver* pObserver = static_cast<SignalObserver*>( pObject );
        JL_SIGNAL_LOG( "Signal %p connecting to Observer %p (object %p, method %p)\n", this, pObserver, pObject, BruteForceCast<void*>(fpMethod) );
        
        Connection c = { fastdelegate::MakeDelegate(pObject, fpMethod), pObserver };
        const bool bAdded = m_oConnections.Add( c );
        JL_ASSERT( bAdded );
        NotifyObserverConnect( pObserver );
    }
    
    // Returns true if the given observer and non-instance function are connected to this signal.
    template< typename Unused >
    bool IsConnected( Unused (*fpFunction)(_P1, _P2, _P3, _P4, _P5, _P6, _P7, _P8) ) const
    {
        return IsConnected( Delegate(fpFunction) );
    }    
    
    // Returns true if the given observer and instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5, _P6, _P7, _P8) ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    // Returns true if the given observer and const instance method are connected to this signal.
    template< class X, class Y, typename Unused >
    bool IsConnected( Y* pObject, Unused (X::*fpMethod)(_P1, _P2, _P3, _P4, _P5, _P6, _P7, _P8) const ) const
    {
        return IsConnected( Delegate(pObject, fpMethod) );
    }
    
    void Emit( _P1 p1, _P2 p2, _P3 p3, _P4 p4, _P5 p5, _P6 p6, _P7 p7, _P8 p8 ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            (*i).d( p1, p2, p3, p4, p5, p6, p7, p8 );
        }
    }
    
    void operator()( _P1 p1, _P2 p2, _P3 p3, _P4 p4, _P5 p5, _P6 p6, _P7 p7, _P8 p8 ) const { Emit( p1, p2, p3, p4, p5, p6, p7, p8 ); }
    
    // Disconnects a non-instance method.
    template< typename Unused >
    void Disconnect( Unused (*fpFunction)(_P1, _P2, _P3, _P4, _P5, _P6, _P7, _P8) )
    {
        Delegate test( fpFunction );
        
        JL_SIGNAL_LOG( "Signal %p removing connections to non-instance method %p\n", this, BruteForceCast<void*>(fpFunction) );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).d == test )
            {
                JL_ASSERT( (*i).pObserver == NULL );
                JL_SIGNAL_LOG( "\tRemoving connection to non-instance method\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }    
    
    // Disconnects all connected instance methods from a single observer    
    void Disconnect( SignalObserver* pObserver )
    {
        if ( ! pObserver )
        {
            return;
        }
        
        JL_SIGNAL_LOG( "Signal %p removing connections to Observer %p\n", this, pObserver );
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\tRemoving connection to observer\n" );
                
                m_oConnections.Remove( i );
                NotifyObserverDisconnect( pObserver );
            }
            else
            {
                ++i;
            }
        }
    }
    
    void DisconnectAll()
    {
        JL_SIGNAL_LOG( "Signal %p disconnecting all observers\n", this );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); ++i )
        {
            SignalObserver* pObserver = (*i).pObserver;
            if ( pObserver )
            {
                NotifyObserverDisconnect( pObserver );
            }
        }
        
        m_oConnections.Clear();
    }
    
private:
    bool IsConnected( const Delegate& d ) const
    {
        for ( ConnectionConstIter i = m_oConnections.const_begin(); i.isValid(); ++i )
        {
            if ( (*i).d == d )
            {
                return true;
            }
        }
        
        return false;        
    }    
    
    void OnObserverDisconnect( SignalObserver* pObserver )
    {
        JL_SIGNAL_LOG( "\tSignal %p received disconnect message from observer %p\n", this, pObserver );
        
        for ( ConnectionIter i = m_oConnections.begin(); i.isValid(); )
        {
            if ( (*i).pObserver == pObserver )
            {
                JL_SIGNAL_LOG( "\t\tRemoving connection to observer\n" );
                m_oConnections.Remove( i );
            }
            else
            {
                ++i;
            }
        }
    }
};

#ifdef FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX

/**
 * Signal<[8 arguments]>: wrapper class for Signal8
 * This wrapper template, in conjunction with the "Signal" macro, will allow you to ignore the argument count in the signal typename.
 */
template< typename TUnused, typename _P1, typename _P2, typename _P3, typename _P4, typename _P5, typename _P6, typename _P7, typename _P8 >
class Signal< TUnused(_P1, _P2, _P3, _P4, _P5, _P6, _P7, _P8) > : public Signal8< _P1, _P2, _P3, _P4, _P5, _P6, _P7, _P8 >
{
public:
    typedef Signal8< _P1, _P2, _P3, _P4, _P5, _P6, _P7, _P8 > TParent;
    Signal() {}
    Signal( ScopedAllocator* pNodeAllocator ) : TParent(pNodeAllocator) {}
    void operator=( const TParent& other ) { *static_cast<TParent*>(this) = other; }
};

#endif // defined( FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX )

} // namespace jl
    
#endif // ! defined( _JL_SIGNAL_DEFINITIONS_H_ )
