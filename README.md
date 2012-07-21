jl_signal
=========

For all your [Observer pattern](http://en.wikipedia.org/wiki/Observer_pattern) needs, a reasonably fast implementation of signals & slots.

[There](http://doc.trolltech.com/signalsandslots.html) [are](http://www.boost.org/libs/signals/) [lots](http://sigslot.sourceforge.net/) [and](http://libsigc.sourceforge.net/) [lots](https://github.com/pbhogan/Signals) of C++ signals & slots systems out there. This one gives you the following:

- No heap allocation
- Automatic signal disconnection when emitters and observers go out of scope
- Dead simple API
- Fast dispatch from signals to observers, via Don Clugston's [FastDelegate](http://www.codeproject.com/Articles/7150/Member-Function-Pointers-and-the-Fastest-Possible)

Some anti-features:

- No thread safety.
- This library does require inheritance for observers, but it's almost completely unobtrusive.

Usage example
-------------

```cpp
// Include the library.
#include "Signal.h"

// Instantiate some allocators used by the signal system.
enum { eMaxConnections = 50 };
jl::FixedSignalConnectionPool< eMaxConnections > oSignalPool;
jl::FixedObserverConnectionPool< eMaxConnections > oSlotPool;

// Initialize the signal system with our allocators
jl::Signal::SetCommonAllocator( &oSignalPool );
jl::SignalObserver::SetCommonAllocator( &oSlotPool );

// A class that will receive signals.
class Orc : public jl::SignalObserver
{
public:
    // This method will be used as a SLOT.
    void Retort() { std::cout << "GRUMBLE GRUMBLE GRUMBLE..."; }

    // Another slot, this one taking parameters.
    void TakeDamage( float fDamage )
    {
        if (fDamage > 20.f) std::cout << "Orc down!";
    }
};

// Another class that receives signals.
class HipsterBystander : public jl::SignalObserver
{
public:
    void UnimpressedComeback()
    {
        std::cout << "Whatever, I think the first movie was better.";
    }
};

// Another class that receives signals.
class Prop : public jl::SignalObserver
{
public:
    void TakeDamage( float fDamage )
    {
        if (fDamage > 10.f) std::cout << "SMASH!";
    }
}

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
        std::cout << "This is my boomstick!";
        BattleCrySignal.Emit();
    };

    void FireMysticalShotgun( float fDamage )
    {
        std::cout << "BLAM!";
        MysticalShotgunSignal.Emit( fDamage );
    }
};

// Instantiate our entities.
Orc rosencrantz, guildenstern;
HipsterBystander chad;
Prop chair;
Wizard merlin;

// Orcs and hipster bystanders respond to battle cries
merlin.BattleCrySignal.connect( &rosencrantz, &Orc::Retort );
merlin.BattleCrySignal.connect( &guildenstern, &Orc::Retort );
merlin.BattleCrySignal.connect( &chad, &HipsterBystander::UnimpressedComeback );

// Orcs and props take damage
merlin.MysticalShotgunSignal.connect( &rosencrantz, &Orc::TakeDamage );
merlin.MysticalShotgunSignal.connect( &guildenstern, &Orc::TakeDamage );    
merlin.MysticalShotgunSignal.connect( &chair, &Prop::TakeDamage );

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
```    