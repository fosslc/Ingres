*     *
*     * SQLDA
*     *
*     * Purpose: Data declaration of structure to hold the
*     *          dynamic SQL descriptor information returned by
*     *          the INGRES run-time routines.
*     *
*     *
*     *		 Definition of the SQLDA for DG ESQL/COBOL only.
*     *
*     * History:
*     *     20-mar-2002 (toumi01)
*     *		Increased max columns from 300 to 1024	
*     *
*     * Copyright (c) 2004 Ingres Corporation
*     *

       01 SQLDA EXTERNAL.
          05 SQLDAID            PIC X(8).
          05 SQLDABC            PIC S9(9) USAGE COMP.
          05 SQLN               PIC S9(4) USAGE COMP VALUE 1024.
          05 SQLD               PIC S9(4) USAGE COMP.
          05 SQLVAR             OCCURS 1024 TIMES.
             07 SQLTYPE         PIC S9(4) USAGE COMP.
             07 SQLLEN          PIC S9(4) USAGE COMP.
             07 SQLDATA         PIC S9(8) USAGE COMP.
             07 SQLIND          PIC S9(8) USAGE COMP.
             07 SQLNAME.
                49 SQLNAMEL     PIC S9(4) USAGE COMP.
                49 SQLNAMEC     PIC X(34).

*     *
*     * SQLDA Type Codes
*     *
*     * Type Name     Value   Length
*     * ---------     -----   ------
*     * DECIMAL       10      SQLLEN = 256 * P + S
*     * CHAR          20      SQLLEN
*     * VARCHAR       21      SQLLEN
*     * INTEGER       30      SQLLEN
*     * FLOAT         31      SQLLEN
*     * TABLE         52      0
*     *
