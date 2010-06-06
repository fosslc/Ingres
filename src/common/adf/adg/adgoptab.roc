/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adfops.h>
#include    <ulf.h>
#include    <adfint.h>
#include    "adgfi_defn.h"

/**
**
**  Name: ADGOPTAB.ROC - ADF's operation table.
**
**  Description:
**      This file contains the operation table initialization which is a table
**      of ADI_OPRATION structures, one per operation.  These structures hold
**      everything the ADF internals will need for any operation.
**
**
**  History:
**      27-jun-86 (thurston)    
**          Initial creation.  The operation table was removed from adginit.c
**          and placed in this file, since it is read-only.  I also removed the
**          coercions from the operation table since I have now decided that
**          the coercion function instances will not have an operation attached
**          to them.
**      27-jul-86 (ericj)
**          Converted for new ADF error handling.
**      11-sep-86 (thurston)
**          Upgraded operations table to account for the four newly added f.i.'s
**          needed by the frontend people to handle the DB_DYC_TYPE datatype.
**      01-oct-86 (thurston)
**          Added operations for "char", "varchar", "like", and "notlike".
**          Also rewrote the comments describing the operations table to better
**          reflect how ADF uses it.
**      16-oct-86 (thurston)
**          Added operations for "hexin", "hexout", and "ifnull".
**	11-nov-86 (thurston)
**	    Changed "hexout" to be "hex", and "hexin" to be an operator, "x".
**	07-jan-87 (thurston)
**	    Removed "x" altogether from this table, since the parser is now
**	    handling hex constants right in the grammar.
**	26-feb-87 (thurston)
**	    Changed GLOBALDEF Adi_operations[] into GLOBALDEF READONLY
**	    Adi_2RO_operations[].
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	09-mar-87 (thurston)
**	    Added the .adi_opqlangs field, to contain the query languages each
**	    operation is defined for.  Also, ...
**	    Added these new operations:
**		"is null"
**		"is not null"
**		"count*"
**	    Removed these operations:
**		"equel_type"
**		"equel_length"
**	    Change the name of:			... its new operation name is:
**		"notlike"		        	"not like"
**	    Made these existing                 ... be synonyms for these,
**	    operations:                         and only defined for QUEL:
**		"countu"		        	"count"
**		"avgu"			        	"avg"
**		"sumu"			        	"sum"
**	    Added these new                     ... to be synonyms for these,
**	    operation names:                    and defined for both SQL & QUEL:
**		"cnt"		        		"count"
**		"average"			        "avg"
**	    Added these new                     ... to be synonyms for these,
**	    operation names:                    and only defined for QUEL:
**		"cntu"		        		"count"
**		"averageu"			        "avg"
**	05-apr-87 (thurston)
**	    Added dbmsinfo and typename.  Also synonymed usercode to username.
**	22-may-87 (thurston)
**	    Changed `typename' to `iitypename'.
**	28-may-87 (thurston)
**	    Added "exists", which will actually have no function instances, but
**	    is needed for the optimizer.
**	07-jun-87 (thurston)
**	    Added `iichar12', `charextract', `date_gmt', `iiuserlen', and
**	    `iistructure'.
**	10-jun-87 (thurston)
**	    Added `ii_notrm_txt' and `ii_notrm_vch'.
**	18-jun-87 (thurston)
**	    Added "not exists", which will actually have no function instances,
**	    but is needed for the optimizer.
**	16-jul-87 (thurston)
**	    Made `_dba' a synonym for `dba' (in both QUEL and SQL), and made
**	    `user', `_username', and `_usercode' synonyms for `username' (also
**	    in both QUEL and SQL).  The underscore variety were added for 5.0
**	    SQL compatibility, while the `user' was added for ANSI SQL compati-
**	    bility.
**	01-sep-87 (thurston)
**	    Added `iihexint'.
**	30-sep-87 (thurston)
**	    Added `ii_tabid_di' and `ii_di_tabid'.
**	26-feb-88 (thurston)
**	    Added `ii_dv_desc'.
**	07-jul-88 (thurston)
**	    Added the items for the new fields for op-usage and valid-systems.
**	06-nov-88 (thurston)
**	    Temporarily commented out the READONLY on the op table.
**	28-feb-89 (fred)
**	    Added initial size variable to be used in size calculations involved
**	    with user defined adt support.
**	06-mar-89 (jrb)
**	    Work done to make this table's contents completely defined at
**	    compile time.  adi_optabfi now called adi_optabfid and is an index
**	    into the fi table instead of an adi_fi_desc ptr.
**	22-may-89 (fred)
**	    Added operators for ii_ext_{type, length}() for use in standard
**	    system catalogs for unexportable types.
**	15-jun-89 (jrb)
**	    Made op table READONLY again.  This should be OK since there are no
**	    pointers in this table (if there were pointers it should be declared
**	    GLOBALCONSTDEF).
**	01-feb-1990 (fred)
**	    Added ii_cpn_dump() operator.  This is mostly used for debugging
**	    peripheral datatypes.
**      06-jul-91 (jrb)
**          Added "<>" as the preferred symbol for the not-equals operator (this
**          is the OPEN SQL symbol).  The old symbol, "!=" is now a synonym.
**          Bug 38180.
**	09-mar-92 (jrb, merged by stevet)
**	    Added ii_iftrue function for outer join project.
**	09-mar-92 (jrb, merged by stevet)
**	    Added resolve_table() operation.
**	    Renamed "xyzzy" to "iixyzzy" to quell objections of certain people.
**  	03-aug-92 (stevet)
**  	    Added gmt_timestamp() operation.
**	29-sep-1992 (fred)
**	    Added "bit" and "varbit" operations.
**      29-sep-1992 (stevet)
**          Added ii_row_count function.
**      03-dec-1992 (stevet)
**          Added session_user, current_user, system_user, and
**          initial_user.
**      09-dec-1992 (rmuth, merged by stevet)
**          Added iitotal_allocated_pages and iitotal_overflow_pages.
**      28-dec-1992 (rblumer, merged by stevet)
**          Changed Adi_2RO_operations table to allow "any" in SQL.
**       9-Apr-1993 (fred)
**          Added byte datatype support operators.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	09-dec-94 (shero03)
**	    Readded iixyzzy().  UDTs and  Spatial types do not load
**	    with the function instance table in that sequence.
**	    iixyzzy() can be deleted in the future from all ADF tables. 
**	09-dec-94 (shero03c)
**	    Removed iixyzzy() again.
**	11-jan-1995 (shero03)
**	    Readded iixyzzy().  Cannot start server: ADF init error
**	07-jun-1996 (ramra01)
**	    Added the long_varchar and long_byte ext interface. Uses the
**	    coerce set of routines rather than the normal funcs. While the
**	    OPERATIONS are defined one needs to add them to the func list
**	    and avoid going thru the coerce routines. This will involve
**	    work to maintain the correlations between the fids and the
**	    indexes as stored in the adfopfis.h. Currently, the addl
**	    len arg will not be supported. 
**	19-feb-1997 (shero03)
**	    Added soundex()
**	06-oct-1998 (shero03)
**	    Add bitwise functions: adu_bit_add, _and, _not, _or, _xor
**	    ii_ipaddr.  Removd xyzzy
**	23-oct-1998 (kitch01)
**		Bug 92735 add _date4() function.
**	20-nov-98 (inkdo01)
**	    Add upper, lower as ANSI synonyms for uppercase, lowercase.
**	12-Jan-1999 (shero03)
**	    Add hash function
**	15-Jan-1999 (shero03)
**	    Add Random functions.
**	25-Jan-1999 (shero03)
**	    Add UUID functions.
**	22-apr-1999 (shero03)
**	    Add SUBSTRING operator.
**	08-Jun-1999 (shero03)
**	    Add unhex function.
**	21-sep-99 (inkdo01)
**	    Made ln synonym for log, power synonym for ** and added
**	    OLAP aggregate operators.
**	13-jun-2000 (thaju02)
**	    Removed operation entries for average, cnt, averageu 
**	    and cntu, since these aggregate functions are
**	    undocumented and result in either incorrect results or
**	    SIGSEGV/SIGBUS in the dbms server. (B74997)
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-apr-2001 (stial01)
**          Added support for tableinfo function
**      17-apr-2001 (stial01)
**          Change tableinfo to iitableinfo
**      25-nov-2003 (gupsh01)
**          Added functions nchar, nvarchar iiutf16_to_utf8, and 
**          iiutf8_to_utf16. 
**	16-dec-03 (inkdo01)
**	    Added character_length, octet_length, bit_length, position, 
**	    atrim - all for standards compatibility.
**	12-apr-04 (inkdo01)
**	    Added int8 for BIGINT, then bigint, integer, smallint and 
**	    tinyint as synonyms for int8, int4, int1, int2.
**	3-dec-04 (inkdo01)
**	    Added collation_weight() for histogram support of Unicode  
**	    and alternate collation data types.
**	17-jan-06 (dougi)
**	    Added "binary", "varbinary" and "long_binary" as synonyms for
**	    "byte", "varbyte" and "long_byte".
**	13-apr-06 (dougi)
**	    Added "isdst" for "is daylight savings" test.
**	30-aug-2006 (gupsh01)
**	    Added new operators for ANSI datetime constant functions
**	    and explicit coercion functions.
**	05-sep-2006 (gupsh01)
**	    Add new operator for interval_ytom, interval_dtos and 
**	    ingresdate (as synonym of date operator). Also reshuffle
**	    the ANSI date/time entries to make them in alphabetical
**	    order.
**	18-oct-2006 (dougi)
**	    Add year(), month(), day(), hour(), minute(), second(), 
**	    microsecond() functions.
**	06-dec-2006 (gupsh01)
**	    Add substr same as substring but more popular.
**	18-dec-2006 (gupsh01)
**	    Added time and timestamp functions as synonyms to
**	    time_wo_tz and timestamp_wo_tz.
**	15-jan-2007 (dougi)
**	    Added "extract" as synonym for "date_part".
**	8-mar-2007 (dougi)
**	    Added round() function (for TPC DS).
**	13-apr-2007 (dougi)
**	    Added trunc[ate](), ceil[ing](), floor(), chr(), ltrim(), rtrim()
**	    for various partner interfaces.
**	16-apr-2007 (dougi)
**	    Added replace(), too. And lpad(), rpad().
**	31-Jul-2007 (kiria01) b117955
**	    With change to fi_defn.awk and integration into jam
**	    the header adfopfis.h has become local header adgfi_defn.h
**	22-Aug-2007 (kiria01) b118999
**	    Added byteextract.
**	26-Dec-2007 (kiria01) SIR119658
**	    Added 'is integer/decimal/float' operators.
**	13-Apr-2008 (kiria01) b119410
**	    Added 'numeric norm' pseudo operator.
**	09-Sep-2008 (kiria01)
**	    Switch the substring & substr entries so that the
**	    ANSI form dominates and allows Star to re-create the
**	    substring command cleanly.
**      10-sep-2008 (gupsh01,stial01)
**          Added 'unorm' pseudo operator.
**	27-Oct-2008 (kiria01) SIR120473
**	    Added patcomp.
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	12-Mar-2009 (kiria01) SIR121788
**	    Added ADI_LNVCHR_OP long_nvarchar
**  22-Apr-2009 (Marty Bowes) SIR 121969
**      Add generate_digit() and validate_digit() for the generation
**      and validation of checksum values.
**      1-Aug-2009 (martin bowes) SIR 122320
**          Added soundex_dm (Daitch-Mokotoff soundex)
**	9-sp-2009 (stephenb)
**	    Add last_identity function.
**      25-sep-2009 (joea)
**          Add "is [not] false", "is [not] true" and "is [not] unknown".
**      23-oct-2009 (joea)
**          Add "boolean" operation.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Add iicmptversion(int), generates string-ish table version number
**	    from a u_u4 dbcmptlvl or relcmptlvl.
**	14-Nov-2009 (kiria01) SIR 122894
**	    Added GREATEST,LEAST,GREATER and LESSER generic polyadic functions.
**	07-Dec-2009 (drewsturgiss) SIR 122882
**	    Added NVL and NVL2, with NVL being an alias of adu_ifnull and NVL2
**	    being a new function.
**	18-Mar-2010 (kiria01) b123438
**	    Added SINGLETON aggregate for scalar sub-query support.
**	25-Mar-2010 (toumi01) SIR 122403
**	    Added "aes_decrypt" and "aes_encrypt".
**      19-apr-2010 (huazh01) 
**          change the definition of SINGLETON aggregate from 
**          ADI_NORM_FUNC to ADI_AGG_FUNC. (b123597)
**/



GLOBALDEF   const	ADI_OPRATION    Adi_2RO_operations[] = {
    /* NOTE:    The only order implied on the operation table has to do with
    **          operation names that are synonyms.  There are cases in this
    **          table where two operation names map to the same op ID.  When
    **          using the adi_opname() routine to retrieve an operation name
    **          from an op ID, the first one in this table should be the one
    **          returned.  Example:  Both "ascii" and "c" map to op ID
    **          ADI_ASCII_OP.  Therefore, "ascii" and "c" are considered
    **          synonyms for the same operation.  However, since "c" comes first
    **          it will be treated as the "preferred" operation name and thus
    **          should be returned as the operation name for op ID ADI_ASCII_OP
    **          by the adi_opname() routine.
    **
    **          For documentation purposes only, I have put this table in an
    **          alphabetic order, except for the "non-prefered" synonyms which
    **          immediately follow their "preferred" synonym.  All synonyms are
    **          also marked with a "SYN" comment at the end of the line.
    */

/*----------------------------------------------------------------------------*/
/*  Each array element in the datatype table looks like this:                 */
/*----------------------------------------------------------------------------*/
/*{ {op_name},          op_id,		type_of_op,	    op_usage,	      */
/*	valid_qlangs,	    valid_systems,				      */
/*		#_fi's,			    index_into_fi_table		    } */
/*----------------------------------------------------------------------------*/

{ {"<>"},		ADI_NE_OP,	ADI_COMPARISON,	    ADI_INFIX,
	DB_SQL|DB_QUEL,	    ADI_INGRES_6|ADI_ANSI|ADI_DB2, 
		ADO_NE_CNT,		    ADZ_NE_FIIDX},		/*SYN*/

{ {"!="},		ADI_NE_OP,	ADI_COMPARISON,	    ADI_INFIX,
	DB_SQL|DB_QUEL,	    ADI_INGRES_6|ADI_ANSI|ADI_DB2, 
		ADO_NE_CNT,		    ADZ_NE_FIIDX},		/* " */

{ {"*"},		ADI_MUL_OP,	ADI_OPERATOR,	    ADI_INFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_MUL_CNT,		    ADZ_MUL_FIIDX},

{ {"**"},		ADI_POW_OP,	ADI_OPERATOR,	    ADI_INFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_POW_CNT,		    ADZ_POW_FIIDX},

{ {"+"},		ADI_ADD_OP,	ADI_OPERATOR,	    ADI_INFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_ADD_CNT,  		    ADZ_ADD_FIIDX},

{ {"-"},		ADI_SUB_OP,	ADI_OPERATOR,	    ADI_INFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_SUB_CNT,  		    ADZ_SUB_FIIDX},

{ {" +"},		ADI_PLUS_OP,    ADI_OPERATOR,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_PLUS_CNT,  		    ADZ_PLUS_FIIDX},

{ {" -"},		ADI_MINUS_OP,   ADI_OPERATOR,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_MINUS_CNT,		    ADZ_MINUS_FIIDX},

{ {"/"},		ADI_DIV_OP,	ADI_OPERATOR,	    ADI_INFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_DIV_CNT,  		    ADZ_DIV_FIIDX},

{ {"<"},		ADI_LT_OP,	ADI_COMPARISON,	    ADI_INFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_LT_CNT,  		    ADZ_LT_FIIDX},

{ {"<="},		ADI_LE_OP,	ADI_COMPARISON,	    ADI_INFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_LE_CNT,  		    ADZ_LE_FIIDX},

{ {"="},		ADI_EQ_OP,	ADI_COMPARISON,	    ADI_INFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_EQ_CNT,  		    ADZ_EQ_FIIDX},

{ {">"},		ADI_GT_OP,	ADI_COMPARISON,	    ADI_INFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_GT_CNT,  		    ADZ_GT_FIIDX},

{ {">="},		ADI_GE_OP,	ADI_COMPARISON,	    ADI_INFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_GE_CNT,  		    ADZ_GE_FIIDX},

{ {"_bintim"},		ADI_BNTIM_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_BNTIM_CNT,		    ADZ_BNTIM_FIIDX},

{ {"_bio_cnt"},		ADI_BIOC_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_BIOC_CNT,  		    ADZ_BIOC_FIIDX},

{ {"_cache_read"},	ADI_CHRD_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CHRD_CNT,  		    ADZ_CHRD_FIIDX},

{ {"_cache_req"},	ADI_CHREQ_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CHREQ_CNT,		    ADZ_CHREQ_FIIDX},

{ {"_cache_rread"},	ADI_CHRRD_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CHRRD_CNT,		    ADZ_CHRRD_FIIDX},

{ {"_cache_size"},	ADI_CHSIZ_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CHSIZ_CNT,		    ADZ_CHSIZ_FIIDX},

{ {"_cache_used"},	ADI_CHUSD_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CHUSD_CNT,		    ADZ_CHUSD_FIIDX},

{ {"_cache_write"},	ADI_CHWR_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CHWR_CNT,  		    ADZ_CHWR_FIIDX},

{ {"_cpu_ms"},		ADI_CPUMS_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CPUMS_CNT,		    ADZ_CPUMS_FIIDX},

{ {"_date"},		ADI_BDATE_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_BDATE_CNT,		    ADZ_BDATE_FIIDX},

{ {"_dio_cnt"},		ADI_DIOC_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_DIOC_CNT,  		    ADZ_DIOC_FIIDX},

{ {"_et_sec"},		ADI_ETSEC_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_ETSEC_CNT,		    ADZ_ETSEC_FIIDX},

{ {"_pfault_cnt"},	ADI_PFLTC_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_PFLTC_CNT,		    ADZ_PFLTC_FIIDX},

{ {"_phys_page"},	ADI_PHPAG_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_PHPAG_CNT,		    ADZ_PHPAG_FIIDX},

{ {"_time"},		ADI_BTIME_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_BTIME_CNT,		    ADZ_BTIME_FIIDX},

{ {"_version"},		ADI_VERSN_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_VERSN_CNT,		    ADZ_VERSN_FIIDX},

{ {"_ws_page"},		ADI_WSPAG_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_WSPAG_CNT,		    ADZ_WSPAG_FIIDX},

{ {"abs"},		ADI_ABS_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_ABS_CNT,  		    ADZ_ABS_FIIDX},

{ {"ansidate"},	ADI_ADTE_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI,  
		ADO_ADTE_CNT,	    ADZ_ADTE_FIIDX},

{ {"any"},		ADI_ANY_OP,	ADI_AGG_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_ANY_CNT,  		    ADZ_ANY_FIIDX},

{ {"atan"},		ADI_ATAN_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_ATAN_CNT,  		    ADZ_ATAN_FIIDX},

{ {"atan2"},		ADI_ATAN2_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_ATAN2_CNT,  		    ADZ_ATAN2_FIIDX},

{ {"tan"},		ADI_TAN_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_TAN_CNT,  		    ADZ_TAN_FIIDX},

{ {"avg"},		ADI_AVG_OP,	ADI_AGG_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_AVG_CNT,  		    ADZ_AVG_FIIDX},		/*SYN*/

{ {"avgu"},		ADI_AVG_OP,	ADI_AGG_FUNC,	    ADI_PREFIX,
	       DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_AVG_CNT,  		    ADZ_AVG_FIIDX},		/* " */

{ {"bigint"},		ADI_I8_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_I8_CNT,  		    ADZ_I8_FIIDX},

{ {"bit"},		ADI_BIT_OP,   	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,     ADI_INGRES_6                 ,  
		ADO_BIT_CNT,		    ADZ_BIT_FIIDX},

{ {"bit_add"},		ADI_BIT_ADD_OP,   ADI_NORM_FUNC,    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_BIT_ADD_CNT,	ADZ_BIT_ADD_FIIDX},

{ {"bit_and"},		ADI_BIT_AND_OP,   ADI_NORM_FUNC,    ADI_PREFIX,
	DB_SQL,     ADI_INGRES_6                 ,  
		ADO_BIT_AND_CNT,	ADZ_BIT_AND_FIIDX},

{ {"bit_length"},	ADI_BLEN_OP,      ADI_NORM_FUNC,    ADI_PREFIX,
	DB_SQL,     ADI_INGRES_6                 ,  
		ADO_BLEN_CNT,		ADZ_BLEN_FIIDX},

{ {"bit_not"},		ADI_BIT_NOT_OP,   ADI_NORM_FUNC,    ADI_PREFIX,
	DB_SQL,     ADI_INGRES_6                 ,  
		ADO_BIT_NOT_CNT,	ADZ_BIT_NOT_FIIDX},

{ {"bit_or"},		ADI_BIT_OR_OP,   ADI_NORM_FUNC,    ADI_PREFIX,
	DB_SQL,     ADI_INGRES_6                 ,  
		ADO_BIT_OR_CNT,	    	ADZ_BIT_OR_FIIDX},

{ {"bit_xor"},		ADI_BIT_XOR_OP,   ADI_NORM_FUNC,    ADI_PREFIX,
	DB_SQL,     ADI_INGRES_6                 ,  
		ADO_BIT_XOR_CNT,	ADZ_BIT_XOR_FIIDX},

{ {"boolean"},          ADI_BOO_OP,       ADI_NORM_FUNC,    ADI_PREFIX,
        DB_SQL,             ADI_INGRES_6,
                ADO_BOO_CNT,              ADZ_BOO_FIIDX},

{ {"byteextract"},	ADI_BYTEXT_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_BYTEXT_CNT,		    ADZ_BYTEXT_FIIDX},

{ {"c"},		ADI_ASCII_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_ASCII_CNT,		    ADZ_ASCII_FIIDX},		/*SYN*/

{ {"ascii"},		ADI_ASCII_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_ASCII_CNT,		    ADZ_ASCII_FIIDX},		/* " */

{ {"char"},		ADI_CHAR_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CHAR_CNT,  		    ADZ_CHAR_FIIDX},

{ {"charextract"},	ADI_CHEXT_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CHEXT_CNT,		    ADZ_CHEXT_FIIDX},

{ {"collation_weight"},	ADI_COWGT_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_COWGT_CNT,		    ADZ_COWGT_FIIDX},

{ {"character_length"},	ADI_CHLEN_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,    	    ADI_INGRES_6|ADI_ANSI        ,  
		ADO_CHLEN_CNT, 		    ADZ_CHLEN_FIIDX},		/*SYN*/

{ {"concat"},		ADI_CNCAT_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CNCAT_CNT,		    ADZ_CNCAT_FIIDX},

{ {"corr"},		ADI_CORR_OP,	ADI_AGG_FUNC,	    ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_CORR_CNT,  		    ADZ_CORR_FIIDX},

{ {"cos"},		ADI_COS_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_COS_CNT,  		    ADZ_COS_FIIDX},

{ {"acos"},		ADI_ACOS_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_ACOS_CNT,  		    ADZ_ACOS_FIIDX},

{ {"count"},		ADI_CNT_OP,	ADI_AGG_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_CNT_CNT,  		    ADZ_CNT_FIIDX},		/*SYN*/

{ {"covar_pop"},	ADI_COVAR_POP_OP,   ADI_AGG_FUNC,   ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_COVAR_POP_CNT,	    ADZ_COVAR_POP_FIIDX},

{ {"covar_samp"},	ADI_COVAR_SAMP_OP,  ADI_AGG_FUNC,   ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_COVAR_SAMP_CNT,	    ADZ_COVAR_SAMP_FIIDX},

{ {"countu"},		ADI_CNT_OP,	ADI_AGG_FUNC,	    ADI_PREFIX,
	       DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CNT_CNT,  		    ADZ_CNT_FIIDX},		/* " */

{ {"count(*)"},		ADI_CNTAL_OP,   ADI_AGG_FUNC,	    ADI_PREFIX,
	DB_SQL        ,     ADI_INGRES_6|ADI_ANSI,  
		ADO_CNTAL_CNT,		    ADZ_CNTAL_FIIDX},

{ {"current_date"},	ADI_CURDATE_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI,  
		ADO_CURDATE_CNT,	    ADZ_CURDATE_FIIDX},	

{ {"current_time"},	ADI_CURTIME_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI,  
		ADO_CURTIME_CNT,	    ADZ_CURTIME_FIIDX},	

{ {"current_timestamp"}, ADI_CURTMSTMP_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI,  
		ADO_CURTMSTMP_CNT,	    ADZ_CURTMSTMP_FIIDX},

{ {"date"},		ADI_DATE_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6         |ADI_DB2,  
		ADO_DATE_CNT,  		    ADZ_DATE_FIIDX},

{ {"ingresdate"},	ADI_DATE_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_DATE_CNT,	    	    ADZ_DATE_FIIDX},		/*SYN*/

{ {"date_gmt"},		ADI_DGMT_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_DGMT_CNT,  		    ADZ_DGMT_FIIDX},

{ {"date_part"},	ADI_DPART_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_DPART_CNT,		    ADZ_DPART_FIIDX},

{ {"date_trunc"},	ADI_DTRUN_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_DTRUN_CNT,		    ADZ_DTRUN_FIIDX},

{ {"dba"},		ADI_DBA_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_DBA_CNT,  		    ADZ_DBA_FIIDX},		/*SYN*/

{ {"_dba"},		ADI_DBA_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_DBA_CNT,  		    ADZ_DBA_FIIDX},		/* " */

{ {"dbmsinfo"},		ADI_DBMSI_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_DBMSI_CNT,		    ADZ_DBMSI_FIIDX},

{ {"decimal"},		ADI_DEC_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6         |ADI_DB2,  
		ADO_DEC_CNT,  		    ADZ_DEC_FIIDX},	        /*SYN*/

{ {"dec"},		ADI_DEC_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6	 |ADI_DB2,  
		ADO_DEC_CNT,  		    ADZ_DEC_FIIDX},	        /* " */

{ {"iiutf8_to_utf16"},		ADI_UTF8TOUTF16_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_UTF8TOUTF16_CNT,		    ADZ_UTF8TOUTF16_FIIDX},

{ {"iiutf16_to_utf8"},		ADI_UTF16TOUTF8_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_UTF16TOUTF8_CNT,		    ADZ_UTF16TOUTF8_FIIDX},

{ {"nchar"},		ADI_NCHAR_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_NCHAR_CNT,		    ADZ_NCHAR_FIIDX},

{ {"nvarchar"},		ADI_NVCHAR_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_NVCHAR_CNT,		    ADZ_NVCHAR_FIIDX},

{ {"numeric"},		ADI_DEC_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6	 |ADI_DB2,
		ADO_DEC_CNT,  		    ADZ_DEC_FIIDX},	        /* " */

{ {"dow"},		ADI_DOW_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_DOW_CNT,  		    ADZ_DOW_FIIDX},

{ {"exists"},		ADI_EXIST_OP,   ADI_AGG_FUNC,	    ADI_PREFIX,
	DB_SQL        ,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_EXIST_CNT,		    ADZ_EXIST_FIIDX},

{ {"exp"},		ADI_EXP_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_EXP_CNT,  		    ADZ_EXP_FIIDX},

{ {"extract"},		ADI_DPART_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_DPART_CNT,		    ADZ_DPART_FIIDX},

{ {"float4"},		ADI_F4_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_F4_CNT,  		    ADZ_F4_FIIDX},

{ {"float8"},		ADI_F8_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_F8_CNT,  		    ADZ_F8_FIIDX},

{ {"greater"},		ADI_GREATER_OP,   ADI_NORM_FUNC,   ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_GREATER_CNT,	    ADZ_GREATER_FIIDX},

{ {"greatest"},		ADI_GREATEST_OP,   ADI_NORM_FUNC,   ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_GREATEST_CNT,	    ADZ_GREATEST_FIIDX},

{ {"hash"},		ADI_HASH_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_HASH_CNT,  		    ADZ_HASH_FIIDX},

{ {"hex"},		ADI_HEX_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_HEX_CNT,  		    ADZ_HEX_FIIDX},

{ {"ifnull"},		ADI_IFNUL_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_IFNUL_CNT,		    ADZ_IFNUL_FIIDX},

{ {"ii_cpn_dump"},	ADI_CPNDMP_OP,  ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,	    ADI_INGRES_6,
		ADO_CPNDMP_CNT,		    ADZ_CPNDMP_FIIDX},
		
{ {"ii_di_tabid"},	ADI_DI2TB_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_DI2TB_CNT,		    ADZ_DI2TB_FIIDX},

{ {"ii_dv_desc"},	ADI_DVDSC_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_DVDSC_CNT,		    ADZ_DVDSC_FIIDX},

{ {"ii_iftrue"},	ADI_IFTRUE_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_IFTRUE_CNT,		    ADZ_IFTRUE_FIIDX},

{ {"ii_ipaddr"},	ADI_IPADDR_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,     ADI_INGRES_6                 ,  
		ADO_IPADDR_CNT,		    ADZ_IPADDR_FIIDX},

{ {"ii_notrm_txt"},	ADI_NTRMT_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_NTRMT_CNT,		    ADZ_NTRMT_FIIDX},

{ {"ii_notrm_vch"},	ADI_NTRMV_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_NTRMV_CNT,		    ADZ_NTRMV_FIIDX},

{ {"ii_tabid_di"},	ADI_TB2DI_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_TB2DI_CNT,		    ADZ_TB2DI_FIIDX},

{ {"iichar12"},		ADI_CHA12_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CHA12_CNT,		    ADZ_CHA12_FIIDX},

{ {"iihexint"},		ADI_HXINT_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_HXINT_CNT,		    ADZ_HXINT_FIIDX},

{ {"iistructure"},	ADI_STRUC_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_STRUC_CNT,		    ADZ_STRUC_FIIDX},

{ {"iitotal_allocated_pages"},  ADI_ALLOCPG_OP,  ADI_NORM_FUNC,     ADI_PREFIX,
        DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,
                ADO_ALLOCPG_CNT,                    ADZ_ALLOCPG_FIIDX},

{ {"iitotal_overflow_pages"},   ADI_OVERPG_OP,  ADI_NORM_FUNC,      ADI_PREFIX,
        DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,
                ADO_OVERPG_CNT,             ADZ_OVERPG_FIIDX},

{ {"iitypename"},	ADI_TYNAM_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_TYNAM_CNT,		    ADZ_TYNAM_FIIDX},

{ {"iiuserlen"},	ADI_USRLN_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_USRLN_CNT,		    ADZ_USRLN_FIIDX},

{ {"int"},		ADI_I4_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_I4_CNT,  		    ADZ_I4_FIIDX},

{ {"int1"},		ADI_I1_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_I1_CNT,  		    ADZ_I1_FIIDX},

{ {"int2"},		ADI_I2_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_I2_CNT,  		    ADZ_I2_FIIDX},

{ {"int4"},		ADI_I4_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_I4_CNT,  		    ADZ_I4_FIIDX},

{ {"int8"},		ADI_I8_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_I8_CNT,  		    ADZ_I8_FIIDX},

{ {"integer"},		ADI_I4_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_I4_CNT,  		    ADZ_I4_FIIDX},

{ {"interval"},		ADI_INTVL_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_INTVL_CNT,		    ADZ_INTVL_FIIDX},

{ {"interval_dtos"},	ADI_INDS_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI,  
		ADO_INYM_CNT,	    ADZ_INDS_FIIDX},		

{ {"interval_ytom"},	ADI_INYM_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI,  
		ADO_INDS_CNT,	    ADZ_INYM_FIIDX},	

{ {"intextract"},	ADI_INTEXT_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_INTEXT_CNT,		    ADZ_INTEXT_FIIDX},

{ {"is decimal"},	ADI_ISDEC_OP,   ADI_COMPARISON,	    ADI_POSTFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_ISDEC_CNT,		    ADZ_ISDEC_FIIDX},

{ {"is false"},        ADI_ISFALSE_OP,  ADI_COMPARISON,     ADI_POSTFIX,
        DB_SQL,             ADI_INGRES_6|ADI_ANSI,
                ADO_ISFALSE_CNT,            ADZ_ISFALSE_FIIDX},

{ {"is float"},		ADI_ISFLT_OP,   ADI_COMPARISON,	    ADI_POSTFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_ISFLT_CNT,		    ADZ_ISFLT_FIIDX},

{ {"is integer"},	ADI_ISINT_OP,   ADI_COMPARISON,	    ADI_POSTFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_ISINT_CNT,		    ADZ_ISINT_FIIDX},

{ {"is not decimal"},	ADI_NODEC_OP,   ADI_COMPARISON,	    ADI_POSTFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_NODEC_CNT,		    ADZ_NODEC_FIIDX},

{ {"is not false"},     ADI_NOFALSE_OP,  ADI_COMPARISON,     ADI_POSTFIX,
        DB_SQL,             ADI_INGRES_6|ADI_ANSI,
                ADO_NOFALSE_CNT,            ADZ_NOFALSE_FIIDX},

{ {"is not float"},	ADI_NOFLT_OP, ADI_COMPARISON,	    ADI_POSTFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_NOFLT_CNT,		    ADZ_NOFLT_FIIDX},

{ {"is not integer"},	ADI_NOINT_OP,   ADI_COMPARISON,	    ADI_POSTFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_NOINT_CNT,		    ADZ_NOINT_FIIDX},

{ {"is not null"},	ADI_NONUL_OP,   ADI_COMPARISON,	    ADI_POSTFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_NONUL_CNT,		    ADZ_NONUL_FIIDX},

{ {"is not true"},      ADI_NOTRUE_OP,   ADI_COMPARISON,    ADI_POSTFIX,
        DB_SQL,             ADI_INGRES_6|ADI_ANSI,  
                ADO_NOTRUE_CNT,             ADZ_NOTRUE_FIIDX},

{ {"is not unknown"},   ADI_NOUNKN_OP,   ADI_COMPARISON,    ADI_POSTFIX,
        DB_SQL,             ADI_INGRES_6|ADI_ANSI,
                ADO_NOUNKN_CNT,             ADZ_NOUNKN_FIIDX},

{ {"is null"},		ADI_ISNUL_OP,   ADI_COMPARISON,	    ADI_POSTFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_ISNUL_CNT,		    ADZ_ISNUL_FIIDX},

{ {"is true"},          ADI_ISTRUE_OP,   ADI_COMPARISON,    ADI_POSTFIX,
        DB_SQL,             ADI_INGRES_6|ADI_ANSI,
                ADO_ISTRUE_CNT,             ADZ_ISTRUE_FIIDX},

{ {"is unknown"},       ADI_ISUNKN_OP,   ADI_COMPARISON,    ADI_POSTFIX,
        DB_SQL,             ADI_INGRES_6|ADI_ANSI,
                ADO_ISUNKN_CNT,             ADZ_ISUNKN_FIIDX},

{ {"least"},		ADI_LEAST_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_LEAST_CNT,		    ADZ_LEAST_FIIDX},

{ {"left"},		ADI_LEFT_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_LEFT_CNT,  		    ADZ_LEFT_FIIDX},

{ {"length"},		ADI_LEN_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6         |ADI_DB2,  
		ADO_LEN_CNT,  		    ADZ_LEN_FIIDX},

{ {"lesser"},		ADI_LESSER_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_LESSER_CNT,		    ADZ_LESSER_FIIDX},

{ {"like"},		ADI_LIKE_OP,	ADI_COMPARISON,	    ADI_INFIX,
	DB_SQL        ,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_LIKE_CNT,  		    ADZ_LIKE_FIIDX},

{ {"ln"},		ADI_LOG_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_LOG_CNT,  		    ADZ_LOG_FIIDX},

{ {"local_time"},	ADI_LCLTIME_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI,  
		ADO_LCLTIME_CNT,	    ADZ_LCLTIME_FIIDX},

{ {"local_timestamp"},	ADI_LCLTMSTMP_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI,  
		ADO_LCLTMSTMP_CNT,	    ADZ_LCLTMSTMP_FIIDX},

{ {"locate"},		ADI_LOC_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_LOC_CNT,  		    ADZ_LOC_FIIDX},

{ {"log"},		ADI_LOG_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_LOG_CNT,  		    ADZ_LOG_FIIDX},

{ {"lowercase"},	ADI_LOWER_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_LOWER_CNT,		    ADZ_LOWER_FIIDX},

{ {"lower"},		ADI_LOWER_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_LOWER_CNT,		    ADZ_LOWER_FIIDX},

{ {"max"},		ADI_MAX_OP,	ADI_AGG_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_MAX_CNT,  		    ADZ_MAX_FIIDX},

{ {"min"},		ADI_MIN_OP,	ADI_AGG_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_MIN_CNT,  		    ADZ_MIN_FIIDX},

{ {"mod"},		ADI_MOD_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_MOD_CNT,  		    ADZ_MOD_FIIDX},

{ {"money"},		ADI_MONEY_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_MONEY_CNT,		    ADZ_MONEY_FIIDX},

{ {"not exists"},	ADI_NXIST_OP,   ADI_AGG_FUNC,	    ADI_PREFIX,
	DB_SQL        ,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_NXIST_CNT,		    ADZ_NXIST_FIIDX},

{ {"not like"},		ADI_NLIKE_OP,   ADI_COMPARISON,	    ADI_INFIX,
	DB_SQL        ,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_NLIKE_CNT,		    ADZ_NLIKE_FIIDX},

{ {"numericnorm"},	ADI_NUMNORM_OP,   ADI_NORM_FUNC,    ADI_PREFIX,
	DB_SQL        ,     ADI_INGRES_6,  
		ADO_NUMNORM_CNT,	ADZ_NUMNORM_FIIDX},
 
{ {"nvl"},		ADI_IFNUL_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_IFNUL_CNT,		    ADZ_IFNUL_FIIDX},

{ {"nvl2"},		ADI_NVL2_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_NVL2_CNT,		    ADZ_NVL2_FIIDX},


{ {"octet_length"},	ADI_OCLEN_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,    	    ADI_INGRES_6|ADI_ANSI        ,  
		ADO_OCLEN_CNT, 		    ADZ_OCLEN_FIIDX},

{ {"pad"},		ADI_PAD_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_PAD_CNT,  		    ADZ_PAD_FIIDX},

{ {"patcomp"},		ADI_PATCOMP_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,     ADI_INGRES_6                 ,  
		ADO_PATCOMP_CNT,  	    ADZ_PATCOMP_FIIDX},

{ {"pi"},		ADI_PI_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_PI_CNT,  		    ADZ_PI_FIIDX},

{ {"position"},		ADI_POS_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,     	    ADI_INGRES_6                 ,  
		ADO_POS_CNT,  		    ADZ_POS_FIIDX},

{ {"power"},		ADI_POW_OP,	ADI_OPERATOR,	    ADI_INFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_POW_CNT,		    ADZ_POW_FIIDX},

{ {"randomf"},		ADI_RANDOMF_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_RANDOMF_CNT,	    ADZ_RANDOMF_FIIDX},

{ {"random"},		ADI_RANDOM_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_RANDOM_CNT,		    ADZ_RANDOM_FIIDX},

{ {"regr_avgx"},	ADI_REGR_AVGX_OP, ADI_AGG_FUNC,     ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_REGR_AVGX_CNT,	    ADZ_REGR_AVGX_FIIDX},

{ {"regr_avgy"},	ADI_REGR_AVGY_OP, ADI_AGG_FUNC,     ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_REGR_AVGY_CNT,	    ADZ_REGR_AVGY_FIIDX},

{ {"regr_count"},	ADI_REGR_COUNT_OP, ADI_AGG_FUNC,    ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_REGR_COUNT_CNT,	    ADZ_REGR_COUNT_FIIDX},

{ {"regr_intercept"},	ADI_REGR_INTERCEPT_OP, ADI_AGG_FUNC, ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_REGR_INTERCEPT_CNT,	    ADZ_REGR_INTERCEPT_FIIDX},

{ {"regr_r2"},		ADI_REGR_R2_OP,    ADI_AGG_FUNC,    ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_REGR_R2_CNT,	    ADZ_REGR_R2_FIIDX},

{ {"regr_slope"},	ADI_REGR_SLOPE_OP, ADI_AGG_FUNC,    ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_REGR_SLOPE_CNT,	    ADZ_REGR_SLOPE_FIIDX},

{ {"regr_sxx"},		ADI_REGR_SXX_OP,   ADI_AGG_FUNC,    ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_REGR_SXX_CNT,	    ADZ_REGR_SXX_FIIDX},

{ {"regr_sxy"},		ADI_REGR_SXY_OP,   ADI_AGG_FUNC,    ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_REGR_SXY_CNT,	    ADZ_REGR_SXY_FIIDX},

{ {"regr_syy"},		ADI_REGR_SYY_OP,   ADI_AGG_FUNC,    ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_REGR_SYY_CNT,	    ADZ_REGR_SYY_FIIDX},

{ {"replace"},		ADI_REPL_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_REPL_CNT,  		    ADZ_REPL_FIIDX},

{ {"resolve_table"},	ADI_RESTAB_OP,  ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,
		ADO_RESTAB_CNT,		    ADZ_RESTAB_FIIDX},

{ {"right"},		ADI_RIGHT_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_RIGHT_CNT,		    ADZ_RIGHT_FIIDX},

{ {"round"},		ADI_ROUND_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_ROUND_CNT,		    ADZ_ROUND_FIIDX},

{ {"ceil"},		ADI_CEIL_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CEIL_CNT,		    ADZ_CEIL_FIIDX},

{ {"ceiling"},		ADI_CEIL_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CEIL_CNT,		    ADZ_CEIL_FIIDX},

{ {"chr"},		ADI_CHR_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CHR_CNT,		    ADZ_CHR_FIIDX},

{ {"floor"},		ADI_FLOOR_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_FLOOR_CNT,		    ADZ_FLOOR_FIIDX},

{ {"trunc"},		ADI_TRUNC_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_TRUNC_CNT,		    ADZ_TRUNC_FIIDX},

{ {"truncate"},		ADI_TRUNC_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_TRUNC_CNT,		    ADZ_TRUNC_FIIDX},

{ {"shift"},		ADI_SHIFT_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_SHIFT_CNT,		    ADZ_SHIFT_FIIDX},

{ {"sign"},		ADI_SIGN_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_SIGN_CNT,  		    ADZ_SIGN_FIIDX},

{ {"sin"},		ADI_SIN_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_SIN_CNT,  		    ADZ_SIN_FIIDX},

{ {"asin"},		ADI_ASIN_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_ASIN_CNT,  		    ADZ_ASIN_FIIDX},

{ {"singleton"},	ADI_SINGLETON_OP,   ADI_AGG_FUNC,  ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_SINGLETON_CNT,  	    ADZ_SINGLETON_FIIDX},

{ {"size"},		ADI_SIZE_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_SIZE_CNT,  		    ADZ_SIZE_FIIDX},

{ {"smallint"},		ADI_I2_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_I2_CNT,  		    ADZ_I2_FIIDX},

{ {"soundex"},		ADI_SOUNDEX_OP,  ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_SOUNDEX_CNT,  	    ADZ_SOUNDEX_FIIDX},


{ {"sqrt"},		ADI_SQRT_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_SQRT_CNT,  		    ADZ_SQRT_FIIDX},

{ {"squeeze"},		ADI_SQZ_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_SQZ_CNT,  		    ADZ_SQZ_FIIDX},

{ {"stddev_pop"},	ADI_STDDEV_POP_OP, ADI_AGG_FUNC,    ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_STDDEV_POP_CNT,	    ADZ_STDDEV_POP_FIIDX},

{ {"stddev_samp"},	ADI_STDDEV_SAMP_OP, ADI_AGG_FUNC,   ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_STDDEV_SAMP_CNT,	    ADZ_STDDEV_SAMP_FIIDX},

{ {"substring"},	ADI_SUBSTRING_OP, ADI_NORM_FUNC,     ADI_PREFIX,
	       DB_SQL,     ADI_INGRES_6                 ,  
		ADO_SUBSTRING_CNT,	    ADZ_SUBSTRING_FIIDX}, 	

{ {"substr"},	ADI_SUBSTRING_OP, ADI_NORM_FUNC,     ADI_PREFIX,
	       DB_SQL,     ADI_INGRES_6                 ,  
		ADO_SUBSTRING_CNT,	    ADZ_SUBSTRING_FIIDX}, 	

{ {"sum"},		ADI_SUM_OP,	ADI_AGG_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_SUM_CNT,  		    ADZ_SUM_FIIDX},		/*SYN*/

{ {"sumu"},		ADI_SUM_OP,	ADI_AGG_FUNC,	    ADI_PREFIX,
	       DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_SUM_CNT,  		    ADZ_SUM_FIIDX},		/* " */

{ {"iitableinfo"},	ADI_TABLEINFO_OP,   ADI_NORM_FUNC,    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_TABLEINFO_CNT,	    ADZ_TABLEINFO_FIIDX},

{ {"text"},		ADI_TEXT_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_TEXT_CNT,  		    ADZ_TEXT_FIIDX},		/*SYN*/

{ {"time"},	ADI_TMWO_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6,  
		ADO_TMWO_CNT,	    ADZ_TMWO_FIIDX},		

{ {"time_local"},	ADI_TME_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI,  
		ADO_TME_CNT,	    ADZ_TME_FIIDX},

{ {"time_with_tz"},	ADI_TMW_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI,  
		ADO_TMW_CNT,	    ADZ_TMW_FIIDX},	

{ {"time_wo_tz"},	ADI_TMWO_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6,  
		ADO_TMWO_CNT,	    ADZ_TMWO_FIIDX},		

{ {"timestamp"},	ADI_TSWO_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI,  
		ADO_TSWO_CNT,	    ADZ_TSWO_FIIDX},	

{ {"timestamp_local"},	ADI_TSTMP_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI,  
		ADO_TSTMP_CNT,	    ADZ_TSTMP_FIIDX},	

{ {"timestamp_with_tz"},	ADI_TSW_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI,  
		ADO_TSW_CNT,	    ADZ_TSW_FIIDX},		

{ {"timestamp_wo_tz"},	ADI_TSWO_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI,  
		ADO_TSWO_CNT,	    ADZ_TSWO_FIIDX},	

{ {"tinyint"},		ADI_I1_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_I1_CNT,  		    ADZ_I1_FIIDX},

{ {"var_pop"},		ADI_VAR_POP_OP, ADI_AGG_FUNC,       ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_VAR_POP_CNT,	    ADZ_VAR_POP_FIIDX},

{ {"var_samp"},		ADI_VAR_SAMP_OP,ADI_AGG_FUNC,       ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_VAR_SAMP_CNT,	    ADZ_VAR_SAMP_FIIDX},

{ {"vchar"},		ADI_TEXT_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_TEXT_CNT,  		    ADZ_TEXT_FIIDX},		/* " */

{ {"trim"},		ADI_TRIM_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_TRIM_CNT,  		    ADZ_TRIM_FIIDX},

{ {"atrim"},		ADI_ATRIM_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_ATRIM_CNT,  	    ADZ_ATRIM_FIIDX},

{ {"ltrim"},		ADI_LTRIM_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_LTRIM_CNT,  	    ADZ_LTRIM_FIIDX},

{ {"rtrim"},		ADI_TRIM_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_TRIM_CNT,  	    ADZ_TRIM_FIIDX},

{ {"lpad"},		ADI_LPAD_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_LPAD_CNT,  	    ADZ_LPAD_FIIDX},

{ {"rpad"},		ADI_RPAD_OP,    ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,	            ADI_INGRES_6                 ,  
		ADO_RPAD_CNT,  	    ADZ_RPAD_FIIDX},

{ {"unhex"},		ADI_UNHEX_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_UNHEX_CNT,     	    ADZ_UNHEX_FIIDX},

{ {"unorm"},		ADI_UNORM_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	0,     0,  
		ADO_UNORM_CNT,     	    ADZ_UNORM_FIIDX},

{ {"uppercase"},	ADI_UPPER_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_UPPER_CNT,		    ADZ_UPPER_FIIDX},

{ {"upper"},		ADI_UPPER_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_UPPER_CNT,		    ADZ_UPPER_FIIDX},

{ {"user"},		ADI_USRNM_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_USRNM_CNT,		    ADZ_USRNM_FIIDX},	    	/*SYN*/

{ {"current_user"},	ADI_USRNM_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_USRNM_CNT,		    ADZ_USRNM_FIIDX},		/*SYN*/

{ {"username"},		ADI_USRNM_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_USRNM_CNT,		    ADZ_USRNM_FIIDX},		/* " */

{ {"usercode"},		ADI_USRNM_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_USRNM_CNT,		    ADZ_USRNM_FIIDX},		/* " */

{ {"_username"},	ADI_USRNM_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_USRNM_CNT,		    ADZ_USRNM_FIIDX},		/* " */

{ {"_usercode"},	ADI_USRNM_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_USRNM_CNT,		    ADZ_USRNM_FIIDX},		/* " */

{ {"uuid_create"}, 	ADI_UUID_CREATE_OP, ADI_NORM_FUNC,  ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_UUID_CREATE_CNT,	    ADZ_UUID_CREATE_FIIDX},

{ {"uuid_to_char"}, 	ADI_UUID_TO_CHAR_OP, ADI_NORM_FUNC,  ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_UUID_TO_CHAR_CNT,	    ADZ_UUID_TO_CHAR_FIIDX},

{ {"uuid_from_char"}, 	ADI_UUID_FROM_CHAR_OP, ADI_NORM_FUNC,  ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_UUID_FROM_CHAR_CNT,	    ADZ_UUID_FROM_CHAR_FIIDX},

{ {"uuid_compare"}, 	ADI_UUID_COMPARE_OP, ADI_NORM_FUNC,  ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_UUID_COMPARE_CNT,	    ADZ_UUID_COMPARE_FIIDX},

{ {"xyzzy"},		ADI_XYZZY_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_XYZZY_CNT,		    ADZ_XYZZY_FIIDX},


{ {"session_user"},	ADI_SESSUSER_OP, ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_SESSUSER_CNT,   	    ADZ_SESSUSER_FIIDX}, 	/*SYN*/

{ {"system_user"},	ADI_SYSUSER_OP, ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_SYSUSER_CNT,   	    ADZ_SYSUSER_FIIDX}, 

{ {"initial_user"},	ADI_INITUSER_OP,   ADI_NORM_FUNC,    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6|ADI_ANSI|ADI_DB2,  
		ADO_INITUSER_CNT,	    ADZ_INITUSER_FIIDX},

{ {"varbit"},		ADI_VBIT_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,     ADI_INGRES_6                 ,  
		ADO_VBIT_CNT,		    ADZ_VBIT_FIIDX},
{ {"varchar"},		ADI_VARCH_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_VARCH_CNT,		    ADZ_VARCH_FIIDX},

{ {"ii_ext_type"},	ADI_ETYPE_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,	    ADI_INGRES_6,
		ADO_ETYPE_CNT,		    ADZ_ETYPE_FIIDX},

{ {"ii_ext_length"},	ADI_ELENGTH_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,	    ADI_INGRES_6,
		ADO_ELENGTH_CNT,	    ADZ_ELENGTH_FIIDX},

{ {"table_key"},	ADI_TABKEY_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,	    ADI_INGRES_6,
		ADO_TABKEY_CNT,	    ADZ_TABKEY_FIIDX},

{ {"object_key"},	ADI_LOGKEY_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,	    ADI_INGRES_6,
		ADO_LOGKEY_CNT,	    ADZ_LOGKEY_FIIDX},

{ {"gmt_timestamp"},	ADI_GMTSTAMP_OP, ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_GMTSTAMP_CNT,    ADZ_GMTSTAMP_FIIDX},

{ {"ii_row_count"},	ADI_ROWCNT_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,	    ADI_INGRES_6,
		ADO_ROWCNT_CNT,	    ADZ_ROWCNT_FIIDX},
{ {"byte"},	        ADI_BYTE_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,	        ADI_INGRES_6,
		ADO_BYTE_CNT,	    ADZ_BYTE_FIIDX},
{ {"varbyte"},	        ADI_VBYTE_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,	        ADI_INGRES_6,
		ADO_VBYTE_CNT,	    ADZ_VBYTE_FIIDX},
{ {"binary"},	        ADI_BYTE_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,	        ADI_INGRES_6,
		ADO_BYTE_CNT,	    ADZ_BYTE_FIIDX},
{ {"varbinary"},        ADI_VBYTE_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,	        ADI_INGRES_6,
		ADO_VBYTE_CNT,	    ADZ_VBYTE_FIIDX},
{ {"ii_lolk"},	        ADI_LOLK_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL,	        ADI_INGRES_6,
		ADO_LOLK_CNT,	    ADZ_LOLK_FIIDX},

{ {"session_priv"},	ADI_SESSION_PRIV_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,	    ADI_INGRES_6,
		ADO_SESSION_PRIV_CNT,   ADZ_SESSION_PRIV_FIIDX},

{ {"iitblstat"},	ADI_IITBLSTAT_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,	    ADI_INGRES_6,
		ADO_IITBLSTAT_CNT,   ADZ_IITBLSTAT_FIIDX},

{ {"long_varchar"},      ADI_LONG_VARCHAR,   ADI_NORM_FUNC,      ADI_PREFIX,
        DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,
                ADO_LVARCH_CNT,              ADZ_LVARCH_FIIDX},

{ {"long_byte"},      ADI_LONG_BYTE,         ADI_NORM_FUNC,      ADI_PREFIX,
        DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,
                ADO_LBYTE_CNT,               ADZ_LBYTE_FIIDX},

{ {"long_binary"},      ADI_LONG_BYTE,         ADI_NORM_FUNC,      ADI_PREFIX,
        DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,
                ADO_LBYTE_CNT,               ADZ_LBYTE_FIIDX},

{ {"long_nvarchar"},      ADI_LNVCHR_OP,   ADI_NORM_FUNC,      ADI_PREFIX,
        DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,
                ADO_LNVCHR_CNT,              ADZ_LNVCHR_FIIDX},

{ {"iipermittype"},	ADI_PTYPE_OP,	ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,	    ADI_INGRES_6,
		ADO_PTYPE_CNT,		    ADZ_PTYPE_FIIDX},

{ {"_date4"},		ADI_BDATE4_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_BDATE4_CNT,		    ADZ_BDATE4_FIIDX},

{ {"isdst"},		ADI_ISDST_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_ISDST_CNT,		    ADZ_ISDST_FIIDX},

{ {"year"},	ADI_YPART_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_YPART_CNT,		    ADZ_YPART_FIIDX},

{ {"quarter"},	ADI_QPART_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_QPART_CNT,		    ADZ_QPART_FIIDX},

{ {"month"},	ADI_MPART_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_MPART_CNT,		    ADZ_MPART_FIIDX},

{ {"week"},	ADI_WPART_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_WPART_CNT,		    ADZ_WPART_FIIDX},

{ {"week_iso"},	ADI_WIPART_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_WIPART_CNT,		    ADZ_WIPART_FIIDX},

{ {"day"},	ADI_DYPART_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_DYPART_CNT,		    ADZ_DYPART_FIIDX},

{ {"hour"},	ADI_HPART_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_HPART_CNT,		    ADZ_HPART_FIIDX},

{ {"minute"},	ADI_MIPART_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_MIPART_CNT,		    ADZ_MIPART_FIIDX},

{ {"second"},	ADI_SPART_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_SPART_CNT,		    ADZ_SPART_FIIDX},

{ {"microsecond"},	ADI_MSPART_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_MSPART_CNT,		    ADZ_MSPART_FIIDX},

{ {"nanosecond"},	ADI_NPART_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_NPART_CNT,		    ADZ_NPART_FIIDX},
		
{ {"last_identity"},	ADI_LASTID_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
			DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
				ADO_LASTID_CNT,		    ADZ_LASTID_FIIDX},

{ {"iicmptversion"},	ADI_CMPTVER_OP,   ADI_NORM_FUNC,	ADI_PREFIX,
			DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_CMPTVER_CNT,	ADZ_CMPTVER_FIIDX},
		
{ {"generate_digit"},	ADI_GENERATEDIGIT_OP,  ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_GENERATEDIGIT_CNT,  	    ADZ_GENERATEDIGIT_FIIDX},

{ {"validate_digit"},	ADI_VALIDATEDIGIT_OP,  ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_VALIDATEDIGIT_CNT,  	    ADZ_VALIDATEDIGIT_FIIDX},

{ {"aes_decrypt"},	ADI_AES_DECRYPT_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_AES_DECRYPT_CNT,    ADZ_AES_DECRYPT_FIIDX},

{ {"aes_encrypt"},	ADI_AES_ENCRYPT_OP,   ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_AES_ENCRYPT_CNT,    ADZ_AES_ENCRYPT_FIIDX},

{ {"soundex_dm"},	ADI_SOUNDEX_DM_OP,  ADI_NORM_FUNC,	    ADI_PREFIX,
	DB_SQL|DB_QUEL,     ADI_INGRES_6                 ,  
		ADO_SOUNDEX_DM_CNT,  	    ADZ_SOUNDEX_DM_FIIDX},

{ {""},			ADI_NOOP,	-1,		    -1,
	      0       ,                     0            ,  
		0,			    ADI_NOFI}

};                                      /* ADF's OPERATION table */
GLOBALDEF   const	i4	    Adi_ops_size = sizeof(Adi_2RO_operations);
