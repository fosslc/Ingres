/*
**Copyright (c) 2004 Ingres Corporation
*/
 
/**
** Name: GENERR.H - Generic Error Message Identifiers
**
** Description:
**      This header contains symbolic definitions for all of the
** generic error messages.
**
** These error messages are returned, usually with the code from the
** originating DBMS, to describe the class of error that has occurred.
** The messages are intended to describe the type of error that has
** ocurred, not the exact error.
**
** History: $Log-for RCS$
**   13-sep-1988 (DaveS)
**      Created new.
**   30-sep-1988 (DaveS)
**      Updated for SQL2-9/88 and 7 character normalization
**/
 
/* generic error codes */
 
#define GE_NO_MORE_DATA       100   /* End of data */
#define GE_OK                   0   /* Successful completion */
 
#define GE_TABLE_NOT_FOUND -30100   /* Table not found in DB */
#define GE_COLUMN_UNKNOWN  -30110   /* Column not known/not in table */
#define GE_CURSOR_UNKNOWN  -30120   /* Unknown cursor */
#define GE_NOT_FOUND       -30130   /* Other database object not found */
#define GE_UNKNOWN_OBJECT  -30140   /* Other unknown/unavailable resource */
 
#define GE_DEF_RESOURCE    -30200   /* Duplicate resource def. */
#define GE_INS_DUP_ROW     -30210   /* Attempt to insert invalid
                                           duplicate row */
 
#define GE_SYNTAX_ERROR    -31000   /* Statement syntax error */
#define GE_INVALID_IDENT   -31100   /* Invalid identifier */
#define GE_UNSUP_LANGUAGE  -31200   /* Unsupported query language */
#define GE_QUERY_ERROR     -32000   /* Inconsistent or incorrect query
                                            specification */
#define GE_LOGICAL_ERROR   -33000   /* Run-time logical error */
#define GE_NO_PRIVILEGE    -34000   /* Not privileged/restricted
                                            operation */
 
#define GE_SYSTEM_LIMIT    -36000   /* System limit exceeded */
#define GE_NO_RESOURCE     -36100   /* Out of needed resource */
#define GE_CONFIG_ERROR    -36200   /* System configuration error */
 
#define GE_COMM_ERROR      -37000   /* Communication or
                                            transmission error */
#define GE_GATEWAY_ERROR   -38000   /* Error in the gateway */
#define GE_HOST_ERROR      -38100   /* Host system error */
#define GE_FATAL_ERROR     -39000   /* Session terminated */
#define GE_OTHER_ERROR     -39100   /* Unmappable error */
 
#define GE_CARDINALITY     -40100   /* Cardinality violation */
#define GE_DATA_EXCEPTION  -40200   /* Data Exception, with subcode.
                                       The subcode is identified by
                                       SQLCODE - GE_DATA_EXCEPTION */
#define GE_CONSTR_VIO      -40300   /* Constraint violation */
#define GE_CUR_STATE_INV   -40400   /* Invalid cursor state */
#define GE_TRAN_STATE_INV  -40500   /* Invalid transaction state */
#define GE_INV_SQL_STMT_ID -40600   /* Invalid SQL Statement ID */
#define GE_TRIGGER_DATA    -40700   /* Triggered data change violation */
#define GE_USER_ID_INV     -41000   /* Invalid user auth. ID */
#define GE_SQL_STMT_INV    -41200   /* Invalid SQL statement */
#define GE_DUP_SQL_STMT_ID -41500   /* Duplicate SQL statement ID */
#define GE_SERIALIZATION   -49900   /* Serialization failure (deadlock) */
 
 
/* Generic error subcodes */
 
#define GESC_NO_SC     00     /* No subcode */
#define GESC_TRUNC     01     /* Character data, right truncation */
#define GESC_NEED_IND  02     /* Null value, no indicator parameter */
#define GESC_NUMOVR    03     /* Exact numeric data, loss of significance
                              **  (decimal overflow) */
#define GESC_ASSGN     04     /* Error in assignment */
#define GESC_FETCH0    05     /* Fetch orientation has value zero */
#define GESC_DTINV     06     /* Invalid datetime format */
#define GESC_DATEOVR   07     /* Datetime field overflow */
#define GESC_RSV08     08     /* Reserved */
#define GESC_INVIND    09     /* Invalid indicator parameter value */
 
#define GESC_CURSINV   10     /* Invalid cursor name */
#define GESC_TYPEINV   15     /* Invalid data type */
 
#define GESC_FIXOVR    20     /* Fixed Point Overflow */
#define GESC_EXPOVR    21     /* Exponent Overflow */
#define GESC_FPDIV     22     /* Fixed Point Divide */
#define GESC_FLTDIV    23     /* Floating Point Divide */
#define GESC_DCDIV     24     /* Decimal Divide */
#define GESC_FXPUNF    25     /* Fixed Point Underflow */
#define GESC_EPUNF     26     /* Exponent Underflow */
#define GESC_DECUNF    27     /* Decimal Underflow */
#define GESC_OTHER     28     /* Other unspecified math exception */
 
#define GESC_MAX       99     /* Max legal subcode */
