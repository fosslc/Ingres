*     *
*     * SQLCA
*     *
*     * Purpose: Data declaration of structure to hold the
*     *          error and status information returned by the
*     *          INGRES runtime routines.
*     *
*     * Copyright (c) 2004 Ingres Corporation
*     *
       01  SQLCA EXTERNAL.
           05 SQLCAID       PIC X(8).
           05 SQLCABC       PIC S9(9) USAGE COMP.
           05 SQLCODE       PIC S9(9) USAGE COMP.
           05 SQLERRM.
              49 SQLERRML   PIC S9(4) USAGE COMP.
              49 SQLERRMC   PIC X(70).
           05 SQLERRP       PIC X(8).
           05 SQLERRD       PIC S9(9) USAGE COMP 
                            OCCURS 6 TIMES.

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

