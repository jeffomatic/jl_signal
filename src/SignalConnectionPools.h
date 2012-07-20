#ifndef _JL_SIGNAL_CONNECTION_POOLS_H_
#define _JL_SIGNAL_CONNECTION_POOLS_H_

#include "ObjectPoolScopedAllocator.h"
#include "SignalDefinitions.h"

namespace jl {

typedef Signal0<void> TDummySignal;

template< unsigned _Size >
class FixedSignalConnectionPool : public FixedObjectPoolAllocator< TDummySignal::eAllocationSize, _Size >
{
};

template< unsigned _Size >
class FixedObserverConnectionPool : public FixedObjectPoolAllocator< SignalObserver::eAllocationSize, _Size >
{
};
    
} // namespace jl

#endif // ! defined( _JL_SIGNAL_CONNECTION_POOLS_H_ )