*+    *
*     * EQUEL/COBOL run-time communications area.
*     *
*     * Purpose: Provide known data types to the runtime routines too
*     *		allow any COBOL data types to interface with II modules.
*     *
*     * Notes:  Comment lines in this file are formatted for both 
*     *		terminal and ansi-standard compilation.
*     *
*     *		Definitions for DG ESQL/COBOL only.
*     *
*     * Copyright (c) 2004 Ingres Corporation
*-    *

*     * Numeric work-spaces.
       01  IIINTS.
           02 III4      PIC S9(9)   USAGE COMP OCCURS 4 TIMES.
       01  IIF8                     USAGE COMP-2.
       01  II0          PIC S9(8)   USAGE COMP VALUE 0.
       01  II1		PIC S9(8)   USAGE COMP VALUE 1.

*     * Result area
       01  IIRES        PIC S9(8)   USAGE COMP.

*     * Short NULL-terminated buffers that simulate C strings.
       01  IIS1.
           02 IISDAT1   PIC X(31).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS2.
           02 IISDAT2   PIC X(31).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS3.
           02 IISDAT3   PIC X(31).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS4.
           02 IISDAT4   PIC X(31).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS5.
           02 IISDAT5   PIC X(31).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS6.
           02 IISDAT6   PIC X(31).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS7.
           02 IISDAT7   PIC X(31).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS8.
           02 IISDAT8   PIC X(31).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS9.
           02 IISDAT9   PIC X(31).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS10.
           02 IISDAT10  PIC X(31).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS11.
           02 IISDAT11  PIC X(31).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS12.
           02 IISDAT12  PIC X(31).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS13.
           02 IISDAT13  PIC X(31).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIS14.
           02 IISDAT14  PIC X(31).
           02 FILLER    PIC X                 VALUE LOW-VALUE.

*     * Long NULL-terminated buffers that simulate C strings.
       01  IIL1.
           02 IILDAT1   PIC X(263).
           02 FILLER    PIC X                 VALUE LOW-VALUE.
       01  IIL2.
           02 IILDAT2   PIC X(263).
           02 FILLER    PIC X                 VALUE LOW-VALUE.

*     * String Descriptor Variables
       01 IITYPE     	PIC S9(8)   USAGE COMP.
       01 IILEN      	PIC S9(8)   USAGE COMP.
