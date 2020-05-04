#ifndef _PUB_TYPEDEF_H_
#define _PUB_TYPEDEF_H_

#ifdef LINUX
#define IDT_API

#ifndef UCHAR
typedef unsigned char           UCHAR;
#endif

#ifndef BYTE
typedef unsigned char           BYTE;
#endif

#ifndef WORD
typedef unsigned short          WORD;
#endif

#ifndef UINT16
typedef unsigned short          UINT16;
#endif

#ifndef DWORD
typedef unsigned long           DWORD;
#endif

#ifndef UINT
typedef unsigned int            UINT;
#endif

#ifndef ULONG
typedef unsigned long           ULONG;
#endif

#ifndef LONG
typedef long                    LONG;
#endif

#ifndef USHORT
typedef unsigned short          USHORT;
#endif


#endif

#ifdef ANDROID
#define IDT_API

#ifndef UCHAR
typedef unsigned char           UCHAR;
#endif

#ifndef BYTE
typedef unsigned char           BYTE;
#endif

#ifndef WORD
typedef unsigned short          WORD;
#endif

#ifndef UINT16
typedef unsigned short          UINT16;
#endif

#ifndef DWORD
typedef unsigned long           DWORD;
#endif

#ifndef UINT
typedef unsigned int            UINT;
#endif

#ifndef ULONG
typedef unsigned long           ULONG;
#endif

#ifndef LONG
typedef long                    LONG;
#endif

#ifndef USHORT
typedef unsigned short          USHORT;
#endif


#endif

#ifdef FREERTOS
#define IDT_API

#ifndef UCHAR
typedef unsigned char           UCHAR;
#endif

#ifndef BYTE
typedef unsigned char           BYTE;
#endif

#ifndef WORD
typedef unsigned short          WORD;
#endif

#ifndef UINT16
typedef unsigned short          UINT16;
#endif

#ifndef DWORD
typedef unsigned long           DWORD;
#endif

#ifndef UINT
typedef unsigned int            UINT;
#endif

#ifndef ULONG
typedef unsigned long           ULONG;
#endif

#ifndef LONG
typedef long                    LONG;
#endif

#ifndef USHORT
typedef unsigned short          USHORT;
#endif


#endif


#endif



