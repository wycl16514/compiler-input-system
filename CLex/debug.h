#ifndef __DEBUG_H
#define __DEBUG_H

    #ifdef DEBUG
    #define   PRIVATE
    #define   D(x) x
    #define   ND(x)
    #else
    #define PRIVATE  static
    #define D(x)
    #define ND(x) x
    #endif

#define PUBLIC

#ifndef max
#define max(a,b) ( ((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) ( ((a) < (b)) ? (a) : (b))
#endif

#endif