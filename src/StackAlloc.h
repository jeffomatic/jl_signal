#pragma once
#ifndef STACK_ALLOC_H_
#define STACK_ALLOC_H_

#if _MSC_VER
    #include <malloc.h>
    #define StackAlloc( x ) _alloca( x )
#else
    #include <alloca.h>
    #define StackAlloc( x ) alloca( x )
#endif

#endif