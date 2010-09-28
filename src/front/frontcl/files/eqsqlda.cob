*     *
*     * SQLDA
*     *
*     * Purpose: Data declaration of structure to hold the
*     *          dynamic SQL descriptor information returned by
*     *          the INGRES run-time routines.
*     *
*     * History:
*     *	    10-feb-1993 (kathryn)
*     *   	Added comments for IISQLHDLR and LONG VARCHAR type codes.
*     *     17-sep-1993 (sandyd)
*     *         Added comments for new byte datatypes.
*     *     22-apr-1994 (timt)    
*     *         Added comment for datatype code for W4GL objects.
*     *     20-mar-2002 (toumi01)
*     *		Increased max columns from 300 to 1024	
*     *     18-nov-2009 (joea)
*     *         Add type code for BOOLEAN as valid in SQLDA.
*     *     29-jul-2010 (hweho01) SIR 121123
*     *         Increased the size of SQLNAMEC from 34 to 258, to match
*     *         with the IISQD_NAMELEN in IISQLDA (iisqlda.h).
*     *
*     * Copyright (c) 2004, 2010 Ingres Corporation
*     *

       01 SQLDA EXTERNAL.
          05 SQLDAID            PIC X(8).
          05 SQLDABC            PIC S9(9) USAGE COMP.
          05 SQLN               PIC S9(4) USAGE COMP.
          05 SQLD               PIC S9(4) USAGE COMP.
          05 SQLVAR             OCCURS 1024 TIMES.
             07 SQLTYPE         PIC S9(4) USAGE COMP.
             07 SQLLEN          PIC S9(4) USAGE COMP.
             07 SQLDATA         USAGE POINTER SYNC.
             07 SQLIND          USAGE POINTER SYNC.
             07 SQLNAME.
                49 SQLNAMEL     PIC S9(4) USAGE COMP.
                49 SQLNAMEC     PIC X(258).

       01 IISQLHDLR EXTERNAL.
          05 SQLARG             USAGE POINTER.
          05 SQLHDLR            PIC S9(9) USAGE COMP.

*     *
*     * SQLDA Type Codes
*     *
*     * Type Name     Value   Length
*     * ---------     -----   ------
*     * DATE          3       25
*     * MONEY         5       8
*     * DECIMAL       10      SQLLEN = 256 * P + S
*     * CHAR          20      SQLLEN
*     * VARCHAR       21      SQLLEN
*     * LONG VARCHAR  22      0
*     * INTEGER       30      SQLLEN
*     * FLOAT         31      SQLLEN
*     * BOOLEAN       38      1
*     * IISQLHDLR     46      0			(DATAHANDLER)
*     * TABLE         52      0
*     * BYTE          23      SQLLEN
*     * BYTE VARYING  24      SQLLEN
*     * LONG BYTE     25      0
*     * OBJECT        45      0			(W4GL)
*     *
