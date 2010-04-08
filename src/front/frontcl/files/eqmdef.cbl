*+    *
*     * Copyright (c) 2004 Ingres Corporation 
*     * File: eqmdef.cbl
*     *
*     * Embedded Query Language/MF COBOL run-time communications area.
*     *
*     * Purpose: Provide known data types to the runtime routines too
*     *		 allow any MF COBOL data types to interface with II modules.
*     *
*     * Notes:  1. Comment lines in this file are formatted for both 
*     *		   terminal and ansi-standard compilation.
*     *         2. In order to allow the COBOL program to interface with
*     *		   the INGRES C run-time system all integer values must
*     * 	   be USAGE COMP-5.  This may be replaced with USAGE COMP
*     * 	   only if you confirm that COMP and COMP-5 are identical
*     * 	   on your machine (for example Sun OS4).  For the sake of
*     * 	   portability, however, you should leave all declarations
*     * 	   as COMP-5.
*     *         3. Because of the USAGE COMP-5 clause all preprocessed files
*     * 	   will cause a single compiler informational message to be
*     * 	   displayed about the use of COMP-5 data items.
*     *
*     * History:
*     *		21-nov-1989 (neil)
*     *		    Extracted from VMS COBOL.
*     *         10-aug-1995 (thoda04)
*     *             Initialize IIPK to ZERO (not 0.0) for DECIMAL-POINT COMMA.
*     *
*-    *

*     * Numeric work-spaces.
       01  III4-1       PIC S9(9)        USAGE COMP-5 SYNC VALUE 0.
       01  III4-2       PIC S9(9)        USAGE COMP-5 SYNC VALUE 0.
       01  III4-3       PIC S9(9)        USAGE COMP-5 SYNC VALUE 0.
       01  III4-4       PIC S9(9)        USAGE COMP-5 SYNC VALUE 0.
       01  III4-5       PIC S9(9)        USAGE COMP-5 SYNC VALUE 0.
       01  II0          PIC S9(9)        USAGE COMP-5 SYNC VALUE 0.
       01  IIIND        PIC S9(4)        USAGE COMP-5 SYNC VALUE 0.
       01  IIF8         PIC X(8)         USAGE COMP-X SYNC VALUE 0.
       01  IIPK         PIC S9(9)V9(9)   USAGE COMP-3 VALUE ZERO.

*     * Result area
       01  IIRES        PIC S9(9)        USAGE COMP-5 SYNC VALUE 0.
       01  IIADDR       USAGE POINTER.

*     * Short NULL-terminated buffers that simulate C strings.
       01  IIS1.
           02 IISDAT1   PIC X(33).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS2.
           02 IISDAT2   PIC X(33).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS3.
           02 IISDAT3   PIC X(33).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS4.
           02 IISDAT4   PIC X(33).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS5.
           02 IISDAT5   PIC X(33).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS6.
           02 IISDAT6   PIC X(33).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS7.
           02 IISDAT7   PIC X(33).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS8.
           02 IISDAT8   PIC X(33).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS9.
           02 IISDAT9   PIC X(33).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS10.
           02 IISDAT10  PIC X(33).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS11.
           02 IISDAT11  PIC X(33).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS12.
           02 IISDAT12  PIC X(33).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS13.
           02 IISDAT13  PIC X(33).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS14.
           02 IISDAT14  PIC X(33).
           02 FILLER    PIC X                 VALUE LOW-VALUE.

*     * Long NULL-terminated buffers that simulate C strings.
       01  IIL1.
           02 IILDAT1   PIC X(263).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIL2.
           02 IILDAT2   PIC X(263).
           02 FILLER    PIC X                 VALUE LOW-VALUE.

*     * A floating point literal buffer in a C simulated string.
       01  IIF8STR.
           02 IIF8BUF   PIC X(63).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
