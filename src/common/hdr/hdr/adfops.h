#ifndef __ADFOPS_H_INCLUDED
#define __ADFOPS_H_INCLUDED
/*
** Copyright (c) 2004,2008 Ingres Corporation
*/

/**
** Name: ADFOPS.H - This file contains the defines of the operators
**                  known to ADF
**
** Description:
**      The parser treats operators slightly different than it treats
**      functions.  Because of this, the parser needs to know the
**      operator ids of all of the operators known to the ADF.  These
**      will also be available through the ADF call "adi_opid()", and
**      therefore must have operator ids that are unique from those
**      assigned to functions.  ADF will reserve a range of operator
**      ids for operators.  This range has been decreed to be 1 - 99.
**      Therefore, when defining new functions to work on an ADT, be
**      sure that The operator ids assigned to them lie outside this
**      range.  Operators will be different from functions in that they
**      will be known to the parser by symbolic constants.  No other
**      symbolic constants for operator ids will be know outside the
**      ADF.  Furthermore, only the parser is currently authorized to
**      "see" those constants defined here.
**
** History: $Log-for RCS$
**      03-feb-86 (thurston)
**          Initial creation.
**      03-mar-86 (thurston)
**          Put the  cast on #defines to please lint.  Now this
**	    file is dependent on the ADF.H header file.
**      26-mar-86 (thurston)
**	    Re-defined all of the symbolic constants to be consistent with
**	    the way ADF uses them.  (That is, the operations table is now
**	    in lexical order by operation name, therefore these constants
**	    have been given specific values.)
**	01-oct-86 (thurston)
**	    Added ADI_LIKE_OP and ADI_NLIKE_OP.
**	11-nov-86 (thurston)
**	    Added ADI_HEXIN_OP.
**	07-jan-87 (thurston)
**	    Removed ADI_HEXIN_OP since the parser is now handling hex constants
**	    right in the grammar.
**	09-mar-87 (thurston)
**	    Added ADI_ISNUL_OP, ADI_NONUL_OP, and ADI_CNTAL_OP.
**	08-apr-87 (thurston)
**	    Moved ADI_ANY_OP into this header file from an internal ADF header
**	    file for OPC to use.
**	28-may-87 (thurston)
**	    Added ADI_EXIST_OP.
**	18-jun-87 (thurston)
**	    Added ADI_NXIST_OP.
**	27-apr-88 (thurston)
**	    Added ADI_DEC_OP.
**	15-Feb-89 (andre)
**	    Added ADI_DBMSI_OP (previously removed from adfint.h)
**	03-mar-89 (andre)
**	    Added opcodes for the constant functions (previously removed from
**	    adfint.h)
**	20-may-89 (andre)
**	    moved ADI_CNT_OP from adfint.h so that PSF would see it.
**      03-dec-1992 (stevet)
**          Added ADI_SESSUSER_OP, ADI_INITUSER_OP and ADI_SYSUSER_OP
**          for FIPS functions.
**      10-jan-1993 (stevet)
**          Moved ADI_VARCH_OP, ADI_CHAR_OP, ADI_ASCII_OP and ADI_TEXT_OP
**          from adfint.h to here.
**      14-jun-1993 (stevet)
**          Added new operator ADI_NULJN_OP to handle nullable key join.
**	20-Apr-1999 (shero03)
**	    Add substring(c FROM n [FOR l])
**	6-jul-00 (inkdo01)
**	    Added random operators to make them visible in OPF.
**	18-dec-00 (inkdo01)
**	    Added aggregate operators to make them visible in OPF.
**      16-apr-2001 (stial01)
**          Added support for iitableinfo function
**      7-nov-01 (inkdo01)
**          Copied ADI_TRIM_OP (for ANSI support) and added ADI_POS_OP to make
**          them visible in PSF.
**	27-may-02 (inkdo01)
**	    Dropped random operators. Alternate solution was found that no longer
**	    requires them to be visible outside of ADF.
**      11-nov-2002 (stial01)
**          Added ADI_RESTAB_OP (b109101)
**	17-jul-2003 (toumi01)
**	    Add to PSF some flattening decisions.
**	    to make ADI_IFNUL_OP visible to PSF move #define from
**	    common/adf/hdr/adfint.h to common/hdr/hdr/adfops.h
**	26-nov-2003 (gupsh01)
**	    Added new operators for functions nchar(), nvarchar(), iiucs_to_utf8, 
**	    iiutf8_to_ucs().
**	21-jan-2004 (gupsh01)
**	    Modified operator id for nchar,ncharchar,iiucs_to_utf8,iiutf8_to_ucs
**	    so that they do not conflict with the ANSI char_length() etc.
**	30-june-05 (inkdo01)
**	    Copied "float8" from internal ADF for avg ==> sum/count transform.
**	20-dec-2006 (dougi)
**	    Added date/time registers so they can be used as column defaults.
**	15-jan_2007 (dougi)
**	    Copied ADI+DPART_OP to allow "extract" implementation in parser.
**	07-Mar-2007 (kiria01) b117768
**	    Moved ADI_CNCAT_OP from adfint.h as it is also accessible
**	    as infix operator:  strA || atrB  ==  CONCAT(strA, strB)
**	21-Mar-2007 (kiria01) b117892
**	    Consolidated operator/function code definition into one table macro.
**	02-Apr-2007 b117892
**	    Added main line symbols not present in 9.0.4.
**	2-apr-2007 (dougi)
**	    Added ROUND for round() function.
**	13-apr-2007 (dougi)
**	    Added ceil(), floor(), trunc(), chr(), ltrim() and rtrim() 
**	    functions for various partner interfaces.
**	16-apr-2007 (dougi)
**	    Then add lpad(), rpad() and replace().
**	2-may-2007 (dougi)
**	    And tan(), acos(), asin(), pi() and sign().
**      22-Aug-2007 (kiria01) b118999
**          Added byteextract
**	26-Dec-2007 (kiria01) SIR119658
**	    Added 'is integer/decimal/float' operators.
**      10-sep-2008 (gupsh01,stial01)
**          Added unorm operator
**	27-Oct-2008 (kiria01) SIR120473
**	    Added patcomp
**      17-Dec-2008 (macde01)
**          Added point,x, and y operators.
**  28-Feb-2009 (thich01)
**      Added operators for the spatial types
**  13-Mar-2009 (thich01)
**      Added WKT and WKB operators to support asText and asBinary
**	12-Mar-2009 (kira01) SIR121788
**	    Added long_nvarchar
**	22-Apr-2009 (Marty Bowes) SIR 121969
**          Add generate_digit() and validate_digit() for the generation
**          and validation of checksum values.
**      01-Aug-2009 (martin bowes) SIR122320
**          Added soundex_dm (Daitch-Mokotoff soundex)
**  25-Aug-2009 (troal01)
**      Added GEOMNAME and GEOMDIMEN
**	9-sep-2009 (stephenb)
**	    Add last_identity
**      25-sep-2009 (joea)
**          Add ISFALSE, NOFALSE, ISTRUE, NOTRUE, ISUNKN and NOUNKN.
**      23-oct-2009 (joea)
**          Add BOO (boolean).
**	12-Nov-2009 (kschendel) SIR 122882
**	    Add iicmptversion.
**	14-Nov-2009 (kiria01) SIR 122894
**	    Added GREATEST and LEAST
**	07-Dec-2009 (drewsturgiss)
**	    Added NVL and NVL2
**      09-Mar-2010 (thich01)
**          Add union and change geom to perimeter to stay consistent with
**          other spatial operators.
**	18-Mar-2010 (kiria01) b123438
**	    Added SINGLETON aggregate for scalar sub-query support.
**	25-Mar-2010 (toumi01) SIR 122403
**	    Add AES_DECRYPT and AES_ENCRYPT.
**	xx-Apr-2010 (iwawong)
**		Added issimple isempty x y numpoints
**		for the coordinate space.
**	14-Jul-2010 (kschendel) b123104
**	    Add ii_true and ii_false to solve outer join constant folding bug.
**	28-Jul-2010 (kiria01) b124142
**	    Added SINGLECHK
**/

/*
**  Defines of other constants.
*/

/*: ADF Operation IDs
**      These are symbolic constants to use for the ADF operation codes,
**      otherwise known as op-id's.
**	The table macro serves to keep the definitions in one place, ensures
**	that they are kept contiguous and provides a means of extracting the
**	symbol name for logging.
**
**	DANGER AND WARNING
**
**	adgfitab.roc is sorted (by hand == human) based upon these operator ids.
**	Changing these operator ids requires that adgfitab.roc be resorted, and
**	that those changes be carried back to adgoptab.roc as well.
**
**	Best Advice:	Don't change the operator id's
*/
/*: PSF and OPF Visible Op IDs
**      These are the symbolic constants defined for the operator ids of the
**      operators known to the ADF.  The parser and optimizer are currently
**      the only facilities authorized to use these symbolic constants outside
**	of ADF.
*/


#define ADI_OPS_MACRO \
_DEFINE(ILLEGAL,        -1  /* sentinel value; not a legal op     */)\
_DEFINE(NE,              0  /* !=                      */)\
_DEFINE(MUL,             1  /* *                       */)\
_DEFINE(POW,             2  /* **                      */)\
_DEFINE(ADD,             3  /* +                       */)\
_DEFINE(SUB,             4  /* -                       */)\
_DEFINE(DIV,             5  /* /                       */)\
_DEFINE(LT,              6  /* <                       */)\
_DEFINE(LE,              7  /* <=                      */)\
_DEFINE(EQ,              8  /* =                       */)\
_DEFINE(GT,              9  /* >                       */)\
_DEFINE(GE,             10  /* >=                      */)\
_DEFINE(BNTIM,          11  /* _bintim                 */)\
_DEFINE(BIOC,           12  /* _bio_cnt                */)\
_DEFINE(CHRD,           13  /* _cache_read             */)\
_DEFINE(CHREQ,          14  /* _cache_req              */)\
_DEFINE(CHRRD,          15  /* _cache_rread            */)\
_DEFINE(CHSIZ,          16  /* _cache_size             */)\
_DEFINE(CHUSD,          17  /* _cache_used             */)\
_DEFINE(CHWR,           18  /* _cache_write            */)\
_DEFINE(CPUMS,          19  /* _cpu_ms                 */)\
_DEFINE(BDATE,          20  /* _date                   */)\
_DEFINE(DIOC,           21  /* _dio_cnt                */)\
_DEFINE(ETSEC,          22  /* _et_sec                 */)\
_DEFINE(PFLTC,          23  /* _pfault_cnt             */)\
_DEFINE(PHPAG,          24  /* _phys_page              */)\
_DEFINE(BTIME,          25  /* _time                   */)\
_DEFINE(VERSN,          26  /* _version                */)\
_DEFINE(WSPAG,          27  /* _ws_page                */)\
_DEFINE(ABS,            28  /* abs                     */)\
_DEFINE(ANY,            29  /* any                     */)\
_DEFINE(ASCII,          30  /* ascii or c              */)\
_DEFINE(ATAN,           31  /* atan                    */)\
_DEFINE(AVG,            32  /* avg                     */)\
_DEFINE(DBMSI,          33  /* dbmsinfo                */)\
_DEFINE(CHAR,           34  /* char                    */)\
_DEFINE(CNCAT,          35  /* concat                  */)\
_DEFINE(TYNAM,          36  /* typename                */)\
_DEFINE(COS,            37  /* cos                     */)\
_DEFINE(CNT,            38  /* count                   */)\
_DEFINE(CNTAL,          39  /* count*                  */)\
_DEFINE(DATE,           40  /* date                    */)\
_DEFINE(DPART,          41  /* date_part               */)\
_DEFINE(DTRUN,          42  /* date_trunc              */)\
_DEFINE(DBA,            43  /* dba                     */)\
_DEFINE(DOW,            44  /* dow                     */)\
_DEFINE(ISNUL,          45  /* is null                 */)\
_DEFINE(NONUL,          46  /* is not null             */)\
_DEFINE(EXP,            47  /* exp                     */)\
_DEFINE(F4,             48  /* float4                  */)\
_DEFINE(F8,             49  /* float8                  */)\
_DEFINE(I1,             50  /* int1                    */)\
_DEFINE(I2,             51  /* int2                    */)\
_DEFINE(I4,             52  /* int4                    */)\
_DEFINE(INTVL,          53  /* interval                */)\
_DEFINE(LEFT,           54  /* left                    */)\
_DEFINE(LEN,            55  /* length                  */)\
_DEFINE(LOC,            56  /* locate                  */)\
_DEFINE(LOG,            57  /* log                     */)\
_DEFINE(LOWER,          58  /* lowercase               */)\
_DEFINE(MAX,            59  /* max                     */)\
_DEFINE(MIN,            60  /* min                     */)\
_DEFINE(MOD,            61  /* mod                     */)\
_DEFINE(MONEY,          62  /* money                   */)\
_DEFINE(PAD,            63  /* pad                     */)\
_DEFINE(RIGHT,          64  /* right                   */)\
_DEFINE(SHIFT,          65  /* shift                   */)\
_DEFINE(SIN,            66  /* sin                     */)\
_DEFINE(SIZE,           67  /* size                    */)\
_DEFINE(SQRT,           68  /* sqrt                    */)\
_DEFINE(SQZ,            69  /* squeeze                 */)\
_DEFINE(SUM,            70  /* sum                     */)\
_DEFINE(EXIST,          71  /* exists                  */)\
_DEFINE(TEXT,           72  /* text or vchar           */)\
_DEFINE(TRIM,           73  /* trim                    */)\
_DEFINE(PLUS,           74  /* u+                      */)\
_DEFINE(MINUS,          75  /* u-                      */)\
_DEFINE(UPPER,          76  /* uppercase               */)\
_DEFINE(USRLN,          77  /* iiuserlen               */)\
_DEFINE(USRNM,          78  /* username                */)\
_DEFINE(VARCH,          79  /* varchar                 */)\
_DEFINE(LIKE,           80  /* like                    */)\
_DEFINE(NLIKE,          81  /* not like                */)\
_DEFINE(HEX,            82  /* hex                     */)\
_DEFINE(IFNUL,          83  /* ifnull                  */)\
_DEFINE(STRUC,          84  /* iistructure             */)\
_DEFINE(DGMT,           85  /* date_gmt                */)\
_DEFINE(CHA12,          86  /* iichar12                */)\
_DEFINE(CHEXT,          87  /* charextract             */)\
_DEFINE(NTRMT,          88  /* ii_notrm_txt            */)\
_DEFINE(NTRMV,          89  /* ii_notrm_vch            */)\
_DEFINE(NXIST,          90  /* not exists              */)\
_DEFINE(XYZZY,          91  /* xyzzy                   */)\
_DEFINE(HXINT,          92  /* iihexint                */)\
_DEFINE(TB2DI,          93  /* ii_tabid_di             */)\
_DEFINE(DI2TB,          94  /* ii_di_tabid             */)\
_DEFINE(DVDSC,          95  /* ii_dv_desc              */)\
_DEFINE(DEC,            96  /* decimal                 */)\
_DEFINE(ETYPE,          97  /* ii_ext_type             */)\
_DEFINE(ELENGTH,        98  /* ii_ext_length           */)\
_DEFINE(LOGKEY,         99  /* object_key              */)\
_DEFINE(TABKEY,        100  /* table_key               */)\
_DEFINE(101,           101  /*   Unused                */)\
_DEFINE(IFTRUE,        102  /* ii_iftrue               */)\
_DEFINE(RESTAB,        103  /* resolve_table           */)\
_DEFINE(GMTSTAMP,      104  /* gmt_timestamp           */)\
_DEFINE(CPNDMP,        105  /* ii_cpn_dump             */)\
_DEFINE(BIT,           106  /* bit                     */)\
_DEFINE(VBIT,          107  /* varbit                  */)\
_DEFINE(ROWCNT,        108  /* ii_row_count            */)\
_DEFINE(SESSUSER,      109  /* session_user            */)\
_DEFINE(SYSUSER,       110  /* system_user             */)\
_DEFINE(INITUSER,      111  /* initial_user            */)\
_DEFINE(ALLOCPG,       112  /* iitotal_allocated_pages */)\
_DEFINE(OVERPG,        113  /* iitotal_overflow_pages  */)\
_DEFINE(BYTE,          114  /* byte                    */)\
_DEFINE(VBYTE,         115  /* varbyte                 */)\
_DEFINE(LOLK,          116  /* ii_lolk(large obj)      */)\
_DEFINE(NULJN,         117  /* a = a or null=null      */)\
_DEFINE(PTYPE,         118  /* ii_permit_type(int)     */)\
_DEFINE(SOUNDEX,       119  /* soundex(char)           */)\
_DEFINE(BDATE4,        120  /* _date4(int)             */)\
_DEFINE(INTEXT,        121  /* intextract              */)\
_DEFINE(ISDST,         122  /* isdst()                 */)\
_DEFINE(CURDATE,       123  /* CURRENT_DATE            */)\
_DEFINE(CURTIME,       124  /* CURRENT_TIME            */)\
_DEFINE(CURTMSTMP,     125  /* CURRENT_TIMESTAMP       */)\
_DEFINE(LCLTIME,       126  /* LOCAL_TIME              */)\
_DEFINE(LCLTMSTMP,     127  /* LOCAL_TIMESTAMP         */)\
_DEFINE(SESSION_PRIV,  128  /* session_priv            */)\
_DEFINE(IITBLSTAT,     129  /* iitblstat               */)\
_DEFINE(130,           130  /*   Unused                */)\
_DEFINE(LVARCH,        131  /* long_varchar            */)\
_DEFINE(LBYTE,         132  /* long_byte               */)\
_DEFINE(BIT_ADD,       133  /* bit_add                 */)\
_DEFINE(BIT_AND,       134  /* bit_and                 */)\
_DEFINE(BIT_NOT,       135  /* bit_not                 */)\
_DEFINE(BIT_OR,        136  /* bit_or                  */)\
_DEFINE(BIT_XOR,       137  /* bit_xor                 */)\
_DEFINE(IPADDR,        138  /* ii_ipaddr(c)            */)\
_DEFINE(HASH,          139  /* hash(all)               */)\
_DEFINE(RANDOMF,       140  /* randomf                 */)\
_DEFINE(RANDOM,        141  /* random                  */)\
_DEFINE(UUID_CREATE,   142  /* uuid_create()           */)\
_DEFINE(UUID_TO_CHAR,  143  /* uuid_to_char(u)         */)\
_DEFINE(UUID_FROM_CHAR,144  /* uuid_from_char(c)       */)\
_DEFINE(UUID_COMPARE,  145  /* uuid_compare(u,u)       */)\
_DEFINE(SUBSTRING,     146  /* substring               */)\
_DEFINE(UNHEX,         147  /* unhex                   */)\
_DEFINE(CORR,          148  /* corr()                  */)\
_DEFINE(COVAR_POP,     149  /* covar_pop()             */)\
_DEFINE(COVAR_SAMP,    150  /* covar_samp()            */)\
_DEFINE(REGR_AVGX,     151  /* regr_avgx()             */)\
_DEFINE(REGR_AVGY,     152  /* regr_avgy()             */)\
_DEFINE(REGR_COUNT,    153  /* regr_count()            */)\
_DEFINE(REGR_INTERCEPT,154  /* regr_intercept()        */)\
_DEFINE(REGR_R2,       155  /* regr_r2()               */)\
_DEFINE(REGR_SLOPE,    156  /* regr_slope()            */)\
_DEFINE(REGR_SXX,      157  /* regr_sxx()              */)\
_DEFINE(REGR_SXY,      158  /* regr_sxy()              */)\
_DEFINE(REGR_SYY,      159  /* regr_syy()              */)\
_DEFINE(STDDEV_POP,    160  /* stddev_pop()            */)\
_DEFINE(STDDEV_SAMP,   161  /* stddev_samp()           */)\
_DEFINE(VAR_POP,       162  /* var_pop()               */)\
_DEFINE(VAR_SAMP,      163  /* var_samp()              */)\
_DEFINE(TABLEINFO,     164  /* iitableinfo()           */)\
_DEFINE(POS,           165  /* ANSI position()         */)\
_DEFINE(ATRIM,         166  /* ANSI trim()             */)\
_DEFINE(CHLEN,         167  /* ANSI char_length()      */)\
_DEFINE(OCLEN,         168  /* ANSI octet_length()     */)\
_DEFINE(BLEN,          169  /* ANSI bit_length()       */)\
_DEFINE(NCHAR,         170  /* nchar()                 */)\
_DEFINE(NVCHAR,        171  /* nvarchar()              */)\
_DEFINE(UTF16TOUTF8,   172  /* ii_utf16_to_utf8()      */)\
_DEFINE(UTF8TOUTF16,   173  /* ii_utf8_to_utf16()      */)\
_DEFINE(I8,            174  /* int8                    */)\
_DEFINE(COWGT,         175  /* collation_weight()      */)\
_DEFINE(ADTE,          176  /* ansidate()              */)\
_DEFINE(TMWO,          177  /* time_wo_tz()            */)\
_DEFINE(TMW,           178  /* time_with_tz()          */)\
_DEFINE(TME,           179  /* time_local()            */)\
_DEFINE(TSWO,          180  /* timestamp_wo_tz()       */)\
_DEFINE(TSW,           181  /* timestamp_with_tz()     */)\
_DEFINE(TSTMP,         182  /* timestamp_local()       */)\
_DEFINE(INDS,          183  /* interval_dtos()         */)\
_DEFINE(INYM,          184  /* interval_ytom()         */)\
_DEFINE(YPART,         185  /* year()                  */)\
_DEFINE(QPART,         186  /* quarter()               */)\
_DEFINE(MPART,         187  /* month()                 */)\
_DEFINE(WPART,         188  /* week()                  */)\
_DEFINE(WIPART,        189  /* week_iso()              */)\
_DEFINE(DYPART,        190  /* day()                   */)\
_DEFINE(HPART,         191  /* hour()                  */)\
_DEFINE(MIPART,        192  /* minute()                */)\
_DEFINE(SPART,         193  /* second()                */)\
_DEFINE(MSPART,        194  /* microsecond()           */)\
_DEFINE(NPART,         195  /* nanosecond()            */)\
_DEFINE(ROUND,         196  /* round()                 */)\
_DEFINE(TRUNC,         197  /* trunc[ate]()            */)\
_DEFINE(CEIL,          198  /* ceil[ing]()             */)\
_DEFINE(FLOOR,         199  /* floor()                 */)\
_DEFINE(CHR,           200  /* chr()                   */)\
_DEFINE(LTRIM,         201  /* ltrim()                 */)\
_DEFINE(LPAD,          202  /* lpad()                  */)\
_DEFINE(RPAD,          203  /* rpad()                  */)\
_DEFINE(REPL,          204  /* replace()               */)\
_DEFINE(ACOS,          205  /* acos()                  */)\
_DEFINE(ASIN,          206  /* asin()                  */)\
_DEFINE(TAN,           207  /* tan()                   */)\
_DEFINE(PI,            208  /* pi()                    */)\
_DEFINE(SIGN,          209  /* sign()                  */)\
_DEFINE(ATAN2,         210  /* atan2()                 */)\
_DEFINE(BYTEXT,        211  /* byteextract()           */)\
_DEFINE(ISINT,         212  /* is integer              */)\
_DEFINE(NOINT,         213  /* is not integer          */)\
_DEFINE(ISDEC,         214  /* is decimal              */)\
_DEFINE(NODEC,         215  /* is not decimal          */)\
_DEFINE(ISFLT,         216  /* is float                */)\
_DEFINE(NOFLT,         217  /* is not float            */)\
_DEFINE(NUMNORM,       218  /* numeric norm            */)\
_DEFINE(UNORM,         219  /* unorm                   */)\
_DEFINE(PATCOMP,       220  /* patcomp                 */)\
_DEFINE(LNVCHR,        221  /* long_nvarchar           */)\
_DEFINE(ISFALSE,       222  /* is false                */)\
_DEFINE(NOFALSE,       223  /* is not false            */)\
_DEFINE(ISTRUE,        224  /* is true                 */)\
_DEFINE(NOTRUE,        225  /* is not true             */)\
_DEFINE(ISUNKN,        226  /* is unknown              */)\
_DEFINE(NOUNKN,        227  /* is not unknown          */)\
_DEFINE(LASTID,	       228  /* last_identity	       */)\
_DEFINE(BOO,           229  /* boolean                 */)\
_DEFINE(CMPTVER,       230  /* iicmptversion           */)\
_DEFINE(GREATEST,      231  /* greatest                */)\
_DEFINE(LEAST,         232  /* least                   */)\
_DEFINE(GREATER,       233  /* greater                 */)\
_DEFINE(LESSER,        234  /* lesser                  */)\
_DEFINE(NVL2,          235  /* NVL2                    */)\
_DEFINE(GENERATEDIGIT, 236  /* generate_digit()        */)\
_DEFINE(VALIDATEDIGIT, 237  /* validate_digit()        */)\
_DEFINE(SINGLETON,     238  /* singleton               */)\
_DEFINE(AES_DECRYPT,   239  /* aes_decrypt             */)\
_DEFINE(AES_ENCRYPT,   240  /* aes_encrypt             */)\
_DEFINE(SOUNDEX_DM,    241  /* soundex_dm              */)\
_DEFINE(IIFALSE,       242  /* ii_false                */)\
_DEFINE(IITRUE,        243  /* ii_true                 */)\
_DEFINE(SINGLECHK,     244  /* singlechk               */)\
_DEFINE(POINT,         245  /* point()                 */)\
_DEFINE(X,             246  /* x(point)                */)\
_DEFINE(Y,             247  /* y(point)                */)\
_DEFINE(BPOINT,        248  /* blob point operators    */)\
_DEFINE(LINE,          249  /* Line operators          */)\
_DEFINE(POLY,          250  /* Polygon operators       */)\
_DEFINE(MPOINT,        251  /* multi point operators   */)\
_DEFINE(MLINE,         252  /* multi Line operators    */)\
_DEFINE(MPOLY,         253  /* multi Polygon operators */)\
_DEFINE(GEOMWKT,       254  /* Well known text ops     */)\
_DEFINE(GEOMWKB,       255  /* Well known binary ops   */)\
_DEFINE(NBR,           256  /* Spatial nbr op          */)\
_DEFINE(HILBERT,       257  /* Spatial hilbert op      */)\
_DEFINE(OVERLAPS,      258  /* overlaps(geom, geom     */)\
_DEFINE(INSIDE,        259  /* inside(geom, geom)    */)\
_DEFINE(PERIMETER,     260  /* perimeter(geom)    */)\
_DEFINE(GEOMNAME,      261  /* iigeomname()            */)\
_DEFINE(GEOMDIMEN,     262  /* iigeomdimensions()      */)\
_DEFINE(DIMENSION,     263  /* dimension(geom)         */)\
_DEFINE(GEOMETRY_TYPE, 264  /* geometrytype(geom)      */)\
_DEFINE(BOUNDARY,	   265  /* boundary(geom)          */)\
_DEFINE(ENVELOPE,	   266  /* envelope(geom)		   */)\
_DEFINE(EQUALS,		   267  /* equals(geom, geom)      */)\
_DEFINE(DISJOINT,	   268	/* disjoint(geom, geom)	   */)\
_DEFINE(INTERSECTS,	   269	/* intersects(geom, geom)  */)\
_DEFINE(TOUCHES,	   270	/* touches(geom, geom)	   */)\
_DEFINE(CROSSES,	   271	/* crosses(geom, geom)	   */)\
_DEFINE(WITHIN,		   272	/* within(geom, geom)	   */)\
_DEFINE(CONTAINS,	   273	/* contains(geom, geom)	   */)\
_DEFINE(RELATE,		   274	/* relate(geom, geom, char)*/)\
_DEFINE(DISTANCE,	   275	/* distance(geom, geom)    */)\
_DEFINE(INTERSECTION,  276	/* intersection(geom, geom)*/)\
_DEFINE(DIFFERENCE,	   277	/* difference(geom, geom)  */)\
_DEFINE(SYMDIFF,	   278	/* symdifference(geom, geom)*/)\
_DEFINE(BUFFER,	   	   279	/* buffer()				   */)\
_DEFINE(CONVEXHULL,	   280	/* convexhull(geom)        */)\
_DEFINE(POINTN,	   	   281	/* pointn(linestring)      */)\
_DEFINE(STARTPOINT,	   282  /* startpoint(curve)	   */)\
_DEFINE(ENDPOINT,	   283  /* endpoint(curve)		   */)\
_DEFINE(ISCLOSED,	   284  /* isclosed(curve)		   */)\
_DEFINE(ISRING,		   285  /* isring(curve)		   */)\
_DEFINE(STLENGTH,	   286  /* st_length(curve)		   */)\
_DEFINE(CENTROID,	   287  /* centroid(surface)	   */)\
_DEFINE(PNTONSURFACE,  288  /* pointonsurface(surface) */)\
_DEFINE(AREA,  		   289  /* area(surface) 		   */)\
_DEFINE(EXTERIORRING,  290  /* exteriorring(polygon)   */)\
_DEFINE(NINTERIORRING, 291  /* numinteriorring(polygon)*/)\
_DEFINE(INTERIORRINGN, 292  /* interiorringn(polygon, i)*/)\
_DEFINE(NUMGEOMETRIES, 293 	/* numgeometries(geomcoll) */)\
_DEFINE(GEOMETRYN,	   294  /* geometryn(geomcoll)	   */)\
_DEFINE(ISEMPTY,       295  /* ISEMPTY(geom)           */)\
_DEFINE(ISSIMPLE,      296  /* ISSIMPLE(geom)          */)\
_DEFINE(UNION,		   297  /* union(geom, geom)       */)\
_DEFINE(SRID,		   298  /* SRID(geom)              */)\
_DEFINE(NUMPOINTS,	   299  /* NUMPOINTS(linestring)   */)\
_DEFINE(TRANSFORM,	   300  /* TRANSFORM(geom)         */)\
_DEFINE(GEOMWKTRAW,    301  /* AsTextRaw(geom)         */)\
_DEFINE(GEOMC,		   302	/* GeomColl operators	   */)\
_DEFINEEND


#define _DEFINEEND
/*
** Define ADI_OP_ID_enum
**
** Note the two synonymns for ADI_LVARCH_OP & ADI_LBYTE_OP
*/
#define _DEFINE(n, v) ADI_##n##_OP=v,
enum ADI_OP_ID_enum {ADI_OPS_MACRO /*, */
			ADI_LONG_VARCHAR=ADI_LVARCH_OP,
			ADI_LONG_BYTE=ADI_LBYTE_OP};
#undef  _DEFINE

/*
** Ensure elements are contiguous
*/
#define _DEFINE(n, v) == v && v+1
#if !(-1 ADI_OPS_MACRO >0)
# error "ADI_OPS_MACRO is not contiguous"
#endif
#undef  _DEFINE
#undef  _DEFINEEND

#endif /*__ADFOPS_H_INCLUDED*/
