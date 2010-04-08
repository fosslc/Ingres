/* 
** Copyright (c) 2004, 2009 Ingres Corporation 
*/ 
/** 
** Name: iiodbc.h - ODBC CLI definitions
** 
** Description: 
**   This file contains type casts and structures used by the Ingres ODBC
**   CLI.
** 
** History:
**   07-jul-04 (Ralph.Loen@ca.com)
**      Created.
**   03-sep-04 (Ralph.Loen@ca.com)  Bug 112975
**      Added TraceInit().
**   17-feb-05 (Raph.Loen@ca.com)  SIR 113913
**      Add information for SQLConfigData(), SQLDataSource(), and
**      SQLInstallerError().
**   14-JUL-2005 (hanje04)
**	Move all II_ODBCDRIVER_FN defines to iiodbcfn.h as GLOBALREFs as mg5_osx
**	doesn't allow multiply defined symbols. GLOBALDEF's are now in 
**	common!odbc!manager manager.c.
**      13-Mar-2007 (Ralph Loen) SIR 117786
**      Added GLOBALREF to ODBC state table.  Added Unicode/multi-byte
**      conversion routines.
**   30-Jan-2008 (Ralph Loen) SIR 119723
**      Added sqlstate SQL_HY107.
**   23-Mar-2009 (Ralph Loen) Bug 121838
**      Add support for SQLGetFunctions().
*/

#define BOOL bool

# define ODBC_TR_TRACE               0x0001
# define ODBC_EX_TRACE               0x0003
# define ODBC_IN_TRACE               0x0005
# define ODBC_TRACE_LEVEL            GetODBCTrace()
# define ODBC_EXEC_TRACE(level) if ( (level) <= ODBC_TRACE_LEVEL )  TRdisplay
# define ODBC_TRACE(level, fnc) if ( (level) <= ODBC_TRACE_LEVEL )  fnc

# define ODBC_TRACE_ENTRY(level, fnc, ret) \
    if ( (level) <= ODBC_TRACE_LEVEL )  (ret) = (fnc)

# define ODBC_INITTRACE()      IIodbc_initTrace()
# define ODBC_TERMTRACE()      IIodbc_termTrace()

#define SQL_HANDLE_IIODBC_CB   0  /* special handle type for locking */

/*
** Unicode API code extensions.  See sql.h.
*/

#define SQL_API_SQLCOLATTRIBUTEW      2000
#define SQL_API_SQLCONNECTW           2001
#define SQL_API_SQLDESCRIBECOLW       2002
#define SQL_API_SQLERRORW             2003
#define SQL_API_SQLEXECDIRECTW        2004
#define SQL_API_SQLGETCONNECTATTRW    2005
#define SQL_API_SQLGETCURSORNAMEW     2006
#define SQL_API_SQLSETDESCFIELDW      2007
#define SQL_API_SQLGETDESCFIELDW      2008
#define SQL_API_SQLGETDESCRECW        2009
#define SQL_API_SQLGETDIAGFIELDW      2010
#define SQL_API_SQLGETDIAGRECW        2011
#define SQL_API_SQLPREPAREW           2012
#define SQL_API_SQLSETCONNECTATTRW    2013
#define SQL_API_SQLSETCURSORNAMEW     2014
#define SQL_API_SQLCOLUMNSW           2015
#define SQL_API_SQLGETCONNECTOPTIONW  2016
#define SQL_API_SQLGETINFOW           2017
#define SQL_API_SQLGETTYPEINFOW       2018
#define SQL_API_SQLSETCONNECTOPTIONW  2019
#define SQL_API_SQLSPECIALCOLUMNSW    2020
#define SQL_API_SQLSTATISTICSW        2021
#define SQL_API_SQLTABLESW            2022
#define SQL_API_SQLDRIVERCONNECTW     2023
#define SQL_API_SQLGETSTMTATTRW       2024
#define SQL_API_SQLSETSTMTATTRW       2025
#define SQL_API_SQLNATIVESQLW         2026
#define SQL_API_SQLPRIMARYKEYSW       2027
#define SQL_API_SQLPROCEDURECOLUMNSW  2028
#define SQL_API_SQLPROCEDURESW        2039
#define SQL_API_SQLFOREIGNKEYSW       2030
#define SQL_API_SQLTABLEPRIVILEGESW   2031
#define SQL_API_SQLDRIVERSW           2032

/*
**  SQL STATE values
**  Note: These values must match the error code table in error.c.
*/
#define SQL_01000        0
#define SQL_01004        1
#define SQL_01S02        2
#define SQL_01S05        3
#define SQL_07001        4
#define SQL_07006        5
#define SQL_07009        6  /* invalid descriptor index */
#define SQL_22001        7
#define SQL_22002        8
#define SQL_22003        9
#define SQL_22008       10
#define SQL_22018       11
#define SQL_22026       12
#define SQL_23000       13
#define SQL_24000       14
#define SQL_25000       15
#define SQL_34000       16
#define SQL_3C000       17
#define SQL_42000       18
#define SQL_IM001       19
#define SQL_IM002       20
#define SQL_IM003       21
#define SQL_IM007       22
#define SQL_IM009       23
#define SQL_HY000       24
#define SQL_HY001       25
#define SQL_HY004       26  /* invalid SQL data type */
#define SQL_HY007       27
#define SQL_HY008       28  /* operation canceled */
#define SQL_HY009       29
#define SQL_HY010       30
#define SQL_HY011       31  /* attribute cannot be set now */
#define SQL_HY013       32
#define SQL_HY015       33  /* no cursor name available */
#define SQL_HY016       34
#define SQL_HY019       35
#define SQL_HY021       36
#define SQL_HY024       37
#define SQL_HY090       38  /* invalid string or buffer length */
#define SQL_HY091       39  /* invalid descriptor field identifier */
#define SQL_HY092       40
#define SQL_S1094       41  /* invalid scale (2.x only) */
#define SQL_HY096       42  /* invalid information type */
#define SQL_HY103       43  /* invalid precision or scale */
#define SQL_HY104       44  /* invalid retrieval argumenT */
#define SQL_HY105       45  /* invalid parameter type */
#define SQL_HY106       46
#define SQL_HYC00       47
#define SQL_S1DE0       48
#define SQL_HY107       49
#define SQL_HY095       50

#define MAX_SQLSTATE_TABLE 51

#define II_GENERAL_ERR            0
#define II_INVALID_HWND           1
#define II_INVALID_REQUEST_TYPE   2
#define II_INVALID_NAME           3
#define II_INVALID_KEYWORD_VALUE  4
#define II_REQUEST_FAILED         5
#define II_LOAD_LIB_FAILED        6
#define II_OUT_OF_MEM             7

#define MAX_INSTALLER_TABLE 8

/*
**  Installer error values
**  Note: These values must match the installer error code table in error.c.
*/
#define SQL_01000        0
/* 
** Name: IIODBC_STATE - CLI error state table.
** 
** Description: 
**   Maps SQL state value to assciated message.
** 
** History: 
**   07-Jul-04 (loera01)
**      Created.
*/ 
typedef struct 
{
    char *value;
    char descript[80];
}  IIODBC_STATE;

/*
**  ODBC execution states.
*/

#define E0 0 
#define E1 1
#define E2 2
#define C0 3
#define C1 4
#define C2 5
#define C3 6
#define C4 7
#define C5 8
#define C6 9
#define S0 10
#define S1 11
#define S2 12
#define S3 13
#define S4 14
#define S5 15
#define S6 16
#define S7 17
#define S8 18
#define S9 19
#define S10 20
#define S11 21
#define S12 22
#define D0 23
#define D1 24

#define MAX_STATES 25
#define MAX_STRLEN 4

GLOBALREF char cli_state[MAX_STATES][MAX_STRLEN];

/*
** Function prototype for driver API functions.
*/
typedef SQLRETURN (SQL_API *IIODBC_DRIVER_FN)();

SQLINTEGER SQL_API SQLGetPrivateProfileString(
    const char *  szSection, const char *szEntry,
    const char *szDefault, char *szBuffer,
    SQLINTEGER cbBuffer, const char *szFile);

i4 GetODBCTrace();
RETCODE ConvertUCS2ToUCS4( u_i2 *p2, u_i4 *p4, SQLINTEGER len);
RETCODE ConvertUCS4ToUCS2( u_i4 *p4, u_i2 *p2, SQLINTEGER len);
RETCODE ConvertWCharToChar( SQLWCHAR * szWideValue, SQLINTEGER cbWideValue,
    char * rgbValue, SQLINTEGER cbValueMax, SWORD * pcbValue,
    SDWORD * pdwValue, SQLINTEGER * plenValue);
RETCODE ConvertCharToWChar( char *szValue, SQLINTEGER  cbValue,
    SQLWCHAR *rgbWideValue, SQLINTEGER cbWideValueMax, SWORD *pcbValue,
    SDWORD *pdwValue, SQLINTEGER  *plenValue);

