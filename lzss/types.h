#ifndef __TYPES_H__
#define __TYPES_H__


#ifndef NULL
#define NULL                0
#endif

typedef char                SINT8;
typedef short               SINT16;
typedef int                 SINT32;
typedef __int64             SINT64;

typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
typedef unsigned __int64    UINT64;

typedef float               FLOAT;
typedef double              DOUBLE;
typedef long double         EDOUBLE; // double and long double are the same for MS-VC compilers

typedef void*               PVOID;

typedef short               wchar;


#endif