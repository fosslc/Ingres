/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : portto64.h, Header file.
**    Project  : Ingres II
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manage the porting to 64 bits
**
** History:
**
** 27-Feb-2003 (uk$so01)
**    created
**/

#if !defined (_PORTING_TO_64_BITS_HEADER)
#define _PORTING_TO_64_BITS_HEADER

#if !defined (WIN64)

#if !defined (DWORD_PTR)
#define DWORD_PTR DWORD
#endif
#if !defined (LONG_PTR)
#define LONG_PTR LONG
#endif

#endif // WIN64

#endif // _PORTING_TO_64_BITS_HEADER
