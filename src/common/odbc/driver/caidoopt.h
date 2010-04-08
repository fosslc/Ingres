/*
**  CAIDOOPT.H
**
**
**  CAIDODBC specific connection and statement options:
**
**  Note: these can also be set in the CAIDDSI.INI file.
**
**  (C) Copyright 1994 Ingres Corporation
**
*/

#ifndef _INC_CAIDOOPT
#define _INC_CAIDOOPT

/*
**  As of 11/29/93 the CAIDODBC specific option numbers were NOT registered
**  with Microsoft.  As a result it is possile that they could conflict
**  with those used by other vendors.  The range of option numbers
**  assigned by Microsoft will probably be different than that currently
**  used, which will require thay applications using them be recompiled.
**
**  As of 9/20/95, we have requested a block of these options from Microsoft,
**  but have not yet received a reply.  We have also requested a block of
**  driver specific data types, and have also not yet received a reply.
**
**  When we are registered, the following #define will be removed:
*/
#define CAID_NOT_REGISTERED

#define CAID_OPT            SQL_CONNECT_OPT_DRVR_START + 6600

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                       CAIDODBC Connect Options:                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define CAID_TRACE          CAID_OPT + 0   /* CAIDODBC driver trace flags:  */
                                            /*                               */
#define TRACE_ODBCCALL      0x0003          /*   Trace exported functions    */
#define TRACE_INTLCALL      0x0005          /*   Trace internal functions    */
#define TRACE_PARMS         0x0004          /*   Trace function parms        */
#define TRACE_LOCKS         0x0008          /*   Trace thread locks          */
#define SNAP_SQL            0x0010          /*   Snap SQL syntax             */
#define SNAP_ENV            0x0100          /*   Snap environment block      */
#define SNAP_DBC            0x0200          /*   Snap connection block       */
#define SNAP_STMT           0x0400          /*   Snap statement block        */
#define SNAP_SQLDA          0x0800          /*   Snap SQLDA                  */

#define CAID_ACCESSIBLE     CAID_OPT + 1    /* Use ACCESSIBLE_TABLES view    */
                                            /* for SQLTables:                */
                                            /*   vParam = 0 off              */
                                            /*   vParam = 1 on               */
#define OPT_ACCESSIBLE_DFLT 1

#define CAID_CAT_TABLE      CAID_OPT + 2    /* Specify name of               */
                                            /* ACCESSIBLE_TABLES             */
                                            /* view for SQLTables if         */
                                            /* CAID_ACCESSIBLE is on         */
                                            /* vParam-->table name,          */
                                            /* SYSCA.ACCESSIBLE_TABLES       */
#define OPT_CAT_TABLE_DFLT  "SYSCA.ACCESSIBLE_TABLES"

#define CAID_CACHE          CAID_OPT + 3    /* Cache SQLTables result set    */
                                            /*   vParam = 0 off              */
                                            /*   vParam = 1 on               */
#define OPT_CACHE_DFLT      0

#define CAID_ENABLE_ENSURE  CAID_OPT + 4    /* Enable the ENSURE option of   */
                                            /* the SQLStatistics API.  This  */
                                            /* causes an UPDATE STATSTICS    */
                                            /* command to be issued, which   */
                                            /* can cause deadlocks, timeouts */
                                            /* or very slow response.        */
                                            /*   vParam = 0 disabled         */
                                            /*   vParam = 1 enabled          */

#define CAID_COMMIT_BEHAVIOR CAID_OPT + 5   /* Set cursor behavior on        */
                                            /* commit and rollback:          */
#define OPT_CLOSE_DELETE        0           /*   COMMIT WORK, delete syntax  */
#define OPT_CLOSE               1           /*   COMMIT WORK, keep syntax    */
#define OPT_PRESERVE            2           /*   COMMIT WORK CONTINUE        */
#define OPT_COMMIT_DFLT         OPT_CLOSE


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                      CAIDODBC Statement Options:                          */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define CAID_FETCH_ROWS     CAID_OPT  + 20  /* # rows for bulk fetch         */
                                            /* vParam = # rows               */
#define OPT_FETCH_ROWS_DFLT 100

#define CAID_FETCH_DOUBLE   CAID_OPT  + 21  /* Fetch REAL as DOUBLE          */
                                            /* vParam = 0 off (default)      */
                                            /* vParam = 1 on                 */
#define OPT_FETCH_DOUBLE_DFLT 0


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                  CAIDODBC Driver Specific Data Types:                     */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define CAID_TYPE           SQL_TYPE_DRIVER_START - 6600
#define CAID_GRAPHIC        CAID_TYPE - 1
#define CAID_VARGRAPHIC     CAID_TYPE - 0
#define CAID_TYPE_END       CAID_VARGRAPHIC

#endif
