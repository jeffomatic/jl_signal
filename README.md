jl_signal
=========

For all your [Observer pattern](http://en.wikipedia.org/wiki/Observer_pattern) needs, a reasonably fast implementation of signals & slots, featuring:

 - Fast, stack-allocatable, comparable, containable delegate/callback objects using Don Clugston's nifty [FastDelegate](http://www.codeproject.com/Articles/7150/Member-Function-Pointers-and-the-Fastest-Possible) library
 - Configurable allocation for all container classes, defaulting to a very fast fixed-size block allocator
 - Automatic disconnection on observer destruction, grudgingly implemented with an observer base class

This doc is WIP.