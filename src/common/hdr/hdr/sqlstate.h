/*
**    Copyright (c) 1992, Ingres Corporation
*/

/*
** Name: SQLSTATE.H - Contains #defines for all ANSI sqlstates
**
** Description:
**      This file contains the #define entries for all user errors
**      known to Ingres.  All are constructed by "oring" the
**      E_US_MASK mask with the error number in question.
**
**      Note that although users are used to seeing errors in
**      decimal, we still construct the error numbers in hex
**      so that they satisfy all the silly compilers on which
**      we have to run.
**
** History:
**	21-sep-92 (andre)
**	    updated descriptions/numbers of the SQLSTATEs from the SQL92 +
**	    defined constants (base 36; 0=0, ... 9=9, A=10, ... Z=35) to
**	    correspond to classes and subclasses
**	08-oct-93 (rblumer)
**	    added subclasses for each of the 4 types of REFERENTIAL constraint
**	    violations; while I was at it, also added subclasses for CHECK and
**	    UNIQUE constraints.  See subclasses 23500-23505.
**	10-mar-94 (andre)
**	    defined SS5000N_OPERAND_TYPE_MISMATCH, 
**	    SS5000O_INVALID_FUNC_ARG_TYPE, SS5000P_TIMEOUT_ON_LOCK_REQUEST, 
**	    SS5000Q_DB_REORG_INVALIDATED_QP, and SS5000R_RUN_TIME_LOGICAL_ERROR
**	15-mar-94 (andre)
**	    removed definitions of 37000- and 2A000-series SQLSTATEs as they are
**	    being removed from SQL92.
**	10-oct-1996 (canor01)
**	    Change SS50000_MISC_ING_ERRORS to SS50000_MISC_ERRORS.
**	20-oct-98 (inkdo01)
**	    Added SS23001_REF_RESTRICT_VIOLATION for ref constraint violation
**	    on delete/update of referenced table with RESTRICT action.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-mar-02 (inkdo01)
**	    Added SS2200H_SEQ_EXCEEDED for next value for sequence overflow.
*/


/*
** Terminology:
**
**    <extended statement/cursor name> -
**      extended name consists of keyword GLOBAL or LOCAL followed by <parameter
**      name> or <embedded variable name>.  Scope of GLOBAL <extended statement
**      name> or <extended cursor name> is SQL-session whereas scope of LOCAL
**      <extended statement name> or <extended cursor name> is the <module> in
**      which it appears.
**
**      One may prepare a <cursor specification> giving it <extended statement
**      name> in which case one may be able to declare a dynamic extended cursor
**      by using <allocate cursor statement>.
**
**    <row subquery> -
**      specifies a row derived from a <query expression>;
**      must have degree > 1 and cardinality <= 1
**
**    <scalar subquery> -
**      specifies a scalar value derived from a <query expression>;
**      must have degree == 1 and cardinality <= 1
**
**    SQL-agent -
**      an implementation-dependent entity that causes the execution of
**      SQL-statements
**
**    procedure -
**      SQL92 defines a procedure as consisting of a <procedure name>, a
**      sequence of <parameter declaration>s, and a single <SQL procedure
**      statement>; SQL92 defines a mechanism for deriving from an ESQL program
**      of a host language program and a MODULE consisting of procedures and
**      declarations - thus references to procedures should be understood to
**      mean references to ESQL statements which were a part of ESQL program
**
**    character repertoire -
**      a set of characters used for a specific purpose or application
**
*/

/*******************************************************************************
*******************************************************************************/

/*
** ambiguous cursor name                3C000   (pp. 370, 385)
**
** shall be returned if
**   - a user is attempting to prepare a statement P which is a
**    <preparable dynamic delete statement: positioned> or a
**    <preparable dynamic update statement: positioned> and there exist both an
**    extended dynamic cursor and a dynamic cursor with the same name as that
**    referenced in P or
**  - a user is attempting to EXECUTE IMMEDIATE a statement P which is a
**    <preparable dynamic delete statement: positioned> or a
**    <preparable dynamic update statement: positioned> and there exist both an
**    extended dynamic cursor and a dynamic cursor with the same name as that
**    referenced in P 
**
** Generic Error Mapping:
**    This error cannot currently occur in Ingres because it requires extended 
**    dynamic cursors.  The closest approximation is E_GE9DD0_CUR_STATE_INV.
*/

#define      SS3C000_AMBIG_CURS_NAME          "3C000" /* E_GE9DD0_CUR_STATE_INV */
#define      SS_3C000_CLASS                   120 /* (3 * 36 + 12) */
#define      SS_3C000_GE_MAP                  E_GE9DD0_CUR_STATE_INV

/*
** cardinality violation                21000   (pp. 165, 316)
**
**   shall be returned if cardinality of a <row subquery> or a <scalar subquery>
**   is greater than 1 or if cardinality of a result of a <query specification>
**   in <select statement: single row> is greater than 1
**
** Generic Error Mapping:
**    E_GE9CA4_CARDINALITY
*/

#define      SS21000_CARD_VIOLATION           "21000" /* E_GE9CA4_CARDINALITY */
#define      SS_21000_CLASS                   73 /* 2 * 26 + 1 */
#define      SS_21000_GE_MAP                  E_GE9CA4_CARDINALITY

/*
** connection exception         08000   (pp. 56, 287, 342, 344, 346, 445)
**
** Generic Error Mapping:
**	E_GE98BC_OTHER_ERROR
*/

#define      SS08000_CONNECTION_EXCEPTION     "08000" /* E_GE98BC_OTHER_ERROR */
#define      SS_08000_CLASS                   8
#define      SS_08000_GE_MAP                  E_GE98BC_OTHER_ERROR

#define	     SS5000B_OTHER_ERROR              E_GE98BC_OTHER_ERROR

/*
** connection does not exist    08003   (pp. 287, 344, 346, 445)
**
**   shall be returned if an attempt is made 
**    - to execute a procedure (read "ESQL-statement") by an SQL-agent and there
**      is no current SQL-session and SQL-agent may not connect to a default
**      SQL-session associated with SQL-agent 
**      or
**    - to select an SQL-connection from the available SQL-connections by means
**      of <set connection statement> and either
**        - DEFAULT is specified and there is no default SQL-connection that is
**          current or dormant for the current SQL-agent or
**        - <connection name> is specified which does not identify an
**          SQL-session that is current or dormant for the current SQL-agent
**      or
**    - to terminate an SQL-connection and either
**        - DEFAULT is specified and there is no default SQL-connection that is
**          current or dormant for the current SQL-agent or
**        - <connection name> is specified which does not identify an
**          SQL-session that is current or dormant for the current SQL-agent
**      or
**    - to execute a <direct SQL statement> by an SQL-agent and there is no
**      current SQL-session and SQL-agent may not connect to a default
**      SQL-session associated with SQL-agent
**
** Generic Error Mapping:
**    Ingres behavior is at odds with the standard on two counts:
**    (1) In some cases, Ingres does not detect connection errors
**        when the connection statement is executed.  For example, switching 
**        to an invalid or 'dead' connection via set_sql doesn't yield 
**        an error (E_GE9088_COMM_ERROR) until the first database statement 
**        is issued.
**    (2) Ingres doesn't support a DEFAULT connection.  However, if
**        an application issues database SQL without an established 
**        connection Ingres reports a E_GE9088_COMM_ERROR.
**    The one case which does map well occurs when a DISCONNECT of a 
**    non-existent session is issued.  A Generic Error of E_GE80E8_LOGICAL_ERROR
**    is returned.  This value will be used for mapping purposes.
**
** NOTE:
**    Connection errors have undergone significant change in SQL92 since
**    the March, 1992 draft.  We will need to update the '08xxx' section
**    accordingly.
*/

#define      SS08003_NO_CONNECTION            "08003" /* E_GE80E8_LOGICAL_ERROR */
#define      SS_08003_SUBCLASS                3
#define      SS_08003_GE_MAP                  E_GE80E8_LOGICAL_ERROR

/*
** connection failure           08006   (pp. (56), 344)
**
**   shall be returned if an existing dormant connection could not be made
**   current by means of <set connection statement>
**
** Generic Error Mapping:
**    Ingres doesn't detect this at set_sql, but at the next database
**    statement (see "08003" discussion, above). 
**    E_GE9088_COMM_ERROR
*/

#define      SS08006_CONNECTION_FAILURE       "08006" /* E_GE9088_COMM_ERROR */
#define      SS_08006_SUBCLASS                6
#define      SS_08006_GE_MAP                  E_GE9088_COMM_ERROR

/*
** connection name in use       08002   (p. 342)
**
**   shall be returned if an attempt is made to establish an SQL-connection by
**   means of <connect statement> and either
**    - the user specified a connection name that matches a name of a
**      connecgtions that has already been established by the current SQL-agent
**      and has not been disconnected or
**    - if DEFAULT was specified and a default SQL-connection has already been
**      established by the current SQL-agent and has not been disconnected
**
** Generic Error Mapping:
**    E_GE80E8_LOGICAL_ERROR.
*/

#define      SS08002_CONNECT_NAME_IN_USE      "08002" /* E_GE80E8_LOGICAL_ERROR */
#define      SS_08002_SUBCLASS                2
#define      SS_08002_GE_MAP                  E_GE80E8_LOGICAL_ERROR

/*
** SQL-client unable to establish SQL-connection        08001   (p. 342)
**
**   shall be returned if the SQL-client cannot establish the SQL-connection
**   (by means of <connect statement>)
**
** Generic Error Mapping:
**    On a connection failure, Ingres returns a E_GE98BC_OTHER_ERROR.
**    This seems lame, but who said compatibility needed to 
**    make sense?  Use E_GE98BC_OTHER_ERROR.
*/

#define      SS08001_CANT_GET_CONNECTION      "08001" /* E_GE98BC_OTHER_ERROR */
#define      SS_08001_SUBCLASS                1
#define      SS_08001_GE_MAP                  E_GE98BC_OTHER_ERROR

/*
** SQL-server rejected establishment of SQL-connection  08004   (p. 342)
**
**   shall be returned if the SQL-server rejects the establishment of the
**   SQL-connection (by means of <connect statement>)
**
** Generic Error Mapping:
**    When a server is refusing connections due to an iimonitor 
**    'set server shut,' we issue a E_GE94D4_HOST_ERROR.
*/

#define      SS08004_CONNECTION_REJECTED      "08004" /* E_GE94D4_HOST_ERROR */
#define      SS_08004_SUBCLASS                4
#define      SS_08004_GE_MAP                  E_GE94D4_HOST_ERROR

/*
** transaction resolution unknown   08007 (p. 56)
**
** shall be returned if loss of current SQL-connection is detected during
** the execution of a <commit statement>; indicates that the SQL-implementation
** cannot verify whether the SQL-transaction was committed successfully,
** rolled back, or left active.
**
** Generic Error Mapping:
**	E_GE9088_COMM_ERROR
*/

#define      SS08007_XACT_RES_UNKNOWN         "08007" /* E_GE9088_COMM_ERROR */
#define      SS_08007_SUBCLASS                7
#define      SS_08007_GE_MAP                  E_GE9088_COMM_ERROR

/*
** LDB is unavailable (INGRES-specific SQLSTATE) 	08500
**
** shall be returned if
**	an attempt to connect to an LDB was rejected (because the specified 
**	LDB is unavailable or does not exist)
**
** Generic Error Mapping:
**	E_GE75BC_UNKNOWN_OBJECT
*/

#define      SS08500_LDB_UNAVAILABLE          "08500" /* E_GE75BC_UNKNOWN_OBJECT */
#define      SS_08500_SUBCLASS                6480 /* 5 * 36**2 */
#define      SS_08500_GE_MAP                  E_GE75BC_UNKNOWN_OBJECT

/*
** data exception               22000   (pp. 88, 89, 99, 107-109, 115-122, 127,
**                                           130, 131, 133, 176, 191-194, 287,
**                                           298, 354, 364, 367)
**
** Generic Error Mapping:
**   E_GE9D08_DATAEX_NOSUB
*/

#define      SS22000_DATA_EXCEPTION           "22000" /* E_GE9D08_DATAEX_NOSUB */
#define      SS_22000_CLASS                   74 /* 2 * 36 + 2 */
#define      SS_22000_GE_MAP                  E_GE9D08_DATAEX_NOSUB

/*
** character not in repertoire  22021   (p. 88)
** 
** shall be returned if any specification or operation attempts to
** cause an item of a character type to contain a character that is not a
** member of the character repertoire associated with the character
** item.
**
** Generic Error Mapping:
**    This error cannot occur in Ingres; Ingres SQL does not support 
**    selectable character sets.
**    E_GE9D08_DATAEX_NOSUB
*/

#define      SS22021_CHAR_NOT_IN_RPRTR        "22021" /* E_GE9D08_DATAEX_NOSUB */
#define      SS_22021_SUBCLASS                73 /* 2 * 36 + 1 */
#define      SS_22021_GE_MAP                  E_GE9D08_DATAEX_NOSUB

/*
** datetime field overflow      22008   (p. 133)
**
** shall be returned if as a result of evaluating a <datetime value expression>
** any <datetime field> of the result is outside the permissible range of
** values for the field or the result is invalid based on the natural rules
** for dates and times
**
** Generic Error Mapping:
**    E_GE9D0E_DATAEX_DTINV 
*/

#define      SS22008_DATETIME_FLD_OVFLOW      "22008" /* E_GE9D0E_DATAEX_DTINV */
#define      SS_22008_SUBCLASS                8
#define      SS_22008_GE_MAP                  E_GE9D0E_DATAEX_DTINV

/*
** division by zero             22012   (p. 127)
**
** shall be returned if the value of a divisor in a <numeric value expression>
** is 0
**
** Generic Error Mapping:
**   There are separate Generic Errors for fixed point and floating point
**   divide by zero.  The solution to this problem is not very elegant, but 
**   that's the best I could come up with:
**    - ALL DBMS messages that presently map into E_GE9D1E_DATAEX_FPDIV,
**	E_GE9D1F_DATAEX_FLTDIV, and E_GE9D20_DATAEX_DCDIV will map into this
**	SQLSTATE.  
**    - A static array of structures (division_by_zero[]) mapping DBMS errors 
**	corresponding to this SQLSTATE into appropriate Generic Error will be 
**	declared inside the function performing (SQLSTATE[, dbms error]) -> GE 
**	mapping; this will enable it to return appropriate GE when presented 
**	with ("22012", dbms_error) pair
**    - (*THIS IS IMPORTANT*) in the future, for every new DBMS messages which 
**	maps into "22012", if it is desired that it be 
**	mapped into one of the above-mentioned generic errors, a new row will 
**	need to be added to division_by_zero[]; otherwise, the function will 
**	return E_GE9D24_DATAEX_OTHER.
**
**   E_GE9D24_DATAEX_OTHER
*/

#define      SS22012_DIVISION_BY_ZERO         "22012" /* E_GE9D24_DATAEX_OTHER */
#define      SS_22012_SUBCLASS                38 /* 1 * 36 + 2 */
#define      SS_22012_GE_MAP                  E_GE9D24_DATAEX_OTHER

/*
** error in assignment          22005   (pp. 364, 367)
**
** shall be returned if 
**      - requesting information from an SQL descriptor area by means of 
**        <get descriptor statement>, and DATA is specified as the
**        <descriptor item name>, and the datatype of the variable or
**        parameter which is associated with the <descriptor item name>
**        does not match the type specified by the item descriptor area
**      or
**      - setting information in an SQL descriptor area by means of
**        <set descriptor statement>, and DATA is specified as the
**        <descriptor item name>, and the datatype of the variable or
**        parameter which is associated with the <descriptor item name>
**        does not match the data type specified by the item descriptor area
**
** Generic Error Mapping:
**    E_GE9D0C_DATAEX_ASSGN 
*/

#define      SS22005_ASSIGNMENT_ERROR         "22005" /* E_GE9D0C_DATAEX_ASSGN */
#define      SS_22005_SUBCLASS                5
#define      SS_22005_GE_MAP                  E_GE9D0C_DATAEX_ASSGN

/*
** indicator overflow           22022   (p. 191)
**
** shall be returned if in an assignment of V to T for retrieving SQL-data
** V is not the null value and T has an indicator and the data type of T
** is character string or bit string and the length M of V is greater than the
** length of T (which requires that the indicator be set to M) and M exceeds the
** maximum value that the indicator can contain
**
** Generic Error Mapping:
**    E_GE9D11_DATAEX_INVIND 
*/

#define      SS22022_INDICATOR_OVFLOW         "22022" /* E_GE9D11_DATAEX_INVIND */
#define      SS_22022_SUBCLASS                74 /* 2 * 36 + 2 */
#define      SS_22022_GE_MAP                  E_GE9D11_DATAEX_INVIND

/*
** interval field overflow      22015   (pp. 122, 192, 194)
**
** shall be returned if 
**      - specifying data conversion by means of <cast specification> and
**        the target datatype TD is interval and either
**         - the datatype of the <cast operand> SD is exact numeric and the 
**           representation of <cast operand> in the datatype TD would result 
**           in the loss of leading significant digits or
**         - the datatype of the <cast operand> SD is interval, and TD
**           and SD have different interval precisions, and converting
**           the <cast operand> to a scalar according to the natural
**           rules for intervals as defined in the Gregorian calendar
**           and normalizing the result to conform to the datetime
**           qualifier "P TO Q" of TD would result in loss of precision
**           of the leading datetime field
**      or
**      - in an assignment of V to T for storing or retrieving SQL-data the 
**        datatype of T is interval and there is no representation of the 
**        value of V in the datatype of T
**
** Generic Error Mapping:
**    E_GE9D0F_DATAEX_DATEOVR
*/

#define      SS22015_INTERVAL_FLD_OVFLOW      "22015" /* E_GE9D0F_DATAEX_DATEOVR*/
#define      SS_22015_SUBCLASS                41 /* 1 * 36 + 5 */
#define      SS_22015_GE_MAP                  E_GE9D0F_DATAEX_DATEOVR

/*
** invalid character value for cast     22018   (pp. 116-122)
**
** shall be returned if specifying data conversion by means of 
** <cast specification> and TD is the <data type> of <cast target>
** and SD is the data type of the <cast operand> and
**      - TD is exact numeric, SD is a character string, and the 
**        <cast operand> does not comprise a <signed numeric literal> or
**      - TD is approximate numeric, SD is a character string, and the 
**        <cast operand> does not comprise a <signed numeric literal> or
**      - TD is a fixed-length character string, SD is exact numeric, and
**        the character repertoire of TD does not include some 
**        <SQL language character>s required to represent the value of
**        the <cast operand> as a <exact numeric literal> or
**      - TD is a fixed-length character string, SD is approximate numeric, and
**        the character repertoire of TD does not include some
**        <SQL language character>s required to represent the value of
**        the <cast operand> as an <approximate numeric literal> or
**      - TD is a fixed-length character string, SD is a datetime data
**        type or an interval data type and the character repertoire of
**        TD does not include some <SQL language character>s required to 
**        represent the value of the <cast operand> as a <literal> or 
**      - TD is a variable-length character string, SD is exact numeric, and
**        the character repertoire of TD does not include some 
**        <SQL language character>s required to represent the value of
**        the <cast operand> as a <exact numeric literal> or
**      - TD is a variable-length character string, SD is approximate numeric, 
**        and the character repertoire of TD does not include some
**        <SQL language character>s required to represent the value of
**        the <cast operand> as an <approximate numeric literal> or
**      - TD is a variable-length character string, SD is a datetime data
**        type or an interval data type and the character repertoire of
**        TD does not include some <SQL language character>s required to 
**        represent the value of the <cast operand> as a <literal> or 
**      - TD is the datetime data type DATE, TIME, or TIMESTAMP, SD is a 
**        character string, and a value of type TD may not be derived from 
**        the <cast operand> or
**      - TD is interval, SD is a character string, and a value of type
**        TD may not be derived from the <cast operand> or
**
** Generic Error Mapping:
**   A test (fetching a non-numeric character string with the int4 operator)
**   yielded:
**   E_GE7918_SYNTAX_ERROR
*/ 

#define      SS22018_INV_VAL_FOR_CAST         "22018" /* E_GE7918_SYNTAX_ERROR */
#define      SS_22018_SUBCLASS                44 /* 1 * 36 + 8 */
#define      SS_22018_GE_MAP                  E_GE7918_SYNTAX_ERROR

/*
** invalid datetime format      22007   (p. 89)
**
** shall be returned if any specification or operation attempts to cause
** an item of type datetime to take a value where the <datetime field> YEAR
** has the value not in (1, 9999)
**
** Generic Error Mapping:
**   E_GE9D0F_DATAEX_DATEOVR
*/

#define      SS22007_INV_DATETIME_FMT         "22007" /* E_GE9D0F_DATAEX_DATEOVR */
#define      SS_22007_SUBCLASS                7
#define      SS_22007_GE_MAP                  E_GE9D0F_DATAEX_DATEOVR

/*
** invalid escape character     22019   (p. 176)
**
** shall be returned if ESCAPE-clause was specified in <like predicate>
** and the length of characters in the result of <character value expression> 
** of the <escape character> is not equal to 1 
**
** Generic Error Mapping:
**   E_GE7918_SYNTAX_ERROR
*/

#define      SS22019_INV_ESCAPE_CHAR          "22019" /* E_GE7918_SYNTAX_ERROR */
#define      SS_22019_SUBCLASS                45 /* 1 * 36 + 9 */
#define      SS_22019_GE_MAP                  E_GE7918_SYNTAX_ERROR

/*
** invalid escape sequence      22025   (p. 176)
**
** shall be returned if P is the result of the <character value
** expression> of <pattern> in <like predicate>, E is the result of
** <character value expression> of the <escape character> in <like
** predicate> and there is not a partitioning of the string P into substring
** s.t. each substring has length 1 or 2, no substring of length 1 is the
** escape character E, and each substring of length 2 is the escape
** character E followed either by escape character E, '_', or '%'
**
** Generic Error Mapping:
**   E_GE7918_SYNTAX_ERROR
*/

#define      SS22025_INV_ESCAPE_SEQ           "22025" /* E_GE7918_SYNTAX_ERROR */
#define      SS_22025_SUBCLASS                77 /* 2 * 36 + 5 */
#define      SS_22025_GE_MAP                  E_GE7918_SYNTAX_ERROR

/*
** invalid parameter value      22023   (p. 287)
**
** shall be returned if the value of input parameter to a procedure
** provided by the SQL-agent or the value of an output parameter
** resulting from the execution of the <procedure> falls outside the set of
** allowed values of the data type of the parameter and IF the
** implementation-defined effect is the raising of exception condition
**
** Generic Error Mapping:
**   My test program was able to insert out-of-range integer values into
**   a smallint column without error.  I'm not sure how to generate this
**   condition in Ingres, but let's just assign it
**   E_GE9D08_DATAEX_NOSUB
*/

#define      SS22023_INV_PARAM_VAL            "22023" /* E_GE9D08_DATAEX_NOSUB */
#define      SS_22023_SUBCLASS                75 /* 2 * 36 + 3 */
#define      SS_22023_GE_MAP                  E_GE9D08_DATAEX_NOSUB

/*
** invalid time zone displacement value         22009   (pp. 133, 354)
**
** shall be returned if 
**      - <time zone> is specified or implied in a <datetime value expression> 
**        and its value is not between INTERVAL'-12:59' and INTERVAL'+13:00' or
**      - <interval value specification> is specified as the 
**        <set time zone value> in <set local time zone statement> and its 
**        value is the null value and is not between INTERVAL'-12:59' and
**        INTERVAL'+13:00'
**
** Generic Error Mapping:
**   Ingres doesn't have timezones in its intervals, so this can't happen.
**   E_GE9D0F_DATAEX_DATEOVR
*/

#define      SS22009_INV_TZ_DISPL_VAL         "22009" /* E_GE9D0F_DATAEX_DATEOVR */
#define      SS_22009_SUBCLASS                9
#define      SS_22009_GE_MAP                  E_GE9D0F_DATAEX_DATEOVR

/*
** null value, no indicator parameter   22002   (pp. 191, 364)
**
** shall be returned if 
**      - in an assignment of V to T for retrieving SQL-data V is the null 
**        value and no indicator is specified for T or
**      - a <get descriptor statement> is executed to get the value of
**        DATA without getting the value of INDICATOR and the value of
**        INDICATOR is negative
**
** Generic Error Mapping:
**   E_GE9D0A_DATAEX_NEED_IND
*/

#define      SS22002_NULLVAL_NO_IND_PARAM     "22002" /* E_GE9D0A_DATAEX_NEED_IND */
#define      SS_22002_SUBCLASS                2
#define      SS_22002_GE_MAP                  E_GE9D0A_DATAEX_NEED_IND

/*
** numeric value out of range   22003   (pp. 99, 115, 116, 127, 192, 194)
**
** shall be returned if
**      - <set function type> SUM is specified and the sum is not within
**        the range of the data type of the result 
**      or
**      - data conversion is specified by means of <cast specification> and 
**        TD is the <data type> of <cast target> and SD is the data type of 
**        the <cast operand> and SV is the value of the <cast operand>
**        and either
**          - TD is exact numeric and SD is exact numeric and there is no
**            representation of SV in data type TD that does not lose any 
**            leading significant digits after rounding or truncating if 
**            necessary or
**          - TD is exact numeric and SD is an interval data type and
**            there is no representation of SV in data type TD that does 
**            not lose any leading significant digits or
**          - TD is approximate numeric and SD is exact numeric or
**            approximate numeric and there is no representation of SV in
**            data type TD that does not lose any leading significant digits 
**            after rounding or truncating if necessary
**      or
**      - the type of the result of an arithmetic operation is exact
**        numeric and either
**          - the operator is not division and the mathematical result of
**            the operation is not exactly representable with the precision 
**            and scale of the result type or
**          - the operator is division and the approximate mathematical
**            result of the operation represented with the precision and
**            scale of the result type loses one or more leading significant 
**            digits after rounding or truncating if necessary
**      or
**      - the type of the result of an arithmetic operation is approximate
**        numeric and the exponent of the approximate mathematical result
**        of the operation is not within the implementation-defined exponent 
**        range for the result type
**      or
**      - in an assignment of V to T for retrieving or storing SQL-data the 
**        data type of T is numeric and there is no approximation obtained by 
**        rounding or truncation of the numerical value of V for the data 
**        type of T
**
** Generic Error Mapping:
**   This condition applies to both exact and approximate numerics.
**   The Generic Errors for overflow are specific to the datatype class.
**   The solution to this problem is not very elegant, but that's the best I 
**   could come up with:
**    - ALL DBMS messages that presently map into E_GE9D0B_DATAEX_NUMOVR,
**	E_GE9D1C_DATAEX_FIXOVR, E_GE9D1D_DATAEX_EXPOVR, E_GE9D21_DATAEX_FXPUNF,
**	E_GE9D22_DATAEX_EPUNF, and E_GE9D23_DATAEX_DECUNF will map into this
**	SQLSTATE.  
**    - A static array of structures (numeric_exception[]) mapping DBMS errors 
**	corresponding to this SQLSTATE into appropriate Generic Error will be 
**	declared inside the function performing (SQLSTATE[, dbms error]) -> GE 
**	mapping; this will enable it to return appropriate GE when presented 
**	with ("22003", dbms_error) pair
**    - (*THIS IS IMPORTANT*) in the future, for every new DBMS messages which 
**	maps into "22003", if it is desired that it be 
**	mapped into one of the above-mentioned generic errors, a new row will 
**	need to be added to numeric_exception[]; otherwise, the function will 
** 	return E_GE9D24_DATAEX_OTHER.
**
**   E_GE9D24_DATAEX_OTHER
*/

#define      SS22003_NUM_VAL_OUT_OF_RANGE     "22003" /* E_GE9D24_DATAEX_OTHER */
#define      SS_22003_SUBCLASS                3
#define      SS_22003_GE_MAP                  E_GE9D24_DATAEX_OTHER

/*
** string data, length mismatch 22026   (p. 194)
**
** shall be returned if in an assignment of V to T for storing SQL-data
** the data type of T is fixed length bit string of length (in bits) L
** and the length (in bits) of V is less than L
**
** Generic Error Mapping:
**   This condition applies to the fixed length 'bit' datatype and cannot
**   currently happen in Ingres.
**   E_GE9D08_DATAEX_NOSUB
*/

#define      SS22026_STRING_LEN_MISMATCH      "22026" /* E_GE9D08_DATAEX_NOSUB */
#define      SS_22026_SUBCLASS                78 /* 2 * 36 + 6 */
#define      SS_22026_GE_MAP                  E_GE9D08_DATAEX_NOSUB

/*
** string data, right truncation        22001   (pp. 117-120, 130, 131, 193-194)
**
** shall be returned if 
**      - data conversion is specified by means of <cast specification> and 
**        TD is the <data type> of <cast target> and and SD is the data type 
**        of the <cast operand> and SV is the value of the <cast operand> 
**        and TD is fixed-length character string or TD is variable-length 
**        character string and either
**            - SD is exact numeric and the length of representation of SV as
**              an <exact numeric literal> is greater than the length of TD or
**            - SD is approximate numeric and the length of representation 
**              of SV as an <approximate numeric literal> is greater than 
**              the length of TD or
**            - SD is a datetime data type or an interval data type and 
**              the length of SV represented as a <literal> is greater than 
**              the length of TD
**      or
**      - in a <string value expression> specifying <concatenation> or 
**        <bit concatenation> length of the result of <concatenation> or
**        <bit concatenation> of two strings is greater than the
**        implmentation-defined maximum length of a variable-length 
**        character string VL and the portion of the result that exceeds VL
**        contains at least one non-<space> character in the case of 
**        <concatenation> or at least one 1-valued bit in case of
**        <bit concatenation>
**      or
**      - in an assignment of V (of length VL) to T (of length TL) for storing
**	  SQL-data if the data type of T is fixed-length character string or
**	  variable-length character string and VL > TL and one or more of the
**	  rightmost VL-TL characters of V are not <space>s
**	or
**	- in an assignment of V to T for storing SQL-data if the data type of T
**	  is fixed-length bit string or variable-length bit string and the
**	  length (in bits) of V is greater than the length (in bits) of T
**
** Generic Error Mapping:
**   E_GE9D09_DATAEX_TRUNC
*/

#define      SS22001_STRING_RIGHT_TRUNC       "22001" /* E_GE9D09_DATAEX_TRUNC */
#define      SS_22001_SUBCLASS                1
#define      SS_22001_GE_MAP                  E_GE9D09_DATAEX_TRUNC

/*
** sequence value exceeded      2200H
**
** shall be returned if request for next value of a sequence generator 
** returns a value outside of the range minvalue <= n <= maxvalue and
** the cycle option of the sequence is NO CYCLE.
**
** Generic Error Mapping:
**   I don't know how this stuff works, but it originates as a QE00A0_SEQ_LIMIT_EXCEEDED
*/

#define      SS2200H_SEQ_EXCEEDED             "2200H" /* E_GE80E8_LOGICAL_ERROR */ 
#define      SS_2200H_SUBCLASS                18 /* 18 */
#define      SS_2200H_GE_MAP                  E_GE80E8_LOGICAL_ERROR

/*
** substring error              22011   (pp. 107, 109)
**
** shall be returned if specification of <character substring function>
** or <bit substring function> are specified in a way that would place
** the end of the substring before its beginning (although I cannot see how
** this could happen unless the length of substring were specified to be a
** negative number)
**
** Generic Error Mapping:
**   Perhaps this condition applies the Heisenberg Uncertainty Principle to SQL.
**   E_GE80E8_LOGICAL_ERROR
*/

#define      SS22011_SUBSTRING_ERROR          "22011" /* E_GE80E8_LOGICAL_ERROR */
#define      SS_22011_SUBCLASS                37 /* 1 * 36 + 1 */
#define      SS_22011_GE_MAP                  E_GE80E8_LOGICAL_ERROR

/*
** trim error                   22027   (p. 108)        ** same as string data,
**                                                      ** length mismatch
**
** shall be returned if length of (implicit or explicit) <trim character>
** in <trim function> is not 1
**
** Generic Error Mapping:
**   Ingres' trim function does not permit the specification of a trim
**   character, therefore this error cannot occur in Ingres.
**   E_GE7918_SYNTAX_ERROR
*/

#define      SS22027_TRIM_ERROR               "22027" /* E_GE7918_SYNTAX_ERROR */
#define      SS_22027_SUBCLASS                79 /* 2 * 36 + 7 */
#define      SS_22027_GE_MAP                  E_GE7918_SYNTAX_ERROR

/*
** unterminated C string                22024   (p. 298)
**
** shall be returned if in ESQL program the least significant character of 
** the value of the input parameter to a <procedure> does not contain the
** implementation-defined null character that terminates a C character
** string
**
** Generic Error Mapping:
**   I think this has to do with the SQL Module Language, which we do
**   not support.
**   E_GE98BC_OTHER_ERROR
*/

#define      SS22024_UNTERM_C_STRING          "22024" /* E_GE98BC_OTHER_ERROR */
#define      SS_22024_SUBCLASS                76 /* 2 * 36 + 4 */
#define      SS_22024_GE_MAP                  E_GE98BC_OTHER_ERROR

/*
** invalid data type (INGRES-specific SQLSTATE)		22500
**
** shall be returned if
**	invalid data type was specified
**
** Generic Error Mapping:
**	E_GE9D17_DATAEX_TYPEINV
*/

#define      SS22500_INVALID_DATA_TYPE        "22500" /* E_GE9D17_DATAEX_TYPEINV */
#define      SS_22500_SUBCLASS                6480 /* 5 * 36**2 */
#define      SS_22500_GE_MAP                  E_GE9D17_DATAEX_TYPEINV

/* 
** dependent privilege descriptors still exist          2B000   (p. 280)
**
** shall be returned if REVOKE ... RESTRICT was specified and there are 
** some privilege descriptors which would become abandoned as a result of
** revocation of specified privilege or GRANT OPTION FOR specified
** privilege
**
** Generic Error Mapping:
**   E_GE7D00_QUERY_ERROR
*/

#define      SS2B000_DEP_PRIV_EXISTS          "2B000" /* E_GE7D00_QUERY_ERROR */
#define      SS_2B000_CLASS                   83 /* 2 * 36 + 11 */
#define      SS_2B000_GE_MAP                  E_GE7D00_QUERY_ERROR

/*
** dynamic SQL error            07000   (pp. 360, 364, 366, 373, 380-383, 388,
**                                           390)
**
** Generic Error Mapping:
**   There are no conditions defined in the IS for this SQLSTATE; all the
**   references are for the specific '07' class errors below.  
**   E_GE7D00_QUERY_ERROR
**   
*/

#define      SS07000_DSQL_ERROR               "07000" /* E_GE7D00_QUERY_ERROR */
#define      SS_07000_CLASS                   7
#define      SS_07000_GE_MAP                  E_GE7D00_QUERY_ERROR

/*
** cursor specification cannot be executed      07003   (p. 383)
**
** shall be returned if an attempt is made to execute a previously
** prepared statement P where P is a <dynamic select statement> which does
** not conform to the Format and Syntax rules of a 
** <dynamic single row select statement>
**
** Generic Error Mapping:
**   E_GE7D00_QUERY_ERROR 
*/

#define      SS07003_CAN_EXEC_CURS_SPEC       "07003" /* E_GE7D00_QUERY_ERROR */
#define      SS_07003_SUBCLASS                3
#define      SS_07003_GE_MAP                  E_GE7D00_QUERY_ERROR

/*
** invalid descriptor count     07008   (pp. 380, 381)
**
** shall be returned if 
**      - a <using clause> is used in a <dynamic open statement> or as 
**        the <parameter using clause> in an <execute statement> and 
**        <using descriptor> is specified and the value of COUNT is greater
**        than the number of <occurences> specified when the <descriptor name> 
**        was allocated or is less than zero or
**      - a <using clause> is used in a <dynamic fetch statement> or as 
**        the <result using clause> in an <execute statement> and
**        <using descriptor> is specified and the value of COUNT is greater
**        than the number of <occurences> specified when the <descriptor name>
**        was allocated or is less than zero
**
** Generic Error Mapping:
**   Ingres has a similar condition wrt SQLDAs.  The generated error is
**   E_GE7D00_QUERY_ERROR 
*/

#define      SS07008_INV_DESCR_CNT            "07008" /* E_GE7D00_QUERY_ERROR */
#define      SS_07008_SUBCLASS                8
#define      SS_07008_GE_MAP                  E_GE7D00_QUERY_ERROR

/*
** invalid descriptor index     07009   (pp. 360, 364, 366)
**
** shall be returned if 
**      - an attempt is made to allocate an SQL descriptor area by means
**        of <allocate descriptor statement> and WITH MAX <occurrencies>
**        was specified and <occurrences> is less than 1 or greater than an
**        implementation-defined maximum value or
**      - an attempt is made to get/set information from an SQL descriptor
**        area by means of <get/set descriptor statement> and the <item
**        number> specified in the statement is less than 1 or greater
**        than the number of <occurrences> specified when the <descriptor
**        name> was allocated
**
** Generic Error Mapping:
**   Ingres doesn't have this condition, but it we did...
**   E_GE7D00_QUERY_ERROR 
*/

#define      SS07009_INV_DESCR_IDX            "07009" /* E_GE7D00_QUERY_ERROR */
#define      SS_07009_SUBCLASS                9
#define      SS_07009_GE_MAP                  E_GE7D00_QUERY_ERROR

/*
** prepared statement not a cursor specification        07005   (p. 388)
**
** shall be returned if prepared statement associated with the 
** <extended statement name> in <allocate cursor statement> is not a 
** <cursor specification>
**
** Generic Error Mapping:
**   Ingres doesn't have this condition, but it we did...
**   E_GE7D00_QUERY_ERROR 
*/

#define      SS07005_STMT_NOT_CURS_SPEC       "07005" /* E_GE7D00_QUERY_ERROR */
#define      SS_07005_SUBCLASS                5
#define      SS_07005_GE_MAP                  E_GE7D00_QUERY_ERROR

/*
** restricted data type attribute violation     07006   (pp 381, 382)
**
** shall be returned if in <using clause> the value of the i-th argument
** if <using arguments> was specified or the value represented by the value
** of DATA in the i-th item descriptor area if <using descriptor> was specified 
** may not be cast into the effective datatype of the i-th 
** <dynamic parameter specification> without violating Syntax Rules for
** <cast specification>
**
** Generic Error Mapping:
**   This looks like the dynamic side of "22018",
**   but all the other '07' class SQLSTATEs map to
**   E_GE7D00_QUERY_ERROR
*/

#define      SS07006_RESTR_DT_ATTR_ERR        "07006" /* E_GE7D00_QUERY_ERROR */
#define      SS_07006_SUBCLASS                6
#define      SS_07006_GE_MAP                  E_GE7D00_QUERY_ERROR

/*
** using clause does not match dynamic parameter specification  07001   (p. 380)
**
** shall be returned if 
**      - a <using clause> is used in a <dynamic open statement> or as a
**        <parameter using clause> in an <execute statement> and PS is
**        the prepared <dynamic select statement> referenced by the 
**        <dynamic open statement> or the <execute statement> and D is
**        the number of <dynamic parameter specification>s in PS and either
**          - <using arguments> is specified and the number of <argument>s is
**            different from D or
**          - <using descriptor> is specified and either
**              - the value of COUNT is different from D or
**              - the first D item descriptor areas are not valid as
**                specified in Subclause 17.1 "Description of SQL item 
**                descriptor area" or
**              - the value of INDICATOR is not negative, and the value
**                of DATA is not a valid value of data type indicated by
**                TYPE
**
** Generic Error Mapping:
**   The last condition listed above (invalid datatype) yields a Generic 
**   Error of E_GE9D17_DATAEX_TYPEINV.  All the others are E_GE7D00_QUERY_ERROR,
**   so we'll use
**   E_GE7D00_QUERY_ERROR
**  
*/

#define      SS07001_USING_PARM_MISMATCH      "07001" /* E_GE7D00_QUERY_ERROR */
#define      SS_07001_SUBCLASS                1
#define      SS_07001_GE_MAP                  E_GE7D00_QUERY_ERROR

/*
** using clause does not match target specification             07002   (p. 381)
**
** shall be returned if 
**      - a <using clause> is used in a <dynamic fetch statement> or as a
**        <result using clause> in an <execute statement> and PS is
**        the prepared <dynamic select statement> referenced by the 
**        <dynamic fetch statement> or the <execute statement> and D is
**        the the degree of the table specified by PS and either
**          - <using arguments> is specified and the number of <argument>s is
**            different from D or
**          - <using descriptor> is specified and either
**              - the value of COUNT is different from D or
**              - the first D item descriptor areas are not valid as
**                specified in Subclause 17.1 "Description of SQL item 
**                descriptor area" 
**
** Generic Error Mapping:
**   E_GE7D00_QUERY_ERROR
*/

#define      SS07002_USING_TARG_MISMATCH      "07002" /* E_GE7D00_QUERY_ERROR */
#define      SS_07002_SUBCLASS                2
#define      SS_07002_GE_MAP                  E_GE7D00_QUERY_ERROR

/*
** using clause required for dynamic parameters         07004   (pp. 383, 390)
** 
** shall be returned if 
**      - an attempt is made to execute a prepared statement containing 
**        <dynamic parameter specification>s and a <parameter using clause> 
**        is not specified or
**      - an attempt is made to associate input parameters with a <cursor
**        specification> and to open the cursor and the prepared statement
**        associated with the <dynamic cursor name> contains <dynamic parameter
**        specification>s and a <using clause> is not specified
**
** Generic Error Mapping:
**   E_GE7D00_QUERY_ERROR
*/

#define      SS07004_NEED_USING_FOR_PARMS     "07004" /* E_GE7D00_QUERY_ERROR */
#define      SS_07004_SUBCLASS                4
#define      SS_07004_GE_MAP                  E_GE7D00_QUERY_ERROR

/*
** using clause required for result fields              07007   (pp. 383)
** 
** shall be returned if an attempt is made to execute a prepared 
** <dynamic single row select statement> and a <result using clause> 
** is not specified 
**
** Generic Error Mapping:
**   E_GE7D00_QUERY_ERROR
*/

#define      SS07007_NEED_USING_FOR_RES       "07007" /* E_GE7D00_QUERY_ERROR */
#define      SS_07007_SUBCLASS                7
#define      SS_07007_GE_MAP                  E_GE7D00_QUERY_ERROR

/*
** context mismatch (INGRES-specific SQLSTATE)		07500
**
** shall be returned if
**	an attempt is made to execute a dynamically prepared statement and
**	the context of the SQL-session (i.e. the effective user,group, and 
**	role id) does not match that which was in effect when the query was 
**	prepared
**
** Generic Error Mapping:
**	E_GE98BC_OTHER_ERROR
*/

#define      SS07500_CONTEXT_MISMATCH         "07500" /* E_GE98BC_OTHER_ERROR */
#define      SS_07500_SUBCLASS                6480 /* 5 * 36**2 */
#define      SS_07500_GE_MAP                  E_GE98BC_OTHER_ERROR


/*
** feature not supported                0A000   (pp. 341, 344)
**
** Generic Error Mapping:
**   A feature we don't support?  Hard to imagine!
**   E_GE98BC_OTHER_ERROR
*/

#define      SS0A000_FEATUR_NOT_SUPPORTED     "0A000" /* E_GE98BC_OTHER_ERROR */
#define      SS_0A000_CLASS                   10
#define      SS_0A000_GE_MAP                  E_GE98BC_OTHER_ERROR

/* 
** multiple server transactions                    0A001   (pp. 341, 344)
**
** shall be returned if a <set connection statement> or a <connect statement>
** is executed after the first transaction-initiating SQL-statement executed
** by the current SQL-transaction and the implementation does not support
** transactions that affect more than one SQL-environment
**
** Generic Error Mapping:
**   In SQL92, being able to switch connections in the middle of a transaction
**   implies that you support 'global transactions.'  The Ingres model of
**   multiple active transactions (one per connection) is NOT permitted
**   by SQL92 transaction semantics.
**   E_GE98BC_OTHER_ERROR
*/

#define      SS0A001_MULT_SERVER_XACTS        "0A001" /* E_GE98BC_OTHER_ERROR */
#define      SS_0A001_SUBCLASS                1
#define      SS_0A001_GE_MAP                  E_GE98BC_OTHER_ERROR

/*
** invalid query language (INGRES-specific SQLSTATE)	0A500
**
** shall be returned if
**	unknown or disallowed query language was specified
**
** Generic Error Mapping
**	E_GE79E0_UNSUP_LANGUAGE
*/

#define      SS0A500_INVALID_QRY_LANG         "0A500" /* E_GE79E0_UNSUP_LANGUAGE */
#define      SS_0A500_SUBCLASS                6480 /* 5 * 36**2 */
#define      SS_0A500_GE_MAP                  E_GE79E0_UNSUP_LANGUAGE

/*
** integrity constraint violation       23000   (pp. 122, 194, 209)
**
** shall be returned if
**      - when constraint is effectively checked, it is not satisfied (other
**        than the case when this happens as a result of executing a
**        <commit statement> in which case SQLSTATE is set to
**        "transaction rollback - integrity constraint violation") or
**      - a <cast specification> contains a <domain name> and that
**        <domain name> refers to a domain that contains a <domain constraint>
**        and the result of <cast specification> does not satisfy the
**        <check constraint> of the <domain constraint> or
**      - in an assignment of V to T for storing SQL-data if the column
**        definition of T includes the name of a domain whose domain descriptor
**        includes a domain constraint D and D is not satisfied
**
** Generic Error Mapping:
**   E_GE9D6C_CONSTR_VIO
*/

#define      SS23000_CONSTR_VIOLATION         "23000" /* E_GE9D6C_CONSTR_VIO */
#define      SS_23000_CLASS                   75 /* 2 * 36 + 3 */
#define      SS_23000_GE_MAP                  E_GE9D6C_CONSTR_VIO

/*
** subclasses for the specific types of ANSI constraints
** 	(INGRES-specific SQLSTATEs)
**
** shall be returned if
**	23500	- CHECK  constraint violation
**	23501	- UNIQUE constraint violation 
**	        REFERENTIAL constraint violations:
**	23001	- on UPDATE/DELETE of REFERENCED table with RESTRICT action
**	23502	- on INSERT into REFERENCING/foreign key table
**	23503	- on UPDATE of   REFERENCING/foreign key table
**	23504	- on DELETE from REFERENCED/primary key table
**	23505	- on UPDATE of   REFERENCED/primary key table
**
** Generic Error Mapping:
**   E_GE9D6C_CONSTR_VIO (same as SS23000) for all except UNIQUE violations,
**   which will map to E_GE7602_INS_DUP_ROW for 6.4 backward-compatibility
*/

#define	     SS23001_REF_RESTRICT_VIOLATION   "23001" /* E_GE9D6C_CONSTR_VIO */
#define      SS23500_CHECK_CONS_VIOLATION     "23500" /* E_GE9D6C_CONSTR_VIO */

#define      SS23501_UNIQUE_CONS_VIOLATION    "23501" /* E_GE7602_INS_DUP_ROW*/
#define      SS_23501_SUBCLASS                6481 /* 5 * 36**2 + 1 */
#define      SS_23501_GE_MAP                  E_GE7602_INS_DUP_ROW

#define      SS23502_REF_FK_INS_VIOLATION     "23502" /* E_GE9D6C_CONSTR_VIO */
#define      SS23503_REF_FK_UPD_VIOLATION     "23503" /* E_GE9D6C_CONSTR_VIO */
#define      SS23504_REF_PK_DEL_VIOLATION     "23504" /* E_GE9D6C_CONSTR_VIO */
#define      SS23505_REF_PK_UPD_VIOLATION     "23505" /* E_GE9D6C_CONSTR_VIO */


/*
** invalid authorization specification  28000   (pp. 286, 341, 342, 352)
**
** shall be returned if
**      - the module has an explicit <module authorization identifier> MAI which
**        is different from the SQL-session <authorization identifier> SAI and
**        the implementation precludes SAI from invoking a <procedure> in a
**        <module> with explicit <module authorization identifier> MAI or
**      - <user name> specified in <connect statement> or in
**	  <set session authorization identifier statement> violates
**        implementation-defined restrictions and/or does not conform to the
**	  Format and Syntax rules of an <authorization identifier>
**
** Generic Error Mapping:
**   E_GEA028_USER_ID_INV
*/

#define      SS28000_INV_AUTH_SPEC            "28000" /* E_GEA028_USER_ID_INV */
#define      SS_28000_CLASS                   80 /* 2 * 36 + 8 */
#define      SS_28000_GE_MAP                  E_GEA028_USER_ID_INV

/*
** invalid catalog name         3D000   (p. 349)
**
** shall be returned if a <value specification> specified in a
** <set catalog statement> does not conform to the Format and Syntax Rules of
** a <catalog name> 
**
** Generic Error Mapping:
**   An SQL catalog is the SQL92 term for a database.  Database name syntax
**   is only checked by createdb (E_DU3010_BAD_DBNAME), but it doesn't 
**   need/have an associated Generic Error.  The CONNECT statement doesn't 
**   check dbname syntax, only existence.
**   E_GE98BC_OTHER_ERROR
*/

#define      SS3D000_INV_CAT_NAME             "3D000" /* E_GE98BC_OTHER_ERROR */
#define      SS_3D000_CLASS                   121 /* 3 * 36 + 13 */
#define      SS_3D000_GE_MAP                  E_GE98BC_OTHER_ERROR

/*
** invalid character set name   2C000   (p. 351)
**
** shall be returned if a <value specification> specified in a
** <set names statement> does not conform to the Format and Syntax Rules of
** a <character set name>
**
** Generic Error Mapping:
**   This condition requires character set support from Intermediate SQL.
**   E_GE7918_SYNTAX_ERROR
*/

#define      SS2C000_INV_CH_SET_NAME          "2C000" /* E_GE7918_SYNTAX_ERROR */
#define      SS_2C000_CLASS                   84 /* 2 * 36 + 12 */
#define      SS_2C000_GE_MAP                  E_GE7918_SYNTAX_ERROR

/*
** invalid condition number     35000   (pp. 334, 406)
**
** shall be returned if
**      - a <number of conditions> specified in a <diagnostics size> clause of
**        a <set transaction statement> is less than 1 or
**      - <condition information> was specified in <get diagnostics statement>
**        and the value of <condition number> is less than 1 or greater than the
**        number of conditions stored in the diagnostics area
**
** Generic Error Mapping:
**   E_GE7D00_QUERY_ERROR
*/

#define      SS35000_INV_COND_NUM             "35000" /* E_GE7D00_QUERY_ERROR */
#define      SS_35000_CLASS                   113 /* 3 * 36 + 5 */
#define      SS_35000_GE_MAP                  E_GE7D00_QUERY_ERROR

/*
** invalid connection name      2E000   (p. 342)
**
** shall be returned if <connection  name> in <connect statement> does not
** conform to the Format and Syntax Rules of an <identifier>
**
** Generic Error Mapping:
**   In Ingres, this condition is caught by LIBQ, generating a 
**   "E_LQ00BE, Attempt to switch to a non-existent session" condition.
**   No error condition is raised, and processing (incorrectly, in my
**   opinion) continues using the previous current connection.
**   This should probably be fixed.  In SQL92, sessions are identified
**   by SQL identifiers, not numbers, so the correct Generic Error is
**   E_GE797C_INVALID_IDENT
*/

#define      SS2E000_INV_CONN_NAME            "2E000" /* E_GE797C_INVALID_IDENT */
#define      SS_2E000_CLASS                   86 /* 2 * 36 + 14 */
#define      SS_2E000_GE_MAP                  E_GE797C_INVALID_IDENT

/*
** invalid cursor name          34000   (pp. 370, 385, 388, 390)
**
** shall be returned if
**      - an attempt was made to PREPARE or EXECUTE IMMEDIATE a
**        <preparable dynamic delete/update statement: positioned> and there is
**        neither an extended dynamic cursor nor a dynamic cursor with the name
**        of <cursor name> or
**      - a value of <extended cursor name> in <allocate cursor statement> does
**        not conform to the Format and Syntax Rules of an <identifier> or
**      - a value of the <extended cursor name> in <allocate cursor statement>
**        is identical to the value of the <extended cursor name> of any other
**        cursor allocated in the scope of the <extended cursor name> or
**      - the <dynamic cursor name> in <dynamic open statement> is an <extemded
**        cursor name> whose value does not identify a cursor allocated in the
**        scope of the <extended cursor name>
**
** Generic Error Mapping:
**   E_GE75A8_CURSOR_UNKNOWN
*/

#define      SS34000_INV_CURS_NAME            "34000" /* E_GE75A8_CURSOR_UNKNOWN */
#define      SS_34000_CLASS                   112 /* 3 * 36 + 4 */
#define      SS_34000_GE_MAP                  E_GE75A8_CURSOR_UNKNOWN

/*
** invalid cursor state         24000   (pp. 310, 313, 315, 318, 326, 375, 408)
**
** shall be returned if
**      - an attempt is made to open a cursor CR by means of <open statement>
**        and CR is not in the closed state or
**      - an attempt is made to position a cursor CR on a specified row of a
**        table and retrieve values from that row by means of <fetch statement>
**        and CR is not in the open state or
**      - an attempt is made to close cursor CR by means of <close statement>
**        and CR is not in the open state or
**      - an attempt is made to delete or update a row of a table and the
**        cursor is not positioned on a row or
**      - an attempt is made to deallocate a prepared SQL statement by means of
**        <deallocate prepared statement> and the statement is the
**        <cursor specification> of an open cursor
**
** Generic Error Mapping:
**   E_GE9DD0_CUR_STATE_INV
*/

#define      SS24000_INV_CURS_STATE           "24000" /* E_GE9DD0_CUR_STATE_INV */
#define      SS_24000_CLASS                   76 /* 2 * 36 + 4 */
#define      SS_24000_GE_MAP                  E_GE9DD0_CUR_STATE_INV

/*
** invalid schema name          3F000   (p. 350)
**
** shall be returned if the value of <value specification> in <set schema
** statement> does not conform to the Format and Syntax Rules of a <schema name>
**
** Generic Error Mapping:
**   This Generic Error is for 'other database object not found'
**   E_GE797C_INVALID_IDENT
*/

#define      SS3F000_INV_SCHEMA_NAME          "3F000" /* E_GE797C_INVALID_IDENT */
#define      SS_3F000_CLASS                   123 /* 3 * 36 + 15 */
#define      SS_3F000_GE_MAP                  E_GE797C_INVALID_IDENT

/*
** invalid SQL descriptor name  33000   (pp. 360, 362, 364, 366, 377)
**
** shall be returned if
**      - the value of <descriptor name> in <allocate descriptor statement> does
**        not conform to the Format and Syntax Rules of an <identifier> or a
**        descriptor with the same name (ignoring leading/trailing blanks) and
**        <scope option> has been aloocated and has not been deallocated or
**      - the <descriptor name> in <deallocate descriptor statement>,
**        <get descriptor statement>, <set descriptor statement>, or a
**        <using clause> identifies an SQL item descriptor area that is not
**        currently allocated 
**
** Generic Error Mapping:
**   E_GE75BC_UNKNOWN_OBJECT
*/

#define      SS33000_INV_SQL_DESCR_NAME       "33000" /* E_GE75BC_UNKNOWN_OBJECT */
#define      SS_33000_CLASS                   111 /* 3 * 36 + 3 */
#define      SS_33000_GE_MAP                  E_GE75BC_UNKNOWN_OBJECT

/*
** invalid SQL statement name   26000   (pp. 375, 376, 383, 388, 390)
**
** shall be returned if
**      - an <SQL statement name> in <deallocate prepared statement>,
**        <describe statement>, or <execute statement> does not
**        identify a statement prepared in the scope of the <SQL statement name>
**        or
**      - the value of the <extended statement name> in <allocate cursor
**        statement> does not identify a statement previously prepared in the
**        scope of <extended statement name> or
**      - in <dynamic open statement> if the <statement name> of the associated
**        <dynamic declare cursor> is not associated with a prepared statement
**
** Generic Error Mapping:
**   We now return the wonderfully polite, 
**       E_US0900, The dynamically defined statement 's2' not found.  
**       Perhaps a PREPARE or DESCRIBE wasn't successful.
**   E_GE75B2_NOT_FOUND
*/

#define      SS26000_INV_SQL_STMT_NAME        "26000" /* E_GE75B2_NOT_FOUND */
#define      SS_26000_CLASS                   78 /* 2 * 36 + 6 */
#define      SS_26000_GE_MAP                  E_GE75B2_NOT_FOUND

/*
** invalid transaction state    25000   (pp. 288, 318, 320, 323, 326, 328, 334,
**                                           346, 352, 445, 446)
** shall be returned if
**      - a <procedure> containing an <SQL schema statement> is called by an
**        SQL-agent and the access mode of the current SQL-transaction is
**        read-only or
**      - a non-dynamic or dynamic execution of an <SQL data statement> or the
**        execution of <SQL dynamic data statement>, <dynamic select statement>,
**        or <dynamic single row select statement> occurs within the same
**        SQL-transaction as the non-dynamic or dynamic execution of an
**        SQL-schema statement and this is not allowed by the SQL implementation
**        or
**      - an attempt is made to delete a row of a table on which a cursor is
**        positioned by means of <delete statement: positioned> and the access
**        mode of the current SQL-transaction is read-only and the table is
**        not a temporary table or
**      - an attempt is made to delete rows of a table by means of
**        <delete statement: searched> and the access mode of the current
**        SQL-transaction is read-only and the table is not a temporary table or
**      - an attempt is made to create new rows in a table by means of
**        <insert statement> and the access mode of the current
**        SQL-transaction is read-only and the table is not a temporary table or
**      - an attempt is made to update a row of a table on which a cursor is
**        positioned by means of <update statement: positioned> and the access
**        mode of the current SQL-transaction is read-only and the table is not
**        a temporary table or
**      - an attempt is made to update rows of a table by means of
**        <update statement: searched> and the access mode of the current
**        SQL-transaction is read-only and the table is not a temporary table or
**      - <set transaction statement> is executed when an SQL-transaction is
**        currently active or
**      - any SQL-connection if the list of SQL-connections being terminated by
**        means of <disconnect statement> is active or
**      - <set session authorization identifier statement>,
**        <set group authorization identifier statement> or
**        <set role authorization identifier statement> is executed and an
**        SQL-transaction is currently active or
**      - an SQL-transaction is active and a
**        <direct implementation-defined statement> which may not be
**        associated with an active SQL-transaction is executed or
**	- a <direct SQL statement> contains an <SQL schema statement> and the
**	  access mode of the current SQL-transaction is read-only or
**      - execution of a <direct SQL data statement> occurs within the same
**        SQL-transaction as the execution of an SQL-schema statement and this
**        is not allowed by the SQL-implementation
**
** Generic Error Mapping:
**   Ingres doesn't have explicit 'read only' transactions, but there are
**   numerous statements which cannot be executed within a multi-statement
**   transaction.
**   E_GE9E34_TRAN_STATE_INV
*/

#define      SS25000_INV_XACT_STATE           "25000" /* E_GE9E34_TRAN_STATE_INV */
#define      SS_25000_CLASS                   77 /* 2 * 36 + 5 */
#define      SS_25000_GE_MAP                  E_GE9E34_TRAN_STATE_INV

/*
** invalid transaction termination      2D000   (pp. 337, 339)
**
** shall be returned if
**      - an attempt is made to terminate the current SQL-transaction by means
**        of <commit statement> and the current SQL-transaction is a part of an
**        encompassing transaction that is controlled by an agent other than the
**        SQL-agent or
**      - an attempt is made to terminate the current SQL-transaction by means
**        of explicitly executed <rollback statement> and the current
**        SQL-transaction is a part of an encompassing transaction that is
**        controlled by an agent other than the SQL-agent 
**
** Generic Error Mapping:
**   If we supported this kind of stuff, then we'd probably issue a 
**   E_GE9E34_TRAN_STATE_INV
*/

#define      SS2D000_INV_XACT_TERMINATION     "2D000" /* E_GE9E34_TRAN_STATE_INV */
#define      SS_2D000_CLASS                   85 /* 2 * 36 + 13 */
#define      SS_2D000_GE_MAP                  E_GE9E34_TRAN_STATE_INV

/*
** no data                      02000   (pp. 314, 316, 320, 323, 329, 364, 419,
**					     446, 447)
** "no data" is a completion condition meaning that it must be returned in the
** diagnostics area but the execution of the statement must proceed unless some
** exception is also raised (see "Exceptions", 3.3.4.1)
**
** shall be returned if
**      - <fetch statement> attempts to position the cursor outside the range of
**        row numbers of that table or
**      - cardinality of the result of <query specification> in
**        <select statement: single row> is 0 (i.e. the result of
**	  <query specification> is empty) or
**      - no rows are deleted by <delete statement: searched> or
**      - the cardinality of the result of the <query expression> in <insert
**        statement> is 0 (i.e. the result of <query expression> is empty) or
**      - the set of rows updated in <update statement: searched> is empty or
**      - when getting information from an SQL descriptor area by means of <get
**        descriptor statement>, <item number> specified in the statement is
**        greater than the value of COUNT or
**      - cardinality of the result of <query specification> in
**        <direct select statement: multiple rows> is 0
**
** Generic Error Mapping:
**   This seems equivalent to a SQLCODE of 100.
**   E_GE0064_NO_MORE_DATA
*/

#define      SS02000_NO_DATA                  "02000" /* E_GE0064_NO_MORE_DATA */
#define      SS_02000_CLASS                   2
#define      SS_02000_GE_MAP                  E_GE0064_NO_MORE_DATA

/*
** Remote Database Access               HZ000   (no index)
**
** Generic Error Mapping:
**   All the RDA errors should probably be considered communications errors.
**   E_GE9088_COMM_ERROR
*/

#define      SSHZ000_RDA                      "HZ000" /* E_GE9088_COMM_ERROR */
#define      SS_HZ000_CLASS                   647 /* 17 * 36 + 35 */
#define      SS_HZ000_GE_MAP                  E_GE9088_COMM_ERROR

/*
** successful completion        00000   (pp. 37, 289)
**
** selbstverstandlich
**
** Generic Error Mapping:
**   I'm OK, you're OK, the Generic Error is
**   E_GE0000_OK
*/

#define      SS00000_SUCCESS                  "00000" /* E_GE0000_OK */
#define      SS_00000_CLASS                   0
#define      SS_00000_GE_MAP                  E_GE0000_OK

/*
** syntax error or access rule violation        42000   (pp. 10, 11, 407, 408)
**
** shall be returned if
**      - any condition required by Syntax Rules is not satisfied when the
**        evaluation of Access or General Rules is attempted, and the
**        implementation is neither processing non-conforming SQL language nor
**        processing conforming SQL language in a non-conforming manner or
**      - any condition in Access Rules is not satisfied
**
** Generic Error Mapping:
**   E_GE7918_SYNTAX_ERROR
*/

#define      SS42000_SYN_OR_ACCESS_ERR        "42000" /* E_GE7918_SYNTAX_ERROR */
#define      SS_42000_CLASS                   146 /* 4 * 36 + 2 */
#define      SS_42000_GE_MAP                  E_GE7918_SYNTAX_ERROR

/*
** table not found (INGRES-specific SQLSTATE)	42500
**
** shall be returned if
**	in the course of processing a statement, some table or view referenced
**	in the query (or a table or view referenced by an object (e.g. view
**	(should never happen) or a dbproc) used in the query) could not be
**	found. 
**
** Generic Error Mapping
**	E_GE7594_TABLE_NOT_FOUND
*/

#define      SS42500_TBL_NOT_FOUND            "42500" /* E_GE7594_TABLE_NOT_FOUND */
#define      SS_42500_SUBCLASS                6480 /* 5 * 36**2 */
#define      SS_42500_GE_MAP                  E_GE7594_TABLE_NOT_FOUND

/*
** column not found (INGRES-specific SQLSTATE)	42501
**
** shall be returned if
**	in the course of processing a statement, a column specified in the
**	statement appears to not exist.  
**
** Generic Error Mapping
**	E_GE759E_COLUMN_UNKNOWN
*/

#define      SS42501_COL_NOT_FOUND            "42501" /* E_GE759E_COLUMN_UNKNOWN */
#define      SS_42501_SUBCLASS                6481 /* 5 * 36**2 + 1 */
#define      SS_42501_GE_MAP                  E_GE759E_COLUMN_UNKNOWN

/*
** duplicate object name (INGRES-specific SQLSTATE)	42502
**
** shall be returned if
**	an attempt is made to create an object with the same name as an already
**	existing object.  
**
** Generic Error Mapping:
**	E_GE75F8_DEF_RESOURCE
*/

#define      SS42502_DUPL_OBJECT              "42502" /* E_GE75F8_DEF_RESOURCE */
#define      SS_42502_SUBCLASS                6482 /* 5 * 36**2 + 2 */
#define      SS_42502_GE_MAP                  E_GE75F8_DEF_RESOURCE

/*
** insufficient privilege (INGRES-specific SQLSTATE)     42503
**
** shall be returned if
**	the current SQL-session lacks some privilege(s) required to perform 
**	specified operation.  
**
** Generic Error Mapping:
**	E_GE84D0_NO_PRIVILEGE
*/

#define      SS42503_INSUF_PRIV               "42503" /* E_GE84D0_NO_PRIVILEGE */
#define      SS_42503_SUBCLASS                6483 /* 5 * 36**2 + 3 */
#define      SS_42503_GE_MAP                  E_GE84D0_NO_PRIVILEGE

/*
** cursor not found (INGRES-specific SQLSTATE)     42504
**
** shall be returned if
**	a cursor whose name was specified is now known in the current
**	SQL-session.  
**
** Generic Error Mapping:
**	E_GE75A8_CURSOR_UNKNOWN
*/

#define      SS42504_UNKNOWN_CURSOR           "42504" /* E_GE75A8_CURSOR_UNKNOWN */
#define      SS_42504_SUBCLASS                6484 /* 5 * 36**2 + 4 */
#define      SS_42504_GE_MAP                  E_GE75A8_CURSOR_UNKNOWN

/*
** object not found (INGRES-specific SQLSTATE)     42505
**
** shall be returned if
**	an object other than a table or a view (e.g. dbproc, dbevent, etc) was
**	not found.  
**
** Generic Error Mapping:
**	E_GE75B2_NOT_FOUND
*/

#define      SS42505_OBJ_NOT_FOUND            "42505" /* E_GE75B2_NOT_FOUND */
#define      SS_42505_SUBCLASS                6485 /* 5 * 36**2 + 5 */
#define      SS_42505_GE_MAP                  E_GE75B2_NOT_FOUND

/*
** invalid identifier (INGRES-specific SQLSTATE)     42506
**
** shall be returned if 
**	an identifier was found to contain invalid characters, was too long or 
**	too short.  
**
** Generic Error Mapping:
**	E_GE797C_INVALID_IDENT
*/

#define      SS42506_INVALID_IDENTIFIER       "42506" /* E_GE797C_INVALID_IDENT */
#define      SS_42506_SUBCLASS                6486 /* 5 * 36**2 + 6 */
#define      SS_42506_GE_MAP                  E_GE797C_INVALID_IDENT

/*
** reserved identifier (INGRES-specific SQLSTATE)   42507
**
** shall be returned if 
**	an attempt was made to create an object (e.g. table, column, 
**	database procedure, etc.) with a name that is reserved for internal 
** 	use by INGRES.  
**
** Generic Error Mapping:
**	E_GE797C_INVALID_IDENT
*/

#define      SS42507_RESERVED_IDENTIFIER      "42507" /* E_GE797C_INVALID_IDENT */
#define      SS_42507_SUBCLASS                6487 /* 5 * 36**2 + 7 */
#define      SS_42507_GE_MAP                  E_GE797C_INVALID_IDENT 

/*
** transaction rollback         40000   (pp. 55, 56, 209, 337, 497)
**
** if an error other than failure to satisfy some constraint prevents commitment
** of the SQL-transaction using <commit statement>, an exception condition
** TRANSACTION ROLLBACK with an implementation-defined subclass value must be
** raised
**
** Generic Error Mapping:
**   We don't generate a separate condition when a transaction is rolled
**   back.  In SQL92, multiple conditions may be returned as the result
**   of a single SQL statement.  This one is probably one of those
**   'secondary' conditions accessible via GET DIAGNOSTICS.  Since
**   GET DIAGNOSTICS is not in Entry SQL, we probably do not need to
**   separate these into additional messages for core FIPS 127-2, but
**   will need to in order to pass some of the 127-2 optional packages.
**   E_GE98BC_OTHER_ERROR
*/

#define      SS40000_XACT_ROLLBACK            "40000" /* E_GE98BC_OTHER_ERROR */
#define      SS_40000_CLASS                   144 /* 4 * 36 */
#define      SS_40000_GE_MAP                  E_GE98BC_OTHER_ERROR

/*
** integrity constraint violation       40002   (pp. 209, 337)
**
** shall be returned if a constraint was checked and was not satisfied as a
** result of executing a <commit statement>
**
** Generic Error Mapping:
**   E_GE9D6C_CONSTR_VIO
*/

#define      SS40002_CONSTR_VIOLATION         "40002" /* E_GE9D6C_CONSTR_VIO */
#define      SS_40002_SUBCLASS                2
#define      SS_40002_GE_MAP                  E_GE9D6C_CONSTR_VIO

/*
** serialization failure                40001   (p. 55)
**
** shall be issued if the implementation implicitly initiates execution of
** <rollback statement> when it detects the inability to guarantee the
** serializability of two or more concurrent SQL-transactions
**
** Generic Error Mapping:
**   E_GEC2EC_SERIALIZATION
*/

#define      SS40001_SERIALIZATION_FAIL       "40001" /* E_GEC2EC_SERIALIZATION */
#define      SS_40001_SUBCLASS                1
#define      SS_40001_GE_MAP                  E_GEC2EC_SERIALIZATION

/*
** statement completion unknown		40003	     (p. 56)
**
** shall be returned if an SQL-implementation detects the loss of the current
** SQL-connectionduring execution of any SQL-statement.  This exception
** condition indicates that the results of the actions performed in the
** SQL-server on behalf of the statement are unknown to the SQL-agent
**
** Generic Error Mapping:
**	E_GE9088_COMM_ERROR
*/

#define      SS40003_STMT_COMPL_UNKNOWN       "40003" /* E_GE9088_COMM_ERROR */
#define      SS_40003_SUBCLASS                3
#define      SS_40003_GE_MAP                  E_GE9088_COMM_ERROR

/*
** triggered data change violation	27000        (pp. 232, 407)
**
** shall be returned if any attempt is made within an SQL-statement to update
** some data item to a value that is distinct from the value to which that data
** item was previously updated within the same SQL statement (this also includes
** cascading updates of data items in the referencing table)
**
** Generic Error Mapping:
**   E_GE9EFC_TRIGGER_DATA
*/

#define      SS27000_TRIG_DATA_CHNG_ERR       "27000" /* E_GE9EFC_TRIGGER_DATA */
#define      SS_27000_CLASS                   79 /* 2 * 36 + 7 */
#define      SS_27000_GE_MAP                  E_GE9EFC_TRIGGER_DATA

/*
** warning                      01000   (pp. 37, 53, 99, 117-120, 191, 192,
**                                           222, 232, 234, 248, 271, 275, 280,
**                                           289, 318, 320, 326, 329, 347,
**                                           377, 379, 407, 446)
**
** Generic Error Mapping:
**   Ingres condition handling doesn't really support warnings. 
**   Once it does, SQLSTATE warnings will, presumably, map to the Generic
**   Error warning value.
**   E_GE0032_WARNING
*/

#define      SS01000_WARNING                  "01000" /* E_GE0032_WARNING */
#define      SS_01000_CLASS                   1
#define      SS_01000_GE_MAP                  E_GE0032_WARNING

/*
** cursor operation conflict    01001   (pp. 232, 318, 320, 326, 329)
**
** shall be returned if
**      - an <update rule> of a <referential constraint definition> attempts to
**        update a row that has been deleted by any
**        <delete statement: positioned> that identifies some cursor CR that
**        is still open or
**      - an <update rule> of a <referential constraint definition> attempts to
**        update a row that has been updated by any
**        <update statement: positioned> that identifies some cursor CR that is
**        still open or
**      - a <delete rule> of a <referential constraint definition> attempts to
**        mark for deletion such a row (as described above) or
**	- a row R is deleted or updated through an updateable cursor CR and
**	    - while CR is open, R has been marked for deletion by any
**	      <delete statement: searched>, marked for deletion by any
**            <delete statement: positioned> that identifies any cursor other
**	      than CR, updated by any <update statement: searched>, or updated
**	      by any <update statement: positioned> that identifies any cursor
**	      other than CR
**
** Generic Error Mapping:
**   E_GE0032_WARNING
*/

#define      SS01001_CURS_OPER_CONFLICT       "01001" /* E_GE0032_WARNING */
#define      SS_01001_SUBCLASS                1
#define      SS_01001_GE_MAP                  E_GE0032_WARNING

/*
** disconnect error             01002   (p. 347)
**
** shall be returned if any error is detected during execution of a
** <disconnect statement>
**
** Generic Error Mapping:
**   Apparently, Ingres disconnections can't fail.
**   E_GE0032_WARNING
*/

#define      SS01002_DISCONNECT_ERROR         "01002" /* E_GE0032_WARNING */
#define      SS_01002_SUBCLASS                2
#define      SS_01002_GE_MAP                  E_GE0032_WARNING

/*
** implicit zero-bit padding    01008   (pp. 118-120, 192, 222)
**
** shall be returned if
**      - when performing data conversion from a fixed- or variable-length bit
**        string to a fixed- or variable-length character string, zero-bit
**        padding was required or
**      - when performing data conversion to a fixed- or variable-length bit
**        string, zero-bit padding was required or
**      - in an assignment of V to T for retrieving SQL-data data type of T is
**        fixed-length bit string of length L and the length in bits of V is
**        less than L or
**      - when processing a <default clause> if the attribute data type is
**        fixed-length bit string and the length in bits of the <literal>
**        representing the <default option> is less than the length of attribute
**
** Generic Error Mapping:
**   Ingres hasn't got bit string datatypes yet.
**   E_GE0032_WARNING
*/

#define      SS01008_IMP_ZERO_BIT_PADDING     "01008" /* E_GE0032_WARNING */
#define      SS_01008_SUBCLASS                8
#define      SS_01008_GE_MAP                  E_GE0032_WARNING

/*
** insufficient item descriptor areas   01005   (pp. 377, 379)
**
** shall be returned if
**      - processing a <using clause> of a <describe output statement> and the
**        prepared statement that is being described is a
**        <dynamic select statement> or a <dynamic single row select statement>
**        and the degree of the table defined by the prepared statement is
**        greater than the <occurrences> specified when the <descriptor name>
**        was allocated or
**      - processing a <using clause> of a <describe input statement> and the
**        number of <dynamic parameter specification>s in the prepared statement
**        is greater than the <occurrences> specified when the <descriptor name>
**        was allocated
**
** Generic Error Mapping:
**   In Ingres this condition is detected by comparison of the SQLDA sqld
**   and sqln fields after a DESCRIBE.  Once Ingres supports SQL92 Dynamic
**   SQL descriptor statements, then we'll need to signal this condition.
**   E_GE0032_WARNING
*/

#define      SS01005_INSUF_DESCR_AREAS        "01005" /* E_GE0032_WARNING */
#define      SS_01005_SUBCLASS                5
#define      SS_01005_GE_MAP                  E_GE0032_WARNING

/*
** null value eliminated in set function        01003   (p. 99)
**
** shall be returned if one or more null values were eliminated as a result of
** applying the <value expression> of a <set function specification> to the
** argument of the <set function specification>
**
** Generic Error Mapping:
**   Ingres silently eliminates nulls in set functions.
**   E_GE0032_WARNING
*/

#define      SS01003_NULL_ELIM_IN_SETFUNC     "01003" /* E_GE0032_WARNING */
#define      SS_01003_SUBCLASS                3
#define      SS_01003_GE_MAP                  E_GE0032_WARNING

/*
** privilege not granted                01007   (pp. 275)
**
** shall be returned for each combination of <grantee> and <action> specified in
** <privileges> in <grant statement> s.t. the privilege to perform that <action>
** could not be granted to a <grantee> or, if ALL PRIVILEGES was speciifed and
** no privilege could be granted, then shall be returned for each <grantee>
**
** Generic Error Mapping:
**   Failures to grant privileges in Ingres are currently errors mapped to
**   the E_GE84D0_NO_PRIVILEGE Generic Error.  But this one is a warning.
**   E_GE0032_WARNING
*/

#define      SS01007_PRIV_NOT_GRANTED         "01007" /* E_GE0032_WARNING */
#define      SS_01007_SUBCLASS                7
#define      SS_01007_GE_MAP                  E_GE0032_WARNING

/*
**  privilege not revoked               01006   (p. 280)
**
** shall be returned for each combination of <grantee> and <action> specified in
** <privileges> in <revoke statement> s.t. the privilege to perform that
** <action> could not be revoked form the <grantee> or, if ALL PRIIVLEGES was
** speciifed and no privilege could be revoked, then shall be returned for each
** <grantee>
**
** Generic Error Mapping:
**   Failures to revoke privileges in Ingres are currently errors mapped to
**   the E_GE84D0_NO_PRIVILEGE Generic Error.  But this one is a warning.
**   E_GE0032_WARNING
*/

#define      SS01006_PRIV_NOT_REVOKED         "01006" /* E_GE0032_WARNING */
#define      SS_01006_SUBCLASS                6
#define      SS_01006_GE_MAP                  E_GE0032_WARNING

/*
** query expression too long for information schema        0100A        (p. 248)
**
** shall be returned if the character representation of the <query expression>
** in the <view definition> cannot be represented in the Information Schema
** without truncation
**
** Generic Error Mapping:
**   Ingres stores view text as an (unlimited) series of text segments in
**   iiqrytext.  This can't happen to us now.  When we support the VIEWS
**   INFORMATION_SCHEMA System View, then we'll need to worry about it.
**   E_GE0032_WARNING
*/

#define      SS0100A_QRY_EXPR_TOO_LONG        "0100A" /* E_GE0032_WARNING */
#define      SS_0100A_SUBCLASS                10
#define      SS_0100A_GE_MAP                  E_GE0032_WARNING

/*
** search condition too long for information schema     01009   (pp. 234, 271)
**
** shall be returned if the character representation of the <search condition>
** of <check constraint definition> or of <assertion definition> cannot be
** represented in the Information Schema without truncation
**
** Generic Error Mapping:
**   E_GE0032_WARNING
*/

#define      SS01009_SEARCH_COND_TOO_LONG     "01009" /* E_GE0032_WARNING */
#define      SS_01009_SUBCLASS                9
#define      SS_01009_GE_MAP                  E_GE0032_WARNING

/*
** string data, right truncation        01004   (pp. 117-120, 191-192)
**
** shall be returned if data conversion is specified by means of 
** <cast specification> and TD is the <data type> of <cast target> and 
** SD is the data type of the <cast operand> and SV is the value of 
** the <cast operand> and either
**   - TD is fixed-length character string or TD is variable-length    
**     character string and either
**       - SD is fixed-length character string or a variable-length 
**         character string and length of SV is greater than the length 
**         of TD and the portion of SV that extends beyond the length 
**         of TD contains one or more non-<space> characters or
**       - SD is fixed-length bit string or a variable-length bit 
**         string and the length of SV exprressed as a character string 
**         is greater than the length of TD or
**   or
**   - TD is fixed-length bit string or TD is variable-length bit
**     string and the length in bits of SV is greater than the 
**     length in bits of TD
**   or
**   - in an assignment of V to T for retrieving SQL-data if the data
**     type of T is fixed-length character string or variable-length 
**     character string or fixed-length bit string or variable-length 
**     bit string and the length of V is greater than the length of T
**
** Generic Error Mapping:
**   Awful complicated, isn't it?
**   Looks like the bad stuff is handled by '22001', so these must be
**   much less severe.
**   E_GE0032_WARNING
*/

#define      SS01004_STRING_RIGHT_TRUNC       "01004" /* E_GE0032_WARNING */
#define      SS_01004_SUBCLASS                4
#define      SS_01004_GE_MAP                  E_GE0032_WARNING

/*
** LDB table not dropped (INGRES-specific SQLSTATE)	01500
**
** shall be returned if
**	when processing DROP [TABLE|VIEW] statement in STAR, the STAR object has
**	been dropped, but the LDB table could not be dropped because it no 
**	longer existed
**
** Generic Error Mapping:
**	E_GE0032_WARNING
*/

#define      SS01500_LDB_TBL_NOT_DROPPED      "01500" /* E_GE0032_WARNING */
#define      SS_01500_SUBCLASS                6480 /* 5 * 36**2 */
#define      SS_01500_GE_MAP                  E_GE0032_WARNING

/*
** DSQL UPDATE or DELETE will affect entire table (INGRES-specific SQLSTATE)
**						       01501
**
** shall be returned if a dynamically prepared UPDATE or DELETE contained no
** WHERE-clause
**
** Generic Error Mapping:
**	E_GE0032_WARNING
*/

#define	     SS01501_NO_WHERE_CLAUSE	      "01501" /* E_GE0032_WARNING */
#define      SS_01501_SUBCLASS                6481 /* 5 * 36**2 + 1 */
#define      SS_01501_GE_MAP                  E_GE0032_WARNING

/*
** with check option violation  44000   (pp. 247, 408)
**
** shall be returned if
**      - an update operation on a view which is either itself defined
**        WITH [CASCADED] CHECK OPTION or is defined on top of another view
**	  which was defined WITH [CASCADED] CHECK OPTION would result in a row
**	  that would not appear in the view defined WITH [CASCADED] CHECK OPTION
**	  or
**      - an update operation on a view which is either itself defined
**	  WITH LOCAL CHECK OPTION or is defined on top of another view
**        which was defined WITH LOCAL CHECK OPTION would result in a row
**	  that would not appear in the view defined WITH LOCAL CHECK OPTION but
**	  would appear in the view or table on which it is defined (or, as SQL92
**	  puts it "in the simply underlying table of the simply underlying table
**	  of the <query expression>" of the view defined WITH LOCAL CHECK
**	  OPTION)
**
** Generic Error Mapping:
**   E_GE7D00_QUERY_ERROR
*/

#define      SS44000_CHECK_OPTION_ERR         "44000" /* E_GE7D00_QUERY_ERROR */
#define      SS_44000_CLASS                   148 /* 4 * 36 + 4 */
#define      SS_44000_GE_MAP                  E_GE7D00_QUERY_ERROR

/*
** miscellaneous INGRES-specific errors		50000
**
** shall be returned if
**	one of miscellaneous INGRES-specific errors was encountered.  The only
**	criteria that unifies errors in this class is that we could not find
**	mapping for some existing generic errors and had to define new SQLSTATEs
**	to correspond to them.
**
** Generic Error Mapping:
**	E_GE98BC_OTHER_ERROR
*/

#define      SS50000_MISC_ERRORS              "50000" /* E_GE98BC_OTHER_ERROR */
#define      SS_50000_CLASS                   180 /* 5 * 36 */
#define      SS_50000_GE_MAP                  E_GE98BC_OTHER_ERROR

/*
** invalid duplicate row (INGRES-specific SQLSTATE) 50001
**
** shall be returned if
**	an attempt is made to insert an invalid duplicate row (as would happen
**	if a table was create WITH NODUPLICAYTES or if there is a UNIQUE
**	clustering or secondary index defined on it)
**
** Generic Error Mapping:
**	E_GE7602_INS_DUP_ROW
*/

#define      SS50001_INVALID_DUP_ROW          "50001" /* E_GE7602_INS_DUP_ROW */
#define      SS_50001_SUBCLASS                1
#define      SS_50001_GE_MAP                  E_GE7602_INS_DUP_ROW

/*
** limit has been exceeded (INGRES-specific SQLSTATE)    50002
**
** shall be returned if
**	some system-imposed limit (e.g. number of sessions, number of columns in
**	a table, etc.) has been exceeded or a value specified by the user falls
**	outside of system-imposed limits (e.g. precision of a FLOAT())
**
** Generic Error Mapping:
**	E_GE8CA0_SYSTEM_LIMIT
*/

#define      SS50002_LIMIT_EXCEEDED           "50002" /* E_GE8CA0_SYSTEM_LIMIT */
#define      SS_50002_SUBCLASS                2
#define      SS_50002_GE_MAP                  E_GE8CA0_SYSTEM_LIMIT

/*
** resource exhausted (INGRES-specific SQLSTATE)    50003
**
** shall be returned if
**	some system resource (e.g. memory) has been exhausted
**
** Generic Error Mapping:
**	E_GE8D04_NO_RESOURCE
*/

#define      SS50003_EXHAUSTED_RESOURCE       "50003" /* E_GE8D04_NO_RESOURCE */
#define      SS_50003_SUBCLASS                3
#define      SS_50003_GE_MAP                  E_GE8D04_NO_RESOURCE

/*
** system configuration error (INGRES-specific SQLSTATE)    50004
**
** shall be returned if
**	a system configuration error is encountered.
**
** Generic Error Mapping:
**	E_GE8D68_CONFIG_ERROR
*/

#define      SS50004_SYS_CONFIG_ERROR         "50004" /* E_GE8D68_CONFIG_ERROR */
#define      SS_50004_SUBCLASS                4
#define      SS_50004_GE_MAP                  E_GE8D68_CONFIG_ERROR

/*
** gateway-related error (INGRES-specific SQLSTATE)	    50005
**
** shall be returned if
**	gateway-related error is encountered
**
** Generic Error Mapping:
**	E_GE9470_GATEWAY_ERROR
*/

#define      SS50005_GW_ERROR                 "50005" /* E_GE9470_GATEWAY_ERROR */
#define      SS_50005_SUBCLASS                5
#define      SS_50005_GE_MAP                  E_GE9470_GATEWAY_ERROR

/*
** fatal error (INGRES-specific SQLSTATE)		    50006
**
** shall be returned if
**	a fatal error has occurred (server will be shut down)
**
** Generic Error Mapping:
**	E_GE9858_FATAL_ERROR
*/

#define      SS50006_FATAL_ERROR              "50006" /* E_GE9858_FATAL_ERROR */
#define      SS_50006_SUBCLASS                6
#define      SS_50006_GE_MAP                  E_GE9858_FATAL_ERROR

/*
** invalid SQL statement id (INGRES-specific SQLSTATE)	    50007
**
** shall be returned if
**	an identifier for an SQL statement contained invalid characters or was
**	too long
**
** Generic Error Mapping:
**	E_GE9E98_INV_SQL_STMT_ID
*/
#define      SS50007_INVALID_SQL_STMT_ID      "50007" /* E_GE9E98_INV_SQL_STMT_ID */
#define      SS_50007_SUBCLASS                7
#define      SS_50007_GE_MAP                  E_GE9E98_INV_SQL_STMT_ID

/*
** unsupported statement (INGRES-specific SQLSTATE)	    50008
**
** shall be returned if
**	a statement entered by the user was recognized as currently invalid or
**	unsupported
**
** Generic Error Mapping:
**	E_GEA0F0_SQL_STMT_INV
*/

#define      SS50008_UNSUPPORTED_STMT         "50008" /* E_GEA0F0_SQL_STMT_INV */
#define      SS_50008_SUBCLASS                8
#define      SS_50008_GE_MAP                  E_GEA0F0_SQL_STMT_INV

/*
** database procedure error raised (INGRES-specific SQLSTATE)	    50009
**
** this is a "special" SQLSTATE which should be only returned for errors raised
** in a database procedure.  No error messages (in *.MSG files) should be mapped
** into this sqlstate
**
** Generic Error Mapping:
**	E_GEA154_RAISE_ERROR
*/

#define      SS50009_ERROR_RAISED_IN_DBPROC   "50009" /* E_GEA154_RAISE_ERROR */
#define      SS_50009_SUBCLASS                9
#define      SS_50009_GE_MAP                  E_GEA154_RAISE_ERROR

/*
** query error (INGRES-specific SQLSTATE)           5000A
**
** shall be returned if
**	a query, while syntactically correct, was logically inconsistent, 
**	conflicting, or otherwise incorrect.
**
** Generic Error Mapping:
**	E_GE7D00_QUERY_ERROR
*/

#define      SS5000A_QUERY_ERROR              "5000A" /* E_GE7D00_QUERY_ERROR */
#define      SS_5000A_SUBCLASS                10
#define      SS_5000A_GE_MAP                  E_GE7D00_QUERY_ERROR

/*
** internal error (INGRES-specific SQLSTATE)           5000B
**
** this SQLSTATE shall be assigned to errors which will NEVER be returned 
** to the user
**
** Generic Error Mapping:
**	E_GE98BC_OTHER_ERROR
*/

#define      SS5000B_INTERNAL_ERROR           "5000B" /* E_GE98BC_OTHER_ERROR */
#define      SS_5000B_SUBCLASS                11
#define      SS_5000B_GE_MAP                  E_GE98BC_OTHER_ERROR

/*
** fetch orientation has value zero (INGRES-specific SQLSTATE)	5000C
**
** shall be returned if
**	data exception has occurred because fetch orientation has value zero
** 
** Note that at the time when GE->SQLSTATE conversion took place, there were no
** error messages mapped into E_GE9D0D_DATAEX_FETCH0, and my explanation of 
** this SQLSTATE is probably inadequate
**
** Generic error mapping:
**	E_GE9D0D_DATAEX_FETCH0
*/

#define      SS5000C_FETCH_ORIENTATION        "5000C" /* E_GE9D0D_DATAEX_FETCH0 */
#define      SS_5000C_SUBCLASS                12
#define      SS_5000C_GE_MAP                  E_GE9D0D_DATAEX_FETCH0

/*
** invalid cursor name (INGRES-specific SQLSTATE)	5000D
**
** shall be returned if
**	internal cursor name was invalid
**
** Note that at the time when GE->SQLSTATE conversion took place, there were no
** error messages mapped into E_GE9D0D_DATAEX_FETCH0, and my explanation of
** this SQLSTATE is probably inadequate
**
** Generic Error Mapping:
**	E_GE9D12_DATAEX_CURSINV
*/

#define      SS5000D_INVALID_CURSOR_NAME      "5000D" /* E_GE9D12_DATAEX_CURSINV */
#define      SS_5000D_SUBCLASS                13
#define      SS_5000D_GE_MAP                  E_GE9D12_DATAEX_CURSINV

/*
** duplicate SQL statement id (INGRES-specific SQLSTATE)	5000E
**
** shall be returned if
** 	an identifier for an SQL statement, such as a repeat query parameter,
**	was already active or known
**
** Note that at the time when GE->SQLSTATE conversion took place, there were no
** error messages mapped into E_GE9D0D_DATAEX_FETCH0, and my explanation of
** this SQLSTATE is probably inadequate
**
** Generic Error Mapping:
** 	E_GEA21C_DUP_SQL_STMT_ID
*/

#define      SS5000E_DUP_STMT_ID              "5000E" /* E_GEA21C_DUP_SQL_STMT_ID */
#define      SS_5000E_SUBCLASS                14
#define      SS_5000E_GE_MAP                  E_GEA21C_DUP_SQL_STMT_ID

/*
** textual information (INGRES-specific SQLSTATE)        5000F
**
** This SQLSTATE should never be returned to user applications.
** It should be assigned to messages used internally by the Ingres system to
** contain textual information.
**
** Generic Error Mapping:
**	E_GE98BC_OTHER_ERROR
*/

#define      SS5000F_TEXTUAL_INFO             "5000F" /* E_GE98BC_OTHER_ERROR */
#define      SS_5000F_SUBCLASS                15
#define      SS_5000F_GE_MAP                  E_GE98BC_OTHER_ERROR

/*
** database procedure message (INGRES-specific SQLSTATE)	5000G
**
** this is a "special" SQLSTATE which should be only returned for database 
** procedure messages.  No error messages (in *.MSG files) should be mapped 
** into this sqlstate
**
** Generic Error Mapping:
**	none
*/

#define	    SS5000G_DBPROC_MESSAGE	     "5000G" /* no GE mapping */
#define     SS_5000G_SUBCLASS		     16

/*
** unknown/unavailable resource (INGRES-specific SQLSTATE)      5000H
**
** shall be returned if a specified resource (other than a DBMS object) is 
** unknown or unavailable.  This SQLSTATE was is a catch-all for errors that 
** map into E_GE75BC_UNKNOWN_OBJECT
**
** Generic Error Mapping:
**	E_GE75BC_UNKNOWN_OBJECT
*/

#define     SS5000H_UNAVAILABLE_RESOURCE     "5000H" /* E_GE75BC_UNKNOWN_OBJECT */
#define     SS_5000H_SUBCLASS                17
#define     SS_5000H_GE_MAP                  E_GE75BC_UNKNOWN_OBJECT

/*
** unexpected LDB schema change (INGRES-specific SQLSTATE)      5000I
**
** shall be returned if LDB schema was changed in a way that rendered a STAR 
** object obsolete (for instance, the LDB table could have been dropped and 
** recreated with differentr schema (e.g. names or number of columns))
**
** Generic Error Mapping:
**	E_GE98BC_OTHER_ERROR
*/

#define     SS5000I_UNEXP_LDB_SCHEMA_CHNG    "5000I" /* E_GE98BC_OTHER_ERROR */
#define     SS_5000I_SUBCLASS                18
#define     SS_5000I_GE_MAP                  E_GE98BC_OTHER_ERROR

/*
** inconsistent DBMS catalog (INGRES-specific SQLSTATE)	       5000J
**
** shall be returned if a DBMS catalog was found to be 
** inconsistent
**
** Generic Error Mapping:
**	E_GE98BC_OTHER_ERROR (to be determined)
*/

#define     SS5000J_INCONSISTENT_DBMS_CAT    "5000J" /* E_GE98BC_OTHER_ERROR */
#define     SS_5000J_SUBCLASS                19
#define     SS_5000J_GE_MAP                  E_GE98BC_OTHER_ERROR

/*
** SQLSTATE status code unavailable (INGRES-specific SQLSTATE)    5000K
**
** shall be returned if a user requested SQLSTATE status code value, 
** but it was unavailable (presumably because the error was returned 
** by a pre-6.5 BE)
**
** Generic Error Mapping:
**	E_GE98BC_OTHER_ERROR
*/

#define     SS5000K_SQLSTATE_UNAVAILABLE     "5000K" /* E_GE98BC_OTHER_ERROR */
#define     SS_5000K_SUBCLASS                20
#define     SS_5000K_GE_MAP                  E_GE98BC_OTHER_ERROR

/*
** protocol error (INGRES-specific SQLSTATE)		    5000L
**
** shall be returned if an FE encountered a protocol error
**
** Generic Error Mapping:
**	E_GE9088_COMM_ERROR
*/

#define	    SS5000L_PROTOCOL_ERROR	     "5000L" /* E_GE9088_COMM_ERROR */
#define     SS_5000L_SUBCLASS                21
#define     SS_5000L_GE_MAP                  E_GE9088_COMM_ERROR

/*
** IPC error (INGRES-specific SQLSTATE)		    5000M
**
** shall be returned if an IPC error is encountered
**
** Generic Error Mapping:
**	E_GE9088_COMM_ERROR
*/

#define     SS5000M_IPC_ERROR		     "5000M" /* E_GE9088_COMM_ERROR */
#define     SS_5000M_SUBCLASS                22
#define     SS_5000M_GE_MAP                  E_GE9088_COMM_ERROR

/*
** operand type mismatch (INGRES-specific SQLSTATE)             5000N
**
** shall be returned if types of operands supplied with an operator are 
** incompatible (i.e. trying to subtract an absolute date from interval)
**
** Generic Error Mapping:
**	E_GE80E8_LOGICAL_ERROR
*/

#define	    SS5000N_OPERAND_TYPE_MISMATCH   "5000N" /* E_GE80E8_LOGICAL_ERROR */
#define     SS_5000N_SUBCLASS               23
#define     SS_5000N_GE_MAP		    E_GE80E8_LOGICAL_ERROR

/*
** invalid function argument type (INGRES-specific SQLSTATE)             5000O
**
** shall be returned if argument of invalid type was passed to a function
** (e.g. interval passed to dow())
**
** Generic Error Mapping:
**	E_GE98BC_OTHER_ERROR
*/

#define     SS5000O_INVALID_FUNC_ARG_TYPE   "5000O" /* E_GE98BC_OTHER_ERROR */
#define     SS_5000O_SUBCLASS               24
#define     SS_5000O_GE_MAP		    E_GE98BC_OTHER_ERROR

/*
** timeout on lock request (INGRES-specific SQLSTATE)             5000P
**
** shall be returned if a lock request timed out
**
** Generic Error Mapping:
**	E_GE98BC_OTHER_ERROR
*/

#define     SS5000P_TIMEOUT_ON_LOCK_REQUEST "5000P" /* E_GE98BC_OTHER_ERROR */
#define     SS_5000P_SUBCLASS               25
#define     SS_5000P_GE_MAP		    E_GE98BC_OTHER_ERROR

/*
** database reorganization rendered QP invalid (INGRES-specific SQLSTATE)
**									5000Q
**
** shall be returned if a QP was rendered invalid by database reorganization 
** activity (e.g. this SQLSTATE will be returned when attempting to execute a 
** prepared statement referencing a gable whose timestamp has changed since 
** the time the statement was prepared)
**
** Generic Error Mapping:
**	E_GE80E8_LOGICAL_ERROR
*/

#define    SS5000Q_DB_REORG_INVALIDATED_QP "5000Q" /* E_GE80E8_LOGICAL_ERROR */
#define    SS_5000Q_SUBCLASS                26
#define    SS_5000Q_GE_MAP		    E_GE80E8_LOGICAL_ERROR

/*
** run-time logical error (INGRES-specific SQLSTATE) 	5000R
**
** shall be returned if a run-time logical error (e.g. user attempted to modify
** a table with duplicate rows to a UNIQUE structure) has been discovered 
**
** Generic Error Mapping:
**	E_GE80E8_LOGICAL_ERROR
*/

#define    SS5000R_RUN_TIME_LOGICAL_ERROR   "5000R" /* E_GE80E8_LOGICAL_ERROR */
#define    SS_5000R_SUBCLASS                27
#define    SS_5000R_GE_MAP		    E_GE80E8_LOGICAL_ERROR

/* external function declaration */
FUNC_EXTERN i4
ss_2_ge(char *sql_state, i4 dbms_error);
FUNC_EXTERN i4
ss_encode(char *sqlstate);
FUNC_EXTERN VOID
ss_decode(char *sql_state, i4 encoded_ss);
