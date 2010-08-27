*+    *
*     * File: eqmsqlda.cbl
*     *
*     * Embedded MF COBOL run-time Dynamic SQL descriptor area.
*     *
*     * Purpose: Data declaration of structure to hold the Dynamic
*     *		 SQL data descriptions.
*     *
*     * Notes:  1. Comment lines in this file are formatted for both 
*     *		   terminal and ansi-standard compilation.
*     *         2. See eqmdef.cbl for data type descriptions.
*     *
*     * History:
*     *		21-nov-1989 (neil)
*     *		    Extracted from VMS COBOL.
*     *	        27-apr-1990 (barbara)
*     *             Default number of sqlvars increased to 300.
*     *         10-feb-1993 (kathryn)
*     *             Add define for DB_LVCH_TYPE.
*     *         22-apr-1994 (timt)
*     *             Added datatype code for W4GL objects.
*     *		    Added new byte datatypes.
*     *         17-Sep-2003 (bonro01)
*     *             Added alignment fields for 64-bit platforms.
*     *	        10-feb-2004 (toumi01)
*     *             Default number of sqlvars increased to 1024.
*     *         27-sep-2004 (kodse01)
*     *             Corrected the copyright statement comment.
*     *         29-Jul-2010 (hweho01) SIR 121123
*     *             Increased the size of SQLNAMEC to 258, to match 
*     *             with the IISQD_NAMELEN in IISQLDA (iisqlda.h).
*     *
*     * Copyright (c) 2004, 2010 Ingres Corporation
*-    *
       78 IISQ-MAX-COLS        VALUE 1024.

       01 SQLDA.
          05 SQLDAID            PIC X(8).
          05 SQLDABC            PIC S9(9) USAGE COMP-5.
          05 SQLN               PIC S9(4) USAGE COMP-5
                                VALUE IISQ-MAX-COLS.
          05 SQLD               PIC S9(4) USAGE COMP-5.
          05 SQLVAR             OCCURS IISQ-MAX-COLS TIMES.
             07 SQLTYPE         PIC S9(4) USAGE COMP-5.
             07 SQLLEN          PIC S9(4) USAGE COMP-5.
# ifdef i64_hpu
             07 SQLALIGN1       PIC X(4).
# endif
             07 SQLDATA         USAGE POINTER.
             07 SQLIND          USAGE POINTER.
             07 SQLNAME.
                49 SQLNAMEL     PIC S9(4) USAGE COMP-5.
                49 SQLNAMEC     PIC X(258).
# ifdef i64_hpu
             07 SQLALIGN2       PIC X(4).
# endif

*     *
*     * SQLDA Type Codes
*     *
*     * ---------     -----   ------
*     * DATE          3       25
*     * MONEY         5       8
*     * DECIMAL       10      SQLLEN = 256 * P + S
*     * CHAR          20      SQLLEN
*     * VARCHAR       21      SQLLEN
*     * INTEGER       30      SQLLEN
*     * FLOAT         31      SQLLEN
*     * TABLE-FIELD   52      0
*     * LONG VARCHAR  22      0 
*     *
       78 IISQ-DTE-TYPE        VALUE 3.
       78 IISQ-DTE-LEN         VALUE 25.
       78 IISQ-MNY-TYPE        VALUE 5.
       78 IISQ-DEC-TYPE        VALUE 10.
       78 IISQ-CHA-TYPE        VALUE 20.
       78 IISQ-VCH-TYPE        VALUE 21.
       78 IISQ-LVCH-TYPE       VALUE 22.
       78 IISQ-INT-TYPE        VALUE 30.
       78 IISQ-FLT-TYPE        VALUE 31.
       78 IISQ-TBL-TYPE        VALUE 52.
       78 IISQ-BYTE-TYPE       VALUE 23.
       78 IISQ-VBYTE-TYPE      VALUE 24.
       78 IISQ-LBYTE-TYPE      VALUE 25.
       78 IISQ-OBJ-TYPE        VALUE 45.
