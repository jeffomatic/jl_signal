jl_signal
=========

For all your [Observer pattern](http://en.wikipedia.org/wiki/Observer_pattern) needs, a reasonably fast implementation of signals & slots.

[There](http://doc.trolltech.com/signalsandslots.html) [are](http://www.boost.org/libs/signals/) [lots](http://sigslot.sourceforge.net/) [and](http://libsigc.sourceforge.net/) [lots](https://github.com/pbhogan/Signals) of C++ signals & slots systems out there. This one gives you the following:

<dl>
  <dt>Dead simple API</dt>
  <dd>The library has a minimal initialization step, but after that, it's little more than calling `Connect()` and `Emit()`. A tiny bit of template syntax is required when you declare your signals, but you can use a macro, `JL_SIGNAL()`, to simplify eliminate that. No other macros are used.</dd>
  <dt>No heap allocation</dt>
  <dd>By default, the system uses fixed-size block allocators, which are much faster than heap allocation. They neither cause nor suffer from memory fragmentation.</dd>
  <dt>Automatic signal disconnection</dt>
  <dd>Signals work by caching pointers to observers. If an observer goes out of scope, the library makes sure to remove all internal references to that observer, so you don't have to handle it yourself.</dd>
  <dt>Fast dispatch from signals to observers</dt>
  <dd>Signals are implemented with Don Clugston's <a href="http://www.codeproject.com/Articles/7150/Member-Function-Pointers-and-the-Fastest-Possible">FastDelegate</a> library, which can execute arbitrary callbacks as fast as is theoretically possible.The library has some well-tested hacks under the hood, but should work for most mainstream modern compilers, including GCC, Visual C++ (cl), and LLVM/clang (not documented by Clugston, but it worked for me).</dd>
  <dt>No external dependencies</dt>
  <dd>The library is mostly self-contained, with minimal dependencies on the standard library. The FastDelegate header file is bundled with the project source.</dd>
</dl>

Some anti-features:

- No thread safety.
- This library does require inheritance for observers, but it's almost completely unobtrusive.

Usage example
-------------

```cpp
#include "Signal.h" // base library
#include "SignalConnectionPools.h" // some default allocators
#include <iostream> // for output

// A class that will receive signals.
class Orc : public jl::SignalObserver
{
public:
    // This method will be used as a SLOT.
    void Retort() { std::cout << "GRUMBLE GRUMBLE GRUMBLE...\n"; }
    
    // Another slot, this one taking parameters.
    void TakeDamage( float fDamage )
    {
        if (fDamage >= 20.f) std::cout << "Orc down!\n";
    }
};

// Another class that receives signals.
class HipsterBystander : public jl::SignalObserver
{
public:
    void UnimpressedComeback()
    {
        std::cout << "Whatever, I think the first movie was better.\n";
    }
};

// Another class that receives signals.
class Prop : public jl::SignalObserver
{
public:
    void TakeDamage( float fDamage )
    {
        if (fDamage >= 10.f) std::cout << "SMASH!\n";
    }
};

// A class that broadcasts signals.
class Wizard
{
public:
    // Here are two signals, each of which can be used to call an arbitrary
    // number of slot methods.
    JL_SIGNAL() BattleCrySignal; // calls slots that take no arguments
    JL_SIGNAL( float ) MysticalShotgunSignal; // calls slots that take a single float argument
    
    void BattleCry()
    {
        std::cout << "This is my boomstick!\n";
        BattleCrySignal.Emit();
    };
    
    void FireMysticalShotgun( float fDamage )
    {
        std::cout << "BLAM!\n";
        MysticalShotgunSignal.Emit( fDamage );
    }
};

int main()
{
	// Instantiate some allocators used by the signal system.
	enum { eMaxConnections = 50 };
	jl::StaticSignalConnectionPool< eMaxConnections > oSignalPool;
	jl::StaticObserverConnectionPool< eMaxConnections > oSlotPool;
    
	// Initialize the signal system with our allocators
	jl::SignalBase::SetCommonAllocator( &oSignalPool );
	jl::SignalObserver::SetCommonAllocator( &oSlotPool );
    
	// Instantiate our entities.
	Orc rosencrantz, guildenstern;
	HipsterBystander chad;
	Prop chair;
	Wizard merlin;
    
	// Orcs and hipster bystanders respond to battle cries
	merlin.BattleCrySignal.Connect( &rosencrantz, &Orc::Retort );
	merlin.BattleCrySignal.Connect( &guildenstern, &Orc::Retort );
	merlin.BattleCrySignal.Connect( &chad, &HipsterBystander::UnimpressedComeback );
    
	// Orcs and props take damage
	merlin.MysticalShotgunSignal.Connect( &rosencrantz, &Orc::TakeDamage );
	merlin.MysticalShotgunSignal.Connect( &guildenstern, &Orc::TakeDamage );    
	merlin.MysticalShotgunSignal.Connect( &chair, &Prop::TakeDamage );
    
	// Emit a signal
	merlin.BattleCry();
    
	// Output:
	// Merlin: This is my boomstick!
	// Rosencrantz: GRUMBLE GRUMBLE GRUMBLE...
	// Guildenstern: GRUMBLE GRUMBLE GRUMBLE...
	// Chad: Whatever, I think the first move was better.
    
	// Emit another signal
	merlin.FireMysticalShotgun( 20.f );
    
	// Output:
	// Merlin: BLAM!
	// Rosencrantz: Orc down!
	// Guildenstern: Orc down!
	// Chair: SMASH!
    
	return 0;
}
```    