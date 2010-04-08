/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : port.h
//
//     
//
********************************************************************/
#ifndef _PORT_H
#define _PORT_H

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif

#ifdef WIN32
    #define EXPORT
    #define __export
    #define _fmemset memset
    #define _fmemcpy memcpy
    #define LONG2POINT(l,pt)\
                ((pt).x=(SHORT)LOWORD(l),(pt).y=(SHORT)HIWORD(l))
    #define IsGDIObject(x)\
                ((HGDIOBJ)(x) == (HGDIOBJ)NULL ? FALSE : TRUE)
    #define CONTAINER_CHILD "CA_ContainerChild32"


#ifdef MAINWIN
// multiple definition if defined in header : moved to dbadlg1/util.c
extern unsigned long GetTextExtent(HDC hdc, LPSTR lpstr, size_t cb);
#else
__inline unsigned long GetTextExtent(HDC hdc, LPSTR lpstr, size_t cb);
__inline unsigned long GetTextExtent(HDC hdc, LPSTR lpstr, size_t cb)
{
    SIZE size;
    GetTextExtentPoint32(hdc, lpstr, cb, &size);
    return (DWORD)MAKELONG(size.cx, size.cy);
}
#endif

#else       // WIN32
    #ifndef EXPORT
        #define EXPORT   __export
    #endif  // EXPORT
    #define CONTAINER_CHILD "CA_ContainerChild"
    #define TCHAR   char
    #define LPTSTR  LPSTR
    #define LPCTSTR LPCSTR
    #define GWL_USERDATA    0
#endif      // WIN32

#endif  // _PORT_H
