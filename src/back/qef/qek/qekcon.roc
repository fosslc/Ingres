/*
** Copyright (c) 1988, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <qefkon.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefqry.h>
#include    <qefcat.h>

#include    <ex.h>
#include    <tm.h>
#include    <ade.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefdsh.h>
#include    <qefqp.h>

#include    <dudbms.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**
**  Name: QEKCON.ROC - Private and global readonly variables 
**
**  Description:
**      This file contains all the global readonly variables and search
**	function utilizing these variables.
**
**	qek_c1_str_to_code  - match a keyword string and return its QEK code 
**      qek_c2_val_to_code  - match a cap_value string and return its numeric
**			      code 
**	IIQE_01_str	    - table of strings of length 1 and associated codes
**	IIQE_02_str	    - table of strings of length 2 and associated codes
**	IIQE_03_str	    - table of strings of length 3 and associated codes
**	IIQE_04_str	    - table of strings of length 4 and associated codes
**	IIQE_05_str	    - table of strings of length 5 and associated codes
**	IIQE_06_str	    - table of strings of length 6 and associated codes
**	IIQE_08_str	    - table of strings of length 8 and associated codes
**	IIQE_09_str	    - table of strings of length 9 and associated codes
**	IIQE_10_str	    - table of strings of length 10 and their codes
**	IIQE_11_str	    - table of strings of length 11 and their codes
**	IIQE_12_str	    - table of strings of length 12 and their codes
**	IIQE_14_str	    - table of strings of length 14 and their codes
**	IIQE_15_str	    - table of strings of length 15 and their codes
**	IIQE_16_str	    - table of strings of length 16 and their codes
**	IIQE_17_str	    - table of strings of length 17 and their codes
**	IIQE_18_str_tab	    - table of pointers to subtables 
**
**	IIQE_c0_usr_ingres  - string containing "$ingres"
**	IIQE_c1_iidbdb	    - string containing "iidbdb"
**	IIQE_c2_abort	    - string containing "abort;"
**	IIQE_c3_commit	    - string containing "commit;"
**	IIQE_c4_rollback    - string containing "rollback;"
**	IIQE_c5_low_ingres  - string containing "ingres"
**	IIQE_c6_cap_ingres  - string containing "INGRES"
**	IIQE_c7_sql_tab	    - table of SQL keywords
**	IIQE_c8_ddb_cat	    - table of internal STAR catalog names
**	IIQE_c9_ldb_cat	    - table of standard catalog names
**	IIQE_10_col_tab	    - table of catalog column names
**	IIQE_11_con_tab	    - table of system and performance function 
**			      constructs
**	IIQE_27_cap_names   - table of capability names
**	IIQE_30_alias_prefix 
**			    - global variable containing string "long_"
**	IIQE_31_qec_beg_sess
**			    - string "QEC_BEGIN_SESSION"
**	IIQE_32_qec_end_sess
**			    - string "QEC_END_SESSION"
**	IIQE_33_integer	    - global variable containing string "INTEGER"
**	IIQE_34_float	    - global variable containing string "FLOAT"
**	IIQE_35_text	    - global variable containing string "TEXT"
**	IIQE_36_date	    - global variable containing string "DATE"
**	IIQE_37_money	    - global variable containing string "MONEY"
**	IIQE_38_char	    - global variable containing string "CHAR"
**	IIQE_39_varchar	    - global variable containing string "VARCHAR"
**	IIQE_40_decimal	    - global variable containing string "DECIMAL"
**
**	IIQE_41_qet_t9_ok_w_ldbs
**			    - string "QET_T9_OK_W_LDBS"
**	IIQE_42_ing_60	    - string "II9.2"
**	IIQE_43_heap	    - string "HEAP"
**
**	IIQE_50_act_tab	    - global table of STAR action type names
**	IIQE_51_rqf_tab	    - global table of RQF operation names
**	IIQE_52_tpf_tab	    - global table of TPF operation names
**	IIQE_53_lo_qef_tab  - global table of general QEF operation names,
**	IIQE_54_hi_qef_tab  - global table of DDB-specific QEF operation names,
**
**	IIQE_61_qefsess	    - global variable containing string "QEF session"
**	IIQE_62_3dots	    - global variable containing string "..."
**	IIQE_63_qperror	    - global variable containing string 
**			      "ERROR detected in query plan"
**	IIQE_64_callerr	    - global variable containing string 
**			      "ERROR occurred while calling"
**	IIQE_65_tracing	    - global variable containing string "Tracing"
**
**  History:
**      23-jun-88 (carl)
**          written
**      10-dec-88 (carl)
**	    modified for catalog redefinition
**      22-jan-89 (carl)
**	    modified to process OWNER_NAME, etc
**      21-jul-90 (carl)
**	    removed:
**		IIQE_c0_iildb, 
**		IIQE_c19_scb_conn,
**		IIQE_c22_ddb_desc, IIQE_c23_conn_info, 
**		IIQE_c25_1ldb_info, IIQE_c26_obj_desc, 
**		IIQE_c44_qry_info
**	    added:
**		1)  IIQE_50_act_tab[] containing DDB action names,
**		    IIQE_51_rqf_tab[] containing RQF operation names,
**		    IIQE_52_tpf_tab[] containing TPF operation names,
**		    IIQE_53_lo_qef_tab[] containing local QEF operation names,
**		    IIQE_54_hi_qef_tab[] containing DDB QEF operation names,
**		3)  IIQE_61_qefsess[], a string containing "QEF session", 
**		2)  IIQE_62_3dots[], a string containing "...",
**		4)  IIQE_63_qperror[], a string containing 
**		    "ERROR detected in query plan!", 
**		5)  IIQE_64_callerr[], a string containing 
**		    "ERROR occurred while calling",
**		6)  IIQE_65_tracing, a string containing "Tracing"
**		7)  IIQE_c44_qee_destroy, IIQE_45_qee_fetch, IIQE_46_qee_d2_tmp,
**		    IIQE_47_qee_d3_agg, IIQE_48_qee_d4_tds, IIQE_49_d6_malloc,
**		    IIQE_31_qec_begin_sess
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	10-dec-93 (robf)
**          Updated core catalog names (iidatabase) to new standard catalog
**	    info (iidatabase_info). 
**	21-mar-1995 (rudtr01)
**	    removed IIQE_60_ing_60 from comments since it no longer exists
**	    modified IIQE_42_ing_60 from "ING6.0" to "OPING1.1" for bug
**	    67496.
**	05-dec-95 (hanch04)
**	    bug 72307, change "OPING1.1" to "OPING1.2"
**	18-jul-96 (hanch04)
**	    change "OPING1.2" to "OI2.0"
**	09-nov-1998 (mcgem01)
**	    Change "OI2.0" to "II2.5"
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      02-Mar-2001 (hanch04)
**          Bumped release id to external "II 2.6" internal 860 or 8.6
**      07-Apr-2003 (hanch04)
**          Bumped release id to external "II 2.65" internal 865 or 8.65
**      13-jan-2004 (sheco02)
**          Bumped release id to external "II 3.0" internal 900 or 9.0
**	16-Jul-2004 (schka24)
**	    Remove unused qee strings.
**      17-Jan-2006 (hweho01)
**          Changed IIQE_42_ing_60 from "II3.0" to "ING9.0".
**      01-Feb-2006 (hweho01)
**          Modiied  IIQE_42_ing_60 to "II9.0", need to maintain the 
**          compatibility with current/previous releases. 
**      27-Apr-2006 (hweho01)
**          Changed IIQE_42_ing_60 from "II9.0" to "II9.1" for
**          new release.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	04-Jun-2009 (bonro01)
**          Changed IIQE_42_ing_60 to "II9.4" for new release.
**	10-Jun-2009 (hweho01)
**          Changed IIQE_42_ing_60 to "II10.0" for new release.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


/*}
** Name: IIQE_01_str - Table of strings of length 1 and associated codes.
**
** Description:
**      This table contains strings of 1 character and their associated codes.
**
** History:
**      23-jun-88 (carl)
**          written
*/

GLOBALDEF const QEK_K1_STR_CODE IIQE_01_str[] =   
{
    "Y",		QEK_1VAL_Y,	
    "y",		QEK_1VAL_Y,	
    "N",		QEK_2VAL_N,	
    "n",		QEK_2VAL_N,	
    "P",		QEK_11VAL_P,	
    "p",		QEK_11VAL_P,	
    NULL,		0		/* table terminator */
};


/*}
** Name: IIQE_02_str - Table of strings of length 2 and associated codes.
**
** Description:
**      This table contains strings of 2 characters and their associated codes.
**
** History:
**      22-jan-89 (carl)
**          written
*/

GLOBALDEF const QEK_K1_STR_CODE IIQE_02_str[] =   
{
    "NO",		QEK_8VAL_NO,	
    "no",		QEK_8VAL_NO,	
    NULL,		0		/* table terminator */
};



/*}
** Name: IIQE_03_str - Table of strings of length 3 and associated codes.
**
** Description:
**      This table contains strings of 3 characters and their associated codes.
**
** History:
**      22-jan-89 (carl)
**          written
*/

GLOBALDEF const QEK_K1_STR_CODE IIQE_03_str[] =   
{
    "YES",		QEK_9VAL_YES,	
    "yes",		QEK_9VAL_YES,	
    NULL,		0		/* table terminator */
};


/*}
** Name: IIQE_04_str - Table of strings of length 4 and associated codes.
**
** Description:
**      This table contains strings of 4 characters and their associated codes.
**
** History:
**      23-jun-88 (carl)
**          written
*/

GLOBALDEF const QEK_K1_STR_CODE IIQE_04_str[] =   
{
    "NONE",		QEK_3VAL_NONE,	    
    "none",		QEK_3VAL_NONE,	    
    "STAR",		QEK_4VAL_STAR,	    
    "star",		QEK_4VAL_STAR,	    
    "TIDS",		QEK_10CAP_TIDS,	    
    "tids",		QEK_10CAP_TIDS,	    
    NULL,		0		    /* table terminator */
};


/*}
** Name: IIQE_05_str - Table of strings of length 5 and associated codes.
**
** Description:
**      This table contains strings of 5 characters and their associated codes.
**
** History:
**      23-jun-88 (carl)
**          written
*/

GLOBALDEF const QEK_K1_STR_CODE IIQE_05_str[] =  
{
    "LOWER",		QEK_5VAL_LOWER,		
    "lower",		QEK_5VAL_LOWER,		
    "UPPER",		QEK_7VAL_UPPER,		
    "upper",		QEK_7VAL_UPPER,		
    "MIXED",		QEK_6VAL_MIXED,		
    "mixed",		QEK_6VAL_MIXED,		
    NULL,		0		    /* table terminator */
};


/*}
** Name: IIQE_06_str - Table of strings of length 6 and associated codes.
**
** Description:
**      This table contains strings of 6 characters and their associated codes.
**
** History:
**      23-jun-88 (carl)
**          written
*/

GLOBALDEF const QEK_K1_STR_CODE IIQE_06_str[] =  
{
    "INGRES",		QEK_5CAP_INGRES,
    "ingres",		QEK_5CAP_INGRES,
    "QUOTED",		QEK_10VAL_QUOTED,
    "quoted",		QEK_10VAL_QUOTED,
    NULL,		0			/* table terminator */
};


/*}
** Name: IIQE_08_str - Table of strings of length 8 and associated codes.
**
** Description:
**      This table contains strings of 8 characters and their associated codes.
**
** History:
**      23-jun-88 (carl)
**          written
*/

GLOBALDEF const QEK_K1_STR_CODE IIQE_08_str[] =  
{
    "SLAVE2PC",		QEK_9CAP_SLAVE2PC,
    "slave2pc",		QEK_9CAP_SLAVE2PC,
    NULL,		0		    /* table terminator */
};


/*}
** Name: IIQE_09_str - Table of strings of length 9 and associated codes.
**
** Description:
**      This table contains strings of 9 characters and their associated codes.
**
** History:
**      23-jun-88 (carl)
**          written
*/

GLOBALDEF const QEK_K1_STR_CODE IIQE_09_str[] =  
{
    "DBMS_TYPE",	QEK_2CAP_DBMS_TYPE,
    "dbms_type",	QEK_2CAP_DBMS_TYPE,
    NULL,		0		    /* table terminator */
};


/*}
** Name: IIQE_10_str - Table of strings of length 10 and their codes
**
** Description:
**      This table contains all the LDB strings of 10 characters
**  and their associated codes.
**
** History:
**      23-jun-88 (carl)
**          written
**	14-jun-91 (teresa)
**	    added "_TID_LEVEL" and "_tid_level" for bug 34684
*/

GLOBALDEF const  QEK_K1_STR_CODE IIQE_10_str[] = /* table of capabilities */
{
    "SAVEPOINTS",	QEK_8CAP_SAVEPOINTS,	    
    "savepoints",	QEK_8CAP_SAVEPOINTS,	    
    "OWNER_NAME",	QEK_12CAP_OWNER_NAME,	    
    "owner_name",	QEK_12CAP_OWNER_NAME,
    "_TID_LEVEL",	QEK_14CAP_TID_LEVEL,
    "_tid_level",	QEK_14CAP_TID_LEVEL,
    NULL,		0		    /* table terminator */
};

/*}
** Name: IIQE_11_str - Table of strings of length 11 and their codes
**
** Description:
**      This table contains all the LDB strings of 11 characters
**  and their associated codes.
**
** History:
**      23-jun-88 (carl)
**          written
*/

GLOBALDEF const  QEK_K1_STR_CODE IIQE_11_str[] = /* table of capabilities */
{
    "DISTRIBUTED",	QEK_4CAP_DISTRIBUTED,	    
    "distributed",	QEK_4CAP_DISTRIBUTED,	    
    NULL,		0		    /* table terminator */
};


/*}
** Name: IIQE_12_str - Table of strings of length 12 and their codes
**
** Description:
**      This table contains all the LDB strings of 12 characters
**  and their associated codes.
**
** History:
**      23-jun-88 (carl)
**          written
*/

GLOBALDEF const  QEK_K1_STR_CODE IIQE_12_str[] = /* table of capabilities */
{
    "DB_NAME_CASE",	QEK_3CAP_DB_NAME_CASE,	    /* length 12 */
    "db_name_case",	QEK_3CAP_DB_NAME_CASE,	    /* length 12 */
    NULL,		0			    /* table terminator */
};


/*}
** Name: IIQE_14_str - Table of strings of length 14 and their codes
**
** Description:
**      This table contains all the LDB strings of 14 characters
**  and their associated codes.
**
** History:
**      23-jun-88 (carl)
**          written
**	21-apr-93 (barbara)
**	    Added open/sql_level for delimited id support.
*/

GLOBALDEF const  QEK_K1_STR_CODE IIQE_14_str[] = /* table of capabilities */
{
    "UNIQUE_KEY_REQ",	QEK_11CAP_UNIQUE_KEY_REQ,   /* length 14 */
    "unique_key_req",	QEK_11CAP_UNIQUE_KEY_REQ,   /* length 14 */
    "OPEN/SQL_LEVEL",	QEK_15CAP_OPENSQL_LEVEL,    /* length 14 */
    "open/sql_level",	QEK_15CAP_OPENSQL_LEVEL,    /* length 14 */
    NULL,		0			    /* table terminator */
};


/*}
** Name: IIQE_15_str - Table of strings of length 15 and their codes
**
** Description:
**      This table contains all the LDB strings of 15 characters
**  and their associated codes.
**
** History:
**      15-apr-89 (carl)
**          written
*/

GLOBALDEF const  QEK_K1_STR_CODE IIQE_15_str[] = /* table of capabilities */
{
    "PHYSICAL_SOURCE",	QEK_13CAP_PHYSICAL_SOURCE,  /* length 15 */
    "physical_source",	QEK_13CAP_PHYSICAL_SOURCE,  /* length 15 */
    NULL,		0			    /* table terminator */
};


/*}
** Name: IIQE_16_str - Table of strings of length 16 and their codes
**
** Description:
**      This table contains all the LDB strings of 16 characters
**  and their associated codes.
**
** History:
**      23-jun-88 (carl)
**          written
*/

GLOBALDEF const  QEK_K1_STR_CODE IIQE_16_str[] = /* table of capabilities */
{
    "COMMON/SQL_LEVEL",	QEK_1CAP_COMMONSQL_LEVEL,  
    "common/sql_level",	QEK_1CAP_COMMONSQL_LEVEL,  
    "INGRES/SQL_LEVEL",	QEK_7CAP_INGRESSQL_LEVEL,  
    "ingres/sql_level",	QEK_7CAP_INGRESSQL_LEVEL,  
    NULL,		0			/* table terminator */
};


/*}
** Name: IIQE_17_str - Table of strings of length 17 and their codes
**
** Description:
**      This table contains all the LDB strings of 17 characters
**  and their associated codes.
**
** History:
**      23-jun-88 (carl)
**          written
**	10-may-93 (barbara)
**	    Added DB_DELIMITED_CASE capability
**	02-sep-93 (barbara)
**	    Added DB_REAL_USER_CASE capability
*/

GLOBALDEF const QEK_K1_STR_CODE IIQE_17_str[] = /* table of capabilities */
{
    "INGRES/QUEL_LEVEL",    QEK_6CAP_INGRESQUEL_LEVEL,	
    "ingres/quel_level",    QEK_6CAP_INGRESQUEL_LEVEL,	
    "DB_DELIMITED_CASE",    QEK_16CAP_DELIM_NAME_CASE,
    "db_delimited_case",    QEK_16CAP_DELIM_NAME_CASE,
    "DB_REAL_USER_CASE",    QEK_17CAP_REAL_USER_CASE,
    "db_real_user_case",    QEK_17CAP_REAL_USER_CASE,
    NULL,		    0			    /* table terminator */
};


/*}
** Name: IIQE_18_str_tab - Table of pointers to subtables 
**
** Description:
**      This table contains pointers to subtables of predefined strings 
**  and representation codes.
**
** History:
**      23-jun-88 (carl)
**          written
*/

#define	QEK_18MAX_LENGTH	    18	/* range limit */

GLOBALDEF const  QEK_K1_STR_CODE	*IIQE_18_str_tab[QEK_18MAX_LENGTH] 
    = /* table of pointers */
{
    NULL,		    /* 0 */
    IIQE_01_str,	    /* 1 */
    IIQE_02_str,	    /* 2 */
    IIQE_03_str,	    /* 3 */
    IIQE_04_str,	    /* 4 */
    IIQE_05_str,	    /* 5 */
    IIQE_06_str,	    /* 6 */
    NULL,		    /* 7 */
    IIQE_08_str,	    /* 8 */
    IIQE_09_str,	    /* 9 */
    IIQE_10_str,	    /* 10 */
    IIQE_11_str,	    /* 11 */
    IIQE_12_str,	    /* 12 */
    NULL,		    /* 13 */
    IIQE_14_str,	    /* 14 */
    IIQE_15_str,	    /* 15 */
    IIQE_16_str,	    /* 16 */
    IIQE_17_str		    /* 17 */
};


/*{
** Name: qek_c1_str_to_code - Match a string and return its code.
**
** Description:
**      This routine matches the given string with the predefined set
**  and return its code if successful.
**
** Inputs:
**	    i_str_p			string of length DB_CAPVAL_MAXLEN
**
** Outputs:
**	Returns:
**	    string code
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-jun-88 (carl)
**          written.
**	08-nov-93 (anitap)
**	    Fixed casting problem.
**	18-nov-93 (rblumer)
**	    Re-fixed casting problem (it was right the first time).
*/

i4
qek_c1_str_to_code(
char		*i_str_p )
{
    char		in_str[DB_CAPVAL_MAXLEN + 1];
    i4			len;
    QEK_K1_STR_CODE    *entry_p;
    bool		eot_b;		/* used to determine if end of a table
					** is reached */

    len = qed_u0_trimtail(i_str_p, (u_i4) DB_CAPVAL_MAXLEN, in_str);

    /* index to get pointer to subtable */

    if (len >= QEK_18MAX_LENGTH)
    {
	return((i4) QEK_0CAP_UNKNOWN);
    }
    entry_p = (QEK_K1_STR_CODE *)IIQE_18_str_tab[len];

    if (entry_p == NULL)		/* subtable does not exist */
	return((i4) QEK_0CAP_UNKNOWN);	/* no match */
    
    /* search subtable for a match */

    eot_b = FALSE;
    while (! eot_b)
    {
	if (STcompare(in_str, entry_p->qek_s1_str_p) == 0)
	    return(entry_p->qek_s2_code);
	
	++entry_p;			/* increment to next entry */
	if (entry_p->qek_s1_str_p == NULL)
	    eot_b = TRUE;
    } 

    /* no match */

    return ((i4) QEK_0CAP_UNKNOWN);
}


/*{
** Name: qek_c2_val_to_code - Decode a level string and return code.
**
** Description:
**      This routine decodes an LDB language level string and return its 
**  code if successful.
**
** Inputs:
**	    i_str_p			string of length DB_CAPVAL_MAXLEN
**
** Outputs:
**	Returns:
**	    level code
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-jun-88 (carl)
**          written
*/


i4
qek_c2_val_to_code(
char		*i_str_p )
{
    char	in_str[DB_CAPVAL_MAXLEN + 1];
    char	*digit_p;		/* used to point to above */
    i4		len;
    i4		lvl_code;
    

    len = qed_u0_trimtail(i_str_p, (u_i4) DB_CAPVAL_MAXLEN, in_str);

    for (digit_p = in_str, lvl_code = 0; len > 0; len--, digit_p++)
    {
	if (lvl_code > 0)
	    lvl_code = lvl_code * 10;	

	switch(*digit_p)
	{
	case '0':
	    break;
	case '1':
	    lvl_code += 1;
	    break;
	case '2':
	    lvl_code += 2;
	    break;
	case '3':
	    lvl_code += 3;
	    break;
	case '4':
	    lvl_code += 4;
	    break;
	case '5':
	    lvl_code += 5;
	    break;
	case '6':
	    lvl_code += 6;
	    break;
	case '7':
	    lvl_code += 7;
	    break;
	case '8':
	    lvl_code += 8;
	    break;
	case '9':
	    lvl_code += 9;
	    break;
	default:
	    /* an error condition, return 0 */
	    len = 0;
	    lvl_code = 0;
	    break;
	}	
    } 

    return (lvl_code);
}


/*}
** Name: IIQE_c0_usr_ingres - String containing "$ingres"
**
** Description:
**
** History:
**      04-aug-90 (carl)
**          written
*/

GLOBALDEF const char	IIQE_c0_usr_ingres[] = "$ingres";


/*}
** Name: IIQE_c1_iidbdb - String containing "iidbdb"
**
** Description:
**
** History:
**      05-jul-88 (carl)
**          written
*/

GLOBALDEF const char	IIQE_c1_iidbdb[] = "iidbdb";


/*}
** Name: IIQE_c2_abort - String containing "abort;"
**
** Description:
**
** History:
**      05-jul-88 (carl)
**          written
*/


GLOBALDEF const char	IIQE_c2_abort[] = "abort;";


/*}
** Name: IIQE_c3_commit - String containing "commit;"
**
** Description:
**      This global variable contains the string "commit"
**
** History:
**      05-jul-88 (carl)
**          written
*/

GLOBALDEF const char	IIQE_c3_commit[] = "commit;";


/*}
** Name: IIQE_c4_rollback - String containing "rollback;"
**
** Description:
**      This global variable contains the string "rollback;"
**
** History:
**      05-jul-88 (carl)
**          written
*/

GLOBALDEF const char	IIQE_c4_rollback[] = "rollback;";


/*}
** Name: IIQE_c5_low_ingres - String containing "ingres"
**
** Description:
**      This global variable contains the string "ingres"
**
** History:
**      05-jul-88 (carl)
**          written
*/

GLOBALDEF const char	IIQE_c5_low_ingres[] = "ingres";


/*}
** Name: IIQE_c6_cap_ingres - String containing "INGRES"
**
** Description:
**      This global variable contains the string "INGRES"
**
** History:
**      05-jul-88 (carl)
**          written
*/

GLOBALDEF const char	IIQE_c6_cap_ingres[] = "INGRES";


/*}
** Name: IIQE_c7_sql_tab - Table of SQL keywords.
**
** Description:
**      This global table contains SQL keywords.
**
** History:
**      05-jul-88 (carl)
**          written
*/

GLOBALDEF const char	*IIQE_c7_sql_tab[SQL_11_WHERE + 1] =
{
    "and",			    /* SQL_00_AND */
    "delete",			    /* SQL_01_DELETE */
    "from",			    /* SQL_02_FROM */
    "into",			    /* SQL_03_INTO */
    "insert",			    /* SQL_04_INSERT */
    "like",			    /* SQL_05_LIKE */
    "or",			    /* SQL_06_OR */
    "select",			    /* SQL_07_SELECT */
    "set",			    /* SQL_08_SET */
    "update",			    /* SQL_09_UPDATE */
    "values",			    /* SQL_10_VALUES */
    "where"			    /* SQL_11_WHERE */
};


/*}
** Name: IIQE_c8_ddb_cat - Table of internal STAR catalog names.
**
** Description:
**      This global table contains internal STAR catalog names.
**
** History:
**      05-jul-88 (carl)
**          written
**	19-nov-92 (teresa)
**	    added procedures
*/

GLOBALDEF const char	*IIQE_c8_ddb_cat[TDD_32_VIEWS + 1] =
{
    "",				/* TDD_00_? */
    "iidd_alt_columns",		/* TDD_01_ALT_COLUMNS */
    "iidd_columns",		/* TDD_02_COLUMNS */
    "iidd_dbcapabilities",	/* TDD_03_DBCAPABILITIES */
    "iidd_dbconstants",		/* TDD_04_DBCONSTANTS */
    "iidd_ddb_dbdepends",	/* TDD_05_DDB_DBDEPENDS */
    "iidd_ddb_ldbids",		/* TDD_06_DDB_LDBIDS */
    "iidd_ddb_ldb_columns",	/* TDD_07_DDB_LDB_COLUMNS */
    "iidd_ddb_ldb_dbcaps",	/* TDD_08_DDB_LDB_DBCAPS */
    "",				/* TDD_09_? */
    "",				/* TDD_10_? */
    "",				/* TDD_11_? */
    "",				/* TDD_12_? */
    "",				/* TDD_13_? */
    "iidd_ddb_long_ldbnames",	/* TDD_14_DDB_LONG_LDBNAMES */
    "iidd_ddb_objects",		/* TDD_15_DDB_OBJECTS */
    "iidd_ddb_object_base",	/* TDD_16_DDB_OBJECT_BASE */
    "",				/* TDD_17_? */
    "iidd_ddb_tableinfo",	/* TDD_18_DDB_TABLEINFO */
    "iidd_ddb_tree",		/* TDD_19_DDB_TREE */
    "iidd_histograms",		/* TDD_20_HISTOGRAMS */
    "iidd_indexes",		/* TDD_21_INDEXES */
    "iidd_index_columns",	/* TDD_22_INDEX_COLUMNS */
    "",				/* TDD_23_? */
    "iidd_integrities",		/* TDD_24_INTEGRITIES */
    "",				/* TDD_25_? */
    "",				/* TDD_26_? */
    "iidd_permits",		/* TDD_27_PERMITS */
    "iidd_procedures",		/* TDD_28_PROCEDURES */
    "iidd_registrations",	/* TDD_29_REGISTRATIONS */
    "iidd_stats",		/* TDD_30_STATS */
    "iidd_tables",		/* TDD_31_TABLES */
    "iidd_views"		/* TDD_32_VIEWS */
};


/*}
** Name: IIQE_c9_ldb_cat - Table of standard catalog names.
**
** Description:
**      This global table contains standard catalog names.
**
** History:
**      05-jul-88 (carl)
**          written
**	19-nov-92 (teresa)
**	    add support for iiprocedures standard catalog.
**	10-dec-93 (robf)
**          add iidatabase_info, iiusers
*/

GLOBALDEF const char	*IIQE_c9_ldb_cat[TII_25_IIUSERS + 1] =
{
    "",				/* TII_00_? */
    "iialt_columns",		/* TII_01_ALT_COLUMNS */
    "iidbcapabilities",		/* TII_02_DBCAPABILITIES */
    "iidbconstants",		/* TII_03_DBCONSTANTS */
    "iicolumns",		/* TII_04_COLUMNS */
    "",				/* TII_05_? */
    "iihistograms",		/* TII_06_HISTOGRAMS */
    "iiindexes",		/* TII_07_INDEXES */
    "iiindex_columns",		/* TII_08_INDEX_COLUMNS */
    "",				/* TII_09_? */
    "",				/* TII_10_? */
    "iiintegrities",		/* TII_11_INTEGRITIES */
    "iipermits",		/* TII_12_PERMITS */
    "iiphysical_tables",	/* TII_13_PHYSICAL_TABLES */
    "",				/* TII_14_? */
    "",				/* TII_15_? */
    "iistats",			/* TII_16_STATS */
    "iitables",			/* TII_17_TABLES */
    "iiprocedures",		/* TII_18_PROCEDURES */
    "iiviews",			/* TII_19_VIEWS */
    "iidatabase",		/* TII_20_DATABASE */
    "iiuser",			/* TII_21_USER */
    "iidbaccess",		/* TII_22_DBACCESS */
    "iistar_cdbs",		/* TII_23_STAR_CDBS */
    "iidatabase_info",		/* TII_24_IIDATABASE_INFO */
    "iiusers"			/* TII_25_IIUSERS */
};


/*}
** Name: IIQE_10_col_tab - Table of catalog column names.
**
** Description:
**      This global table contains catalog column names.
**
** History:
**      05-jul-88 (carl)
**          written
**	21-dec-92 (teresa)
**	    added procedure_name, procedure_owner
**	10-dec-93 (robf)
**	    added database_name, database_owner, user_name, internal_status
*/

GLOBALDEF const char	*IIQE_10_col_tab[COL_53_INTERNAL_STATUS + 1] =
{
    "alter_date",	    /* COL_00_ALTER_DATE */
    "column_name",	    /* COL_01_COLUMN_NAME */
    "create_date",	    /* COL_02_CREATE_DATE */
    "table_name",	    /* COL_03_TABLE_NAME */
    "table_owner",	    /* COL_04_TABLE_OWNER */
    "table_type",	    /* COL_05_TABLE_TYPE */
    "cdb_node",		    /* COL_06_CDB_NODE */
    "cdb_dbms",		    /* COL_07_CDB_DBMS */
    "cdb_name",		    /* COL_08_CDB_NAME */
    "ddb_name",		    /* COL_09_DDB_NAME */
    "access",		    /* COL_10_ACCESS */
    "name",		    /* COL_11_NAME */
    "base_name",	    /* COL_12_BASE_NAME */
    "base_owner",	    /* COL_13_BASE_OWNER */
    "status",		    /* COL_14_STATUS */
    "dbname",		    /* COL_15_DBNAME */
    "usrname",		    /* COL_16_USRNAME */
    "own",		    /* COL_17_OWN */
    "dbservice",	    /* COL_18_DBSERVICE */
    "db_id",		    /* COL_19_DB_ID */
    "ldb_id",		    /* COL_20_LDB_ID */
    "ldb_node",		    /* COL_21_LDB_NODE */
    "ldb_dbms",		    /* COL_22_LDB_DBMS */
    "ldb_longname",	    /* COL_23_LDB_LONGNAME */
    "ldb_database",	    /* COL_24_LDB_DATABASE */
    "treetabbase",	    /* COL_25_TREETABBASE */
    "treetabidx",	    /* COL_26_TREETABIDX */
    "table_stats",	    /* COL_27_TABLE_STATS */
    "long_ldb_name",	    /* COL_28_LONG_LDB_NAME */
    "long_ldb_id",	    /* COL_29_LONG_LDB_ID */
    "long_ldb_alias",	    /* COL_30_LONG_LDB_ALIAS */
    "cap_capability",	    /* COL_31_CAP_CAPABILITY */
    "modify_date",	    /* COL_32_MODIFY_DATE */
    "cap_level",	    /* COL_33_CAP_LEVEL */
    "cap_value",	    /* COL_34_CAP_VALUE */
    "key_id",		    /* COL_35_KEY_ID*/
    "object_name",	    /* COL_36_OBJECT_NAME */
    "object_owner",	    /* COL_37_OBJECT-OWENR */
    "object_base",	    /* COL_38_OBJECT_BASE */
    "object_index",	    /* COL_39_OBJECT_INDEX */
    "index_name",	    /* COL_40_INDEX_NAME */
    "index_owner",	    /* COL_41_INDEX_OWNER */
    "inid1",		    /* COL_42_INID1 */
    "inid2",		    /* COL_43_INID2 */
    "deid1",		    /* COL_44_DEID1 */
    "deid2",		    /* COL_45_DEID2 */
    "procedure_name",	    /* COL_46_PROCEDURE_NAME */
    "procedure_owner",	    /* COL_47_PROCEDURE_OWNER */
    "database_name",	    /* COL_48_DATABASE_NAME */
    "database_owner",	    /* COL_49_DATABASE_OWNER */
    "database_service",	    /* COL_50_DATABASE_SERVICE */
    "database_id",	    /* COL_51_DATABASE_ID */
    "user_name",	    /* COL_52_USER_NAME */
    "internal_status",	    /* COL_53_INTERNAL_STATUS */
};


/*}
** Name: IIQE_11_con_tab - Table of system and performance function constructs.
**
** Description:
**      This global table contains system and performance function constructs.
**
** History:
**      05-jul-88 (carl)
**          written
*/

GLOBALDEF const char	*IIQE_11_con_tab[QEK_1CON_USER_NAME + 1] =
{
    "dba_name",			    /* 0 - QEK_0CON_DBA_NAME */
    "user_name"			    /* 1 - QEK_1CON_USER_NAME */
};


/*}
** Name: IIQE_c12_rqr - Global variable containing an initialized RQR_CB.
**
** Description:
**      This global variable contains an initialized RQR_CB template.
**
** History:
**      05-jul-88 (carl)
**          written
**	09-apr-91 (seng)
**	    Initializer needs to have a set of curly braces on RS/6000 port.
*/

GLOBALDEF const RQR_CB    IIQE_c12_rqr = { NULL };


/*}
** Name: IIQE_c13_tpr - Global variable containing an initialized TPR_CB.
**
** Description:
**      This global variable contains an initialized TPR_CB template.
**
** History:
**      05-jul-88 (carl)
**          written
**	09-apr-91 (seng)
**	    Initializer needs to have a set of curly braces on RS/6000 port.
*/

GLOBALDEF const TPR_CB    IIQE_c13_tpr = { NULL };



/*}
** Name: IIQE_27_cap_names - Table of capability names
**
** Description:
**      This table contains capabilities names.
**
** History:
**      23-jun-88 (carl)
**          written
**	02-sep-93 (barbara)
**	    Added OPEN/SQL_LEVEL to table.
**	21-sep-93 (barbara)
**	    Appended comma after "OWNER_NAME" and increased size of table.
*/

GLOBALDEF const char *IIQE_27_cap_names[QEK_12NAME_OPENSQL_LEVEL + 1] = 

    /* table of capability names */
{
    "COMMON/SQL_LEVEL",	    /* QEK_0NAME_COMMONSQL_LEVEL */
    "DBMS_TYPE",	    /* QEK_1NAME_DBMS_TYPE */
    "DB_NAME_CASE",	    /* QEK_2NAME_DB_NAME_CASE */
    "DISTRIBUTED",	    /* QEK_3NAME_DISTRIBUTED */
    "INGRES",		    /* QEK_4NAME_INGRES */
    "INGRES/QUEL_LEVEL",    /* QEK_5NAME_INGRESQUEL_LEVEL */
    "INGRES/SQL_LEVEL",	    /* QEK_6NAME_INGRESSQL_LEVEL */
    "SAVEPOINTS",	    /* QEK_7NAME_SAVEPOINTS */
    "SLAVE2PC",		    /* QEK_8NAME_SLAVE2PC */
    "TIDS",		    /* QEK_9NAME_TIDS */
    "UNIQUE_KEY_REQ",	    /* QEK_10NAME_UNIQUE_KEY_REQ */
    "OWNER_NAME",	    /* QEK_11NAME_OWNER_NAME */
    "OPEN/SQL_LEVEL"	    /* QEK_12NAME_OPENSQL_LEVEL */
};


/*}
** Name: IIQE_30_alias_prefix - Global variable containing string "long_"
**
** Description:
**      This global variable contains a string for alias prefix.
**
** History:
**      10-oct-88 (carl)
**          written
*/

GLOBALDEF const char	    IIQE_30_alias_prefix[] = "long_";


/*}
** Name: IIQE_31_beg_sess - string "QEC_BEGIN_SESSION"
**
** Description:
**      This global variable contains above string.
**
** History:
**      16-sep-90 (carl)
**          written
*/

GLOBALDEF const char	    IIQE_31_beg_sess[] = "QEC_BEGIN_SESSION";


/*}
** Name: IIQE_32_end_sess - string "QEC_END_SESSION"
**
** Description:
**      This global variable contains above string.
**
** History:
**      16-sep-90 (carl)
**          written
*/

GLOBALDEF const char	    IIQE_32_end_sess[] = "QEC_END_SESSION";


/*}
** Name: IIQE_33_integer - Global variable containing string "INTEGER".
**
** Description:
**      This global variable contains string "INTEGER".
**
** History:
**      04_nov-88 (carl)
**          written
*/

GLOBALDEF const char	    IIQE_33_integer[] = "INTEGER";


/*}
** Name: IIQE_34_float - Global variable containing string "FLOAT".
**
** Description:
**      This global variable contains string "FLOAT".
**
** History:
**      04_nov-88 (carl)
**          written
*/

GLOBALDEF const char	    IIQE_34_float[] = "FLOAT";


/*}
** Name: IIQE_35_text - Global variable containing string "TEXT".
**
** Description:
**      This global variable contains string "TEXT".
**
** History:
**      04_nov-88 (carl)
**          written
*/

GLOBALDEF const char	    IIQE_35_text[] = "TEXT";


/*}
** Name: IIQE_36_date - Global variable containing string "DATE".
**
** Description:
**      This global variable contains string "DATE".
**
** History:
**      04_nov-88 (carl)
**          written
*/

GLOBALDEF const char	    IIQE_36_date[] = "DATE";


/*}
** Name: IIQE_37_money - Global variable containing string "MONEY".
**
** Description:
**      This global variable contains string "MONEY".
**
** History:
**      04_nov-88 (carl)
**          written
*/

GLOBALDEF const char	    IIQE_37_money[] = "MONEY";


/*}
** Name: IIQE_38_char - Global variable containing string "CHAR".
**
** Description:
**      This global variable contains string "CHAR".
**
** History:
**      04_nov-88 (carl)
**          written
*/

GLOBALDEF const char	    IIQE_38_char[] = "CHAR";


/*}
** Name: IIQE_39_varchar - Global variable containing string "VARCHAR".
**
** Description:
**      This global variable contains string "VARCHAR".
**
** History:
**      04_nov-88 (carl)
**          written
*/

GLOBALDEF const char	    IIQE_39_varchar[] = "VARCHAR";


/*}
** Name: IIQE_40_decimal - Global variable containing string "DECIMAL".
**
** Description:
**      This global variable contains string "DECIMAL".
**
** History:
**      04_nov-88 (carl)
**          written
*/

GLOBALDEF const char	    IIQE_40_decimal[] = "DECIMAL";


/*}
** Name: IIQE_41_qet_t9_ok_w_ldbs - string "QET_T9_OK_W_LDBS"
**
** Description:
**      This global variable contains above string.
**
** History:
**      15-sep-90 (carl)
**          written
*/

GLOBALDEF const char	    IIQE_41_qet_t9_ok_w_ldbs[] = "QET_T9_OK_W_LDBS";


/*}
** Name: IIQE_42_ing_60 - String "II9.1"
**
** Description:
**      This global variable contains above string.
**
** History:
**      20_nov-88 (carl)
**          written
**	21-mar-1995 (rudtr01)
**	    modified IIQE_42_ing_60 from "ING6.0" to "OPING1.1" for bug
**	    67496.
**	05-dec-95 (hanch04)
**	    bug 72307, change "OPING1.1" to "OPING1.2"
**	18-jul-96 (hanch04)
**	    change "OPING1.2" to "OI2.0"
**	09-nov-1998 (mcgem01)
**	    Change "OI2.0" to "II2.5"
**	13-jan-2004 (sheco02)
**	    Change "II2.65" to "II3.0"
**	17-May-2007 (drivi01)
**	    Change "II9.1" to "II9.2"
**	14-Jun-2007 (bonro01)
**	    Change "II9.2" to "II9.3"
**	12-Jul-2010 (bonro01)
**	    Change "II10.0" to "II10.1"
*/

GLOBALDEF const char	    IIQE_42_ing_60[] = "II10.1";


/*}
** Name: IIQE_43_heap - String "HEAP"
**
** Description:
**      This global variable contains above string.
**
** History:
**      11-dec-88 (carl)
**          written
*/

GLOBALDEF const char	    IIQE_43_heap[] = "HEAP";


/*}
** Name: IIQE_50_act_tab - global table containing the names of the STAR 
**			   action types
**
** Description:
**      This global table is used for outputting names of STAR action types.
**
** History:
**      21-jul-90 (carl)
**          written
*/

GLOBALDEF const  char	*IIQE_50_act_tab[] = 
{
    "D0_NIL_DDB (an UNRECOGNIZED action)...\n",
				/* indexed by D0_NIL_DDB - D0_NIL_DDB, 0 */
    "QEA_D1_QRY (a SUBQUERY action)...\n",			
				/* indexed by QEA_D1_QRY - D0_NIL_DDB, 1 */
    "QEA_D2_GET (a GET action following a RETRIEVAL)...\n",
	    			/* indexed by QEA_D2_GET - D0_NIL_DDB, 2 */
    "QEA_D3_XFR (a TRANSFER action)...\n",
				/* indexed by QEA_D3_XFR - D0_NIL_DDB, 3 */
    "QEA_D4_LNK (a REGISTER TABLE action)...\n",			
				/* indexed by QEA_D4_LNK - D0_NIL_DDB, 4 */
    "QEA_D5_DEF (a DEFINE REPEAT QUERY action)...\n",			
				/* indexed by QEA_D5_DEF - D0_NIL_DDB, 5 */
    "QEA_D6_AGG (a RETRIEVAL ACTION returning an intermediate result)...\n",
				/* indexed by QEA_D6_AGG - D0_NIL_DDB, 6 */
    "QEA_D7_OPN (an OPEN CURSOR action)...\n",			
				/* indexed by QEA_D7_OPN - D0_NIL_DDB, 7 */
    "QEA_D8_CRE (a CREATE TABLE action)\n...",			
				/* indexed by QEA_D8_CRE - D0_NIL_DDB, 8 */
    "QEA_D9_UPD (a CURSOR UPDATE action)...\n"			
				/* indexed by QEA_D9_UPD - D0_NIL_DDB, 9 */
};


/*}
** Name: IIQE_51_rqf_tab - global table containing the RQF operation names 
**
** Description:
**      This global table is used for outputting RQF operation names.
**
** History:
**      21-jul-90 (carl)
**          written
*/

GLOBALDEF const  char	*IIQE_51_rqf_tab[] = 
{
    "",				    /* 0 */
    "RQR_STARTUP",		    /* 1 */
    "RQR_SHUTDOWN",		    /* 2 */
    "RQR_S_BEGIN",		    /* 3 */
    "RQR_S_END",		    /* 4 */
    "",				    /* 5 */
    "RQR_READ",			    /* 6 */
    "RQR_WRITE",		    /* 7 */
    "RQR_INTERRUPT",		    /* 8 */
    "RQR_NORMAL",		    /* 9 */
    "RQR_XFR",			    /* 10 */
    "RQR_BEGIN",		    /* 11 */
    "RQR_COMMIT",		    /* 12 */
    "RQR_SECURE",		    /* 13 */
    "RQR_ABORT",		    /* 14 */
    "RQR_DEFINE",		    /* 15 */
    "RQR_EXEC",			    /* 16 */
    "RQR_FETCH",		    /* 17 */
    "RQR_DELETE",		    /* 18 */
    "RQR_CLOSE",		    /* 19 */
    "RQR_T_FETCH",		    /* 20 */
    "RQR_T_FLUSH",		    /* 21 */
    "",				    /* 22 */
    "RQR_CONNECT",		    /* 23 */
    "RQR_DISCONNECT",		    /* 24 */
    "RQR_CONTINUE",		    /* 25 */
    "RQR_QUERY",		    /* 26 */
    "RQR_GET",			    /* 27 */
    "RQR_ENDRET",		    /* 28 */
    "RQR_DATA_VALUES",		    /* 29 */
    "RQR_SET_FUNC",		    /* 30 */
    "RQR_UPDATE",		    /* 31 */
    "RQR_OPEN",			    /* 32 */
    "RQR_MODIFY",		    /* 33 */
    "RQR_CLEANUP",		    /* 34 */
    "RQR_TERM_ASSOC",		    /* 35 */
    "RQR_LDB_ARCH",		    /* 36 */
    "RQR_RESTART",		    /* 37 */
    "RQR_CLUSTER_INFO"		    /* 38 */
};


/*}
** Name: IIQE_52_tpf_tab - global table containing the TPF operation names 
**
** Description:
**      This global table is used for outputting TPF operation names.
**
** History:
**      21-jul-90 (carl)
**          written
*/

GLOBALDEF const  char	*IIQE_52_tpf_tab[] = 
{
    "",				    /* 0 */
    "TPF_STARTUP",		    /* 1 */
    "TPF_SHUTDOWN",		    /* 2 */
    "TPF_INITIALIZE",		    /* 3 */
    "TPF_TERMINATE",		    /* 4 */
    "TPF_SET_DDBINFO",		    /* 5 */    
    "TPF_RECOVER",		    /* 6 */      
    "TPF_COMMIT",		    /* 7 */      
    "TPF_ABORT",		    /* 8 */    
    "TPF_S_DISCNNCT",		    /* 9 */    
    "TPF_C_DISCNNCT",		    /* 10 */      
    "TPF_S_DONE",		    /* 11 */     
    "TPF_S_REFUSED",		    /* 12 */   
    "TPF_READ_DML",		    /* 13 */     
    "TPF_WRITE_DML",		    /* 14 */     
    "TPF_BEGIN_DX",		    /* 15 */     
    "TPF_END_DX",		    /* 16 */   
    "TPF_SAVEPOINT",		    /* 17 */     
    "TPF_SP_ABORT",		    /* 18 */     
    "TPF_AUTO",			    /* 19 */     
    "TPF_NO_AUTO",		    /* 20 */     
    "TPF_EOQ",			    /* 21 */     
    "TPF_1T_DDL_CC",		    /* 22 */     
    "TPF_0F_DDL_CC",		    /* 23 */     
    "TPF_OK_W_LDBS",		    /* 24 */     
    "TPF_P1_RECOVER",		    /* 25 */     
    "TPF_P2_RECOVER",		    /* 26 */     
    "TPF_IS_DDB_OPEN",		    /* 27 */     
    "TPF_TRACE"			    /* 28 */     
};


/*}
** Name: IIQE_53_lo_qef_tab - global table containing the local QEF operation 
**			      names 
**
** Description:
**      This global table is used for outputting local QEF operation names.
**
** History:
**      01-sep-90 (carl)
**          created
*/

GLOBALDEF const  char	*IIQE_53_lo_qef_tab[] = 
{
    "",				    /* 0 */
    "",				    /* 1 */
    "QEC_INITIALIZE",		    /* 2 */
    "QEC_BEGIN_SESSION",	    /* 3 */
    "QEC_DEBUG",		    /* 4 */
    "QEC_ALTER",		    /* 5 */
    "QEC_TRACE",		    /* 6 */
    "QET_BEGIN",		    /* 7 */
    "QET_SAVEPOINT",		    /* 8 */
    "QET_ABORT",		    /* 9 */
    "QET_COMMIT",		    /* 10 */
    "QEQ_OPEN",			    /* 11 */
    "QEQ_FETCH",		    /* 12 */
    "QEQ_REPLACE",		    /* 13 */
    "QEQ_DELETE",		    /* 14 */
    "QEQ_CLOSE",		    /* 15 */
    "QEQ_QUERY",		    /* 16 */
    "QEU_B_COPY",		    /* 17 */
    "QEU_E_COPY",		    /* 18 */
    "QEU_R_COPY",		    /* 19 */
    "QEU_W_COPY",		    /* 20 */
    "QEC_INFO",			    /* 21 */
    "QEC_SHUTDOWN",		    /* 22 */
    "QEC_END_SESSION",		    /* 23 */
    "QEU_BTRAN",		    /* 24 */
    "QEU_ETRAN",		    /* 25 */
    "QEU_APPEND",		    /* 26 */
    "QEU_GET",			    /* 27 */
    "QEU_DELETE",		    /* 28 */
    "QEU_OPEN",			    /* 29 */
    "QEU_CLOSE",		    /* 30 */
    "QEU_DBU",			    /* 31 */
    "QEU_ATRAN",		    /* 32 */
    "QEU_QUERY",		    /* 33 */
    "QEU_CVIEW",		    /* 34 */
    "QEU_DVIEW",		    /* 35 */
    "QEU_CPROT",		    /* 36 */
    "QEU_DPROT",		    /* 37 */
    "QEU_CINTG",		    /* 38 */
    "QEU_DINTG",		    /* 39 */
    "QEU_DSTAT",		    /* 40 */
    "QET_SCOMMIT",		    /* 41 */
    "QET_AUTOCOMMIT",		    /* 42 */
    "QET_ROLLBACK",		    /* 43 */
    "QEQ_ENDRET",		    /* 44 */
    "QEQ_EXECUTE",		    /* 45 */
    "QEU_CREDBP",		    /* 46 */
    "QEU_DRPDBP"		    /* 47 */
};


/*}
** Name: IIQE_54_hi_qef_tab - global table containing the DDB QEF operation 
**			      names 
**
** Description:
**      This global table is used for outputting local QEF operation names.
**
** History:
**      01-sep-90 (carl)
**          created
*/

GLOBALDEF const  char	*IIQE_54_hi_qef_tab[] = 
{
    "QED_IIDBDB",		    /* 500 */
    "QED_DDBINFO",		    /* 501 */		
    "QED_LDBPLUS",		    /* 502 */		
    "QED_USRSTAT",		    /* 503 */		
    "QED_DESCPTR",		    /* 504 */	
    "QED_SESSCDB",		    /* 505 */	
    "",				    /* 506 */
    "QED_C1_CONN",		    /* 507 */		    
    "QED_C2_CONT",		    /* 508 */		
    "QED_C3_DISC",		    /* 509 */		    
    "QED_EXEC_IMM",		    /* 510 */		
    "QED_EXCHANGE",		    /* 511 */		
    "QED_CLINK",		    /* 512 */		
    "QED_DLINK",		    /* 513 */		
    "QED_RLINK",		    /* 514 */		
    "QED_1RDF_QUERY",		    /* 515 */		
    "QED_2RDF_FETCH",		    /* 516 */		
    "QED_3RDF_FLUSH",		    /* 517 */		
    "QED_4RDF_VDATA",		    /* 518 */		
    "QED_5RDF_BEGIN",		    /* 519 */		
    "QED_6RDF_END",		    /* 520 */		
    "QED_7RDF_DIFF_ARCH",	    /* 521 */	
    "QED_IS_DDB_OPEN",		    /* 522 */	
    "QED_1SCF_MODIFY",		    /* 523 */	
    "QED_2SCF_TERM_ASSOC",	    /* 524 */	
    "QED_A_COPY",		    /* 525 */		
    "QED_B_COPY",		    /* 526 */		
    "QED_E_COPY",		    /* 527 */		
    "QED_CONCURRENCY",		    /* 528 */		
    "QED_SET_FOR_RQF",		    /* 529 */		
    "QED_SES_TIMEOUT",		    /* 530 */		    
    "QED_QRY_FOR_LDB",		    /* 531 */		
    "QED_1PSF_RANGE",		    /* 532 */		
    "QED_P1_RECOVER",		    /* 533 */		
    "QED_P2_RECOVER",		    /* 534 */		
    "QED_RECOVER",		    /* 535 */		
    "QED_3SCF_CLUSTER_INFO"	    /* 536 */	
};


/*}
** Name: IIQE_61_qefsess - global variable containing string "QEF session"
**
** Description:
**      This global variable is used for prefixing QEF messages.
**
** History:
**      21-jul-90 (carl)
**          written
*/

GLOBALDEF const  char	IIQE_61_qefsess[] = "QEF";


/*}
** Name: IIQE_62_3dots- String "..."
**
** Description:
**
** History:
**      21-jul-90 (carl)
**          written
*/

GLOBALDEF const  char	IIQE_62_3dots[] = "...";


/*}
** Name: IIQE_63_qperror - String "ERROR detected in query plan!"
**
** Description:
**
** History:
**      21-jul-90 (carl)
**          written
*/

GLOBALDEF const  char	IIQE_63_qperror[] = 
					"ERROR detected in query plan!\n";


/*}
** Name: IIQE_64_callerr - global string "ERROR occurred while calling"
**
** Description:
**
** History:
**      21-jul-90 (carl)
**          written
*/

GLOBALDEF const  char	IIQE_64_callerr[] = 
					"ERROR occurred while calling";


/*}
** Name: IIQE_65_tracing - global variable containing string "Tracing"
**
** Description:
**      This global variable is used for tracing QEF calls.
**
** History:
**      21-jul-90 (carl)
**          created
*/

GLOBALDEF const  char	IIQE_65_tracing[] = "Tracing";


