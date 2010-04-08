*+    *
*     * Copyright (c) 2004 Ingres Corporation 
*     * File: eqmsqlca.cbl
*     *
*     * Embedded MF COBOL run-time SQL communications area.
*     *
*     * Purpose: Data declaration of structure to hold the error and
*     * 	 status information returned by the INGRES/SQL.
*     *
*     * Notes:  1. Comment lines in this file are formatted for both 
*     *		   terminal and ansi-standard compilation.
*     *         2. See eqmdef.cbl for data type descriptions.
*     *
*     * History:
*     *		21-nov-1989 (neil)
*     *		    Extracted from VMS COBOL.
*     *
*-    *
       01  SQLCA.
           05 SQLCAID       PIC X(8)  		   VALUE "SQLCA   ".
           05 SQLCABC       PIC S9(9) USAGE COMP-5 VALUE 136.
           05 SQLCODE       PIC S9(9) USAGE COMP-5.
           05 SQLERRM.
              10 SQLERRML   PIC S9(4) USAGE COMP-5.
              10 SQLERRMC   PIC X(70).
           05 SQLERRP       PIC X(8).
           05 SQLERRD       PIC S9(9) USAGE COMP-5 OCCURS 6 TIMES.
           05 SQLWARN.
              10  SQLWARN0  PIC X(1).
              10  SQLWARN1  PIC X(1).
              10  SQLWARN2  PIC X(1).
              10  SQLWARN3  PIC X(1).
              10  SQLWARN4  PIC X(1).
              10  SQLWARN5  PIC X(1).
              10  SQLWARN6  PIC X(1).
              10  SQLWARN7  PIC X(1).
           05 SQLEXT        PIC X(8).
