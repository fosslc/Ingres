/*
**	Copyright (c) 2004,2008 Ingres Corporation
**	All rights reserved.
*/

/**
** Name: ADFFIIDS.H - Symbolic constants for all function instance IDs.
**
** Description:
**      This file contains the definitions of symbolic constants for all
**      function instance IDs.
**
** History:
**      08-jul-86 (thurston)
**          Initial creation.
**	10-sep-86 (thurston)
**	    Added four more function instance ID's for the front end people.
**	    These will work on the newly added datatype "abrtsstr" (which
**	    stands for, "ABf Run Time String STRing", and has a datatype of
**	    DB_DYC_TYPE).
**	14-oct-86 (thurston)
**	    Added in #defines for all of the new function instances
**	    that handle CHAR and VARCHAR, and the necessary coercions
**	    for LONGTEXT.
**	15-oct-86 (thurston)
**	    Added #defines for all of the comparison function instances
**	    that handle the dynamic string datatype, and for all of the
**	    function instances for the HEXIN, HEXOUT, and IFNULL
**	    functions.
**	07-jan-87 (thurston)
**	    Removed 10 fi ID constants for various reasons.
**	29-jan-87 (thurston)
**	    Removed ADFI_265_C_ADD_C__DYC because it caused ambiguity problem.
**	10-mar-87 (thurston)
**	    Added fi ID constants for all of the ISNULL and ISNOTNULL fi's,
**	    also for the COUNT(*) fi.  Also removed all constants for the AVGU
**	    COUNTU, SUMU, EQUEL_LENGTH, and EQUEL_TYPE functions.  ADF is no
**	    longer concerned with the `distinct' aggregates, and the
**	    EQUEL_LENGTH and EQUEL_TYPE functions will not be supported in 6.0.
**	05-apr-87 (thurston)
**	    Added ADFI_600_DBMSINFO_VARCHAR and ADFI_601_TYPENAME_I.  Also
**	    removed ADFI_242_USERCODE.
**	08-jun-87 (thurston)
**	    Added fi ID constants for `iistructure', `iiuserlen', `date_gmt',
**	    `iichar12', and `charextract'.
**	09-jun-87 (thurston)
**	    Added back in the eight fi IDs for:
**			f * money		f / money
**			i * money		i / money
**			money * f		money / f
**			money * i		money / i
**	    because we were causing an `f' to be coerced into a `money' before
**	    the multiplication happened, thereby loosing a lot of precision.
**	    Furthermore, I had to add four new fi IDs for:
**			f * i			f / i
**			i * f			i / f
**	    in order to avoid the ambiguity problems that caused us to remove
**	    the above eight fi IDs in the first place.
**	10-jun-87 (thurston)
**	    Added two new fi IDs for:
**			text  :  ii_notrm_txt(char)
**			varchar  :  ii_notrm_vch(char)
**	25-jun-87 (thurston)
**	    Added fi IDs for the COPY COERCIONs.
**	03-aug-87 (thurston)
**	    Added fi IDs for the following copy coercions ... need to take the
**	    right most chars, not the left most, to be consistent with 5.0.
**				money -copy-> c
**				money -copy-> char
**				money -copy-> text
**				money -copy-> varchar
**	01-sep-87 (thurston)
**	    Added fi IDs for `iihexint(text)' and `iihexint(varchar)':
**	    ADFI_641_IIHEXINT_TEXT and ADFI_642_IIHEXINT_VARCHAR.
**	02-sep-87 (thurston)
**	    Added fi IDs for `money(money)' and `date(date)':
**	    ADFI_643_MONEY_MONEY and ADFI_644_DATE_DATE.
**	30-sep-87 (thurston)
**	    Added fi IDs for `_bintim()', ii_tabid_di(i,i), and
**	    `ii_di_tabid(char)':  ADFI_645__BINTIM, ADFI_646_II_TABID_DI_I_I,
**	    and ADFI_647_II_DI_TABID_CHAR.
**	15-jan-88 (thurston)
**	    Removed *LOTS* of function instances to conform with the accepted
**	    string proposal that does away with things like string comparison
**	    function instances where the two datatypes are different.
**	15-jan-88 (thurston)
**	    Added the following three function instances to be semetric:
**		ADFI_648_SQUEEZE_C       squeeze(c) ==> text
**		ADFI_649_PAD_C           pad(c)     ==> text
**		ADFI_650_PAD_CHAR        pad(char)  ==> varchar
**	20-jan-88 (thurston)
**	    ********************************************************************
**	    **                                                                **
**	    **   Removed all of the `REMOVED' function instances, and re-     **
**	    **   numbered all of the remaining ones.                          **
**	    **                                                                **
**	    ********************************************************************
**	25-feb-88 (thurston)
**	    Added function instances for concat() and char string `+' to cure
**	    both 5.0 compat issues with trimming of trailing blanks on the first
**	    arg when that arg is text or varchar.
**	26-feb-88 (thurston)
**	    Added function instances for the new `ii_dv_desc()' function.
**	04-nov-88 (thurston)
**	    Added function instance for the new `avg(date)' function.
**	18-apr-89 (mikem)
**	    Logical key development.  Added function id's 466-503 for logical
**	    key development.
**	22-may-89 (fred)
**	    Added function instances for external type and length.
**	22-jun-89 (mikem)
**	    Added function instances for longtext/logical key conversion.
**	11-jul-89 (jrb)
**	    Added 66 function instances for decimal project.
**	09-mar-92 (jrb, merged by stevet)
**	    Added 6 new function instances: 5 to fix bugs and 1 to support
**	    outer joins.
**	09-mar-92 (jrb, merged by stevet)
**          Added 4 fi's for resolve_table function.
**      04-aug-92 (stevet)
**          Added 8 fi's.  TABKEY/LOGKEY <-> VARCHAR/TEXT.  
**          Added new fi for gmt_timestamp().
**	12-aug-1992 (fred)
**	    Added new Fi's for long varchar.
**	29-sep-1992 (fred)
**	    Added new Fi's for bit string types.
**      29-oct-1992 (stevet)
**          Added new fi's for isnull and isnotnull for DB_ALL_TYPE.  Also
**          added fi for ii_row_count.  Also removed the isnull and 
**          isnotnull entries for new 65 datatypes.
**      03-dec-1992 (stevet)
**          Added fi's for one and two arguments versions of CHAR(), VARCHAR(),
**          TEXT() and C() functions using DB_ALL_TYPE.  Removed FI for
**          these functions for 6.5 datatypes.  Also added FI for sesssion_user
**          system_user and initial_user .
**      09-dec-1992 (rmuth, merged by stevet)
**          Add new fi's for iitotal_allocated_pages and iitotal_overflow_pages
**      18-Dec-1992 (fred)
**          Added left, right, upper, lower, length, and size for
**          DB_LVCH_TYPE.
**      28-jan-1993 (stevet)
**          Added alltype fi for ii_dv_desc() to support FIPS constraint.
**       9-Apr-1993 (fred)
**          Added byte fiids.  Also, added ii_lolk, which returns the
**          key logical key for large objects -- to be used primarily
**          by verifydb.
**      14-sep-1993 (stevet)
**          Added money*dec and money/dec function instances.
**      07-apr-1994 (stevet)
**          Added FI for vbyte -> longtext.
**	14-dec-1994 (shero03)
**	    Removed iixyzzy()
**	11-jan-1995 (shero03)
**	    Added iixyzzy()
**	5-jul-1996 (angusm)
**		Add ii_permit_type()
**	19-feb-1997 (shero03)
**	    Added soundex()
**	10-april-1997 (angusm)
**		reorder hex() function id (bug 81454)
**	30-jul-97 (stephenb)
**	    Add fi id's for string <-> decimal coercion
**	10-oct-97 (inkdo01)
**	    Add fi id's for byte/varbyte on remaining data types.
**	06-oct-1998 (shero03)
**	    Add bitwise functions: adu_bit_add, _and, _not, _or, _xor
**	    ii_ipaddr.  Removd xyzzy
**	23-oct-98 (kitch01)
**		Bug 92735. Add fi id for _date4()
**	12-Jan-1999 (shero03)
**	    Add hash function.
**	19-Jan-1999 (shero03)
**	    Add random functions.
**	08-Jun-1999 (shero03)
**	    Add unhex function.
**	21-sep-99 (inkdo01)
**	    Added OLAP aggregate functions.
**	16-jan-01 (inkdo01)
**	    Change ANY_C, COUNT_C to ANY_C_ALL, COUNT_C_ALL to reflect
**	    DB_ALL_TYPE for those FI's.
**      01-mar-2001 (gupsh01)
**          Added function instances for new data types nchar and 
**          nvarchar for unicode support.
**	15-mar-2001 (gupsh01)
**	    Added new function instances for coercion between
**	    nchar to nchar type and nvarchar to nvarchar type. 
**	21-mar-2001 (gupsh01)
**	    Added more functions for supporting unicode data type.
**	28-mar-2001 (gupsh01)
**	    Added coercion function for nchar and cvarchar coercion
**	    to byte, varbyte, lbyte, and ltext type.
**	04-apr-2001 (gupsh01)
**	    Added support for long nvarchar data type.
**      16-apr-2001 (stial01)
**          Added support for iitableinfo function
**	20-apr-2001 (gupsh01)
**	    Added ADFI_1075_LNVARCHAR_TO_LNVARCHAR for 
**	    long_nvarchar->long_nvarchar coercion.
**	29-oct-2001 (gupsh01)
**	    Added function id ADFI_1076_NCHAR_ADD_NCHAR and 
**	    ADFI_1077_NVARCHAR_ADD_NVARCHAR.
**	8-jan-02 (inkdo01)
**	    Added 42 FI codes for ANSI char_length, octet_length, bit_length,
**	    position and trim functions.
**      18-mar-2002 (gupsh01)
**          Added function id ADFI_1078_IFNULL_NCHAR_NCHAR and
**          ADFI_1079_IFNULL_NVARCHAR_NVARCHAR
**      27-mar-2002 (gupsh01)
**          Added coercion functions for coercing from char and
**          varchar, to nchar and nvarchar data types.
**	04-apr-2003 (gupsh01)
**	    Renamed the constants ADFI_998_NCHAR_LIKE_NCHAR to 
**	    ADFI_998_NCHAR_LIKE_NVCHAR, and ADFI_999_NCHAR_NOTLIKE_NCHAR 
**	    to ADFI_999_NCHAR_NOTLIKE_NVCHAR. This will remove ambiguity 
**	    between the parameters in adgfitab.roc and the function 
**	    instance ID. 
**      14-apr-2003 (gupsh01)
**          Removed function instances ADFI_1002_CONCAT_NCHAR_C,
**          ADFI_1003_CONCAT_NCHAR_CHAR, ADFI_1004_CONCAT_NCHAR_VARCHAR,
**          ADFI_1006_CONCAT_NVCHAR_C, ADFI_1007_CONCAT_NVCHAR_TEXT,
**          ADFI_1008_CONCAT_NVCHAR_CHAR, ADFI_1009_CONCAT_NVCHAR_VARCHAR.
**          These function instances should be handled using coercion between
**          unicode and non unicode data types. Also added function instance
**          ADFI_1006_CONCAT_NCHAR_NVCHAR for concat between nchar and nvarchar
**          datatypes.
**	22-apr-2003 (gupsh01)
**	    Rework the previous change.
**	19-may-2003 (gupsh01)
**	    Added new function instances for long_byte() and long_varchar()
**	    functions.
**	25-nov-2003 (gupsh01)
**	    Added new functions instances for nchar(), nvarchar(), 
**	    iiutf8_to_utf16 and iiutf16_to_utf8, and copy coercions between 
**	    nchar,nvarchar and utf8 functions.
**	12-apr-04 (inkdo01)
**	    Added FIs for int8() coercion for BIGINT support.
**	20-apr-04 (drivi01)
**		Added FIs for unicode cercion for c, text, date, money, 
**	07-may-2004 (gupsh01)
**	    Added FIs for float4(), float8(), int1(), int2(), int4() and
**	    int8() and decimal() support for nchar and nvarchar datypes.
**	19-may-2004 (gupsh01)
**	    Added support for nchar and nvarchar to functions char(),varchar()
**	    ascii(), text(), date(), money().
**	14-jul-2004 (gupsh01)
**	    Added coercion functions between char/varchar and UTF8 datatypes.
**	    This function is intended only for internal use for facilitating
**	    ascii copy of unicode data to UTF8. 
**	29-july-04 (inkdo01)
**	    Minor change to security_label removal to keep fitab/filkup 
**	    consistent.
**	3-dec-04 (inkdo01)
**	    Added 6 entries for collation_weight().
**	11-feb-2005 (gupsh01)
**	    Added coercion between char/varchar -> long varchar.
**	28-feb-05 (inkdo01)
**	    Added length(NCLOB) and size(NCLOB).
**	13-apr-06 (dougi)
**	    Added isdst(date).
**	7-june-06 (dougi)
**	    Added FI ids for coercions between int & float and char, varchar,
**	    nchar and nvarchar.
**	04-aug-06 (stial01)
**	    Added entries for substring, position for blobs
**	30-aug-06 (gupsh01)
**	    Added entries for ANSI datetime functions and constant functions
**	05-sep-06 (gupsh01)
**	    Added entries for interval_ytom and interval_dtos functions.
**	13-sep-2006 (dougi)
**	    Removed entries for string/numeric coercions in favour of
**	    solution in pstrslv.c.
**      16-oct-2006 (stial01)
**          Added locator functions.
**	08-dec-2006 (stial01)
**	    Added/updated POSition funcions
**	19-dec-2006 (dougi)
**	    Added LTXT to LTXT coercion & "=" compare to allow UNION of NULLs.
**      19-dec-2006 (stial01)
**          Added ADFI_1273_NVCHAR_ALL_I for nvarchar(all,i)
**	08-jan-2007 (gupsh01)
**	    Added function instances for Adding ANSI date/time tyeps.
**	22-jan-2007 (gupsh01)
**	    Added support for coercions ADFI_1278_NVCHAR_TO_LONGVCHAR and 
**	    ADFI_1279_VCHAR_TO_LONGNVCHAR.
**	12-feb-2007 (stial01)
**	    Undo some of 25-jan-2007 fi cleanup, requires upgradedb -trees
**	8-mar-2007 (dougi)
**	    Add FI for round().
**	09-mar-2007 (gupsh01)
**	    Added support for upper(long nvarchar) and lower(long nvarchar).
**	13-apr-2007 (dougi)
**	    Add FIs for ceil(), floor(), trunc(), chr(), ltrim() and rtrim().
**	16-apr-2007 (dougi)
**	    Then add lpad(), rpad() and replace().
**	2-may-2007 (dougi)
**	    And acos(), asin(), tan(), pi() and sign().
**	10-Aug-2007 (kiria01) b118254
**	    Add extra entries for charextract to support multibyte returns.
**	20-Aug-2007 (toumi01 for kiria01) b118981
**	    Added entries for c + char, char + c, c + varchar to maintain
**	    historical behavior of these operations.
**	21-Aug-2007 (gupsh01)
**	    Added new entries for CHAR_LIKE_VARCHAR and C_LIKE_VARCHAR.
**	    Mark entries with CHAR_LIKE_CHAR and C_LIKE_C with ADI_F4096_LEGACY
**	    flag so that they are not used. (bug 119215)
**      22-Aug-2007 (kiria01) b118999
**          Added byteextract utilising the legacy charextract slots as the
**          functionality is (intentionally) identical.
**	27-Aug-2007 (gupsh01)
**	    Remove entries for CHAR_LIKE_VARCHAR and C_LIKE_VARCHAR as they
**	    cause ambiguity with other types.
**	17-Oct-2007 (kiria01) b117790
**	    Relegate money from implicit coercions.
**	    Add missing N(V)CHR to DECIMAL and missing FLOAT/INT <-> string coercions
**	    to complete the bias of implicit string-number coercions to DECIMAL.
**	03-Nov-2007 (kiria01) b119410
**	    Added Numeric sense comparisons ADFI_?_NUMERIC_EQ etc.
**	27-Dec-2007 (kiria01) b119374
**	    Added comment to ADFI_845_STDDEV_SAMP_FLT definition
**	26-Dec-2007 (kiria01) SIR119658
**	    Added 'is integer/decimal/float' operators.
**	28-Dec-2007 (lunbr01) b119125
**	    Added fi IDs for varbyte returning ii_ipaddr().
**	01-Feb-2008 (kiria01) b119859
**	    Add missing NCHAR(all,i) function as ADFI_1363_NCHAR_ALL_I
**	07-Mar-2008 (kiria01) b120043
**	    Added missing date operators
**	11-Apr-2008 (gupsh01)
**	    Added coercion between varbyte and logical/table keys.
**	12-Apr-2008 (kiria0) b119410
**	    Added ADFI__NUMERIC_NORM for comparing numeric string data.
**	02-Jun-2008 (kiria01) SIR120473
**	    Added LONG datatype varieties of LIKE
**      08-Aug-2008 (horda03) Bug 120634
**          Add comment that the codes 1380 to 1387 have been used. These
**          codes are used in the R1 codeline, and will not be crossed to
**          any other.
**      10-sep-2008 (gupsh01,stial01)
**          Added FIs for UNORM
**      22-sep-2008 (stial01)
**          Added FI's for tab_key/log_key to byte 
**	27-Oct-2008 (kiria01) SIR120473
**	    Added patcomp constants
**	04-Dec-2008 (kiria01) b121297
**	    Added ADFI_1395_DATE_SUB_VCH for relaxed dates
**	12-Dec-2008 (kiria01) b121297
**	    Added ADFI_1396_DATE_ADD_VCH for relaxed dates
**	12-Mar-2009 (kiria01) SIR121788
**	    Added ADFI_1397_LNVCHR_TO_LVCH, ADFI_1398_LVCH_TO_LNVCHR,
**	    ADFI_1399_LVCH_LNVCHR, ADFI_1400_LNVCHR_LVCH,
**	    ADFI_1401_LNVCHR_LBYTE, ADFI_1402_LNVCHR_LNVCHR,
**	    ADFI_1403_LNVCHR_CHAR, ADFI_1404_LNVCHR_C,
**	    ADFI_1405_LNVCHR_TEXT, ADFI_1406_LNVCHR_VARCHAR and
**	    ADFI_1407_LNVCHR_LTXT
**	17-Mar-2009 (kiria01) SIR121788
**	    Added ADFI_1408_LCLOC_TO_LBYTE, ADFI_1409_LCLOC_TO_LNVCHR,
**	    ADFI_1410_LBLOC_TO_LVCH and ADFI_1411_LNLOC_TO_LVCH
**	17-Mar-2009 (toumi01) b121809
**	    Allow ALL datatype for unhex (there should be no need for
**	    coercion e.g. of byte through char). This brings unhex in line
**	    with hex (though the latter was mis-labeled in this regard,
**	    which is also fixed).
**	19-Mar-2009 (kiria01) SIR121788
**	    Added the missing long byte coercions.
**      22-Apr-2009 (Marty Bowes) SIR 121969
**          Add generate_digit() and validate_digit() for the generation
**          and validation of checksum values.
**	31-Aug-2009 (kschendel) b122510
**	    Add FI's for character(ucode,n) type conversions.  We can't use
**	    the (all,n) FI's for these because they call the wrong
**	    converter, and they have the skip-unicode flag on anyway.
**      03-Sep-2009 (coomi01) b122473
**          Add ADFI_1417_BYTE_ALL_I
**	4-sep-2009 (stephenb)
**		Add ADFI_1441_LAST_IDENTITY
**      25-sep-2009 (joea)
**          Add BOO_TO_BOO, BOO_ISFALSE, BOO_NOFALSE, BOO_ISTRUE, BOO_NOTRUE
**          LTXT_TO_BOO, VCH_TO_BOO, BOO_EQ_BOO, BOO_NE_BOO, BOO_LE_BOO,
**          BOO_GT_BOO, BOO_GE_BOO, BOO_LT_BOO, BOO_ISUNKN, BOO_NOUNKN.
**      27-oct-2009 (joea)
**          Add BOO_CHAR, BOO_VCH, BOO_C, BOO_TEXT, CHAR_TO_BOO, C_TO_BOO,
**          TEXT_TO_BOO, I_TO_BOO and BOO_I.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Add iicmptversion(int):varchar.
**	14-Nov-2009 (kiria01) SIR 122894
**	    Added GREATEST,LEAST,GREATER and LESSER generic polyadic functions.
**       01-Dec-2009 (coomi01) b122980
**          Add ADFI_1456_FLTROUND entry.
**    	07-Dec-2009 (drewsturgiss)
**	    Added NVL2, with functions for c_c_c, char_char_char, date_date_date, 
**		f_f_f, dec_dec_dec, i_i_i, money_money_money, text_text_text, 
**		varchar_varchar_varchar, nchar_nchar_nchar, nvarchar_nvarchar_nvarchar
**     14-dec-2009 (drewsturgiss)
**			Replaced multiple NVL2 fi's with a single ALL datatype
**     05-mar-2010 (joea)
**         Add BOO_BOO.
**	13-Mar-2010 (kiria01) b123422
**	    Added ADFI_1461_REPL_NVCH_NVCH_NVCH & ADFI_1462_REPL_VBYT_VBYT_VBYT
**	    and corrected the symbols for some of the longtext functions whoes
**	    names didn't match their actual actions.
**	18-Mar-2010 (kiria01) b123438
**	    Added SINGLETON aggregate for scalar sub-query support.
**      23-Mar-2010 (hanal04) Bug 122436
**          Change ADFI_164_LOCATE_STR_STR to ADFI_164_LOCATE_ALL_ALL
**          and cloaked ADFI_165_LOCATE_UNISTR_UNISTR. C to VARCHAR
**          coercion was stripping trailing from p2 which is not the
**          documented behaviour. 
**	25-Mar-2010 (toumi01) SIR 122403
**	    Add ADFI_1464_AES_DECRYPT_VARBYTE and ADFI_1465_AES_ENCRYPT_VARBYTE.
**/


/*
**      Following are #defines for all function instance IDs.  These are used
**      by the expression evaluator to make it much more readable.
*/

#define ADFI_000_C_EQ_C               (ADI_FI_ID)  0 /* c = c */
#define ADFI_001_TEXT_EQ_TEXT         (ADI_FI_ID)  1 /* text = text */
#define ADFI_002_DATE_EQ_DATE         (ADI_FI_ID)  2 /* date = date */
#define ADFI_003_F_EQ_F               (ADI_FI_ID)  3 /* f = f */
#define ADFI_004_F_EQ_I               (ADI_FI_ID)  4 /* f = i */
#define ADFI_005_I_EQ_F               (ADI_FI_ID)  5 /* i = f */
#define ADFI_006_I_EQ_I               (ADI_FI_ID)  6 /* i = i */
#define ADFI_007_MONEY_EQ_MONEY       (ADI_FI_ID)  7 /* money = money */
#define ADFI_008_C_NE_C               (ADI_FI_ID)  8 /* c != c */
#define ADFI_009_TEXT_NE_TEXT         (ADI_FI_ID)  9 /* text != text */
#define ADFI_010_DATE_NE_DATE         (ADI_FI_ID) 10 /* date != date */
#define ADFI_011_F_NE_F               (ADI_FI_ID) 11 /* f != f */
#define ADFI_012_F_NE_I               (ADI_FI_ID) 12 /* f != i */
#define ADFI_013_I_NE_F               (ADI_FI_ID) 13 /* i != f */
#define ADFI_014_I_NE_I               (ADI_FI_ID) 14 /* i != i */
#define ADFI_015_MONEY_NE_MONEY       (ADI_FI_ID) 15 /* money != money */
#define ADFI_016_C_LE_C               (ADI_FI_ID) 16 /* c <= c */
#define ADFI_017_TEXT_LE_TEXT         (ADI_FI_ID) 17 /* text <= text */
#define ADFI_018_DATE_LE_DATE         (ADI_FI_ID) 18 /* date <= date */
#define ADFI_019_F_LE_F               (ADI_FI_ID) 19 /* f <= f */
#define ADFI_020_F_LE_I               (ADI_FI_ID) 20 /* f <= i */
#define ADFI_021_I_LE_F               (ADI_FI_ID) 21 /* i <= f */
#define ADFI_022_I_LE_I               (ADI_FI_ID) 22 /* i <= i */
#define ADFI_023_MONEY_LE_MONEY       (ADI_FI_ID) 23 /* money <= money */
#define ADFI_024_C_GT_C               (ADI_FI_ID) 24 /* c > c */
#define ADFI_025_TEXT_GT_TEXT         (ADI_FI_ID) 25 /* text > text */
#define ADFI_026_DATE_GT_DATE         (ADI_FI_ID) 26 /* date > date */
#define ADFI_027_F_GT_F               (ADI_FI_ID) 27 /* f > f */
#define ADFI_028_F_GT_I               (ADI_FI_ID) 28 /* f > i */
#define ADFI_029_I_GT_F               (ADI_FI_ID) 29 /* i > f */
#define ADFI_030_I_GT_I               (ADI_FI_ID) 30 /* i > i */
#define ADFI_031_MONEY_GT_MONEY       (ADI_FI_ID) 31 /* money > money */
#define ADFI_032_C_GE_C               (ADI_FI_ID) 32 /* c >= c */
#define ADFI_033_TEXT_GE_TEXT         (ADI_FI_ID) 33 /* text >= text */
#define ADFI_034_DATE_GE_DATE         (ADI_FI_ID) 34 /* date >= date */
#define ADFI_035_F_GE_F               (ADI_FI_ID) 35 /* f >= f */
#define ADFI_036_F_GE_I               (ADI_FI_ID) 36 /* f >= i */
#define ADFI_037_I_GE_F               (ADI_FI_ID) 37 /* i >= f */
#define ADFI_038_I_GE_I               (ADI_FI_ID) 38 /* i >= i */
#define ADFI_039_MONEY_GE_MONEY       (ADI_FI_ID) 39 /* money >= money */
#define ADFI_040_C_LT_C               (ADI_FI_ID) 40 /* c < c */
#define ADFI_041_TEXT_LT_TEXT         (ADI_FI_ID) 41 /* text < text */
#define ADFI_042_DATE_LT_DATE         (ADI_FI_ID) 42 /* date < date */
#define ADFI_043_F_LT_F               (ADI_FI_ID) 43 /* f < f */
#define ADFI_044_F_LT_I               (ADI_FI_ID) 44 /* f < i */
#define ADFI_045_I_LT_F               (ADI_FI_ID) 45 /* i < f */
#define ADFI_046_I_LT_I               (ADI_FI_ID) 46 /* i < i */
#define ADFI_047_MONEY_LT_MONEY       (ADI_FI_ID) 47 /* money < money */
#define ADFI_048_DATE_ADD_DATE        (ADI_FI_ID) 48 /* date + date */
#define ADFI_049_F_ADD_F              (ADI_FI_ID) 49 /* f + f */
#define ADFI_050_I_ADD_I              (ADI_FI_ID) 50 /* i + i */
#define ADFI_051_MONEY_ADD_MONEY      (ADI_FI_ID) 51 /* money + money */
#define ADFI_052_C_ADD_C              (ADI_FI_ID) 52 /* c + c */
#define ADFI_053_TEXT_ADD_C           (ADI_FI_ID) 53 /* text + c */
#define ADFI_054_TEXT_ADD_TEXT        (ADI_FI_ID) 54 /* text + text */
#define ADFI_055_DATE_SUB_DATE        (ADI_FI_ID) 55 /* date - date */
#define ADFI_056_F_SUB_F              (ADI_FI_ID) 56 /* f - f */
#define ADFI_057_I_SUB_I              (ADI_FI_ID) 57 /* i - i */
#define ADFI_058_MONEY_SUB_MONEY      (ADI_FI_ID) 58 /* money - money */
#define ADFI_059_F_MUL_F              (ADI_FI_ID) 59 /* f * f */
#define ADFI_060_I_MUL_I              (ADI_FI_ID) 60 /* i * i */
#define ADFI_061_F_MUL_MONEY          (ADI_FI_ID) 61 /* f * money */
#define ADFI_062_I_MUL_MONEY          (ADI_FI_ID) 62 /* i * money */
#define ADFI_063_MONEY_MUL_F          (ADI_FI_ID) 63 /* money * f */
#define ADFI_064_MONEY_MUL_I          (ADI_FI_ID) 64 /* money * i */
#define ADFI_065_MONEY_MUL_MONEY      (ADI_FI_ID) 65 /* money * money */
#define ADFI_066_F_DIV_F              (ADI_FI_ID) 66 /* f / f */
#define ADFI_067_I_DIV_I              (ADI_FI_ID) 67 /* i / i */
#define ADFI_068_F_DIV_MONEY          (ADI_FI_ID) 68 /* f / money */
#define ADFI_069_I_DIV_MONEY          (ADI_FI_ID) 69 /* i / money */
#define ADFI_070_MONEY_DIV_F          (ADI_FI_ID) 70 /* money / f */
#define ADFI_071_MONEY_DIV_I          (ADI_FI_ID) 71 /* money / i */
#define ADFI_072_MONEY_DIV_MONEY      (ADI_FI_ID) 72 /* money / money */
#define ADFI_073_F_POW_F              (ADI_FI_ID) 73 /* f ** f */
#define ADFI_074_F_POW_I              (ADI_FI_ID) 74 /* f ** i */
#define ADFI_075_PLUS_F               (ADI_FI_ID) 75 /* + f */
#define ADFI_076_PLUS_I               (ADI_FI_ID) 76 /* + i */
#define ADFI_077_PLUS_MONEY           (ADI_FI_ID) 77 /* + money */
#define ADFI_078_MINUS_F              (ADI_FI_ID) 78 /* - f */
#define ADFI_079_MINUS_I              (ADI_FI_ID) 79 /* - i */
#define ADFI_080_MINUS_MONEY          (ADI_FI_ID) 80 /* - money */
#define ADFI_081_ANY_C_ALL            (ADI_FI_ID) 81 /* any(c & all) */
#define ADFI_082_ANY_DATE             (ADI_FI_ID) 82 /* any(date) */
#define ADFI_083_ANY_F                (ADI_FI_ID) 83 /* any(f) */
#define ADFI_084_ANY_I                (ADI_FI_ID) 84 /* any(i) */
#define ADFI_085_ANY_MONEY            (ADI_FI_ID) 85 /* any(money) */
#define ADFI_086_ANY_TEXT             (ADI_FI_ID) 86 /* any(text) */
#define ADFI_087_AVG_F                (ADI_FI_ID) 87 /* avg(f) */
#define ADFI_088_AVG_I                (ADI_FI_ID) 88 /* avg(i) */
#define ADFI_089_AVG_MONEY            (ADI_FI_ID) 89 /* avg(money) */
#define ADFI_090_COUNT_C_ALL          (ADI_FI_ID) 90 /* count(c & all) */
#define ADFI_091_COUNT_DATE           (ADI_FI_ID) 91 /* count(date) */
#define ADFI_092_COUNT_F              (ADI_FI_ID) 92 /* count(f) */
#define ADFI_093_COUNT_I              (ADI_FI_ID) 93 /* count(i) */
#define ADFI_094_COUNT_MONEY          (ADI_FI_ID) 94 /* count(money) */
#define ADFI_095_COUNT_TEXT           (ADI_FI_ID) 95 /* count(text) */
#define ADFI_096_MAX_C                (ADI_FI_ID) 96 /* max(c) */
#define ADFI_097_MAX_DATE             (ADI_FI_ID) 97 /* max(date) */
#define ADFI_098_MAX_F                (ADI_FI_ID) 98 /* max(f) */
#define ADFI_099_MAX_I                (ADI_FI_ID) 99 /* max(i) */
#define ADFI_100_MAX_MONEY            (ADI_FI_ID)100 /* max(money) */
#define ADFI_101_MAX_TEXT             (ADI_FI_ID)101 /* max(text) */
#define ADFI_102_MIN_C                (ADI_FI_ID)102 /* min(c) */
#define ADFI_103_MIN_DATE             (ADI_FI_ID)103 /* min(date) */
#define ADFI_104_MIN_F                (ADI_FI_ID)104 /* min(f) */
#define ADFI_105_MIN_I                (ADI_FI_ID)105 /* min(i) */
#define ADFI_106_MIN_MONEY            (ADI_FI_ID)106 /* min(money) */
#define ADFI_107_MIN_TEXT             (ADI_FI_ID)107 /* min(text) */
#define ADFI_108_SUM_F                (ADI_FI_ID)108 /* sum(f) */
#define ADFI_109_SUM_I                (ADI_FI_ID)109 /* sum(i) */
#define ADFI_110_SUM_MONEY            (ADI_FI_ID)110 /* sum(money) */
#define ADFI_111_ABS_F                (ADI_FI_ID)111 /* abs(f) */
#define ADFI_112_ABS_I                (ADI_FI_ID)112 /* abs(i) */
#define ADFI_113_ABS_MONEY            (ADI_FI_ID)113 /* abs(money) */
#define ADFI_114_ASCII_NCHR_I         (ADI_FI_ID)114 /* ascii(nchar,i) */
#define ADFI_115_CHAR_NCHR_I          (ADI_FI_ID)115 /* char(nchar,i) */
#define ADFI_116_TEXT_NCHR_I          (ADI_FI_ID)116 /* text(nchar,i) */
#define ADFI_117_VARCHAR_NCHR_I       (ADI_FI_ID)117 /* varchar(nchar,i) */
#define ADFI_118_BYTE_NCHR_I          (ADI_FI_ID)118 /* byte(nchar,i) */
#define ADFI_119_VBYTE_NCHR_I         (ADI_FI_ID)119 /* varbyte(nchar,i) */
#define ADFI_120_ATAN_F               (ADI_FI_ID)120 /* atan(f) */
#define ADFI_121_CONCAT_C_C           (ADI_FI_ID)121 /* concat(c,c) */
#define ADFI_122_CONCAT_TEXT_C        (ADI_FI_ID)122 /* concat(text,c) */
#define ADFI_123_CONCAT_TEXT_TEXT     (ADI_FI_ID)123 /* concat(text,text) */
#define ADFI_124_COS_F                (ADI_FI_ID)124 /* cos(f) */
#define ADFI_125_DATE_C               (ADI_FI_ID)125 /* date(c) */
#define ADFI_126_DATE_TEXT            (ADI_FI_ID)126 /* date(text) */
#define ADFI_127_DOW_DATE             (ADI_FI_ID)127 /* dow(date) */
#define ADFI_128_DATE_PART_C_DATE     (ADI_FI_ID)128 /* date_part(c,date) */
#define ADFI_129_DATE_PART_TEXT_DATE  (ADI_FI_ID)129 /* date_part(text,date) */
#define ADFI_130_INTERVAL_C_DATE      (ADI_FI_ID)130 /* interval(c,date) */
#define ADFI_131_INTERVAL_TEXT_DATE   (ADI_FI_ID)131 /* interval(text,date) */
#define ADFI_132_DATE_TRUNC_C_DATE    (ADI_FI_ID)132 /* date_trunc(c,date) */
#define ADFI_133_DATE_TRUNC_TEXT_DATE (ADI_FI_ID)133 /* date_trunc(text,date) */
#define ADFI_134_EXP_F                (ADI_FI_ID)134 /* exp(f) */
#define ADFI_135_FLOAT4_C             (ADI_FI_ID)135 /* float4(c) */
#define ADFI_136_FLOAT4_F             (ADI_FI_ID)136 /* float4(f) */
#define ADFI_137_FLOAT4_I             (ADI_FI_ID)137 /* float4(i) */
#define ADFI_138_FLOAT4_TEXT          (ADI_FI_ID)138 /* float4(text) */
#define ADFI_139_FLOAT4_MONEY         (ADI_FI_ID)139 /* float4(money) */
#define ADFI_140_FLOAT8_C             (ADI_FI_ID)140 /* float8(c) */
#define ADFI_141_FLOAT8_F             (ADI_FI_ID)141 /* float8(f) */
#define ADFI_142_FLOAT8_I             (ADI_FI_ID)142 /* float8(i) */
#define ADFI_143_FLOAT8_TEXT          (ADI_FI_ID)143 /* float8(text) */
#define ADFI_144_FLOAT8_MONEY         (ADI_FI_ID)144 /* float8(money) */
#define ADFI_145_INT1_C               (ADI_FI_ID)145 /* int1(c) */
#define ADFI_146_INT1_F               (ADI_FI_ID)146 /* int1(f) */
#define ADFI_147_INT1_I               (ADI_FI_ID)147 /* int1(i) */
#define ADFI_148_INT1_TEXT            (ADI_FI_ID)148 /* int1(text) */
#define ADFI_149_INT1_MONEY           (ADI_FI_ID)149 /* int1(money) */
#define ADFI_150_INT2_C               (ADI_FI_ID)150 /* int2(c) */
#define ADFI_151_INT2_F               (ADI_FI_ID)151 /* int2(f) */
#define ADFI_152_INT2_I               (ADI_FI_ID)152 /* int2(i) */
#define ADFI_153_INT2_TEXT            (ADI_FI_ID)153 /* int2(text) */
#define ADFI_154_INT2_MONEY           (ADI_FI_ID)154 /* int2(money) */
#define ADFI_155_INT4_C               (ADI_FI_ID)155 /* int4(c) */
#define ADFI_156_INT4_F               (ADI_FI_ID)156 /* int4(f) */
#define ADFI_157_INT4_I               (ADI_FI_ID)157 /* int4(i) */
#define ADFI_158_INT4_TEXT            (ADI_FI_ID)158 /* int4(text) */
#define ADFI_159_INT4_MONEY           (ADI_FI_ID)159 /* int4(money) */
#define ADFI_160_LEFT_C_I             (ADI_FI_ID)160 /* left(c,i) */
#define ADFI_161_LEFT_TEXT_I          (ADI_FI_ID)161 /* left(text,i) */
#define ADFI_162_LENGTH_C             (ADI_FI_ID)162 /* length(c) */
#define ADFI_163_LENGTH_TEXT          (ADI_FI_ID)163 /* length(text) */
#define ADFI_164_LOCATE_ALL_ALL       (ADI_FI_ID)164 /* locate(all,all) */
#define ADFI_165_LOCATE_UNISTR_UNISTR (ADI_FI_ID)165 /* locate(unistr,unistr) */
#define ADFI_166_LOG_F                (ADI_FI_ID)166 /* log(f) */
#define ADFI_167_LOWERCASE_C          (ADI_FI_ID)167 /* lowercase(c) */
#define ADFI_168_LOWERCASE_TEXT       (ADI_FI_ID)168 /* lowercase(text) */
#define ADFI_169_MOD_I_I              (ADI_FI_ID)169 /* mod(i,i) */
#define ADFI_170_MONEY_F              (ADI_FI_ID)170 /* money(f) */
#define ADFI_171_MONEY_I              (ADI_FI_ID)171 /* money(i) */
#define ADFI_172_MONEY_C              (ADI_FI_ID)172 /* money(c) */
#define ADFI_173_MONEY_TEXT           (ADI_FI_ID)173 /* money(text) */
#define ADFI_174_PAD_TEXT             (ADI_FI_ID)174 /* pad(text) */
#define ADFI_175_RIGHT_C_I            (ADI_FI_ID)175 /* right(c,i) */
#define ADFI_176_RIGHT_TEXT_I         (ADI_FI_ID)176 /* right(text,i) */
#define ADFI_177_SHIFT_C_I            (ADI_FI_ID)177 /* shift(c,i) */
#define ADFI_178_SHIFT_TEXT_I         (ADI_FI_ID)178 /* shift(text,i) */
#define ADFI_179_SIN_F                (ADI_FI_ID)179 /* sin(f) */
#define ADFI_180_SIZE_C               (ADI_FI_ID)180 /* size(c) */
#define ADFI_181_SIZE_TEXT            (ADI_FI_ID)181 /* size(text) */
#define ADFI_182_SQRT_F               (ADI_FI_ID)182 /* sqrt(f) */
#define ADFI_183_SQUEEZE_TEXT         (ADI_FI_ID)183 /* squeeze(text) */
#define ADFI_184_ASCII_NVCHR_I        (ADI_FI_ID)184 /* ascii(nvarchar,i) */
#define ADFI_185_CHAR_NVCHR_I         (ADI_FI_ID)185 /* char(nvarchar,i) */
#define ADFI_186_TEXT_NVCHR_I         (ADI_FI_ID)186 /* text(nvarchar,i) */
#define ADFI_187_VARCHAR_NVCHR_I      (ADI_FI_ID)187 /* varchar(nvarchar,i) */
#define ADFI_188_BYTE_NVCHR_I         (ADI_FI_ID)188 /* byte(nvarchar,i) */
#define ADFI_189_VBYTE_NVCHR_I        (ADI_FI_ID)189 /* varbyte(nvarchar,i) */
#define ADFI_190_TRIM_C               (ADI_FI_ID)190 /* trim(c) */
#define ADFI_191_TRIM_TEXT            (ADI_FI_ID)191 /* trim(text) */
#define ADFI_192_UPPERCASE_C          (ADI_FI_ID)192 /* uppercase(c) */
#define ADFI_193_UPPERCASE_TEXT       (ADI_FI_ID)193 /* uppercase(text) */
#define ADFI_194__BINTIM_I            (ADI_FI_ID)194 /* _bintim(i) */
#define ADFI_195__DATE_I              (ADI_FI_ID)195 /* _date(i) */
#define ADFI_196__TIME_I              (ADI_FI_ID)196 /* _time(i) */
#define ADFI_197__CPU_MS              (ADI_FI_ID)197 /* _cpu_ms() */
#define ADFI_198__ET_SEC              (ADI_FI_ID)198 /* _et_sec() */
#define ADFI_199__DIO_CNT             (ADI_FI_ID)199 /* _dio_cnt() */
#define ADFI_200__BIO_CNT             (ADI_FI_ID)200 /* _bio_cnt() */
#define ADFI_201__PFAULT_CNT          (ADI_FI_ID)201 /* _pfault_cnt() */
#define ADFI_202__WS_PAGE             (ADI_FI_ID)202 /* _ws_page() */
#define ADFI_203__PHYS_PAGE           (ADI_FI_ID)203 /* _phys_page() */
#define ADFI_204__CACHE_REQ           (ADI_FI_ID)204 /* _cache_req() */
#define ADFI_205__CACHE_READ          (ADI_FI_ID)205 /* _cache_read() */
#define ADFI_206__CACHE_WRITE         (ADI_FI_ID)206 /* _cache_write() */
#define ADFI_207__CACHE_RREAD         (ADI_FI_ID)207 /* _cache_rread() */
#define ADFI_208__CACHE_SIZE          (ADI_FI_ID)208 /* _cache_size() */
#define ADFI_209__CACHE_USED          (ADI_FI_ID)209 /* _cache_used() */
#define ADFI_210__VERSION             (ADI_FI_ID)210 /* _version() */
#define ADFI_211_DBA                  (ADI_FI_ID)211 /* dba() */
#define ADFI_212_USERNAME             (ADI_FI_ID)212 /* username() */
#define ADFI_213_I_TO_F               (ADI_FI_ID)213 /* i -> f */
#define ADFI_214_F_TO_I               (ADI_FI_ID)214 /* f -> i */
#define ADFI_215_C_TO_DATE            (ADI_FI_ID)215 /* c -> date */
#define ADFI_216_TEXT_TO_DATE         (ADI_FI_ID)216 /* text -> date */
#define ADFI_217_DATE_TO_C            (ADI_FI_ID)217 /* date -> c */
#define ADFI_218_DATE_TO_TEXT         (ADI_FI_ID)218 /* date -> text */
#define ADFI_219_MONEY_TO_C           (ADI_FI_ID)219 /* money -> c */
#define ADFI_220_MONEY_TO_TEXT        (ADI_FI_ID)220 /* money -> text */
#define ADFI_221_C_TO_MONEY           (ADI_FI_ID)221 /* c -> money */
#define ADFI_222_TEXT_TO_MONEY        (ADI_FI_ID)222 /* text -> money */
#define ADFI_223_F_TO_MONEY           (ADI_FI_ID)223 /* f -> money */
#define ADFI_224_I_TO_MONEY           (ADI_FI_ID)224 /* i -> money */
#define ADFI_225_MONEY_TO_F           (ADI_FI_ID)225 /* money -> f */
#define ADFI_226_C_TO_TEXT            (ADI_FI_ID)226 /* c -> text */
#define ADFI_227_TEXT_TO_C            (ADI_FI_ID)227 /* text -> c */
#define ADFI_228_C_TO_C               (ADI_FI_ID)228 /* c -> c */
#define ADFI_229_TEXT_TO_TEXT         (ADI_FI_ID)229 /* text -> text */
#define ADFI_230_F_TO_F               (ADI_FI_ID)230 /* f -> f */
#define ADFI_231_I_TO_I               (ADI_FI_ID)231 /* i -> i */
#define ADFI_232_MONEY_TO_MONEY       (ADI_FI_ID)232 /* money -> money */
#define ADFI_233_DATE_TO_DATE         (ADI_FI_ID)233 /* date -> date */
#define ADFI_234_CHAR_EQ_CHAR         (ADI_FI_ID)234 /* char = char */
#define ADFI_235_VARCHAR_EQ_VARCHAR   (ADI_FI_ID)235 /* varchar = varchar */
#define ADFI_236_CHAR_NE_CHAR         (ADI_FI_ID)236 /* char != char */
#define ADFI_237_VARCHAR_NE_VARCHAR   (ADI_FI_ID)237 /* varchar != varchar */
#define ADFI_238_CHAR_LE_CHAR         (ADI_FI_ID)238 /* char <= char */
#define ADFI_239_VARCHAR_LE_VARCHAR   (ADI_FI_ID)239 /* varchar <= varchar */
#define ADFI_240_CHAR_GT_CHAR         (ADI_FI_ID)240 /* char > char */
#define ADFI_241_VARCHAR_GT_VARCHAR   (ADI_FI_ID)241 /* varchar > varchar */
#define ADFI_242_CHAR_GE_CHAR         (ADI_FI_ID)242 /* char >= char */
#define ADFI_243_VARCHAR_GE_VARCHAR   (ADI_FI_ID)243 /* varchar >= varchar */
#define ADFI_244_CHAR_LT_CHAR         (ADI_FI_ID)244 /* char < char */
#define ADFI_245_VARCHAR_LT_VARCHAR   (ADI_FI_ID)245 /* varchar < varchar */
#define ADFI_246_C_LIKE_C             (ADI_FI_ID)246 /* c like c */
#define ADFI_247_CHAR_LIKE_CHAR       (ADI_FI_ID)247 /* char like char */
#define ADFI_248_TEXT_LIKE_TEXT       (ADI_FI_ID)248 /* text like text */
#define ADFI_249_VARCHAR_LIKE_VARCHAR (ADI_FI_ID)249 /* varchar like varchar */
#define ADFI_250_C_NOTLIKE_C          (ADI_FI_ID)250 /* c notlike c */
#define ADFI_251_CHAR_NOTLIKE_CHAR    (ADI_FI_ID)251 /* char notlike char */
#define ADFI_252_TEXT_NOTLIKE_TEXT    (ADI_FI_ID)252 /* text notlike text */
#define ADFI_253_VARCHAR_NOTLIKE_VARCHA (ADI_FI_ID)253 /* varchar notlike varchar */
#define ADFI_254_CHAR_ADD_CHAR        (ADI_FI_ID)254 /* char + char */
#define ADFI_255_VARCHAR_ADD_C        (ADI_FI_ID)255 /* varchar + c */
#define ADFI_256_VARCHAR_ADD_CHAR     (ADI_FI_ID)256 /* varchar + char */
#define ADFI_257_VARCHAR_ADD_VARCHAR  (ADI_FI_ID)257 /* varchar + varchar */
#define ADFI_258_ANY_CHAR             (ADI_FI_ID)258 /* any(char) */
#define ADFI_259_ANY_VARCHAR          (ADI_FI_ID)259 /* any(varchar) */
#define ADFI_260_COUNT_CHAR           (ADI_FI_ID)260 /* count(char) */
#define ADFI_261_COUNT_VARCHAR        (ADI_FI_ID)261 /* count(varchar) */
#define ADFI_262_MAX_CHAR             (ADI_FI_ID)262 /* max(char) */
#define ADFI_263_MAX_VARCHAR          (ADI_FI_ID)263 /* max(varchar) */
#define ADFI_264_MIN_CHAR             (ADI_FI_ID)264 /* min(char) */
#define ADFI_265_MIN_VARCHAR          (ADI_FI_ID)265 /* min(varchar) */
#define ADFI_276_CONCAT_CHAR_CHAR     (ADI_FI_ID)276 /* concat(char,char) */
#define ADFI_277_CONCAT_VARCHAR_C     (ADI_FI_ID)277 /* concat(varchar,c) */
#define ADFI_278_CONCAT_VARCHAR_CHAR  (ADI_FI_ID)278 /* concat(varchar,char) */
#define ADFI_279_CONCAT_VARCHAR_VARCHAR (ADI_FI_ID)279 /* concat(varchar,varchar) */
#define ADFI_280_DATE_CHAR            (ADI_FI_ID)280 /* date(char) */
#define ADFI_281_DATE_VARCHAR         (ADI_FI_ID)281 /* date(varchar) */
#define ADFI_282_DATE_PART_CHAR_DATE  (ADI_FI_ID)282 /* date_part(char,date) */
#define ADFI_283_DATE_PART_VARCHAR_DATE (ADI_FI_ID)283 /* date_part(varchar,date) */
#define ADFI_284_INTERVAL_CHAR_DATE   (ADI_FI_ID)284 /* interval(char,date) */
#define ADFI_285_INTERVAL_VARCHAR_DATE (ADI_FI_ID)285 /* interval(varchar,date) */
#define ADFI_286_DATE_TRUNC_CHAR_DATE (ADI_FI_ID)286 /* date_trunc(char,date) */
#define ADFI_287_DATE_TRUNC_VARCHAR_DAT (ADI_FI_ID)287 /* date_trunc(varchar,date) */
#define ADFI_288_FLOAT4_CHAR          (ADI_FI_ID)288 /* float4(char) */
#define ADFI_289_FLOAT4_VARCHAR       (ADI_FI_ID)289 /* float4(varchar) */
#define ADFI_290_FLOAT8_CHAR          (ADI_FI_ID)290 /* float8(char) */
#define ADFI_291_FLOAT8_VARCHAR       (ADI_FI_ID)291 /* float8(varchar) */
#define ADFI_292_INT1_CHAR            (ADI_FI_ID)292 /* int1(char) */
#define ADFI_293_INT1_VARCHAR         (ADI_FI_ID)293 /* int1(varchar) */
#define ADFI_294_INT2_CHAR            (ADI_FI_ID)294 /* int2(char) */
#define ADFI_295_INT2_VARCHAR         (ADI_FI_ID)295 /* int2(varchar) */
#define ADFI_296_INT4_CHAR            (ADI_FI_ID)296 /* int4(char) */
#define ADFI_297_INT4_VARCHAR         (ADI_FI_ID)297 /* int4(varchar) */
#define ADFI_298_LEFT_CHAR_I          (ADI_FI_ID)298 /* left(char,i) */
#define ADFI_299_LEFT_VARCHAR_I       (ADI_FI_ID)299 /* left(varchar,i) */
#define ADFI_300_LENGTH_CHAR          (ADI_FI_ID)300 /* length(char) */
#define ADFI_301_LENGTH_VARCHAR       (ADI_FI_ID)301 /* length(varchar) */
#define ADFI_304_LOWERCASE_CHAR       (ADI_FI_ID)304 /* lowercase(char) */
#define ADFI_305_LOWERCASE_VARCHAR    (ADI_FI_ID)305 /* lowercase(varchar) */
#define ADFI_306_MONEY_CHAR           (ADI_FI_ID)306 /* money(char) */
#define ADFI_307_MONEY_VARCHAR        (ADI_FI_ID)307 /* money(varchar) */
#define ADFI_308_PAD_VARCHAR          (ADI_FI_ID)308 /* pad(varchar) */
#define ADFI_309_RIGHT_CHAR_I         (ADI_FI_ID)309 /* right(char,i) */
#define ADFI_310_RIGHT_VARCHAR_I      (ADI_FI_ID)310 /* right(varchar,i) */
#define ADFI_311_SHIFT_CHAR_I         (ADI_FI_ID)311 /* shift(char,i) */
#define ADFI_312_SHIFT_VARCHAR_I      (ADI_FI_ID)312 /* shift(varchar,i) */
#define ADFI_313_SIZE_CHAR            (ADI_FI_ID)313 /* size(char) */
#define ADFI_314_SIZE_VARCHAR         (ADI_FI_ID)314 /* size(varchar) */
#define ADFI_315_SQUEEZE_CHAR         (ADI_FI_ID)315 /* squeeze(char) */
#define ADFI_316_SQUEEZE_VARCHAR      (ADI_FI_ID)316 /* squeeze(varchar) */
#define ADFI_319_TRIM_CHAR            (ADI_FI_ID)319 /* trim(char) */
#define ADFI_320_TRIM_VARCHAR         (ADI_FI_ID)320 /* trim(varchar) */
#define ADFI_321_UPPERCASE_CHAR       (ADI_FI_ID)321 /* uppercase(char) */
#define ADFI_322_UPPERCASE_VARCHAR    (ADI_FI_ID)322 /* uppercase(varchar) */
#define ADFI_331_C_TO_CHAR            (ADI_FI_ID)331 /* c -> char */
#define ADFI_332_C_TO_LONGTEXT        (ADI_FI_ID)332 /* c -> longtext */
#define ADFI_333_C_TO_VARCHAR         (ADI_FI_ID)333 /* c -> varchar */
#define ADFI_334_CHAR_TO_C            (ADI_FI_ID)334 /* char -> c */
#define ADFI_335_CHAR_TO_CHAR         (ADI_FI_ID)335 /* char -> char */
#define ADFI_336_CHAR_TO_DATE         (ADI_FI_ID)336 /* char -> date */
#define ADFI_337_CHAR_TO_LONGTEXT     (ADI_FI_ID)337 /* char -> longtext */
#define ADFI_338_CHAR_TO_MONEY        (ADI_FI_ID)338 /* char -> money */
#define ADFI_339_CHAR_TO_TEXT         (ADI_FI_ID)339 /* char -> text */
#define ADFI_340_CHAR_TO_VARCHAR      (ADI_FI_ID)340 /* char -> varchar */
#define ADFI_341_DATE_TO_CHAR         (ADI_FI_ID)341 /* date -> char */
#define ADFI_342_DATE_TO_LONGTEXT     (ADI_FI_ID)342 /* date -> longtext */
#define ADFI_343_DATE_TO_VARCHAR      (ADI_FI_ID)343 /* date -> varchar */
#define ADFI_344_F_TO_LONGTEXT        (ADI_FI_ID)344 /* f -> longtext */
#define ADFI_345_I_TO_LONGTEXT        (ADI_FI_ID)345 /* i -> longtext */
#define ADFI_346_LONGTEXT_TO_C        (ADI_FI_ID)346 /* longtext -> c */
#define ADFI_347_LONGTEXT_TO_CHAR     (ADI_FI_ID)347 /* longtext -> char */
#define ADFI_348_LONGTEXT_TO_DATE     (ADI_FI_ID)348 /* longtext -> date */
#define ADFI_349_LONGTEXT_TO_F        (ADI_FI_ID)349 /* longtext -> f */
#define ADFI_350_LONGTEXT_TO_I        (ADI_FI_ID)350 /* longtext -> i */
#define ADFI_351_LONGTEXT_TO_LONGTEXT (ADI_FI_ID)351 /* longtext -> longtext */
#define ADFI_352_LONGTEXT_TO_MONEY    (ADI_FI_ID)352 /* longtext -> money */
#define ADFI_353_LONGTEXT_TO_TEXT     (ADI_FI_ID)353 /* longtext -> text */
#define ADFI_354_LONGTEXT_TO_VARCHAR  (ADI_FI_ID)354 /* longtext -> varchar */
#define ADFI_355_MONEY_TO_CHAR        (ADI_FI_ID)355 /* money -> char */
#define ADFI_356_MONEY_TO_LONGTEXT    (ADI_FI_ID)356 /* money -> longtext */
#define ADFI_357_MONEY_TO_VARCHAR     (ADI_FI_ID)357 /* money -> varchar */
#define ADFI_358_TEXT_TO_CHAR         (ADI_FI_ID)358 /* text -> char */
#define ADFI_359_TEXT_TO_LONGTEXT     (ADI_FI_ID)359 /* text -> longtext */
#define ADFI_360_TEXT_TO_VARCHAR      (ADI_FI_ID)360 /* text -> varchar */
#define ADFI_361_VARCHAR_TO_C         (ADI_FI_ID)361 /* varchar -> c */
#define ADFI_362_VARCHAR_TO_CHAR      (ADI_FI_ID)362 /* varchar -> char */
#define ADFI_363_VARCHAR_TO_DATE      (ADI_FI_ID)363 /* varchar -> date */
#define ADFI_364_VARCHAR_TO_LONGTEXT  (ADI_FI_ID)364 /* varchar -> longtext */
#define ADFI_365_VARCHAR_TO_MONEY     (ADI_FI_ID)365 /* varchar -> money */
#define ADFI_366_VARCHAR_TO_TEXT      (ADI_FI_ID)366 /* varchar -> text */
#define ADFI_367_VARCHAR_TO_VARCHAR   (ADI_FI_ID)367 /* varchar -> varchar */
#define ADFI_368_HEX_CHAR             (ADI_FI_ID)368 /* hex(char) */
#define ADFI_369_HEX_TEXT             (ADI_FI_ID)369 /* hex(text) */
#define ADFI_370_HEX_VARCHAR          (ADI_FI_ID)370 /* hex(varchar) */
#define ADFI_371_HEX_ALL              (ADI_FI_ID)371 /* hex(all) */
#define ADFI_372_IFNULL_C_C           (ADI_FI_ID)372 /* ifnull(c,c) */
#define ADFI_373_IFNULL_CHAR_CHAR     (ADI_FI_ID)373 /* ifnull(char,char) */
#define ADFI_374_IFNULL_DATE_DATE     (ADI_FI_ID)374 /* ifnull(date,date) */
#define ADFI_375_IFNULL_F_F           (ADI_FI_ID)375 /* ifnull(f,f) */
#define ADFI_376_IFNULL_I_I           (ADI_FI_ID)376 /* ifnull(i,i) */
#define ADFI_377_IFNULL_MONEY_MONEY   (ADI_FI_ID)377 /* ifnull(money,money) */
#define ADFI_378_IFNULL_TEXT_TEXT     (ADI_FI_ID)378 /* ifnull(text,text) */
#define ADFI_379_IFNULL_VARCHAR_VARCHAR (ADI_FI_ID)379 /* ifnull(varchar,varchar) */
#define ADFI_380_I_ISNULL             (ADI_FI_ID)380 /* i isnull */
#define ADFI_381_F_ISNULL             (ADI_FI_ID)381 /* f isnull */
#define ADFI_382_MONEY_ISNULL         (ADI_FI_ID)382 /* money isnull */
#define ADFI_383_DATE_ISNULL          (ADI_FI_ID)383 /* date isnull */
#define ADFI_384_C_ISNULL             (ADI_FI_ID)384 /* c isnull */
#define ADFI_385_CHAR_ISNULL          (ADI_FI_ID)385 /* char isnull */
#define ADFI_386_TEXT_ISNULL          (ADI_FI_ID)386 /* text isnull */
#define ADFI_387_VARCHAR_ISNULL       (ADI_FI_ID)387 /* varchar isnull */
#define ADFI_388_I_ISNOTNULL          (ADI_FI_ID)388 /* i isnotnull */
#define ADFI_389_F_ISNOTNULL          (ADI_FI_ID)389 /* f isnotnull */
#define ADFI_390_MONEY_ISNOTNULL      (ADI_FI_ID)390 /* money isnotnull */
#define ADFI_391_DATE_ISNOTNULL       (ADI_FI_ID)391 /* date isnotnull */
#define ADFI_392_C_ISNOTNULL          (ADI_FI_ID)392 /* c isnotnull */
#define ADFI_393_CHAR_ISNOTNULL       (ADI_FI_ID)393 /* char isnotnull */
#define ADFI_394_TEXT_ISNOTNULL       (ADI_FI_ID)394 /* text isnotnull */
#define ADFI_395_VARCHAR_ISNOTNULL    (ADI_FI_ID)395 /* varchar isnotnull */
#define ADFI_396_COUNTSTAR            (ADI_FI_ID)396 /* count(*) */
#define ADFI_397_DBMSINFO_VARCHAR     (ADI_FI_ID)397 /* dbmsinfo(varchar) */
#define ADFI_398_IITYPENAME_I         (ADI_FI_ID)398 /* iitypename(i) */
#define ADFI_399_IISTRUCTURE_I        (ADI_FI_ID)399 /* iistructure(i) */
#define ADFI_400_IIUSERLEN_I_I        (ADI_FI_ID)400 /* iiuserlen(i,i) */
#define ADFI_401_DATE_GMT_DATE        (ADI_FI_ID)401 /* date_gmt(date) */
#define ADFI_402_IICHAR12_C           (ADI_FI_ID)402 /* iichar12(c) */
#define ADFI_403_IICHAR12_TEXT        (ADI_FI_ID)403 /* iichar12(text) */
#define ADFI_404_IICHAR12_CHAR        (ADI_FI_ID)404 /* iichar12(char) */
#define ADFI_405_IICHAR12_VARCHAR     (ADI_FI_ID)405 /* iichar12(varchar) */
#define ADFI_406_BYTEEXTRACT_C_I      (ADI_FI_ID)406 /* byteextract(c,i) */
#define ADFI_407_BYTEEXTRACT_TEXT_I   (ADI_FI_ID)407 /* byteextract(text,i) */
#define ADFI_408_BYTEEXTRACT_CHAR_I   (ADI_FI_ID)408 /* byteextract(char,i) */
#define ADFI_409_BYTEEXTRACT_VARCHAR_I (ADI_FI_ID)409 /* byteextract(varchar,i) */
#define ADFI_410_F_MUL_I              (ADI_FI_ID)410 /* f * i */
#define ADFI_411_I_MUL_F              (ADI_FI_ID)411 /* i * f */
#define ADFI_412_F_DIV_I              (ADI_FI_ID)412 /* f / i */
#define ADFI_413_I_DIV_F              (ADI_FI_ID)413 /* i / f */
#define ADFI_414_II_NOTRM_TXT_CHAR    (ADI_FI_ID)414 /* ii_notrm_txt(char) */
#define ADFI_415_II_NOTRM_VCH_CHAR    (ADI_FI_ID)415 /* ii_notrm_vch(char) */
#define ADFI_416_I_COPYTO_CHAR        (ADI_FI_ID)416 /* i -copy-> char */
#define ADFI_417_F_COPYTO_CHAR        (ADI_FI_ID)417 /* f -copy-> char */
#define ADFI_418_I_COPYTO_VARCHAR     (ADI_FI_ID)418 /* i -copy-> varchar */
#define ADFI_419_F_COPYTO_VARCHAR     (ADI_FI_ID)419 /* f -copy-> varchar */
#define ADFI_420_I_COPYTO_C           (ADI_FI_ID)420 /* i -copy-> c */
#define ADFI_421_F_COPYTO_C           (ADI_FI_ID)421 /* f -copy-> c */
#define ADFI_422_I_COPYTO_TEXT        (ADI_FI_ID)422 /* i -copy-> text */
#define ADFI_423_F_COPYTO_TEXT        (ADI_FI_ID)423 /* f -copy-> text */
#define ADFI_424_MONEY_COPYTO_I       (ADI_FI_ID)424 /* money -copy-> i */
#define ADFI_425_CHAR_COPYTO_I        (ADI_FI_ID)425 /* char -copy-> i */
#define ADFI_426_VARCHAR_COPYTO_I     (ADI_FI_ID)426 /* varchar -copy-> i */
#define ADFI_427_C_COPYTO_I           (ADI_FI_ID)427 /* c -copy-> i */
#define ADFI_428_TEXT_COPYTO_I        (ADI_FI_ID)428 /* text -copy-> i */
#define ADFI_429_CHAR_COPYTO_F        (ADI_FI_ID)429 /* char -copy-> f */
#define ADFI_430_VARCHAR_COPYTO_F     (ADI_FI_ID)430 /* varchar -copy-> f */
#define ADFI_431_C_COPYTO_F           (ADI_FI_ID)431 /* c -copy-> f */
#define ADFI_432_TEXT_COPYTO_F        (ADI_FI_ID)432 /* text -copy-> f */
#define ADFI_433_XYZZY_VARCHAR        (ADI_FI_ID)433 /* xyzzy(varchar) */
#define ADFI_434_MONEY_COPYTO_C       (ADI_FI_ID)434 /* money -copy-> c */
#define ADFI_435_MONEY_COPYTO_CHAR    (ADI_FI_ID)435 /* money -copy-> char */
#define ADFI_436_MONEY_COPYTO_TEXT    (ADI_FI_ID)436 /* money -copy-> text */
#define ADFI_437_MONEY_COPYTO_VARCHAR (ADI_FI_ID)437 /* money -copy-> varchar */
#define ADFI_438_IIHEXINT_TEXT        (ADI_FI_ID)438 /* iihexint(text) */
#define ADFI_439_IIHEXINT_VARCHAR     (ADI_FI_ID)439 /* iihexint(varchar) */
#define ADFI_440_MONEY_MONEY          (ADI_FI_ID)440 /* money(money) */
#define ADFI_441_DATE_DATE            (ADI_FI_ID)441 /* date(date) */
#define ADFI_442__BINTIM              (ADI_FI_ID)442 /* _bintim() */
#define ADFI_443_II_TABID_DI_I_I      (ADI_FI_ID)443 /* ii_tabid_di(i,i) */
#define ADFI_444_II_DI_TABID_CHAR     (ADI_FI_ID)444 /* ii_di_tabid(char) */
#define ADFI_445_SQUEEZE_C            (ADI_FI_ID)445 /* squeeze(c) */
#define ADFI_446_PAD_C                (ADI_FI_ID)446 /* pad(c) */
#define ADFI_447_PAD_CHAR             (ADI_FI_ID)447 /* pad(char) */
#define ADFI_448_TEXT_ADD_CHAR        (ADI_FI_ID)448 /* text + char */
#define ADFI_449_TEXT_ADD_VARCHAR     (ADI_FI_ID)449 /* text + varchar */
#define ADFI_450_CONCAT_TEXT_CHAR     (ADI_FI_ID)450 /* concat(text,char) */
#define ADFI_451_CONCAT_TEXT_VARCHAR  (ADI_FI_ID)451 /* concat(text,varchar) */
#define ADFI_452_CHAR_ADD_VARCHAR     (ADI_FI_ID)452 /* char + varchar */
#define ADFI_453_VARCHAR_ADD_TEXT     (ADI_FI_ID)453 /* varchar + text */
#define ADFI_454_CONCAT_CHAR_VARCHAR  (ADI_FI_ID)454 /* concat(char,varchar) */
#define ADFI_455_CONCAT_VARCHAR_TEXT  (ADI_FI_ID)455 /* concat(varchar,text) */
#define ADFI_464_AVG_DATE             (ADI_FI_ID)464 /* avg(date) */
#define ADFI_465_SUM_DATE             (ADI_FI_ID)465 /* sum(date) */
#define ADFI_466_LOGKEY_NE_LOGKEY     (ADI_FI_ID)466 /* logkey != logkey */
#define ADFI_467_LOGKEY_LE_LOGKEY     (ADI_FI_ID)467 /* logkey <= logkey */
#define ADFI_468_LOGKEY_GT_LOGKEY     (ADI_FI_ID)468 /* logkey > logkey */
#define ADFI_469_LOGKEY_GE_LOGKEY     (ADI_FI_ID)469 /* logkey >= logkey */
#define ADFI_470_LOGKEY_LT_LOGKEY     (ADI_FI_ID)470 /* logkey < logkey */
#define ADFI_471_LOGKEY_ISNULL        (ADI_FI_ID)471 /* logkey isnull */
#define ADFI_472_LOGKEY_ISNOTNULL     (ADI_FI_ID)472 /* logkey isnotnull */
#define ADFI_474_LOGKEY_CHAR          (ADI_FI_ID)474 /* logical_key(char) */
#define ADFI_475_LOGKEY_LOGKEY        (ADI_FI_ID)475 /* logical_key(logkey) */
#define ADFI_476_CHAR_TO_LOGKEY       (ADI_FI_ID)476 /* char -> logkey */
#define ADFI_477_LOGKEY_TO_CHAR       (ADI_FI_ID)477 /* logkey -> char */
#define ADFI_478_LOGKEY_TO_LOGKEY     (ADI_FI_ID)478 /* logkey -> logkey */
#define ADFI_479_ANY_LOGKEY           (ADI_FI_ID)479 /* any(logkey) */
#define ADFI_480_COUNT_LOGKEY         (ADI_FI_ID)480 /* count(logkey) */
#define ADFI_481_MAX_LOGKEY           (ADI_FI_ID)481 /* max(logkey) */
#define ADFI_482_MIN_LOGKEY           (ADI_FI_ID)482 /* min(logkey) */
#define ADFI_484_TABKEY_EQ_TABKEY     (ADI_FI_ID)484 /* table_key = table_key */
#define ADFI_485_TABKEY_NE_TABKEY     (ADI_FI_ID)485 /* table_key != table_key*/
#define ADFI_486_TABKEY_LE_TABKEY     (ADI_FI_ID)486 /* table_key <= table_key*/
#define ADFI_487_TABKEY_GT_TABKEY     (ADI_FI_ID)487 /* table_key > table_key */
#define ADFI_488_TABKEY_GE_TABKEY     (ADI_FI_ID)488 /* table_key >= table_key*/
#define ADFI_489_TABKEY_LT_TABKEY     (ADI_FI_ID)489 /* table_key < table_key */
#define ADFI_490_TABKEY_ISNULL        (ADI_FI_ID)490 /* table_key isnull */
#define ADFI_491_TABKEY_ISNOTNULL     (ADI_FI_ID)491 /* table_key isnotnull */
#define ADFI_493_TABKEY_CHAR          (ADI_FI_ID)493 /* table_key(char) */
#define ADFI_494_TABKEY_TABKEY        (ADI_FI_ID)494 /* table_key(table_key) */
#define ADFI_495_CHAR_TO_TABKEY       (ADI_FI_ID)495 /* char -> table_key */
#define ADFI_496_TABKEY_TO_CHAR       (ADI_FI_ID)496 /* table_key -> char */
#define ADFI_497_TABKEY_TO_TABKEY     (ADI_FI_ID)497 /* table_key -> table_key*/
#define ADFI_498_ANY_TABKEY           (ADI_FI_ID)498 /* any(table_key) */
#define ADFI_499_COUNT_TABKEY         (ADI_FI_ID)499 /* count(table_key) */
#define ADFI_500_MAX_TABKEY           (ADI_FI_ID)500 /* max(table_key) */
#define ADFI_501_MIN_TABKEY           (ADI_FI_ID)501 /* min(table_key) */
#define ADFI_503_LOGKEY_EQ_LOGKEY     (ADI_FI_ID)503 /* logkey = logkey */
#define	ADFI_504_II_EXT_TYPE	      (ADI_FI_ID)504 /* ii_ext_type(type, len)*/
#define ADFI_505_II_EXT_LENGTH	      (ADI_FI_ID)505 /* ii_ext_length(ditto) */
#define ADFI_506_LOGKEY_TO_LONGTEXT   (ADI_FI_ID)506 /* logkey -> longtext */
#define ADFI_507_LONGTEXT_TO_LOGKEY   (ADI_FI_ID)507 /* longtext -> logkey */
#define ADFI_508_TABKEY_TO_LONGTEXT   (ADI_FI_ID)508 /* table_key -> longtext */
#define ADFI_509_LONGTEXT_TO_TABKEY   (ADI_FI_ID)509 /* longtext -> table_key */
#define ADFI_510_DEC_EQ_DEC           (ADI_FI_ID)510 /* dec = dec */
#define ADFI_511_DEC_NE_DEC           (ADI_FI_ID)511 /* dec != dec */
#define ADFI_512_DEC_LE_DEC           (ADI_FI_ID)512 /* dec <= dec */
#define ADFI_513_DEC_GT_DEC           (ADI_FI_ID)513 /* dec > dec */
#define ADFI_514_DEC_GE_DEC           (ADI_FI_ID)514 /* dec >= dec */
#define ADFI_515_DEC_LT_DEC           (ADI_FI_ID)515 /* dec < dec */
#define ADFI_516_ALL_ISNULL           (ADI_FI_ID)516 /* all isnull */
#define ADFI_517_ALL_ISNOTNULL        (ADI_FI_ID)517 /* all isnotnull */
#define ADFI_518_DEC_ADD_DEC          (ADI_FI_ID)518 /* dec + dec */
#define ADFI_519_DEC_SUB_DEC          (ADI_FI_ID)519 /* dec - dec */
#define ADFI_520_DEC_MUL_DEC          (ADI_FI_ID)520 /* dec * dec */
#define ADFI_521_F_MUL_DEC            (ADI_FI_ID)521 /* f * dec */
#define ADFI_522_DEC_MUL_F            (ADI_FI_ID)522 /* dec * f */
#define ADFI_523_I_MUL_DEC            (ADI_FI_ID)523 /* i * dec */
#define ADFI_524_DEC_MUL_I            (ADI_FI_ID)524 /* dec * i */
#define ADFI_525_DEC_DIV_DEC          (ADI_FI_ID)525 /* dec / dec */
#define ADFI_526_F_DIV_DEC            (ADI_FI_ID)526 /* f / dec */
#define ADFI_527_DEC_DIV_F            (ADI_FI_ID)527 /* dec / f */
#define ADFI_528_I_DIV_DEC            (ADI_FI_ID)528 /* i / dec */
#define ADFI_529_DEC_DIV_I            (ADI_FI_ID)529 /* dec / i */
#define ADFI_530_PLUS_DEC             (ADI_FI_ID)530 /* + dec */
#define ADFI_531_MINUS_DEC            (ADI_FI_ID)531 /* - dec */
#define ADFI_532_ANY_DEC              (ADI_FI_ID)532 /* any(dec) */
#define ADFI_533_AVG_DEC              (ADI_FI_ID)533 /* avg(dec) */
#define ADFI_534_COUNT_DEC            (ADI_FI_ID)534 /* count(dec) */
#define ADFI_535_MAX_DEC              (ADI_FI_ID)535 /* max(dec) */
#define ADFI_536_MIN_DEC              (ADI_FI_ID)536 /* min(dec) */
#define ADFI_537_SUM_DEC              (ADI_FI_ID)537 /* sum(dec) */
#define ADFI_538_ABS_DEC              (ADI_FI_ID)538 /* abs(dec) */
#define ADFI_539_ASCII_ALL	      (ADI_FI_ID)539 /* ascii(all) */
#define ADFI_540_CHAR_ALL             (ADI_FI_ID)540 /* char(all) */
#define ADFI_541_DEC_C_I              (ADI_FI_ID)541 /* dec(c,i) */
#define ADFI_542_DEC_CHAR_I           (ADI_FI_ID)542 /* dec(char,i) */
#define ADFI_543_DEC_DEC_I            (ADI_FI_ID)543 /* dec(dec,i) */
#define ADFI_544_DEC_F_I              (ADI_FI_ID)544 /* dec(f,i) */
#define ADFI_545_DEC_I_I              (ADI_FI_ID)545 /* dec(i,i) */
#define ADFI_546_DEC_MONEY_I          (ADI_FI_ID)546 /* dec(money,i) */
#define ADFI_547_DEC_TEXT_I           (ADI_FI_ID)547 /* dec(text,i) */
#define ADFI_548_DEC_VARCHAR_I        (ADI_FI_ID)548 /* dec(varchar,i) */
#define ADFI_549_FLOAT4_DEC           (ADI_FI_ID)549 /* float4(dec) */
#define ADFI_550_FLOAT8_DEC           (ADI_FI_ID)550 /* float8(dec) */
#define ADFI_551_IFNULL_DEC_DEC       (ADI_FI_ID)551 /* ifnull(dec,dec) */
#define ADFI_553_INT1_DEC             (ADI_FI_ID)553 /* int1(dec) */
#define ADFI_554_INT2_DEC             (ADI_FI_ID)554 /* int2(dec) */
#define ADFI_555_INT4_DEC             (ADI_FI_ID)555 /* int4(dec) */
#define ADFI_556_MONEY_DEC            (ADI_FI_ID)556 /* money(dec) */
#define ADFI_557_TEXT_ALL             (ADI_FI_ID)557 /* text(all) */
#define ADFI_558_VARCHAR_ALL          (ADI_FI_ID)558 /* varchar(all) */
#define ADFI_559_DEC_COPYTO_C         (ADI_FI_ID)559 /* dec -copy-> c */
#define ADFI_560_DEC_COPYTO_TEXT      (ADI_FI_ID)560 /* dec -copy-> text */
#define ADFI_561_DEC_COPYTO_CHAR      (ADI_FI_ID)561 /* dec -copy-> char */
#define ADFI_562_DEC_COPYTO_VARCHAR   (ADI_FI_ID)562 /* dec -copy-> varchar */
#define ADFI_563_C_COPYTO_DEC         (ADI_FI_ID)563 /* c -copy-> dec */
#define ADFI_564_TEXT_COPYTO_DEC      (ADI_FI_ID)564 /* text -copy-> dec */
#define ADFI_565_CHAR_COPYTO_DEC      (ADI_FI_ID)565 /* char -copy-> dec */
#define ADFI_566_VARCHAR_COPYTO_DEC   (ADI_FI_ID)566 /* varchar -copy-> dec */
#define ADFI_567_DEC_TO_MONEY         (ADI_FI_ID)567 /* dec -> money */
#define ADFI_568_DEC_TO_F             (ADI_FI_ID)568 /* dec -> f */
#define ADFI_569_DEC_TO_I             (ADI_FI_ID)569 /* dec -> i */
#define ADFI_570_DEC_TO_DEC           (ADI_FI_ID)570 /* dec -> dec */
#define ADFI_571_DEC_TO_LONGTEXT      (ADI_FI_ID)571 /* dec -> longtext */
#define ADFI_572_LONGTEXT_TO_DEC      (ADI_FI_ID)572 /* longtext -> dec */
#define ADFI_573_MONEY_TO_DEC         (ADI_FI_ID)573 /* money -> dec */
#define ADFI_574_F_TO_DEC             (ADI_FI_ID)574 /* f -> dec */
#define ADFI_575_I_TO_DEC             (ADI_FI_ID)575 /* i -> dec */
#define ADFI_576_SEC_OBSOLETE1	      (ADI_FI_ID)576 /* notused */
#define ADFI_577_SEC_OBSOLETE2	      (ADI_FI_ID)577 /* notused */
#define ADFI_586_ASCII_ALL_I          (ADI_FI_ID)586 /* ascii(all,i) */
#define ADFI_587_TEXT_ALL_I           (ADI_FI_ID)587 /* text(all,i) */
#define ADFI_595_ROWCNT_I             (ADI_FI_ID)595 /* ii_row_count(i) */
#define ADFI_596_SYSUSER              (ADI_FI_ID)596 /* system_user */
#define ADFI_600_C_LONGTEXT           (ADI_FI_ID)600 /* c(longtext) */
#define ADFI_604_II_IFTRUE_I_ALL      (ADI_FI_ID)604 /* ii_iftrue(i,all) */
#define ADFI_605_RESTAB_TEXT          (ADI_FI_ID)605 /* resolve_table(text,text) */
#define ADFI_606_RESTAB_VARCHAR       (ADI_FI_ID)606 /* resolve_table(varchar,varchar) */
#define ADFI_607_LOGKEY_TO_VARCHAR    (ADI_FI_ID)607 /* logkey -> varchar */
#define ADFI_608_VARCHAR_TO_LOGKEY    (ADI_FI_ID)608 /* varchar -> logkey */
#define ADFI_609_TABKEY_TO_VARCHAR    (ADI_FI_ID)609 /* table_key -> varchar */
#define ADFI_610_VARCHAR_TO_TABKEY    (ADI_FI_ID)610 /* varchar -> table_key */
#define ADFI_611_LOGKEY_TO_TEXT       (ADI_FI_ID)611 /* logkey -> text */
#define ADFI_612_TEXT_TO_LOGKEY       (ADI_FI_ID)612 /* text -> logkey */
#define ADFI_613_TABKEY_TO_TEXT       (ADI_FI_ID)613 /* table_key -> text */
#define ADFI_614_TEXT_TO_TABKEY       (ADI_FI_ID)614 /* text -> table_key */
#define ADFI_615_GMT_TIMESTAMP_I      (ADI_FI_ID)615 /* gmt_timestamp(i) */
#define ADFI_616_VARCHAR_ALL_I        (ADI_FI_ID)616 /* varchar(all, i) */
#define ADFI_617_CHAR_ALL_I           (ADI_FI_ID)617 /* char(all, i) */
#define	ADFI_618_SESSUSER             (ADI_FI_ID)618 /* session_user */
#define ADFI_619_CPNDMP_LVCH	      (ADI_FI_ID)619 /* ii_cpn_dump(lvch) */
#define ADFI_620_CHAR_TO_LVCH	      (ADI_FI_ID)620 /* char -> long varchar */
#define ADFI_621_C_TO_LVCH	      (ADI_FI_ID)621 /* c -> long varchar */
#define ADFI_622_TEXT_TO_LVCH	      (ADI_FI_ID)622 /* text -> long varchar */
#define ADFI_623_VARCHAR_TO_LVCH      (ADI_FI_ID)623 /* varchar -> long varchar */
#define ADFI_624_LVCH_TO_LVCH	      (ADI_FI_ID)624 /* lvch -> long varchar */
#define ADFI_625_LTXT_TO_LVCH	      (ADI_FI_ID)625 /* ltxt -> long varchar */
#define ADFI_626_LVCH_TO_LTXT	      (ADI_FI_ID)626 /* lvch -> longtext */
#define ADFI_627_BIT_NE_BIT           (ADI_FI_ID)627 /* bit != bit */
#define ADFI_628_BIT_NE_VBIT          (ADI_FI_ID)628 /* bit != vbit */
#define ADFI_629_VBIT_NE_VBIT         (ADI_FI_ID)629 /* vbit != vbit */
#define ADFI_630_VBIT_NE_BIT          (ADI_FI_ID)630 /* vbit != bit */
#define ADFI_631_BIT_LT_BIT           (ADI_FI_ID)631 /* bit < bit */
#define ADFI_632_BIT_LT_VBIT          (ADI_FI_ID)632 /* bit < vbit */
#define ADFI_633_VBIT_LT_VBIT         (ADI_FI_ID)633 /* vbit < vbit */
#define ADFI_634_VBIT_LT_BIT          (ADI_FI_ID)634 /* vbit < bit */
#define ADFI_635_BIT_LE_BIT           (ADI_FI_ID)635 /* bit <= bit */
#define ADFI_636_BIT_LE_VBIT          (ADI_FI_ID)636 /* bit <= vbit */
#define ADFI_637_VBIT_LE_VBIT         (ADI_FI_ID)637 /* vbit <= vbit */
#define ADFI_638_VBIT_LE_BIT          (ADI_FI_ID)638 /* vbit <= bit */
#define ADFI_639_BIT_EQ_BIT           (ADI_FI_ID)639 /* bit = bit */
#define ADFI_640_BIT_EQ_VBIT          (ADI_FI_ID)640 /* bit = vbit */
#define ADFI_641_VBIT_EQ_VBIT         (ADI_FI_ID)641 /* vbit = vbit */
#define ADFI_642_VBIT_EQ_BIT          (ADI_FI_ID)642 /* vbit = bit */
#define ADFI_643_BIT_GT_BIT           (ADI_FI_ID)643 /* bit > bit */
#define ADFI_644_BIT_GT_VBIT          (ADI_FI_ID)644 /* bit > vbit */
#define ADFI_645_VBIT_GT_VBIT         (ADI_FI_ID)645 /* vbit > vbit */
#define ADFI_646_VBIT_GT_BIT          (ADI_FI_ID)646 /* vbit > bit */
#define ADFI_647_BIT_GE_BIT           (ADI_FI_ID)647 /* bit >= bit */
#define ADFI_648_BIT_GE_VBIT          (ADI_FI_ID)648 /* bit >= vbit */
#define ADFI_649_VBIT_GE_VBIT         (ADI_FI_ID)649 /* vbit >= vbit */
#define ADFI_650_VBIT_GE_BIT          (ADI_FI_ID)650 /* vbit >= bit */
#define ADFI_651_INITUSER             (ADI_FI_ID)651 /* initial_user */
#define ADFI_652_IITOTAL_ALLOC_PG     (ADI_FI_ID)652 /* iitotal_allocated_pages */
#define ADFI_653_IITOTAL_OVERFL_PG    (ADI_FI_ID)653 /* iitotal_overflow_pages */
#define ADFI_654_II_DV_DESC_ALLTYPE   (ADI_FI_ID)654 /* ii_dv_desc(alltype) */
#define ADFI_655_BIT_ADD_BIT          (ADI_FI_ID)655 /* bit + bit */
#define ADFI_656_VARBYTE_TO_LONGTEXT  (ADI_FI_ID)656 /* varbyte -> longtext */
#define ADFI_657_BYTE_TO_LONGTEXT     (ADI_FI_ID)657 /* byte -> longtext */
#define ADFI_658_VBIT_ADD_VBIT        (ADI_FI_ID)658 /* vbit + vbit */
#define ADFI_659_VARBYTE_TO_VARCHAR   (ADI_FI_ID)659 /* varbyte -> varchar */
#define ADFI_660_BYTE_TO_VARCHAR      (ADI_FI_ID)660 /* byte -> varchar */
#define ADFI_661_VARBYTE_TO_CHAR      (ADI_FI_ID)661 /* varbyte -> char */
#define ADFI_662_BYTE_TO_CHAR         (ADI_FI_ID)662 /* byte -> char */
#define ADFI_663_LBYTE_TO_LBYTE       (ADI_FI_ID)663 /* lbyte -> lvch */
#define ADFI_664_VARBYTE_TO_LBYTE     (ADI_FI_ID)664 /* varbyte -> lbyte */
#define ADFI_665_CONCAT_BIT_BIT       (ADI_FI_ID)665 /* concat(bit,bit) */
#define ADFI_666_BYTE_TO_LBYTE        (ADI_FI_ID)666 /* byte -> lbyte */
#define ADFI_667_LONGTEXT_TO_LBYTE    (ADI_FI_ID)667 /* longtext -> lbyte */
#define ADFI_668_CONCAT_VBIT_VBIT     (ADI_FI_ID)668 /* concat(vbit,vbit) */
#define ADFI_669_TEXT_TO_LBYTE        (ADI_FI_ID)669 /* text -> lbyte */
#define ADFI_670_C_TO_LBYTE           (ADI_FI_ID)670 /* c -> lbyte */
#define ADFI_671_VARCHAR_TO_LBYTE     (ADI_FI_ID)671 /* varchar -> lbyte */
#define ADFI_672_CHAR_TO_LBYTE        (ADI_FI_ID)672 /* char -> lbyte */
#define ADFI_673_LEFT_BIT_I           (ADI_FI_ID)673 /* left(bit,i) */
#define ADFI_674_LEFT_VBIT_I          (ADI_FI_ID)674 /* left(vbit,i) */
#define ADFI_675_LENGTH_BIT           (ADI_FI_ID)675 /* length(bit) */
#define ADFI_676_LENGTH_VBIT          (ADI_FI_ID)676 /* length(vbit) */
#define ADFI_677_LOCATE_BIT_BIT       (ADI_FI_ID)677 /* locate(bit,bit) */
#define ADFI_678_LOCATE_VBIT_VBIT     (ADI_FI_ID)678 /* locate(vbit,vbit) */
#define ADFI_679_SIZE_BIT             (ADI_FI_ID)679 /* size(bit) */
#define ADFI_680_SIZE_VBIT            (ADI_FI_ID)680 /* size(vbit) */
#define ADFI_681_VARBYTE_TO_VARBYTE   (ADI_FI_ID)681 /* varbyte -> byte */
#define ADFI_682_BYTE_TO_VARBYTE      (ADI_FI_ID)682 /* byte -> varbyte */
#define ADFI_683_BIT_VARCHAR          (ADI_FI_ID)683 /* bit(varchar) */
#define ADFI_684_BIT_CHAR             (ADI_FI_ID)684 /* bit(char) */
#define ADFI_685_BIT_BIT              (ADI_FI_ID)685 /* bit(bit) */
#define ADFI_686_BIT_VBIT             (ADI_FI_ID)686 /* bit(vbit) */
#define ADFI_687_VBIT_VARCHAR         (ADI_FI_ID)687 /* vbit(varchar) */
#define ADFI_688_VBIT_CHAR            (ADI_FI_ID)688 /* vbit(char) */
#define ADFI_689_VBIT_BIT             (ADI_FI_ID)689 /* vbit(bit) */
#define ADFI_690_VBIT_VBIT            (ADI_FI_ID)690 /* vbit(vbit) */
#define ADFI_691_CHAR_TO_BIT          (ADI_FI_ID)691 /* char -> bit */
#define ADFI_692_VARCHAR_TO_BIT       (ADI_FI_ID)692 /* varchar -> bit */
#define ADFI_693_BIT_TO_BIT           (ADI_FI_ID)693 /* bit -> bit */
#define ADFI_694_VBIT_TO_BIT          (ADI_FI_ID)694 /* vbit -> bit */
#define ADFI_695_CHAR_TO_VBIT         (ADI_FI_ID)695 /* char -> vbit */
#define ADFI_696_VARCHAR_TO_VBIT      (ADI_FI_ID)696 /* varchar -> vbit */
#define ADFI_697_BIT_TO_VBIT          (ADI_FI_ID)697 /* bit -> vbit */
#define ADFI_698_VBIT_TO_VBIT         (ADI_FI_ID)698 /* vbit -> vbit */
#define ADFI_699_LONGTEXT_TO_VBIT     (ADI_FI_ID)699 /* longtext -> vbit */
#define ADFI_700_LONGTEXT_TO_BIT      (ADI_FI_ID)700 /* longtext -> bit */
#define ADFI_701_BIT_TO_VARCHAR       (ADI_FI_ID)701 /* bit -> varchar */
#define ADFI_702_BIT_TO_CHAR          (ADI_FI_ID)702 /* bit -> char */
#define ADFI_703_BIT_TO_LONGTEXT      (ADI_FI_ID)703 /* bit -> longtext */
#define ADFI_704_VBIT_TO_VARCHAR      (ADI_FI_ID)704 /* vbit -> varchar */
#define ADFI_705_VBIT_TO_CHAR         (ADI_FI_ID)705 /* vbit -> char */
#define ADFI_706_VBIT_TO_LONGTEXT     (ADI_FI_ID)706 /* vbit -> longtext */
#define ADFI_707_LENGTH_LVCH          (ADI_FI_ID)707 /* length(lvch) */
#define ADFI_708_SIZE_LVCH            (ADI_FI_ID)708 /* size(lvch) */
#define ADFI_709_LEFT_LVCH_INT        (ADI_FI_ID)709 /* left(lvch,int) */
#define ADFI_710_RIGHT_LVCH_INT       (ADI_FI_ID)710 /* right(lvch,int) */
#define ADFI_711_UPPERCASE_LVCH       (ADI_FI_ID)711 /* uppercase(lvch) */
#define ADFI_712_LOWERCASE_LVCH       (ADI_FI_ID)712 /* lowercase(lvch) */
#define ADFI_713_CONCAT_LVCH_LVCH     (ADI_FI_ID)713 /* concat(lvch,lvch) */
#define ADFI_714_LVCH_ADD_LVCH        (ADI_FI_ID)714 /* lvch + lvch */

#define ADFI_715_BYTE_NE_BYTE         (ADI_FI_ID)715 /* byte != byte */
#define ADFI_716_VBYTE_NE_VBYTE       (ADI_FI_ID)716 /* vbyte != vbyte */
#define ADFI_717_BYTE_LT_BYTE         (ADI_FI_ID)717 /* byte < byte */
#define ADFI_718_VBYTE_LT_VBYTE       (ADI_FI_ID)718 /* vbyte < vbyte */
#define ADFI_719_BYTE_LE_BYTE         (ADI_FI_ID)719 /* byte <= byte */
#define ADFI_720_VBYTE_LE_VBYTE       (ADI_FI_ID)720 /* vbyte <= vbyte */
#define ADFI_721_BYTE_EQ_BYTE         (ADI_FI_ID)721 /* byte = byte */
#define ADFI_722_VBYTE_EQ_VBYTE       (ADI_FI_ID)722 /* vbyte = vbyte */
#define ADFI_723_BYTE_GT_BYTE         (ADI_FI_ID)723 /* byte > byte */
#define ADFI_724_VBYTE_GT_VBYTE       (ADI_FI_ID)724 /* vbyte > vbyte */
#define ADFI_725_BYTE_GE_BYTE         (ADI_FI_ID)725 /* byte >= byte */
#define ADFI_726_VBYTE_GE_VBYTE       (ADI_FI_ID)726 /* vbyte >= vbyte */
#define ADFI_727_BYTE_ADD_BYTE        (ADI_FI_ID)727 /* byte + byte */
#define ADFI_728_VBYTE_ADD_VBYTE      (ADI_FI_ID)728 /* vbyte + vbyte */
#define ADFI_729_LBYTE_ADD_LBYTE      (ADI_FI_ID)729 /* lbyte + lbyte */
#define ADFI_730_CONCAT_BYTE_BYTE     (ADI_FI_ID)730 /* concat(byte,byte) */
#define ADFI_731_CONCAT_VBYTE_VBYTE   (ADI_FI_ID)731 /* concat(vbyte,vbyte) */
#define ADFI_732_CONCAT_LBYTE_LBYTE   (ADI_FI_ID)732 /* concat(lbyte,lbyte) */
#define ADFI_733_LEFT_BYTE_I          (ADI_FI_ID)733 /* left(byte,i) */
#define ADFI_734_LEFT_VBYTE_I         (ADI_FI_ID)734 /* left(vbyte,i) */
#define ADFI_735_LEFT_LBYTE_I         (ADI_FI_ID)735 /* left(lbyte,i) */
#define ADFI_736_LENGTH_BYTE          (ADI_FI_ID)736 /* length(byte) */
#define ADFI_737_LENGTH_VBYTE         (ADI_FI_ID)737 /* length(vbyte) */
#define ADFI_738_LENGTH_LBYTE         (ADI_FI_ID)738 /* length(lbyte) */
#define ADFI_739_RIGHT_BYTE_I         (ADI_FI_ID)739 /* right(byte,i) */
#define ADFI_740_RIGHT_VBYTE_I        (ADI_FI_ID)740 /* right(vbyte,i) */
#define ADFI_741_RIGHT_LBYTE_I        (ADI_FI_ID)741 /* right(lbyte,i) */
#define ADFI_742_SHIFT_BYTE_I         (ADI_FI_ID)742 /* shift(byte,i) */
#define ADFI_743_SHIFT_VBYTE_I        (ADI_FI_ID)743 /* shift(vbyte,i) */
#define ADFI_744_SHIFT_LBYTE_I        (ADI_FI_ID)744 /* shift(lbyte,i) */
#define ADFI_745_SIZE_BYTE            (ADI_FI_ID)745 /* size(byte) */
#define ADFI_746_SIZE_VBYTE           (ADI_FI_ID)746 /* size(vbyte) */
#define ADFI_747_SIZE_LBYTE           (ADI_FI_ID)747 /* size(lbyte) */
#define ADFI_748_BYTE_CHAR            (ADI_FI_ID)748 /* byte(char) */
#define ADFI_749_VARBYTE_VARCHAR      (ADI_FI_ID)749 /* varbyte(varchar) */
#define ADFI_750_II_LOLK_LVCH         (ADI_FI_ID)750 /* ii_lolk(lvch) */
#define ADFI_751_II_LOLK_LBYTE        (ADI_FI_ID)751 /* ii_lolk(lbyte) */
#define ADFI_752_CHAR_TO_BYTE         (ADI_FI_ID)752 /* char -> byte */
#define ADFI_753_VARCHAR_TO_BYTE      (ADI_FI_ID)753 /* varchar -> byte */
#define ADFI_754_C_TO_BYTE            (ADI_FI_ID)754 /* c -> byte */
#define ADFI_755_TEXT_TO_BYTE         (ADI_FI_ID)755 /* text -> byte */
#define ADFI_756_LONGTEXT_TO_BYTE     (ADI_FI_ID)756 /* longtext -> byte */
#define ADFI_757_BYTE_TO_BYTE         (ADI_FI_ID)757 /* byte -> byte */
#define ADFI_758_VARBYTE_TO_BYTE      (ADI_FI_ID)758 /* varbyte -> byte */
#define ADFI_759_CHAR_TO_VARBYTE      (ADI_FI_ID)759 /* char -> varbyte */
#define ADFI_760_VARCHAR_TO_VARBYTE   (ADI_FI_ID)760 /* varchar -> varbyte */
#define ADFI_761_C_TO_VARBYTE         (ADI_FI_ID)761 /* c -> varbyte */
#define ADFI_762_TEXT_TO_VARBYTE      (ADI_FI_ID)762 /* text -> varbyte */
#define ADFI_763_LONGTEXT_TO_VARBYTE  (ADI_FI_ID)763 /* longtext -> varbyte */
#define ADFI_764_BYTE_ADD_VBYTE       (ADI_FI_ID)764 /* byte + vbyte */
#define ADFI_765_VBYTE_ADD_BYTE       (ADI_FI_ID)765 /* vbyte + byte */
#define ADFI_766_CONCAT_BYTE_VBYTE    (ADI_FI_ID)766 /* concat(byte,vbyte) */
#define ADFI_767_CONCAT_VBYTE_BYTE    (ADI_FI_ID)767 /* concat(vbyte,byte) */
#define ADFI_768_BYTE_VARCHAR         (ADI_FI_ID)768 /* byte(varchar) */
#define ADFI_769_BYTE_BYTE            (ADI_FI_ID)769 /* byte(byte) */
#define ADFI_770_BYTE_VBYTE           (ADI_FI_ID)770 /* byte(vbyte) */
#define ADFI_771_BYTE_CHAR_I          (ADI_FI_ID)771 /* byte(char,i) */
#define ADFI_772_BYTE_VARCHAR_I       (ADI_FI_ID)772 /* byte(varchar,i) */
#define ADFI_773_BYTE_BYTE_I          (ADI_FI_ID)773 /* byte(byte,i) */
#define ADFI_774_BYTE_VBYTE_I         (ADI_FI_ID)774 /* byte(vbyte,i) */
#define ADFI_775_VARBYTE_CHAR         (ADI_FI_ID)775 /* varbyte(char) */
#define ADFI_776_VARBYTE_BYTE         (ADI_FI_ID)776 /* varbyte(byte) */
#define ADFI_777_VARBYTE_VBYTE        (ADI_FI_ID)777 /* varbyte(vbyte) */
#define ADFI_778_VARBYTE_CHAR_I       (ADI_FI_ID)778 /* varbyte(char,i) */
#define ADFI_779_VARBYTE_VARCHAR_I    (ADI_FI_ID)779 /* varbyte(varchar,i) */
#define ADFI_780_VARBYTE_BYTE_I       (ADI_FI_ID)780 /* varbyte(byte,i) */
#define ADFI_781_VARBYTE_VBYTE_I      (ADI_FI_ID)781 /* varbyte(vbyte,i) */
#define ADFI_782_LBYTE_TO_LVCH        (ADI_FI_ID)782 /* lbyte -> lvch */
#define ADFI_783_LVCH_TO_LBYTE        (ADI_FI_ID)783 /* lvch -> lbyte */
#define ADFI_784_DEC_MUL_MONEY        (ADI_FI_ID)784 /* dec * money */
#define ADFI_785_MONEY_MUL_DEC        (ADI_FI_ID)785 /* money * dec */
#define ADFI_786_DEC_DIV_MONEY        (ADI_FI_ID)786 /* dec / money */
#define ADFI_787_MONEY_DIV_DEC        (ADI_FI_ID)787 /* money / dec */
#define ADFI_788_VARBYTE_TO_LONGTEXT  (ADI_FI_ID)788 /* vbyte -> longtext */

#define	ADFI_789_II_PERM_TYPE	      (ADI_FI_ID)789 /*  ii_permit_type(int)*/
#define ADFI_790_SOUNDEX_C            (ADI_FI_ID)790 /* soundex(c)	*/
#define ADFI_791_SOUNDEX_CHAR         (ADI_FI_ID)791 /* soundex(char)	*/
#define ADFI_792_SOUNDEX_TEXT         (ADI_FI_ID)792 /* soundex(text)	*/
#define ADFI_793_SOUNDEX_VARCHAR      (ADI_FI_ID)793 /* soundex(varchar)*/
#define ADFI_794_SOUNDEX_BYTE         (ADI_FI_ID)794 /* soundex(byte)	*/
#define ADFI_795_SOUNDEX_VBYTE        (ADI_FI_ID)795 /* soundex(varbyte)*/
#define ADFI_796_BIT_ADD_BYTE         (ADI_FI_ID)796 /* bit_add(byte,byte)*/
#define ADFI_797_BIT_AND_BYTE         (ADI_FI_ID)797 /* bit_and(byte,byte)*/
#define ADFI_798_BIT_NOT_BYTE         (ADI_FI_ID)798 /* bit_not(byte)	*/
#define ADFI_799_BIT_OR_BYTE          (ADI_FI_ID)799 /* bit_or(byte,byte)*/
#define ADFI_800_BIT_XOR_BYTE         (ADI_FI_ID)800 /* bit_xor(byte,byte)*/
#define ADFI_801_IPADDR_C	      (ADI_FI_ID)801 /* ii_ipaddr(c)	*/
#define ADFI_802_IPADDR_CHAR	      (ADI_FI_ID)802 /* ii_ipaddr(char)	*/
#define ADFI_803_IPADDR_TEXT	      (ADI_FI_ID)803 /* ii_ipaddr(text)	*/
#define ADFI_804_IPADDR_VARCHAR	      (ADI_FI_ID)804 /* ii_ipaddr(varchar)*/
#define ADFI_805_INTEXTRACT_BYTE      (ADI_FI_ID)805 /* intextract(byte,i) */
#define ADFI_806_INTEXTRACT_VARBYTE   (ADI_FI_ID)806 /* intextract(varbyte,i) */
#define ADFI_807_HASH_ALL	      (ADI_FI_ID)807 /* hash(all)	*/
#define ADFI_808_RANDOMF_VOID	      (ADI_FI_ID)808 /* randomf()	*/
#define ADFI_809_RANDOMF_INT_INT      (ADI_FI_ID)809 /* randomf(int, int) */
#define ADFI_810_RANDOMF_FLT_FLT      (ADI_FI_ID)810 /* randomf(flt, flt) */
#define ADFI_811_RANDOM_VOID	      (ADI_FI_ID)811 /* random()	*/
#define ADFI_812_RANDOM_INT_INT	      (ADI_FI_ID)812 /* random(int, int)  */
#define ADFI_813_UUID_CREATE_VOID     (ADI_FI_ID)813 /* uuid_create()	*/
#define ADFI_814_UUID_TO_CHAR_BYTE    (ADI_FI_ID)814 /* uuid_to_char(byte)   */
#define ADFI_815_UUID_FROM_CHAR_C     (ADI_FI_ID)815 /* uuid_from_char(c)    */
#define ADFI_816_UUID_FROM_CHAR_CHAR  (ADI_FI_ID)816 /* uuid_from_char(char) */
#define ADFI_817_UUID_FROM_CHAR_TEXT  (ADI_FI_ID)817 /* uuid_from_char(text) */
#define ADFI_818_UUID_FROM_CHAR_VARCHAR (ADI_FI_ID)818 /* uuid_from_char(varchar)*/
#define ADFI_819_UUID_COMPARE_BYTE_BYTE (ADI_FI_ID)819 /* uuid_compare(byte,byte)*/
#define ADFI_820_SUBSTRING_C_INT      (ADI_FI_ID)820 /* substring(c,i) */
#define ADFI_821_SUBSTRING_CHAR_INT   (ADI_FI_ID)821 /* substring(char,i) */
#define ADFI_822_SUBSTRING_TEXT_INT   (ADI_FI_ID)822 /* substring(text,i) */
#define ADFI_823_SUBSTRING_VARCHAR_INT   (ADI_FI_ID)823 /* substring(text,i) */
#define ADFI_824_SUBSTRING_C_INT_INT  (ADI_FI_ID)824 /* substring(c,i,i) */
#define ADFI_825_SUBSTRING_CHAR_INT_INT (ADI_FI_ID)825 /* substring(char,i,i) */
#define ADFI_826_SUBSTRING_TEXT_INT_INT (ADI_FI_ID)826 /* substring(text,i,i) */
#define ADFI_827_SUBSTRING_VARCHAR_INT_INT (ADI_FI_ID)827 /* substring(text,i,i) */
#define ADFI_828_UNHEX_CHAR           (ADI_FI_ID)828 /* unhex(char) */
#define ADFI_829_UNHEX_TEXT           (ADI_FI_ID)829 /* unhex(text) */
#define ADFI_830_UNHEX_VARCHAR        (ADI_FI_ID)830 /* unhex(varchar) */
#define ADFI_831_UNHEX_ALL            (ADI_FI_ID)831 /* unhex(all) */

#define ADFI_832_CORR_FLT	      (ADI_FI_ID)832 /* corr(flt, flt) */
#define ADFI_833_COVAR_POP_FLT        (ADI_FI_ID)833 /* covar_pop(flt, flt) */
#define ADFI_834_COVAR_SAMP_FLT       (ADI_FI_ID)834 /* covar_samp(flt, flt) */
#define ADFI_835_REGR_AVGX_FLT        (ADI_FI_ID)835 /* regr_avgx(flt, flt) */
#define ADFI_836_REGR_AVGY_FLT        (ADI_FI_ID)836 /* regr_avgy(flt, flt) */
#define ADFI_837_REGR_COUNT_ANY       (ADI_FI_ID)837 /* regr_count(any, any) */
#define ADFI_838_REGR_INTERCEPT_FLT   (ADI_FI_ID)838 /* regr_intercept(flt, flt) */
#define ADFI_839_REGR_R2_FLT          (ADI_FI_ID)839 /* regr_r2(flt, flt) */
#define ADFI_840_REGR_SLOPE_FLT       (ADI_FI_ID)840 /* regr_slope(flt, flt) */
#define ADFI_841_REGR_SXX_FLT         (ADI_FI_ID)841 /* regr_sxx(flt, flt) */
#define ADFI_842_REGR_SXY_FLT         (ADI_FI_ID)842 /* regr_sxy(flt, flt) */
#define ADFI_843_REGR_SYY_FLT         (ADI_FI_ID)843 /* regr_syy(flt, flt) */
#define ADFI_844_STDDEV_POP_FLT       (ADI_FI_ID)844 /* stddev_pop(flt, flt) */
#define ADFI_845_STDDEV_SAMP_FLT      (ADI_FI_ID)845 /* stddev_samp(flt, flt) ** See pslsgram.yi for secondary defn */
#define ADFI_846_VAR_POP_FLT          (ADI_FI_ID)846 /* var_pop(flt, flt) */
#define ADFI_847_VAR_SAMP_FLT         (ADI_FI_ID)847 /* var_samp(flt, flt) */

#define ADFI_848_CHLEN_C              (ADI_FI_ID)848 /* char_length(c) */
#define ADFI_849_CHLEN_CHAR           (ADI_FI_ID)849 /* char_length(char) */
#define ADFI_850_CHLEN_TEXT           (ADI_FI_ID)850 /* char_length(text) */
#define ADFI_851_CHLEN_VARCHAR        (ADI_FI_ID)851 /* char_length(varchar) */
#define ADFI_852_CHLEN_BYTE           (ADI_FI_ID)852 /* char_length(byte) */
#define ADFI_853_CHLEN_VARBYTE        (ADI_FI_ID)853 /* char_length(varbyte) */
#define ADFI_854_CHLEN_LVCH           (ADI_FI_ID)854 /* char_length(lvch) */
#define ADFI_855_CHLEN_LBYTE          (ADI_FI_ID)855 /* char_length(lbyte) */
#define ADFI_856_CHLEN_NCHAR          (ADI_FI_ID)856 /* char_length(nchar) */
#define ADFI_857_CHLEN_NVCHAR         (ADI_FI_ID)857 /* char_length(nvchar) */
#define ADFI_858_OCLEN_C              (ADI_FI_ID)858 /* octet_length(c) */
#define ADFI_859_OCLEN_CHAR           (ADI_FI_ID)859 /* octet_length(char) */
#define ADFI_860_OCLEN_TEXT           (ADI_FI_ID)860 /* octet_length(text) */
#define ADFI_861_OCLEN_VARCHAR        (ADI_FI_ID)861 /* octet_length(varchar) */
#define ADFI_862_OCLEN_BYTE           (ADI_FI_ID)862 /* octet_length(byte) */
#define ADFI_863_OCLEN_VARBYTE        (ADI_FI_ID)863 /* octet_length(varbyte) */
#define ADFI_864_OCLEN_LVCH           (ADI_FI_ID)864 /* octet_length(lvch) */
#define ADFI_865_OCLEN_LBYTE          (ADI_FI_ID)865 /* octet_length(lbyte) */
#define ADFI_866_OCLEN_NCHAR          (ADI_FI_ID)866 /* octet_length(nchar) */
#define ADFI_867_OCLEN_NVCHAR         (ADI_FI_ID)867 /* octet_length(nvchar) */
#define ADFI_868_BLEN_C               (ADI_FI_ID)868 /* bit_length(c) */
#define ADFI_869_BLEN_CHAR            (ADI_FI_ID)869 /* bit_length(char) */
#define ADFI_870_BLEN_TEXT            (ADI_FI_ID)870 /* bit_length(text) */
#define ADFI_871_BLEN_VARCHAR         (ADI_FI_ID)871 /* bit_length(varchar) */
#define ADFI_872_BLEN_BYTE            (ADI_FI_ID)872 /* bit_length(byte) */
#define ADFI_873_BLEN_VARBYTE         (ADI_FI_ID)873 /* bit_length(varbyte) */
#define ADFI_874_BLEN_LVCH            (ADI_FI_ID)874 /* bit_length(lvch) */
#define ADFI_875_BLEN_LBYTE           (ADI_FI_ID)875 /* bit_length(lbyte) */
#define ADFI_876_BLEN_NCHAR           (ADI_FI_ID)876 /* bit_length(nchar) */
#define ADFI_877_BLEN_NVCHAR          (ADI_FI_ID)877 /* bit_length(nvchar) */
#define ADFI_878_POS_STR_STR	      (ADI_FI_ID)878 /* position(str,str) */
#define ADFI_879_POS_UNISTR_UNISTR    (ADI_FI_ID)879 /* position(uni,uni) */
#define ADFI_880_POS_VBYTE_VBYTE      (ADI_FI_ID)880 /* position(vbyte,vbyte) */
#define ADFI_881_POS_STR_LVCH         (ADI_FI_ID)881 /* position(str,lvch) */
#define ADFI_882_POS_UNISTR_LNVCHR    (ADI_FI_ID)882 /* position(uni,lnvch) */
#define ADFI_883_POS_VBYTE_LBYTE      (ADI_FI_ID)883 /* position(vbyte,lbyte) */
#define ADFI_884_ATRIM_C              (ADI_FI_ID)884 /* ANSI trim(c) */
#define ADFI_885_ATRIM_TEXT           (ADI_FI_ID)885 /* ANSI trim(text) */
#define ADFI_886_ATRIM_CHAR           (ADI_FI_ID)886 /* ANSI trim(char) */
#define ADFI_887_ATRIM_VARCHAR        (ADI_FI_ID)887 /* ANSI trim(varchar) */
#define ADFI_888_ATRIM_NCHAR	      (ADI_FI_ID)888 /* ANSI trim(nchar) */
#define ADFI_889_ATRIM_NVCHAR	      (ADI_FI_ID)889 /* ANSI trim(nvchar) */
#define ADFI_890_CHAREXTRACT_C_I      (ADI_FI_ID)890 /* charextract(c,i) */
#define ADFI_891_CHAREXTRACT_TEXT_I   (ADI_FI_ID)891 /* charextract(text,i) */
#define ADFI_892_CHAREXTRACT_CHAR_I   (ADI_FI_ID)892 /* charextract(char,i) */
#define ADFI_893_CHAREXTRACT_VARCHAR_I (ADI_FI_ID)893 /* charextract(varchar,i) */

#define ADFI_954_SESSION_PRIV 		(ADI_FI_ID)954 /* session_priv(char) */
#define ADFI_955_IITBLSTAT 		(ADI_FI_ID)955 /* iitblstat(int) */
#define ADFI_971_CHA_TO_DEC		(ADI_FI_ID)971 /* char -> decimal */
#define ADFI_972_VCH_TO_DEC		(ADI_FI_ID)972 /* varchar -> decimal */
#define ADFI_973_TXT_TO_DEC		(ADI_FI_ID)973 /* text -> decimal */
#define ADFI_974_C_TO_DEC		(ADI_FI_ID)974 /* c -> decimal */
#define ADFI_975_DEC_TO_CHA		(ADI_FI_ID)975 /* decimal -> char */
#define ADFI_976_DEC_TO_VCH		(ADI_FI_ID)976 /* decimal -> varchar */
#define ADFI_977_DEC_TO_TXT		(ADI_FI_ID)977 /* decimal -> text */
#define ADFI_978_DEC_TO_C		(ADI_FI_ID)978 /* decimal -> c */
#define ADFI_979_BYTE_ALL		(ADI_FI_ID)979 /* byte(all) */
#define ADFI_980_VBYTE_ALL		(ADI_FI_ID)980 /* varbyte(all) */
#define ADFI_981__DATE4_I       (ADI_FI_ID)981 /* _date4(i) */

#define ADFI_982_NCHAR_NE_NCHAR         (ADI_FI_ID)982 /* NCHAR != NCHAR */
#define ADFI_983_NCHAR_EQ_NCHAR         (ADI_FI_ID)983 /* NCHAR = NCHAR */
#define ADFI_984_NVCHAR_NE_NVCHAR	(ADI_FI_ID)984 /* NVARCHAR != NVARCHAR */
#define ADFI_985_NVCHAR_EQ_NVCHAR	(ADI_FI_ID)985 /* NVARCHAR =  NVARCHAR */
#define ADFI_986_NCHAR_LT_NCHAR         (ADI_FI_ID)986 /* NCHAR < NCHAR */
#define ADFI_987_NCHAR_GE_NCHAR         (ADI_FI_ID)987 /* NCHAR >= NCHAR */
#define ADFI_988_NVCHAR_LT_NVCHAR	(ADI_FI_ID)988 /* NVARCHAR <  NVARCHAR */
#define ADFI_989_NVCHAR_GE_NVCHAR	(ADI_FI_ID)989 /* NVARCHAR >= NVARCHAR */
#define ADFI_990_NCHAR_LE_NCHAR         (ADI_FI_ID)990 /* NCHAR <= NCHAR */
#define ADFI_991_NCHAR_GT_NCHAR         (ADI_FI_ID)991 /* NCHAR > NCHAR */
#define ADFI_992_NVCHAR_LE_NVCHAR	(ADI_FI_ID)992 /* NVARCHAR <= NVARCHAR */
#define ADFI_993_NVCHAR_GT_NVCHAR	(ADI_FI_ID)993 /* NVARCHAR >  NVARCHAR */
#define ADFI_994_NCHAR_ISNULL		(ADI_FI_ID)994 /* isnull(NCHAR) */
#define ADFI_995_NCHAR_ISNOTNULL	(ADI_FI_ID)995 /* isnotnull(NCHAR) */
#define ADFI_996_NVCHAR_ISNULL		(ADI_FI_ID)996 /* isnull(NVARCHAR) */
#define ADFI_997_NVCHAR_ISNOTNULL	(ADI_FI_ID)997 /* isnotnull(NVARCHAR) */
#define ADFI_998_NCHAR_LIKE_NVCHAR	(ADI_FI_ID)998 /* NCHAR like NVARCHAR */
#define ADFI_999_NCHAR_NOTLIKE_NVCHAR	(ADI_FI_ID)999 /* NCHAR notlike NVARCHAR */
#define ADFI_1000_NVCHAR_LIKE_NVCHAR	(ADI_FI_ID)1000 /* NVARCHAR like NVARCHAR */
#define ADFI_1001_NVCHAR_NOTLIKE_NVCHAR (ADI_FI_ID)1001 /* NVARCHAR notlike NVARCHAR */
#define ADFI_1002_CONCAT_NCHAR_C        (ADI_FI_ID)1002 /* concat(NCHAR,c) */
#define ADFI_1003_CONCAT_NCHAR_CHAR     (ADI_FI_ID)1003 /* concat(NCHAR,char) */
#define ADFI_1004_CONCAT_NCHAR_VARCHAR  (ADI_FI_ID)1004 /* concat(NCHAR,varchar) */
#define ADFI_1005_CONCAT_NCHAR_NCHAR	(ADI_FI_ID)1005 /* concat(NCHAR,NCHAR) */
#define ADFI_1006_CONCAT_NVCHAR_C	(ADI_FI_ID)1006 /* concat(NVARCHAR,c) */
#define ADFI_1007_CONCAT_NVCHAR_TEXT	(ADI_FI_ID)1007 /* concat(NVARCHAR,text) */
#define ADFI_1008_CONCAT_NVCHAR_CHAR	(ADI_FI_ID)1008 /* concat(NVARCHAR,char) */
#define ADFI_1009_CONCAT_NCHAR_NVARCHAR	(ADI_FI_ID)1009 /* concat(NCHAR,NVARCHAR) */
#define ADFI_1010_CONCAT_NVCHAR_NVCHAR	(ADI_FI_ID)1010 /* concat(NVARCHAR,NVARCHAR) */
#define ADFI_1011_CONCAT_NVCHAR_NCHAR	(ADI_FI_ID)1011 /* concat(NVARCHAR,NCHAR) */
#define ADFI_1012_LEFT_NCHAR_I		(ADI_FI_ID)1012 /* left(NCHAR,i) */
#define ADFI_1013_LEFT_NVCHAR_I		(ADI_FI_ID)1013 /* left(NVARCHAR,i) */
#define ADFI_1014_RIGHT_NCHAR_I		(ADI_FI_ID)1014 /* right(NCHAR,i) */
#define ADFI_1015_RIGHT_NVCHAR_I	(ADI_FI_ID)1015 /* right(NVARCHAR,i) */
#define ADFI_1016_LENGTH_NCHAR		(ADI_FI_ID)1016 /* length(NCHAR) */
#define ADFI_1017_LENGTH_NVCHAR		(ADI_FI_ID)1017 /* length(NVARCHAR) */
#define ADFI_1018_TRIM_NCHAR		(ADI_FI_ID)1018 /* trim(NCHAR) */
#define ADFI_1019_TRIM_NVCHAR		(ADI_FI_ID)1019 /* trim(NVARCHAR) */
#define ADFI_1020_VARCHAR_TO_NCHAR      (ADI_FI_ID)1020 /* nchar(varchar) */
#define ADFI_1021_NCHAR_TO_VARCHAR      (ADI_FI_ID)1021 /* varchar(nchar) */
#define ADFI_1022_VARCHAR_TO_NVCHAR     (ADI_FI_ID)1022 /* nvarchar(varchar) */
#define ADFI_1023_NVCHAR_TO_VARCHAR     (ADI_FI_ID)1023 /* varchar(nvarchar) */
#define ADFI_1024_NCHAR_TO_NVCHAR      (ADI_FI_ID)1024 /* nvarchar(nchar) */
#define ADFI_1025_NVCHAR_TO_NCHAR      (ADI_FI_ID)1025 /* nchar(nvarchar) */
#define ADFI_1026_NCHAR_TO_NCHAR	(ADI_FI_ID)1026 /* nchar(nvarchar) */
#define ADFI_1027_NVCHAR_TO_NVCHAR	(ADI_FI_ID)1027 /* nchar(nvarchar) */
#define ADFI_1028_ANY_NCHAR	       	(ADI_FI_ID)1028 /* any(nchar) */	
#define ADFI_1029_ANY_NVCHAR	       	(ADI_FI_ID)1029 /* any(nvarchar) */		
#define ADFI_1030_COUNT_NCHAR	       	(ADI_FI_ID)1030 /* count(nchar) */	
#define ADFI_1031_COUNT_NVCHAR	       	(ADI_FI_ID)1031 /* count(nvarchar) */
#define ADFI_1032_MAX_NCHAR	       	(ADI_FI_ID)1032 /* max(nchar) */	
#define ADFI_1033_MAX_NVCHAR	       	(ADI_FI_ID)1033 /* max(nvarchar) */
#define ADFI_1034_MIN_NCHAR      	(ADI_FI_ID)1034 /* min(nchar) */	
#define ADFI_1035_MIN_NVCHAR	       	(ADI_FI_ID)1035 /* min(nvarchar) */
#define ADFI_1038_LOWERCASE_NCHAR	(ADI_FI_ID)1038 /* lowercase(nchar) */	
#define ADFI_1039_LOWERCASE_NVCHAR     	(ADI_FI_ID)1039 /* lowercase(nvarchar) */
#define ADFI_1040_PAD_NCHAR            	(ADI_FI_ID)1040 /* pad(nchar) */	
#define ADFI_1041_PAD_NVARCHAR		(ADI_FI_ID)1041 /* pad(nvarchar) */
#define ADFI_1042_SHIFT_NCHAR_I		(ADI_FI_ID)1042 /* shift(nchar) */	
#define ADFI_1043_SHIFT_NVCHAR_I	(ADI_FI_ID)1043 /* shift(nvarchar) */
#define ADFI_1044_SIZE_NCHAR	       	(ADI_FI_ID)1044 /* size(nchar) */	
#define ADFI_1045_SIZE_NVCHAR	       	(ADI_FI_ID)1045 /* size(nvarchar) */
#define ADFI_1046_SQUEEZE_NCHAR		(ADI_FI_ID)1046 /* sqeeze(nchar) */	
#define ADFI_1047_SQUEEZE_NVARCHAR	(ADI_FI_ID)1047 /* sqeeze(nvarchar) */
#define ADFI_1048_UPPERCASE_NCHAR	(ADI_FI_ID)1048 /* uppercase(nchar) */		
#define ADFI_1049_UPPERCASE_NVCHAR	(ADI_FI_ID)1049 /* uppercase(nvarchar) */
#define ADFI_1050_CHAREXTRACT_NCHAR_I	(ADI_FI_ID)1050 /* charextract(nchar) */	
#define ADFI_1051_CHAREXTRACT_NVCHAR_I (ADI_FI_ID)1051 /* charextract(nvarchar) */
#define ADFI_1052_SUBSTRING_NCHAR_INT	(ADI_FI_ID)1052 /* substring(nchar,int) */	
#define ADFI_1053_SUBSTRING_NVCHAR_INT	(ADI_FI_ID)1053 /* substring(nvarchar,int) */
#define ADFI_1054_SUBSTRING_NCHAR_INT_INT	(ADI_FI_ID)1054 /* substring(nchar,int,int) */  
#define ADFI_1055_SUBSTRING_NVCHAR_INT_INT	(ADI_FI_ID)1055 /* substring(nvarchar,int,int) */ 
#define ADFI_1056_NCHAR_TO_LONGTEXT     (ADI_FI_ID)1056 /* longtext(nchar) */
#define ADFI_1057_LONGTEXT_TO_NCHAR     (ADI_FI_ID)1057 /* nchar(longtext) */
#define ADFI_1058_LONGTEXT_TO_NVCHAR    (ADI_FI_ID)1058 /* nvarchar(longtext) */
#define ADFI_1059_NVCHAR_TO_LONGTEXT    (ADI_FI_ID)1059 /* longtext(nvarchar) */
#define ADFI_1060_NCHAR_TO_BYTE         (ADI_FI_ID)1060 /* byte(nchar) */
#define ADFI_1061_NVCHAR_TO_BYTE        (ADI_FI_ID)1061 /* byte(nvarchar) */
#define ADFI_1062_NCHAR_TO_VARBYTE      (ADI_FI_ID)1062 /* varbyte(nchar) */
#define ADFI_1063_NVCHAR_TO_VARBYTE     (ADI_FI_ID)1063 /* varbyte(nvarchar) */
#define ADFI_1064_NCHAR_TO_LBYTE        (ADI_FI_ID)1064 /* lbyte(nvarchar) */
#define ADFI_1065_NVCHAR_TO_LBYTE       (ADI_FI_ID)1065 /* lbyte(nvarchar) */
#define ADFI_1066_BYTE_TO_NCHAR         (ADI_FI_ID)1066 /* nchar(byte) */
#define ADFI_1067_BYTE_TO_NVCHAR        (ADI_FI_ID)1067 /* nvarchar(byte) */
#define ADFI_1068_VARBYTE_TO_NCHAR      (ADI_FI_ID)1068 /* nchar(varbyte) */
#define ADFI_1069_VARBYTE_TO_NVCHAR     (ADI_FI_ID)1069 /* nvarchar(varbyte) */
#define ADFI_1070_LONGTEXT_TO_LONGNVCHAR (ADI_FI_ID)1070 /* long_nvarchar(longtext) */
#define ADFI_1071_LONGNVCHAR_TO_LONGTEXT (ADI_FI_ID)1071 /* longtext(long_nvarchar) */
#define ADFI_1072_NCHAR_TO_LONGNVCHAR    (ADI_FI_ID)1072 /* long_nvarchar(nchar) */
#define ADFI_1073_NVCHAR_TO_LONGNVCHAR   (ADI_FI_ID)1073 /* long_nvarchar(nvarchar) */
#define ADFI_1074_TABLEINFO		(ADI_FI_ID)1074  /* iitableinfo */
#define ADFI_1075_LNVARCHAR_TO_LNVARCHAR (ADI_FI_ID)1075 /* long_nvarchar(long_nvarchar) */
#define ADFI_1076_NCHAR_ADD_NCHAR	(ADI_FI_ID)1076 /* nchar + nchar */
#define ADFI_1077_NVARCHAR_ADD_NVARCHAR	(ADI_FI_ID)1077 /* nvarchar + nvarchar */
#define ADFI_1078_IFNULL_NCHAR_NCHAR	(ADI_FI_ID)1078 /* ifnull(nchar,nchar) */
#define ADFI_1079_IFNULL_NVARCHAR_NVARCHAR (ADI_FI_ID)1079 /* ifnull(nvarchar,nvarchar) */
#define ADFI_1080_NCHAR_TO_CHAR         (ADI_FI_ID)1080 /* char(nchar) */
#define ADFI_1081_NCHAR_TO_VARCHAR      (ADI_FI_ID)1081 /* varchar(nchar) */
#define ADFI_1082_NVCHAR_TO_CHAR        (ADI_FI_ID)1082 /* char(nvarchar) */
#define ADFI_1083_NVCHAR_TO_VARCHAR     (ADI_FI_ID)1083 /* varchar(nvarchar) */
#define ADFI_1084_CHAR_TO_NCHAR         (ADI_FI_ID)1084 /* nchar(char) */
#define ADFI_1085_CHAR_TO_NVCHAR        (ADI_FI_ID)1085 /* nvarchar(char) */
#define ADFI_1086_VARCHAR_TO_NCHAR      (ADI_FI_ID)1086 /* nchar(varchar) */
#define ADFI_1087_VARCHAR_TO_NVCHAR     (ADI_FI_ID)1087 /* nvarchar(varchar) */
#define ADFI_1088_LVCH_CHAR		(ADI_FI_ID)1088 /* long varchar(char) */
#define ADFI_1089_LVCH_C		(ADI_FI_ID)1089 /* long varchar(c) */
#define ADFI_1090_LVCH_TEXT		(ADI_FI_ID)1090 /* long varchar(text) */
#define ADFI_1091_LVCH_VARCHAR		(ADI_FI_ID)1091 /* long varchar(varchar) */
#define ADFI_1092_LVCH_LVCH		(ADI_FI_ID)1092 /* long varchar(long varchar) */
#define ADFI_1093_LVCH_LTXT		(ADI_FI_ID)1093 /* long varchar(longtext) */
#define ADFI_1094_LVCH_LBYTE		(ADI_FI_ID)1094 /* long varchar(lbyte) */
#define ADFI_1095_LBYTE_C		(ADI_FI_ID)1095 /* lbyte(c) */
#define ADFI_1096_LBYTE_CHAR		(ADI_FI_ID)1096 /* lbyte(char) */
#define ADFI_1097_LBYTE_VARCHAR		(ADI_FI_ID)1097 /* lbyte(varchar) */
#define ADFI_1098_LBYTE_TEXT		(ADI_FI_ID)1098 /* lbyte(text) */
#define ADFI_1099_LBYTE_LONGTEXT	(ADI_FI_ID)1099 /* lbyte(longtext) */
#define ADFI_1100_LBYTE_BYTE		(ADI_FI_ID)1100 /* lbyte(byte) */
#define ADFI_1101_LBYTE_VARBYTE		(ADI_FI_ID)1101 /* lbyte(varbyte) */
#define ADFI_1102_LBYTE_LBYTE		(ADI_FI_ID)1102 /* lbyte(lbyte) */
#define ADFI_1103_LBYTE_LVCH		(ADI_FI_ID)1103 /* lbyte(lvch) */
#define ADFI_1104_LBYTE_NCHAR		(ADI_FI_ID)1104 /* lbyte(nchar) */
#define ADFI_1105_LBYTE_NVCHAR		(ADI_FI_ID)1105 /* lbyte(nvarchar) */
#define ADFI_1106_NCHAR_ALL		(ADI_FI_ID)1106 /* nchar(all) */
#define ADFI_1107_NVCHAR_ALL		(ADI_FI_ID)1107 /* nvarchar(all) */
#define ADFI_1108_UTF16TOUTF8_NCHR	(ADI_FI_ID)1108 /* iiutf16_to_utf8(nchar) */
#define ADFI_1109_UTF16TOUTF8_NVCHR	(ADI_FI_ID)1109 /* iiutf16_to_utf8(nvarchar) */
#define ADFI_1110_UTF8TOUTF16_NCHR	(ADI_FI_ID)1110 /* iiutf8_to_utf16(nchar) */
#define ADFI_1111_UTF8TOUTF16_NVCHR	(ADI_FI_ID)1111 /* iiutf8_to_utf16(nvarchar) */
#define ADFI_1112_NCHAR_COPYTO_UTF8	(ADI_FI_ID)1112 /* nchar -copy-> utf8 */
#define ADFI_1113_NVCHAR_COPYTO_UTF8	(ADI_FI_ID)1113 /* nvarchar -copy-> utf8 */
#define ADFI_1114_UTF8_COPYTO_NCHAR	(ADI_FI_ID)1114 /* utf8 -copy-> nchar */
#define ADFI_1115_UTF8_COPYTO_NVCHAR	(ADI_FI_ID)1115 /* utf8 -copy-> nvarchar */
#define ADFI_1116_INT8_C                (ADI_FI_ID)1116 /* int8(c) */
#define ADFI_1117_INT8_F                (ADI_FI_ID)1117 /* int8(f) */
#define ADFI_1118_INT8_I                (ADI_FI_ID)1118 /* int8(i) */
#define ADFI_1119_INT8_TEXT             (ADI_FI_ID)1119 /* int8(text) */
#define ADFI_1120_INT8_MONEY            (ADI_FI_ID)1120 /* int8(money) */
#define ADFI_1121_INT8_DEC              (ADI_FI_ID)1121 /* int8(dec) */
#define ADFI_1122_INT8_CHAR             (ADI_FI_ID)1122 /* int8(char) */
#define ADFI_1123_INT8_VARCHAR          (ADI_FI_ID)1123 /* int8(varchar) */
#define ADFI_1124_NCHAR_TO_CHR		(ADI_FI_ID)1124 /* chr(nchar) */
#define ADFI_1125_CHR_TO_NCHAR		(ADI_FI_ID)1125 /* nchar(chr) */
#define ADFI_1126_NVCHAR_TO_CHR		  (ADI_FI_ID)1126 /* chr(nvarchar) */
#define ADFI_1127_CHR_TO_NVCHAR		  (ADI_FI_ID)1127 /* nvarchar(chr) */
#define ADFI_1128_NCHAR_TO_TXT		(ADI_FI_ID)1128 /* text(nchar) */
#define ADFI_1129_TXT_TO_NCHAR		(ADI_FI_ID)1129 /* nchar(text) */
#define ADFI_1130_NVCHAR_TO_TXT		(ADI_FI_ID)1130 /* text(nvarchar) */
#define ADFI_1131_TXT_TO_NVCHAR		(ADI_FI_ID)1131 /* nvarchar(text) */
#define ADFI_1132_NCHAR_TO_DATE		(ADI_FI_ID)1132 /* nchar -> date */
#define ADFI_1133_DATE_TO_NCHAR		(ADI_FI_ID)1133 /* date -> nchar */
#define ADFI_1134_NVCHAR_TO_DATE	(ADI_FI_ID)1134 /* nvarchar -> date */
#define ADFI_1135_DATE_TO_NVCHAR	(ADI_FI_ID)1135 /* date -> nvarchar */
#define ADFI_1136_NCHAR_TO_MONEY	(ADI_FI_ID)1136 /* nchar -> money */
#define ADFI_1137_MONEY_TO_NCHAR	(ADI_FI_ID)1137 /* money -> nchar */
#define ADFI_1138_NVCHAR_TO_MONEY	(ADI_FI_ID)1138 /* nvarchar -> money */
#define ADFI_1139_MONEY_TO_NVCHAR	(ADI_FI_ID)1139 /* money -> nvarchar */
#define ADFI_1140_NCHAR_TO_LOGKEY	(ADI_FI_ID)1140 /* nchar -> logical_key */
#define ADFI_1141_LOGKEY_TO_NCHAR   (ADI_FI_ID)1141 /* logical_key -> nchar */
#define ADFI_1142_NVCHAR_TO_LOGKEY	 (ADI_FI_ID)1142 /* nvarchar -> logical_key */
#define ADFI_1143_LOGKEY_TO_NVCHAR	(ADI_FI_ID)1143 /* logical_key -> nvarchar */
#define ADFI_1144_NCHAR_TO_TABKEY	(ADI_FI_ID)1144 /* nchar -> table_key */
#define ADFI_1145_TABKEY_TO_NCHAR	(ADI_FI_ID)1145 /* table_key -> nchar */
#define ADFI_1146_NVCHAR_TO_TABKEY	(ADI_FI_ID)1146 /* nvarchar -> table_key */
#define ADFI_1147_TABKEY_TO_NVCHAR	(ADI_FI_ID)1147 /* table_key -> nvarchar */
#define ADFI_1156_INT1_NCHAR		(ADI_FI_ID)1156 /* int1(nchar) */
#define ADFI_1157_INT1_NVCHAR		(ADI_FI_ID)1157 /* int1(nvarchar) */
#define ADFI_1158_INT2_NCHAR		(ADI_FI_ID)1158 /* int2(nchar) */
#define ADFI_1159_INT2_NVCHAR		(ADI_FI_ID)1159 /* int2(nvarchar) */
#define ADFI_1160_INT4_NCHAR		(ADI_FI_ID)1160 /* int4(nchar) */
#define ADFI_1161_INT4_NVCHAR		(ADI_FI_ID)1161 /* int4(nvarchar) */
#define ADFI_1162_INT8_NCHAR		(ADI_FI_ID)1162 /* int8(nchar) */
#define ADFI_1163_INT8_NVCHAR		(ADI_FI_ID)1163 /* int8(nvarchar) */
#define ADFI_1164_FLOAT4_NCHAR		(ADI_FI_ID)1164 /* float4(nchar) */
#define ADFI_1165_FLOAT4_NVCHAR		(ADI_FI_ID)1165 /* float4(nvarchar) */
#define ADFI_1166_FLOAT8_NCHAR		(ADI_FI_ID)1166 /* float8(nchar) */
#define ADFI_1167_FLOAT8_NVCHAR		(ADI_FI_ID)1167 /* float8(nvarchar) */
#define ADFI_1168_DEC_NCHAR_I		(ADI_FI_ID)1168 /* decimal(nchar) */
#define ADFI_1169_DEC_NVCHAR_I		(ADI_FI_ID)1169 /* decimal(nvarchar) */
#define ADFI_1170_CHAR_NCHAR		(ADI_FI_ID)1170 /* char(nchar) */
#define ADFI_1171_CHAR_NVCHAR		(ADI_FI_ID)1171 /* char(nchar) */
#define ADFI_1172_VARCHAR_NCHAR		(ADI_FI_ID)1172 /* varchar(nchar) */
#define ADFI_1173_VARCHAR_NVCHAR	(ADI_FI_ID)1173 /* varchar(nvarchar) */
#define ADFI_1174_TEXT_NCHAR		(ADI_FI_ID)1174 /* text(nchar) */
#define ADFI_1175_TEXT_NVCHAR		(ADI_FI_ID)1175 /* text(nvarchar) */
#define ADFI_1176_ASCII_NCHAR		(ADI_FI_ID)1176 /* ascii(nchar) */
#define ADFI_1177_ASCII_NVCHAR		(ADI_FI_ID)1177 /* ascii(nvarchar) */
#define ADFI_1178_DATE_NCHAR		(ADI_FI_ID)1178 /* date(nchar) */
#define ADFI_1179_DATE_NVCHAR		(ADI_FI_ID)1179 /* date(nvarchar) */
#define ADFI_1180_MONEY_NCHAR		(ADI_FI_ID)1180 /* money(nchar) */
#define ADFI_1181_MONEY_NVCHAR		(ADI_FI_ID)1181 /* money(nvarchar) */
#define ADFI_1182_CHAR_COPYTO_UTF8	(ADI_FI_ID)1182 /* char -copy-> utf8 */
#define ADFI_1183_VARCHAR_COPYTO_UTF8	(ADI_FI_ID)1183 /* varchar -copy-> utf8 */
#define ADFI_1184_UTF8_COPYTO_CHAR	(ADI_FI_ID)1184 /* utf8 -copy-> char */
#define ADFI_1185_UTF8_COPYTO_VARCHAR	(ADI_FI_ID)1185 /* utf8 -copy-> varchar */
#define ADFI_1186_CHAR_UTF8		(ADI_FI_ID)1186 /* varchar -copy-> utf8 */
#define ADFI_1187_VARCHAR_UTF8		(ADI_FI_ID)1187 /* char -copy-> utf8 */
#define ADFI_1188_UTF8_CHAR		(ADI_FI_ID)1188 /* utf8 -copy-> char */
#define ADFI_1189_UTF8_VARCHAR		(ADI_FI_ID)1189 /* utf8 -copy-> varchar */
#define	ADFI_1190_COWGT_C		(ADI_FI_ID)1190	/* collation_weight(c) */
#define	ADFI_1191_COWGT_CHAR		(ADI_FI_ID)1191	/* collation_weight(char) */
#define	ADFI_1192_COWGT_VARCHAR		(ADI_FI_ID)1192	/* collation_weight(varchar) */
#define	ADFI_1193_COWGT_TEXT		(ADI_FI_ID)1193	/* collation_weight(text) */
#define	ADFI_1194_COWGT_NCHAR		(ADI_FI_ID)1194	/* collation_weight(nchar) */
#define	ADFI_1195_COWGT_NCHAR_INT	(ADI_FI_ID)1195	/* collation_weight(nchar, int) */
#define	ADFI_1196_COWGT_NVCHAR		(ADI_FI_ID)1196	/* collation_weight(nvchar) */
#define	ADFI_1197_COWGT_NVCHAR_INT	(ADI_FI_ID)1197	/* collation_weight(nvchar, int) */
#define ADFI_1198_CHAR_TO_LONGNVCHAR    (ADI_FI_ID)1198 /* long_nvarchar(char) */
#define ADFI_1199_VCHAR_TO_LONGNVCHAR   (ADI_FI_ID)1199 /* long_nvarchar(varchar) */
#define ADFI_1200_LENGTH_LONGNVCHAR	(ADI_FI_ID)1200 /* length(long_nvarchar) */
#define ADFI_1201_SIZE_LONGNVCHAR	(ADI_FI_ID)1201 /* size(long_nvarchar) */
#define ADFI_1202_ISDST_DATE		(ADI_FI_ID)1202 /* isdst(date) */
#define ADFI_1203_POS_LVCH_LVCH       (ADI_FI_ID)1203 /* position(lvch,lvch) */
#define ADFI_1204_POS_LNVCHR_LNVCHR   (ADI_FI_ID)1204 /* position(lnvch,lnvch)*/
#define ADFI_1205_POS_LBYTE_LBYTE     (ADI_FI_ID)1205 /* position(lbyte,lbyte)*/
#define ADFI_1206_POS_STR_STR_I       (ADI_FI_ID)1206 /* position(str,str,i) */
#define ADFI_1207_POS_UNISTR_UNISTR_I (ADI_FI_ID)1207 /* position(uni,uni,i) */
#define ADFI_1208_POS_VBYTE_VBYTE_I   (ADI_FI_ID)1208 /* position(vbyte,vbyte,i)*/
#define ADFI_1209_POS_STR_LVCH_I      (ADI_FI_ID)1209 /* position(str,lvch,i) */
#define ADFI_1210_POS_UNISTR_LNVCHR_I (ADI_FI_ID)1210 /* position(uni,lnvch,i)*/
#define ADFI_1211_POS_BYTE_LBYTE_I    (ADI_FI_ID)1211 /* position(vbyte,lbyte,i)*/
#define ADFI_1212_POS_LVCH_LVCH_I     (ADI_FI_ID)1212 /* position(lvch,lvch,i) */
#define ADFI_1213_POS_LNVCHR_LNVCHR_I (ADI_FI_ID)1213 /* position(lnvch,lnvch,i)*/
#define ADFI_1214_POS_LBYTE_LBYTE_I   (ADI_FI_ID)1214 /* position(lbyte,lbyte,i)*/
#define ADFI_1219_SUBSTRING_LVCH_I      (ADI_FI_ID)1219 /* substring(lvch,int) */
#define ADFI_1220_SUBSTRING_LBYTE_I	(ADI_FI_ID)1220 /* substring(lbyte,int) */
#define ADFI_1221_SUBSTRING_LNVCHR_I	(ADI_FI_ID)1221 /* substring(lnvchr,int) */
#define ADFI_1222_SUBSTRING_LVCH_I_I	(ADI_FI_ID)1222 /* substring(lvch,int,int) */
#define ADFI_1223_SUBSTRING_LBYTE_I_I	(ADI_FI_ID)1223 /* substring(lbyte,int,int) */
#define ADFI_1224_SUBSTRING_LNVCHR_I_I	(ADI_FI_ID)1224 /* substring(lnvchr,int,int) */
#define ADFI_1233_CURRENT_DATE		(ADI_FI_ID)1233
#define ADFI_1234_CURRENT_TIME		(ADI_FI_ID)1234
#define ADFI_1235_CURRENT_TMSTMP	(ADI_FI_ID)1235
#define ADFI_1236_LOCAL_TIME		(ADI_FI_ID)1236
#define ADFI_1237_LOCAL_TMSTMP		(ADI_FI_ID)1237
#define ADFI_1238_ANSI_DATE		(ADI_FI_ID)1238
#define ADFI_1239_ANSI_TMWO		(ADI_FI_ID)1239
#define ADFI_1240_ANSI_TMW		(ADI_FI_ID)1240
#define ADFI_1241_ANSI_LCTME		(ADI_FI_ID)1241
#define ADFI_1242_ANSI_TSWO		(ADI_FI_ID)1242
#define ADFI_1243_ANSI_TSW		(ADI_FI_ID)1243
#define ADFI_1244_ANSI_TSTMP		(ADI_FI_ID)1244
#define ADFI_1245_ANSI_INDS		(ADI_FI_ID)1245
#define ADFI_1246_ANSI_INYM		(ADI_FI_ID)1246
#define	ADFI_1247_YEARPART		(ADI_FI_ID)1247
#define	ADFI_1248_QUARTERPART		(ADI_FI_ID)1248
#define	ADFI_1249_MONTHPART		(ADI_FI_ID)1249
#define	ADFI_1250_WEEKPART		(ADI_FI_ID)1250
#define	ADFI_1251_WEEKISOPART		(ADI_FI_ID)1251
#define	ADFI_1252_DAYPART		(ADI_FI_ID)1252
#define	ADFI_1253_HOURPART		(ADI_FI_ID)1253
#define	ADFI_1254_MINUTEPART		(ADI_FI_ID)1254
#define	ADFI_1255_SECONDPART		(ADI_FI_ID)1255
#define	ADFI_1256_MICROSECONDPART	(ADI_FI_ID)1256
#define	ADFI_1257_NANOSECONDPART	(ADI_FI_ID)1257
#define ADFI_1258_LNLOC_TO_LNVCHR	(ADI_FI_ID)1258
#define ADFI_1259_LBLOC_TO_LBYTE	(ADI_FI_ID)1259
#define ADFI_1260_LCLOC_TO_LVCH		(ADI_FI_ID)1260
#define ADFI_1261_LNVCHR_TO_LNLOC	(ADI_FI_ID)1261
#define ADFI_1262_LBYTE_TO_LBLOC	(ADI_FI_ID)1262
#define ADFI_1263_LVCH_TO_LCLOC		(ADI_FI_ID)1263
#define ADFI_1264_TMWO_SUB_TMWO         (ADI_FI_ID)1264 /* time - time */ 
#define ADFI_1265_TMW_SUB_TMW		(ADI_FI_ID)1265
#define ADFI_1266_TME_SUB_TME		(ADI_FI_ID)1266 
#define ADFI_1267_TSWO_SUB_TSWO         (ADI_FI_ID)1267 
#define ADFI_1268_TSW_SUB_TSW		(ADI_FI_ID)1268 
#define ADFI_1269_TSTMP_SUB_TSTMP	(ADI_FI_ID)1269 
#define ADFI_1270_ANSIDATE_SUB_ANSIDATE (ADI_FI_ID)1270
#define	ADFI_1271_LTXT_TO_LTXT		(ADI_FI_ID)1271
#define	ADFI_1272_LTXT_EQ_LTXT		(ADI_FI_ID)1272
#define ADFI_1273_NVCHAR_ALL_I		(ADI_FI_ID)1273
#define ADFI_1274_DATE_ADD_INYM		(ADI_FI_ID)1274
#define ADFI_1275_DATE_ADD_INDS		(ADI_FI_ID)1275
#define ADFI_1276_INYM_ADD_DATE		(ADI_FI_ID)1276
#define ADFI_1277_INDS_ADD_DATE		(ADI_FI_ID)1277
#define ADFI_1278_NVCHAR_TO_LONGVCHAR   (ADI_FI_ID)1278 /* long_varchar(nvarchar) */
#define ADFI_1279_VCHAR_TO_LONGNVCHAR   (ADI_FI_ID)1279 /* long_nvarchar(varchar) */
#define ADFI_1280_VARBYTE_ALL_I		(ADI_FI_ID)1280 /* varbyte(all,i) */
#define ADFI_1281_LOWERCASE_LNVCHR	(ADI_FI_ID)1281 /* lowercase(long nvarchar) */
#define ADFI_1282_UPPERCASE_LNVCHR	(ADI_FI_ID)1282 /* uppercase(long nvarchar) */
#define	ADFI_1283_DECROUND		(ADI_FI_ID)1283	/* round(dec, i) */
#define	ADFI_1284_DECTRUNC		(ADI_FI_ID)1284	/* trunc(dec) */
#define ADFI_1285_DECCEIL		(ADI_FI_ID)1285 /* ceil[ing](dec) */
#define ADFI_1286_DECFLOOR		(ADI_FI_ID)1286 /* floor(dec) */
#define ADFI_1287_CHR			(ADI_FI_ID)1287 /* chr(int) */
#define ADFI_1288_LTRIM_CHAR		(ADI_FI_ID)1288 /* ltrim(char) */
#define ADFI_1289_LTRIM_VARCHAR		(ADI_FI_ID)1289 /* ltrim(varchar) */
#define ADFI_1290_LTRIM_NCHAR		(ADI_FI_ID)1290 /* ltrim(nchar) */
#define ADFI_1291_LTRIM_NVCHR		(ADI_FI_ID)1291 /* ltrim(nvarchar) */
#define ADFI_1292_RTRIM_CHAR		(ADI_FI_ID)1292 /* rtrim(char) */
#define ADFI_1293_RTRIM_VARCHAR		(ADI_FI_ID)1293 /* rtrim(varchar) */
#define ADFI_1294_RTRIM_NCHAR		(ADI_FI_ID)1294 /* rtrim(nchar) */
#define ADFI_1295_RTRIM_NVCHR		(ADI_FI_ID)1295 /* rtrim(nvarchar) */
#define ADFI_1296_LOWERCASE_I		(ADI_FI_ID)1296 /* lower(i) ?? */
#define ADFI_1297_UPPERCASE_I		(ADI_FI_ID)1297 /* upper(i) ?? */
#define ADFI_1298_LPAD_VARCHAR		(ADI_FI_ID)1298 /* lpad(vch, n) */
#define ADFI_1299_LPAD_CHAR		(ADI_FI_ID)1299 /* lpad(char, n) */
#define ADFI_1300_LPAD_VARCHAR_VARCHAR	(ADI_FI_ID)1300 /* lpad(vch, n, vch) */
#define ADFI_1301_LPAD_CHAR_VARCHAR	(ADI_FI_ID)1301 /* lpad(char, n, vch) */
#define ADFI_1302_RPAD_VARCHAR		(ADI_FI_ID)1302 /* rpad(vch, n) */
#define ADFI_1303_RPAD_CHAR		(ADI_FI_ID)1303 /* rpad(char, n) */
#define ADFI_1304_RPAD_VARCHAR_VARCHAR	(ADI_FI_ID)1304 /* rpad(vch, n, vch) */
#define ADFI_1305_RPAD_CHAR_VARCHAR	(ADI_FI_ID)1305 /* rpad(char, n, vch) */
#define ADFI_1306_REPL_VARCH_VARCH_VARCH (ADI_FI_ID)1306 /* replace(vch, vch, vch) */
#define ADFI_1307_REPL_CHAR_VARCH_VARCH (ADI_FI_ID)1307 /* replace(char, vch, vch) */
#define ADFI_1308_ACOS_F		(ADI_FI_ID)1308 /* acos(f) */
#define ADFI_1309_ASIN_F		(ADI_FI_ID)1309 /* asin(f) */
#define ADFI_1310_TAN_F			(ADI_FI_ID)1310 /* tan(f) */
#define ADFI_1311_PI			(ADI_FI_ID)1311 /* pi() */
#define ADFI_1312_SIGN_I		(ADI_FI_ID)1312 /* sign(int) */
#define ADFI_1313_SIGN_F		(ADI_FI_ID)1313 /* sign(flt) */
#define ADFI_1314_SIGN_DEC		(ADI_FI_ID)1314 /* sign(dec) */
#define ADFI_1315_ATAN2_F_F		(ADI_FI_ID)1315 /* atan2(f, f) */
#define ADFI_1316_C_ADD_CHAR		(ADI_FI_ID)1316 /* c + char */
#define ADFI_1317_CHAR_ADD_C		(ADI_FI_ID)1317 /* char + c */
#define ADFI_1318_C_ADD_VARCHAR		(ADI_FI_ID)1318 /* c + varchar */
#define ADFI_1319_NCHR_TO_DEC		(ADI_FI_ID)1319 /* nchar -> decimal */
#define ADFI_1320_NVCHR_TO_DEC		(ADI_FI_ID)1320 /* nvarchar -> decimal */
#define ADFI_1321_C_TO_I		(ADI_FI_ID)1321 /* c -> i */
#define ADFI_1322_TXT_TO_I		(ADI_FI_ID)1322 /* text -> i */
#define ADFI_1323_CHAR_TO_I		(ADI_FI_ID)1323 /* char -> i */
#define ADFI_1324_VARCHAR_TO_I		(ADI_FI_ID)1324 /* varchar -> i */
#define ADFI_1325_NCHAR_TO_I		(ADI_FI_ID)1325 /* nchar -> i */
#define ADFI_1326_NVARCHAR_TO_I		(ADI_FI_ID)1326 /* nvarchar -> i*/
#define ADFI_1327_C_TO_F		(ADI_FI_ID)1327 /* c -> f */
#define ADFI_1328_TXT_TO_F		(ADI_FI_ID)1328 /* text -> f */
#define ADFI_1329_CHAR_TO_F		(ADI_FI_ID)1329 /* char -> f */
#define ADFI_1330_VARCHAR_TO_F		(ADI_FI_ID)1330 /* varchar -> f */
#define ADFI_1331_NCHAR_TO_F		(ADI_FI_ID)1331 /* nchar -> f */
#define ADFI_1332_NVARCHAR_TO_F		(ADI_FI_ID)1332 /* nvarchar -> f */
#define ADFI_1333_I_TO_C		(ADI_FI_ID)1333 /* i -> c */
#define ADFI_1334_F_TO_C		(ADI_FI_ID)1334 /* f -> c */
#define ADFI_1335_I_TO_TXT		(ADI_FI_ID)1335 /* i -> text */
#define ADFI_1336_F_TO_TXT		(ADI_FI_ID)1336 /* f -> text */
#define ADFI_1337_I_TO_CHAR		(ADI_FI_ID)1337 /* i -> char */
#define ADFI_1338_F_TO_CHAR		(ADI_FI_ID)1338 /* f -> char */
#define ADFI_1339_I_TO_VARCHAR		(ADI_FI_ID)1339 /* i -> varchar */
#define ADFI_1340_F_TO_VARCHAR		(ADI_FI_ID)1340 /* f -> varchar */
#define ADFI_1341_DEC_TO_NCHAR		(ADI_FI_ID)1341 /* decimal -> nchar */
#define ADFI_1342_I_TO_NCHAR		(ADI_FI_ID)1342 /* i -> nchar */
#define ADFI_1343_F_TO_NCHAR		(ADI_FI_ID)1343 /* f -> nchar */
#define ADFI_1344_DEC_TO_NVCHAR		(ADI_FI_ID)1344 /* decimal -> nvarchar -> */
#define ADFI_1345_I_TO_NVCHAR		(ADI_FI_ID)1345 /* i -> nvarchar */
#define ADFI_1346_F_TO_NVCHAR		(ADI_FI_ID)1346 /* f -> nvarchar */
#define ADFI_1347_NUMERIC_EQ		(ADI_FI_ID)1347 /* all = all */
#define ADFI_1348_NUMERIC_NE		(ADI_FI_ID)1348 /* all <> all */
#define ADFI_1349_NUMERIC_LT		(ADI_FI_ID)1349 /* all < all */
#define ADFI_1350_NUMERIC_GE		(ADI_FI_ID)1350 /* all >= all */
#define ADFI_1351_NUMERIC_GT		(ADI_FI_ID)1351 /* all > all */
#define ADFI_1352_NUMERIC_LE		(ADI_FI_ID)1352 /* all <= all */
#define ADFI_1353_IPADDR_C		(ADI_FI_ID)1353 /* ii_ipaddr(c) */
#define ADFI_1354_IPADDR_CHAR		(ADI_FI_ID)1354 /* ii_ipaddr(char) */
#define ADFI_1355_IPADDR_TEXT		(ADI_FI_ID)1355 /* ii_ipaddr(text) */
#define ADFI_1356_IPADDR_VARCHAR	(ADI_FI_ID)1356 /* ii_ipaddr(varchar) */
#define ADFI_1357_ALL_ISINT		(ADI_FI_ID)1357 /* all IS INTEGER */
#define ADFI_1358_ALL_ISNOTINT		(ADI_FI_ID)1358 /* all IS NOT INTEGER */
#define ADFI_1359_ALL_ISDEC		(ADI_FI_ID)1359 /* all IS DECIMAL */
#define ADFI_1360_ALL_ISNOTDEC		(ADI_FI_ID)1360 /* all IS NOT DECIMAL */
#define ADFI_1361_ALL_ISFLT		(ADI_FI_ID)1361 /* all IS FLOAT */
#define ADFI_1362_ALL_ISNOTFLT		(ADI_FI_ID)1362 /* all IS NOT FLOAT */
#define ADFI_1363_NCHAR_ALL_I		(ADI_FI_ID)1363 /* nchar(all,i) */
#define ADFI_1364_DTE_MUL_F		(ADI_FI_ID)1364 /* dte * f */
#define ADFI_1365_F_MUL_DTE		(ADI_FI_ID)1365 /* f * dte */
#define ADFI_1366_DTE_DIV_F		(ADI_FI_ID)1366 /* dte / f */
#define ADFI_1367_DTE_DIV_DTE		(ADI_FI_ID)1367 /* dte / dte */
#define ADFI_1368_MINUS_DTE		(ADI_FI_ID)1368 /* - dte */
#define ADFI_1369_PLUS_DTE		(ADI_FI_ID)1369 /* + dte */
#define ADFI_1370_VARBYTE_TO_LOGKEY	(ADI_FI_ID)1370 /* varbyte -> logkey */
#define ADFI_1371_LOGKEY_TO_VARBYTE	(ADI_FI_ID)1371 /* logkey -> varbyte */
#define ADFI_1372_VARBYTE_TO_TABKEY	(ADI_FI_ID)1372 /* varbyte -> table_key */
#define ADFI_1373_TABKEY_TO_VARBYTE	(ADI_FI_ID)1373 /* table_key -> varbyte */
#define ADFI_1374_VARBYTE_TO_LVCH	(ADI_FI_ID)1374 /* varbyte -> long varchar */
#define ADFI_1375_NUMERIC_NORM		(ADI_FI_ID)1375 /* all -> byte */
#define ADFI_1376_LVCH_LIKE_VCH		(ADI_FI_ID)1376 /* long varchar like varchar */
#define ADFI_1377_LVCH_NOTLIKE_VCH	(ADI_FI_ID)1377 /* long varchar not like varchar */
#define ADFI_1378_LNVCHR_LIKE_NVCHR	(ADI_FI_ID)1378 /* long nvarchar like nvarchar */
#define ADFI_1379_LNVCHR_NOTLIKE_NVCHR	(ADI_FI_ID)1379 /* long nvarchar not like nvarchar */
/* horda03 - ADFI_1380 to ADFI_1387 used for string concatenation bug 120634 in R1
**           these codes must not be reused
*/
#define ADFI_1380_CONCAT_CHAR_C         DO NOT USE
#define ADFI_1381_CONCAT_CHAR_TEXT      DO NOT USE
#define ADFI_1382_CONCAT_TEXT_C         DO NOT USE
#define ADFI_1383_CONCAT_C_CHAR         DO NOT USE
#define ADFI_1384_CHAR_ADD_TEXT         DO NOT USE
#define ADFI_1385_TEXT_ADD_C            DO NOT USE
#define ADFI_1386_C_ADD_CHAR            DO NOT USE
#define ADFI_1387_C_ADD_TEXT            DO NOT USE
#define ADFI_1388_UNORM_UTF8		(ADI_FI_ID)1388 /* str -> vchar */
#define ADFI_1389_UNORM_UNICODE		(ADI_FI_ID)1389 /* unistr -> nvchar*/
#define ADFI_1390_LOGKEY_TO_BYTE	(ADI_FI_ID)1390 /* logkey -> byte */
#define ADFI_1391_TABKEY_TO_BYTE	(ADI_FI_ID)1391 /* tabkey -> byte */
#define ADFI_1392_PATCOMP_VCH		(ADI_FI_ID)1392 /* pattern = pat_compile(varchar) */
#define ADFI_1393_PATCOMP_CHA		(ADI_FI_ID)1393 /* pattern = pat_compile(char) */
#define ADFI_1394_PATCOMP_NVCHR		(ADI_FI_ID)1394 /* pattern = pat_compile(nvarchar) */
#define ADFI_1395_DATE_SUB_VCH		(ADI_FI_ID)1395 /* date - vch -> date */
#define ADFI_1396_DATE_ADD_VCH		(ADI_FI_ID)1396 /* date + vch -> date */
#define ADFI_1397_LNVCHR_TO_LVCH	(ADI_FI_ID)1397	/* lnvarchar -> lvarchar */
#define ADFI_1398_LVCH_TO_LNVCHR	(ADI_FI_ID)1398	/* lvarchar -> lnvarchar */
#define ADFI_1399_LVCH_LNVCHR		(ADI_FI_ID)1399	/* long_varchar(lnvarchar) */
#define ADFI_1400_LNVCHR_LVCH		(ADI_FI_ID)1400	/* long_nvarchar(lvarchar) */
#define ADFI_1401_LNVCHR_LBYTE		(ADI_FI_ID)1401	/* long_nvarchar(lbyte) */
#define ADFI_1402_LNVCHR_LNVCHR		(ADI_FI_ID)1402	/* long_nvarchar(long_nvarchar) */
#define ADFI_1403_LNVCHR_CHAR		(ADI_FI_ID)1403	/* long_nvarchar(char) */
#define ADFI_1404_LNVCHR_C		(ADI_FI_ID)1404	/* long_nvarchar(c) */
#define ADFI_1405_LNVCHR_TEXT		(ADI_FI_ID)1405	/* long_nvarchar(text) */
#define ADFI_1406_LNVCHR_VARCHAR	(ADI_FI_ID)1406	/* long_nvarchar(varchar) */
#define ADFI_1407_LNVCHR_LTXT		(ADI_FI_ID)1407	/* long_nvarchar(ltxt) */
#define ADFI_1408_LCLOC_TO_LBYTE	(ADI_FI_ID)1408 /* long varchar locator -> long byte */
#define ADFI_1409_LCLOC_TO_LNVCHR	(ADI_FI_ID)1409 /* long varchar locator -> long nvarchar */
#define ADFI_1410_LBLOC_TO_LVCH		(ADI_FI_ID)1410 /* long byte locator -> long varchar */
#define ADFI_1411_LBLOC_TO_LNVCHR	(ADI_FI_ID)1411 /* long byte locator -> long nvarchar */
#define ADFI_1412_LNLOC_TO_LBYTE	(ADI_FI_ID)1412 /* long nvarchar locator -> lbyte */
#define ADFI_1413_LNLOC_TO_LVCH		(ADI_FI_ID)1413 /* long nvarchar locator -> long varchar */
#define ADFI_1414_LNVCHR_TO_LBYTE	(ADI_FI_ID)1414	/* lnvarchar -> lbyte */
#define ADFI_1415_LBYTE_TO_LNVCHR	(ADI_FI_ID)1415	/* lbyte -> lnvarchar */
#define ADFI_1416_LBYTE_LNVCHR		(ADI_FI_ID)1416 /* lbyte(long_nvarchar) */
#define ADFI_1417_BYTE_ALL_I		(ADI_FI_ID)1417 /* byte(all,i) */
#define ADFI_1418_BOO_ISFALSE       (ADI_FI_ID)1418 /* boolean IS FALSE */
#define ADFI_1419_BOO_NOFALSE       (ADI_FI_ID)1419 /* boolean IS NOT FALSE */
#define ADFI_1420_BOO_ISTRUE        (ADI_FI_ID)1420 /* boolean IS TRUE */
#define ADFI_1421_BOO_NOTRUE        (ADI_FI_ID)1421 /* boolean IS NOT TRUE */
#define ADFI_1422_BOO_TO_BOO        (ADI_FI_ID)1422 /* boolean -> boolean */
#define ADFI_1423_LTXT_TO_BOO       (ADI_FI_ID)1423 /* ltxt -> boolean */
#define ADFI_1424_VCH_TO_BOO        (ADI_FI_ID)1424 /* varchar -> boolean */
#define ADFI_1425_BOO_EQ_BOO        (ADI_FI_ID)1425 /* boolean = boolean */
#define ADFI_1426_BOO_NE_BOO        (ADI_FI_ID)1426 /* boolean != boolean */
#define ADFI_1427_BOO_LE_BOO        (ADI_FI_ID)1427 /* boolean <= boolean */
#define ADFI_1428_BOO_GT_BOO        (ADI_FI_ID)1428 /* boolean > boolean */
#define ADFI_1429_BOO_GE_BOO        (ADI_FI_ID)1429 /* boolean >= boolean */
#define ADFI_1430_BOO_LT_BOO        (ADI_FI_ID)1430 /* boolean < boolean */
#define ADFI_1431_BOO_ISUNKN        (ADI_FI_ID)1431 /* boolean IS UNKNOWN */
#define ADFI_1432_BOO_NOUNKN        (ADI_FI_ID)1432 /* boolean IS NOT UNKNOWN */
#define ADFI_1433_BOO_COPYTO_C      (ADI_FI_ID)1433 /* boolean -copy-> c */
#define ADFI_1434_BOO_COPYTO_TEXT   (ADI_FI_ID)1434 /* boolean -copy-> text */
#define ADFI_1435_BOO_COPYTO_CHAR   (ADI_FI_ID)1435 /* boolean -copy-> char */
#define ADFI_1436_BOO_COPYTO_VCH    (ADI_FI_ID)1436 /* boolean -copy-> varchar */
#define ADFI_1437_C_COPYTO_BOO      (ADI_FI_ID)1437 /* c -copy-> boolean */
#define ADFI_1438_TEXT_COPYTO_BOO   (ADI_FI_ID)1438 /* text -copy-> boolean */
#define ADFI_1439_CHAR_COPYTO_BOO   (ADI_FI_ID)1439 /* char -copy-> boolean */
#define ADFI_1440_VCH_COPYTO_BOO    (ADI_FI_ID)1440 /* varchar -copy-> boolean */
#define ADFI_1441_LAST_IDENTITY		(ADI_FI_ID)1441 /* last_identity() */
#define ADFI_1442_BOO_CHAR          (ADI_FI_ID)1442 /* boolean(char) */
#define ADFI_1443_BOO_VCH           (ADI_FI_ID)1443 /* boolean(varchar) */
#define ADFI_1444_BOO_C             (ADI_FI_ID)1444 /* boolean(c) */
#define ADFI_1445_BOO_TEXT          (ADI_FI_ID)1445 /* boolean(text) */
#define ADFI_1446_CHAR_TO_BOO       (ADI_FI_ID)1446 /* char -> boolean */
#define ADFI_1447_C_TO_BOO          (ADI_FI_ID)1447 /* c -> boolean */
#define ADFI_1448_TEXT_TO_BOO       (ADI_FI_ID)1448 /* text -> boolean */
#define ADFI_1449_I_TO_BOO          (ADI_FI_ID)1449 /* i -> boolean */
#define ADFI_1450_BOO_I             (ADI_FI_ID)1450 /* boolean(i) */
#define ADFI_1451_CMPTVER	    (ADI_FI_ID)1451 /* iicmptversion(i):vch */
#define ADFI_1452_GREATEST_ALL_ALL	(ADI_FI_ID)1452 /* all := greatest(all,all) */
#define ADFI_1453_LEAST_ALL_ALL		(ADI_FI_ID)1453 /* all := least(all,all) */
#define ADFI_1454_GREATER_ALL_ALL	(ADI_FI_ID)1454 /* all := greater(all,all) */
#define ADFI_1455_LESSER_ALL_ALL	(ADI_FI_ID)1455 /* all := lesser(all,all) */
#define ADFI_1456_FLTROUND          (ADI_FI_ID)1456     /* round(flt, i) */
#define ADFI_1457_NVL2_ANY		    (ADI_FI_ID)1457 /* nvl2(all,all,all) */
#define ADFI_1458_GENERATEDIGIT  	(ADI_FI_ID)1458 /* generate_digit() */
#define ADFI_1459_VALIDATEDIGIT	        (ADI_FI_ID)1459 /* validate_digit() */
#define ADFI_1460_BOO_BOO           (ADI_FI_ID)1460 /* boolean(boolean) */
#define ADFI_1461_REPL_NVCH_NVCH_NVCH	(ADI_FI_ID)1461 /* replace(nvarchar,nvarchar,nvarchar) */
#define ADFI_1462_REPL_VBYT_VBYT_VBYT	(ADI_FI_ID)1462 /* replace(vbyte,vbyte,vbyte) */
#define ADFI_1463_SINGLETON_ALL		(ADI_FI_ID)1463 /* all := singleton(all) */
#define ADFI_1464_AES_DECRYPT_VARBYTE   (ADI_FI_ID)1464 /* decrypt varbyte */
#define ADFI_1465_AES_ENCRYPT_VARBYTE   (ADI_FI_ID)1465 /* encrypt varbyte */

