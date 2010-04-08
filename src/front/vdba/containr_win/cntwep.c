/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CNTWEP.C
 *
 * WEP code for the container DLL.  This function is placed in a separate
 * segment so marking the segment as FIXED is not hard on the system.
 * history:
 *  22-Jun-2001 (noifr01)
 *   (sir 105104) removed code that didn't compile any more, and was doing
 *   nothing
 ****************************************************************************/

#include <windows.h>

/****************************************************************************
 * WEP
 *
 * Purpose:
 *  Required DLL Exit function.  Does nothing.
 *
 * Parameters:
 *  int           nExitType   - WEP_SYSTEM_EXIT - system is being shut down
 *                              WEP_FREE_DLL    - DLL has just been unloaded
 * Return Value:
 *  int           1 if successful; else 0
 ****************************************************************************/

int WINAPI WEP( int nExitType )
    {
    return 1;
    }
