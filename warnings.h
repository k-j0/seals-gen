#pragma once

#define W_PRAGMA(a) _Pragma(#a)

#ifdef _MSC_VER
    #define WARNING_PUSH W_PRAGMA(warning(push))
    #define WARNING_POP W_PRAGMA(warning(pop))
    #define WARNING_DISABLE_OMP_PRAGMAS             W_PRAGMA(warning(disable: 6993))
    #define WARNING_DISABLE_STRING_NUL_TERMINATED   W_PRAGMA(warning(disable: 6054))
    #define WARNING_DISABLE_STACK_SIZE              W_PRAGMA(warning(disable: 6262))
    #define WARNING_DISABLE_UNINITIALIZED_COMPONENT W_PRAGMA(warning(disable: 26495))
    #define WARNING_DISABLE_UNSCOPED_ENUM           W_PRAGMA(warning(disable: 26812))
#else
    #define WARNING_PUSH
    #define WARNING_POP
    #define WARNING_DISABLE_OMP_PRAGMAS
    #define WARNING_DISABLE_STRING_NUL_TERMINATED
    #define WARNING_DISABLE_STACK_SIZE
    #define WARNING_DISABLE_UNINITIALIZED_COMPONENT
    #define WARNING_DISABLE_UNSCOPED_ENUM
#endif

