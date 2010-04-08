/*
//  SQLSTATE.H
//
//  SQLSTATE codes as used in IDMSODBC
//
//  Copyright (c) 1992 Ingres Corporation
//
//  Change History
//  --------------
//
//  date        programmer          description
//
//  12/03/1992  Dave Ross          Initial coding
*/

#ifndef _INC_SQLSTATE
#define _INC_SQLSTATE

#define SQST_SUCCESS               "00000"
#define SQST_DISCONNECT_ERROR      "01002"
#define SQST_DATA_TRUNCATED        "01004"
#define SQST_PRIV_NOT_REVOKED      "01006"
#define SQST_INV_CONNECT_ATTRIBUTE "01S00"
#define SQST_INV_NUMBER_PARMS      "07001"
#define SQST_DATA_TYPE_ATTR_VIOL   "07006"
#define SQST_CONNECT_FAILURE       "08001"
#define SQST_CONNECTION_IN_USE     "08002"
#define SQST_CONNECTION_NOT_OPEN   "08003"
#define SQST_CONNECT_REJECTED      "08004"
#define SQST_CONNECT_FAIL_TRAN     "08007"
#define SQST_COMMUNICATION_FAILURE "08S01"
#define SQST_INV_INSERT_VALUES     "21S01"
#define SQST_INV_COLUMN_LIST       "21S02"
#define SQST_STRING_TRUNCATED      "22001"
#define SQST_INV_NUMERIC_VALUE     "22003"
#define SQST_ASSIGNMENT_ERROR      "22005"
#define SQST_DATETIME_OVERFLOW     "22008"
#define SQST_DIVISION_BY_ZERO      "22012"
#define SQST_STRING_LEN_ERROR      "22026"
#define SQST_CONSTRAINT_VIOLATION  "23000"
#define SQST_INV_CURSOR_STATE      "24000"
#define SQST_INV_TRAN_STATE        "25000"
#define SQST_INV_AUTH_SPEC         "28000"
#define SQST_INV_CURSOR_NAME       "34000"
#define SQST_SYNTAX_ERROR_1        "37000"
#define SQST_DUP_CURSOR_NAME       "3C000"
#define SQST_SERIALIZATION_FAILURE "40001"
#define SQST_SYNTAX_ERROR_2        "42000"
#define SQST_OPERATION_ABORTED     "70100"
#define SQST_TABLE_EXISTS          "S0001"
#define SQST_TABLE_NOT_FOUND       "S0002"
#define SQST_INDEX_EXISTS          "S0011"
#define SQST_INDEX_NOT_FOUND       "S0012"
#define SQST_COLUMN_EXISTS         "S0021"
#define SQST_COLUMN_NOT_FOUND      "S0022"
#define SQST_GENERAL_ERROR         "S1000"
#define SQST_MALLOC_ERROR          "S1001"
#define SQST_INV_COLUMN_NUMBER     "S1002"
#define SQST_INV_PROG_TYPE         "S1003"
#define SQST_INV_SQL_TYPE          "S1004"
#define SQST_CANCELED              "S1008"
#define SQST_INV_ARG_VALUE         "S1009"
#define SQST_FUNC_SEQ_ERROR        "S1010"
#define SQST_INV_TRAN_OPCODE       "S1012"
#define SQST_NO_CURSOR_NAME        "S1015"
#define SQST_INV_BUFFER_LENGTH     "S1090"
#define SQST_INV_DESCRIPTOR_TYPE   "S1091"
#define SQST_INV_OPTION_TYPE       "S1092"
#define SQST_INV_PARM_NUMBER       "S1093"
#define SQST_INV_SCALE_VALUE       "S1094"
#define SQST_INV_FUNCTION_TYPE     "S1095"
#define SQST_INV_INFO_TYPE         "S1096"
#define SQST_INV_COLUMN_TYPE       "S1097"
#define SQST_INV_SCOPE_TYPE        "S1098"
#define SQST_INV_NULLABLE_TYPE     "S1099"
#define SQST_INV_UNIQUE_OPTION     "S1100"
#define SQST_INV_ACCURACY_OPTION   "S1101"
#define SQST_INV_TABLE_TYPE        "S1102"
#define SQST_INV_DIRECTION_OPTION  "S1103"
#define SQST_INV_FETCH_TYPE        "S1106"
#define SQST_INV_ROW_VALUE         "S1107"
#define SQST_INV_CONCURRENCY_OPT   "S1108"
#define SQST_INV_CURS_POS_NO_KEYS  "S1109"
#define SQST_INV_DRIVER_COMPLETION "S1110"
#define SQST_DRIVER_CANT_DO        "S1C00"
#define SQST_NO_DATA_PENDING       "S1DE0"
#define SQST_TIMEOUT               "S1T00"

#endif  /* _INC_SQLSTATE */
