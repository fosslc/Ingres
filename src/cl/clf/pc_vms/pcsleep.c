/*
**  Copyright (c) 2008 Ingres Corporation
**  All rights reserved.
**
**  Name:
**      PCsleep.c
**
**  Function:
**      PCsleep
**
**  Arguments:
**      u_i4  msecs
**
**  Result:
**      Uninterrupted sleep
**
**  History:
**      10-Dec-2008   (stegr01)
**      Port from assembler to C for VMS itanium port
**
**/

/* use the common unix file so we only maintain one */

#include "ing_src:[cl.clf.pc_unix]pcsleep.c"

