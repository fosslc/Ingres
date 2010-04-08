/*
** Copyright (c) 2004 Ingres Corporation
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
** occurred, not the exact error.
**
** Two sets of #defines exist: one set begins with GE_ and the other set
** conforms to the error naming convention for release 6 and begin with
** E_GExxxx_ (where xxxx is a four digit hexadecimal number).  Both exist
** because the former set was developed for front-end use and it was thought
** that they would be exposed to customers.  The second set is recommended for
** RTI's use since they follow the established convention.
**
** History:
**   13-sep-1988 (DaveS)
**      Created new.
**   30-sep-1988 (DaveS)
**      Updated for SQL2-9/88 and 7 character normalization
**   23-apr-1989 (jrb)
**      Added second set of error defines for consistency with RTI error naming
**	conventions.
**   16-may-1989 (jrb)
**	Fixed wrong hex values in E_GE75BC_UNKNOWN_OBJECT and
**	E_GE98BC_OTHER_ERROR and added error for user-raised errors.
**   19-may-1989 (jrb)
**	Added #defs for WARNING code.
**	 29-oct-1992 (walt)
**		Changed '08' and '09' to '8' and '9' in defns of GESC_RSV08 and 
**		GESC_RSV09 to silence compiler warnings about non-octal digits.
**/
 
/* generic error codes */
 
#define GE_NO_MORE_DATA       100	/* End of data */
#define E_GE0064_NO_MORE_DATA		100

#define GE_OK                   0	/* Successful completion */
#define E_GE0000_OK		0

#define GE_WARNING		50	/* Warning occurred */
#define E_GE0032_WARNING		50
 
#define GE_TABLE_NOT_FOUND	30100   /* Table not found in DB */
#define E_GE7594_TABLE_NOT_FOUND	30100

#define GE_COLUMN_UNKNOWN	30110   /* Column not known/not in table */
#define E_GE759E_COLUMN_UNKNOWN		30110

#define GE_CURSOR_UNKNOWN	30120   /* Unknown cursor */
#define E_GE75A8_CURSOR_UNKNOWN		30120

#define GE_NOT_FOUND		30130   /* Other database object not found */
#define E_GE75B2_NOT_FOUND		30130

#define GE_UNKNOWN_OBJECT	30140   /* Other unknown/unavailable resource */
#define E_GE75BC_UNKNOWN_OBJECT		30140
 

#define GE_DEF_RESOURCE    30200   /* Duplicate resource def. */
#define E_GE75F8_DEF_RESOURCE		30200

#define GE_INS_DUP_ROW     30210   /* Attempt to insert invalid duplicate row */
#define E_GE7602_INS_DUP_ROW		30210

 
#define GE_SYNTAX_ERROR    31000   /* Statement syntax error */
#define E_GE7918_SYNTAX_ERROR		31000

#define GE_INVALID_IDENT   31100   /* Invalid identifier */
#define E_GE797C_INVALID_IDENT		31100

#define GE_UNSUP_LANGUAGE  31200   /* Unsupported query language */
#define E_GE79E0_UNSUP_LANGUAGE		31200

#define GE_QUERY_ERROR     32000   /* Inconsistent or incorrect query
                                            specification */
#define E_GE7D00_QUERY_ERROR		32000

#define GE_LOGICAL_ERROR   33000   /* Run-time logical error */
#define E_GE80E8_LOGICAL_ERROR		33000

#define GE_NO_PRIVILEGE    34000   /* Not privileged/restricted operation */
#define E_GE84D0_NO_PRIVILEGE		34000
 

#define GE_SYSTEM_LIMIT    36000   /* System limit exceeded */
#define E_GE8CA0_SYSTEM_LIMIT		36000

#define GE_NO_RESOURCE     36100   /* Out of needed resource */
#define E_GE8D04_NO_RESOURCE		36100

#define GE_CONFIG_ERROR    36200   /* System configuration error */
#define E_GE8D68_CONFIG_ERROR		36200
 

#define GE_COMM_ERROR      37000   /* Communication or transmission error */
#define E_GE9088_COMM_ERROR		37000

#define GE_GATEWAY_ERROR   38000   /* Error in the gateway */
#define E_GE9470_GATEWAY_ERROR		38000

#define GE_HOST_ERROR      38100   /* Host system error */
#define E_GE94D4_HOST_ERROR		38100

#define GE_FATAL_ERROR     39000   /* Session terminated */
#define E_GE9858_FATAL_ERROR		39000

#define GE_OTHER_ERROR     39100   /* Unmappable error */
#define E_GE98BC_OTHER_ERROR		39100
 

#define GE_CARDINALITY     40100   /* Cardinality violation */
#define E_GE9CA4_CARDINALITY		40100

#define GE_DATA_EXCEPTION  40200   /* Data Exception, with subcode.
                                       The subcode is identified by
                                       SQLCODE - GE_DATA_EXCEPTION */

/* The following codes are a combination of the above error with the	    */
/* subcodes added in (see generic error subcodes later in this file)	    */
#define E_GE9D08_DATAEX_NOSUB		40200
#define E_GE9D09_DATAEX_TRUNC		40201
#define E_GE9D0A_DATAEX_NEED_IND	40202
#define E_GE9D0B_DATAEX_NUMOVR		40203
#define E_GE9D0C_DATAEX_ASSGN		40204
#define E_GE9D0D_DATAEX_FETCH0		40205
#define E_GE9D0E_DATAEX_DTINV		40206
#define E_GE9D0F_DATAEX_DATEOVR		40207
#define E_GE9D10_DATAEX_RSV08		40208
#define E_GE9D11_DATAEX_INVIND		40209
#define E_GE9D12_DATAEX_CURSINV		40210
#define E_GE9D17_DATAEX_TYPEINV		40215
#define E_GE9D1C_DATAEX_FIXOVR		40220
#define E_GE9D1D_DATAEX_EXPOVR		40221
#define E_GE9D1E_DATAEX_FPDIV		40222
#define E_GE9D1F_DATAEX_FLTDIV		40223
#define E_GE9D20_DATAEX_DCDIV		40224
#define E_GE9D21_DATAEX_FXPUNF		40225
#define E_GE9D22_DATAEX_EPUNF		40226
#define E_GE9D23_DATAEX_DECUNF		40227
#define E_GE9D24_DATAEX_OTHER		40228

#define GE_CONSTR_VIO      40300   /* Constraint violation */
#define E_GE9D6C_CONSTR_VIO		40300

#define GE_CUR_STATE_INV   40400   /* Invalid cursor state */
#define E_GE9DD0_CUR_STATE_INV		40400

#define GE_TRAN_STATE_INV  40500   /* Invalid transaction state */
#define E_GE9E34_TRAN_STATE_INV		40500

#define GE_INV_SQL_STMT_ID 40600   /* Invalid SQL Statement ID */
#define E_GE9E98_INV_SQL_STMT_ID	40600

#define GE_TRIGGER_DATA    40700   /* Triggered data change violation */
#define E_GE9EFC_TRIGGER_DATA		40700

#define GE_USER_ID_INV     41000   /* Invalid user auth. ID */
#define E_GEA028_USER_ID_INV		41000

#define GE_SQL_STMT_INV    41200   /* Invalid SQL statement */
#define E_GEA0F0_SQL_STMT_INV		41200

#define GE_RAISE_ERROR     41300   /* Error was raised by user */
#define E_GEA154_RAISE_ERROR		41300

#define GE_DUP_SQL_STMT_ID 41500   /* Duplicate SQL statement ID */
#define E_GEA21C_DUP_SQL_STMT_ID	41500

#define GE_SERIALIZATION   49900   /* Serialization failure (deadlock) */
#define E_GEC2EC_SERIALIZATION		49900
 
 
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
#define GESC_RSV08      8     /* Reserved */
#define GESC_INVIND     9     /* Invalid indicator parameter value */
 
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
