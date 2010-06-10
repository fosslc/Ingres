/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: ADUINT.H - Internal definitions for ADU.
**
** Description:
**      This file defines functions, typedefs, constants, and variables
**      used internally by ADU.
**
** History: 
**      31-dec-1992 (stevet)
**          Created.
**      08-March-1993 (fred)
**  	    Removed adu_redeem() extern.  This is now done in adp.h,
**          since this routine is now called from scf (for copy).
**      16-Apr-1993 (fred)
**          Added adu_lolk().
**	28-feb-1996 (thoda04)
**	    Added missing function prototypes for adu_10secid_rawcmp() 
**	    and adu_10seclbl_rawcmp().
**	11-jun-1996 (prida01)
**	    Add extern for function adu_lvarcharcmp.
**	5-jul-96 (angusm)
**		Add proto for adu_perm_type()
**	19-feb-1997 (shero03)
**	    Add adu_19strsoundex()
**	10-oct-97 (inkdo01)
**	    Add adu_2alltobyte() for byte/varbyte function support.
**	06-oct-1998 (shero03)
**	    Add bitwise functions: adu_bit_add, _and, _not, _or, _xor
**	    ii_ipaddr.  Removd xyzzy
**	23-oct-98 (kitch01)
**		Bug 92735. Add adu_1cvrt_date4().
**	12-Jan-1999 (shero03)
**	    Add hash function.
**	15-Jan-1999 (shero03)
**	    Add random functions.
**	25-Jan-1999 (shero03)
**	    Add UUID functions.
**	20-Apr-1999 (shero03)
**	    Add substring(c FROM n [FOR l])
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	08-Jun-1999 (shero03)
**	    Add unhex function.
**	22-sep-99 (inkdo01)
**	    Add OLAP aggregate functions + ADU_AGNEXT2_FUNC for binary aggs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-feb-2001 (gupsh01)
**	    Added unicode functions.
**	15-march-2001 (gupsh01)
**	    Added new unicode functions for like, left, 
**	    right, length, trim, and coercion.
**	26-march-2001 (gupsh01)
**	    Modified prototype definitions for locate,
**	    shift and substring functions for unicode data type.
**      30-march-2001 (gupsh01)
**	    Added new functions to support unicode string coercion.	
**      16-apr-2001 (stial01)
**          Added support for tableinfo function
**	20-apr-2001 (abbjo03)
**	    Add support for LIKE for Unicode characters.
**	22-june-01 (inkdo01)
**	    Change structure parm to OLAP aggregate funcs for new aggregate
**	    code.
**	09-jan-02 (inkdo01)
**	    Add func refs to handle ANSI char_length, octet_length, bit_length,
**	    position and trim functions.
**      21-mar-2002 (gupsh01)
**          Added function declarations for adu_nvchr_unitochar and
**          adu_nvchr_chartouni for coercion from unicode to character datatypes
**	16-dec-03 (inkdo01)
**	    Re-added char_length, bit_length, octet_length, position and trim.
**	09-july-2004 (gupsh01)
**	    Added function adu_nvchr_strutf8conv. This is only meant for internal
**	    use in ascii copy of unicode data, when we require to convert a varchar 
**	    string describing the null value in the copy statement to be converted
**	    to UTF8 internal type.
**	20-dec-04 (inkdo01)
**	    Added adu_collweight() and adu_ucollweight() to compute collation
**	    weight() scalar function.
**	02-feb-2005 (raodi01)
**	    Added function declaration check_uuid_mac
**      28-apr-2005 (raodi01)
**          Added the function declaration in the source file itself 
**	20-Oct-2005 (hanje04)
**	    Move declaration of adu_nvchr_chartouni() to adunchar.c as
**	    it's a static function and GCC 4.0 doesn't like it being 
**	    a FUNC_EXTERN and a static.
**	13-apr-06 (dougi)
**	    Added adu_date_isdst() for isdst() function.
**	30-May-2006 (jenjo02)
**	    Changed adu_17structure from ADU_NORM1_FUNC to ADU_NORM2_FUNC.
**	29-Aug-2006 (gupsh01)
**	    Added prototype for ANSI datetime function adu_21ansi_strtodate.
**	18-oct-2006 (dougi)
**	    Added entries for year(), quarter(), month(), week(), week_iso(), 
**	    day(), hour(), minute(), second(), microsecond() functions for date 
**	    extraction.
**      23-oct-2006 (stial01)
**          Back out Ingres2007 change to iistructure()
**      08-dec-2006 (stial01)
**          Added prototype for adu_26positionfrom
**      19-dec-2006 (stial01)
**          Added prototype for adu_nvchr_2args
**      25-jan-2007 (stial01)
**          Added prototype for adu_varbyte_2arg
**	20-feb-2007 (gupsh01)
**	    Added prototype for adu_nvchr_toupper and adu_nvchr_tolower.
**	8-may-2007 (dougi)
**	    Added prototype for adu_nvchr_utf8comp().
**	19-oct-2007 (gupsh01)
**	    Added adu_ulexcomp() to support Quel pattern matching.
**	03-Nov-2007 (kiria01) b119410
**	    Added prototype for adu_lexnumcomp().
**	05-nov-2007 (gupsh01)
**	    Pass in the quel pattern matching flag to adu_nvchr_utf8comp.
**	26-Dec-2007 (kiria01) SIR119658
**	    Added 'is integer/decimal/float' operators.
**	07-Mar-2008 (kiria01) b120043
**	    Added adu_date_mul, adu_date_div, adu_date_minus and adu_date_plus
**	12-mar-2008 (gupsh01)
**	    Add prototype for function adu_utf8cbldkey.
**	11-apr-2008 (gupsh01)
**	    Add semantics to the routine adu_utf8cbldkey and also add 
**	    new routine adu_nvchr_utf8_bldkey for building keys in UTf8 
**	    installations.
**	12-Apr-2008 (kiria01) b119410
**	    Added adu_numeric_norm
**	06-Jun-2008 (kiria01) SIR120473
**	    Added prototype for extended LIKE support
**	14-Oct-2008 (gupsh01)
**	    Modified the prototype for adu_movestring function to contain
**	    the type of the input string. This is useful when the input
**	    string contains known byte datatype and copied to a character
**	    result type of type char/varchar. In this case interpretation 
**	    of data as characters and applying CM functions should be 
**	    avoided. A valid input for this parameter is DB_NODT which
**	    will signify that the caller ensures that the data in the 
**	    input field is character data with semantic meaning. 
**	27-Oct-2008 (kiria01) SIR120473
**	    Added prototype for separated pattern compiler.
**      17-Dec-2008 (macde01)
**          Added new function refs associated with DB_PT_TYPE datatype.
**	05-Jan-2008 (kiria01) SIR120473
**	    Switch eptr to esc_dv to allow for tracking datatype
**  28-Feb-2009 (thich01)
**      Added geo_fromText function ref for spatial
**  06-Mar-2009 (thich01)
**      Added type specific fromText function refs for spatial.
**  13-Mar-2009 (thich01)
**      Added funtion refs for asText and asBinary
**  20-Mar-2009 (thich01)
**      Added fromBinary function refs for spatial
**      22-Apr-2009 (Marty Bowes) SIR 121969
**          Add adu_strgenerate_digit() and adu_strvalidate_digit()
**          for the generation and validation of checksum values.
**      01-Aug-2009 (martin bowes) SIR122320
**          Added soundex_dm (Daitch-Mokotoff soundex)
**  20-Aug-2009 (thich01)
**      Add generic geom from text and from wkb.
**  25-Aug-2009 (troal01)
**      Added iigeomname, iigeomdimensions function refs for spatial
**	aug-2009 (stephenb)
**		Remove various protos that are called outside ADF and
**		prototype them in a more public location.
**      03-Sep-2009 (coomi01) b122473
**          Add function declarator for adu_3alltobyte()
**	9-sep-2009 (stephenb)
**	    Add adu_last_id prototype.
**      06-oct-2009 (joea)
**          Add prototype for adu_bool_coerce.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Add prototype for adu_cmptversion.
**	14-Nov-2009 (kiria01) SIR 122894
**	    Added adu_greatest and adu_least generic polyadic functions.
**      16-Nov-2009 (coomi01) b122840
**          Add prototype for adu_longcmp().
**	16-Nov-2009 (kschendel) SIR 122882
**	    Move adu_cmptversion to adf.h, dmf wants it.
**  16-Nov-2009 (coomi01) b122840
**     Added NVL2
**       01-Dec-2009 (coomi01) b122980
**          Add adu_fltround prototype
**      10-Mar-2010 (thich01)
**          Add overlaps and inside.
**	xx-Apr-2010 (iwawong)
**		Added issimple isempty x y numpoints
**		for the coordinate space.
**      02-Apr-2010 (thich01)
**          Add rtree_cmp and a specific inside function since it is different
**          than contains.
**	14-apr-2010 (toumi01) SIR 122403
**	    Add adu_aesdecrypt and adu_aesencrypt.
**	10-May-2010 (kschendel) b123712
**	    Added float trunc, ceil, floor.
*/


/*
**  Forward and/or External function references for normal functions.
*/

typedef DB_STATUS ADU_NORM0_FUNC (ADF_CB         *adf_scb,
				  DB_DATA_VALUE  *rdv);


typedef DB_STATUS ADU_NORM1_FUNC (ADF_CB         *adf_scb,
				  DB_DATA_VALUE  *dv1,
				  DB_DATA_VALUE  *rdv);

typedef DB_STATUS ADU_NORM2_FUNC (ADF_CB         *adf_scb,
				  DB_DATA_VALUE  *dv1,
				  DB_DATA_VALUE  *dv2,
				  DB_DATA_VALUE  *rdv);

typedef DB_STATUS ADU_NORM3_FUNC (ADF_CB         *adf_scb,
				  DB_DATA_VALUE  *dv1,
				  DB_DATA_VALUE  *dv2,
				  DB_DATA_VALUE  *dv3,
				  DB_DATA_VALUE  *rdv);

typedef DB_STATUS ADU_NORM4_FUNC (ADF_CB         *adf_scb,
				  DB_DATA_VALUE  *dv1,
				  DB_DATA_VALUE  *dv2,
				  DB_DATA_VALUE  *dv3,
				  DB_DATA_VALUE  *dv4,
				  DB_DATA_VALUE  *rdv);

FUNC_EXTERN ADU_NORM1_FUNC adu_ascii;       /* Routine to convert a variety of
                                            ** datatypes into c, text, char,
					    ** varchar, or longtext.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_ascii_2arg;  /* Support the two the
    	    	    	    	    	    ** arguments version of c(), 
                                            ** text(), char() and varchar()
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_copascii;    /* Routine to convert numeric
                                            ** datatypes into c, text, char, or
					    ** varchar using right justification
					    ** and misc other checks for COPY.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_bit2str;	    /* Bit types to character types */
FUNC_EXTERN ADU_NORM1_FUNC adu_str2bit;	    /* and the other way around */
FUNC_EXTERN ADU_NORM1_FUNC adu_mb_move_bits;/* Bit to bit conversion */
FUNC_EXTERN ADU_NORM1_FUNC adu_bitlength;   /* Length of a bit operation */
FUNC_EXTERN ADU_NORM1_FUNC adu_bitsize;     /* Provide declared size for bit 
					    ** types 
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_1flt_coerce; /* Routine to coerce a variety of
                                            ** datatypes into the f datatype.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_1dec_coerce; /* Routine to coerce a variety of
                                            ** datatypes into decimal.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_1int_coerce; /* Routine to coerce a variety of
                                            ** datatypes into the i datatype.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_bool_coerce; /* Routine to coerce a boolean
					    ** into another boolean.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_pad;         /* Routine to pad a text or varchar
                                            ** data value with as many spaces
                                            ** as it can hold. 
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_squeezewhite;/* Routine to compress white space
                                            ** from a text or varchar data
                                            ** value.  This removes all leading
                                            ** and trailing blanks, tabs,
                                            ** new-lines, form-feeds, and
                                            ** replaces all other white space
                                            ** with a single blank. 
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_trim;        /*Routine returns a text or varchar
					    ** data value that is the same as
                                            ** the input string data value,
                                            ** except with trailing blanks
                                            ** removed. 
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_atrim;       /* Same as adu_trim , only using 
					    ** ANSI semantics (also supports
					    ** LEADING, TRAILING, BOTH options).
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_dgmt;        /* Routine to format an absolute
                                            ** date (in GMT), in the form:
                                            ** 'yyyy.mm.dd hh:mm:ss GMT  '.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_1dayofweek;  /* Routine to give the day of the
                                            ** week for a supplied date data
                                            ** value.  The result is placed in a
                                            ** c data value.  The input must be
                                            ** an absolute date.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_date_isdst;  /* Routine to determine if a date 
					    ** falls in daylight savings time.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_1date_addu;  /* Routine to add 2 date data values
                                            ** together.  At least one of the
                                            ** dates must be an interval.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_7date_subu;  /* Routine to subtract one date data
                                            ** value from another.  Subtracting
                                            ** an absolute date from an interval
                                            ** is not allowed.  All other
                                            ** combinations are OK. 
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_intrvl_in;   /* Routine to convert a date
                                            ** interval into a floating-point
                                            ** constant in user-specified units.
                                            ** Allowable units are "seconds",
                                            ** "secs", "minutes", "mins",
                                            ** "hours", "hrs", "days", "weeks",
                                            ** "wks", "months", "mos",
                                            ** "quarters", "qtrs", "years", or
                                            ** "yrs". 
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_date_mul;    /* Routine to handle multiplication
					    ** of intervals with floats:
					    ** interval = interval * float
					    ** interval = float * interval
					    */
FUNC_EXTERN ADU_NORM2_FUNC adu_date_div;    /* Routine to handle division
					    ** of intervals with floats:
					    ** interval = interval / float
					    ** float = interval / interval
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_date_plus;   /* Routine to handle unary
					    ** plus of intervals
					    ** interval = + interval
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_date_minus;  /* Routine to handle unary
					    ** minus of intervals
					    ** interval = -interval
					    */
FUNC_EXTERN ADU_NORM2_FUNC adu_dtruncate;   /* Routine to truncate a date data
                                            ** value to a specified level of
                                            ** granularity.  This strips off
                                            ** things like seconds, minutes,
                                            ** hours, days, etc. 
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_dextract;    /* Routine to get one component of
                                            ** a date data value.  For instance,
                                            ** the "month" of "10-jan-1957"
                                            ** would be 1, while the "day" of
                                            ** "10-jan-1957" would be 10. 
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_year_part;  /* Stub that calls adu_dextract()
					    ** to retrieve specified component.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_quarter_part;  /* Stub that calls adu_dextract()
					    ** to retrieve specified component.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_month_part;  /* Stub that calls adu_dextract()
					    ** to retrieve specified component.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_week_part;  /* Stub that calls adu_dextract()
					    ** to retrieve specified component.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_weekiso_part;  /* Stub that calls adu_dextract()
					    ** to retrieve specified component.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_day_part;  /* Stub that calls adu_dextract()
					    ** to retrieve specified component.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_hour_part;  /* Stub that calls adu_dextract()
					    ** to retrieve specified component.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_minute_part;  /* Stub that calls adu_dextract()
					    ** to retrieve specified component.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_second_part;  /* Stub that calls adu_dextract()
					    ** to retrieve specified component.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_microsecond_part;  /* Stub that calls adu_dextract()
					    ** to retrieve specified component.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_nanosecond_part;  /* Stub that calls adu_dextract()
					    ** to retrieve specified component.
					    */

FUNC_EXTERN ADU_NORM1_FUNC adu_dbmsinfo;    /* Routine to perform the dbmsinfo()
                                            ** func.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_2dec_convert;/* Routine to convert a variety of
					    ** datatypes into the decimal
					    ** datatype.
					    */
FUNC_EXTERN ADU_NORM0_FUNC adu_datenow;     /* Routine to get an INGRES 
					    ** internal date for now.
					    */
FUNC_EXTERN ADU_NORM2_FUNC adu_decround;    /* Routine to compute round()
					    ** on decimal operand.
					    */
FUNC_EXTERN ADU_NORM2_FUNC adu_fltround;    /* Routine to compute round()
					    ** on float operand.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_decceil;     /* Routine to compute ceiling()
					    ** on decimal operand.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_fltceil;     /* Routine to compute ceiling()
					    ** on float operand.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_decfloor;    /* Routine to compute floor()
					    ** on decimal operand.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_fltfloor;    /* Routine to compute floor()
					    ** on float operand.
					    */
FUNC_EXTERN ADU_NORM2_FUNC adu_dectrunc;    /* Routine to compute trunc()
					    ** on decimal operand.
					    */
FUNC_EXTERN ADU_NORM2_FUNC adu_flttrunc;    /* Routine to compute trunc()
					    ** on float operand.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_chr;         /* Routine to compute chr()
					    ** on integer operand.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_ltrim;       /* Routine to compute ltrim()
					    ** on string operand.
					    */
FUNC_EXTERN ADU_NORM2_FUNC adu_27lpad;      /* Routine to compute lpad() 
					    ** on string operand.
					    */
FUNC_EXTERN ADU_NORM2_FUNC adu_28rpad;      /* Routine to compute rpad() 
					    ** on string operand.
					    */
FUNC_EXTERN ADU_NORM3_FUNC adu_27alpad;      /* Routine to compute lpad() 
					    ** on 2 string operands.
					    */
FUNC_EXTERN ADU_NORM3_FUNC adu_28arpad;      /* Routine to compute rpad() 
					    ** on 2 string operands.
					    */
FUNC_EXTERN ADU_NORM3_FUNC adu_29replace;   /* Routine to execute replace()
                                            ** function.
					    */
FUNC_EXTERN ADU_NORM3_FUNC adu_29replace_raw;/* Routine to execute replace()
                                            ** function for byte.
					    */
FUNC_EXTERN ADU_NORM3_FUNC adu_29replace_uni;/* Routine to execute replace()
                                            ** function for unicode.
					    */
FUNC_EXTERN ADU_NORM2_FUNC adu_restab;      /* Routine to execute resolve_table
                                            ** function.
					    */
FUNC_EXTERN ADU_NORM2_FUNC adu_allocated_pages;/* Routine to get total 
					    ** allocated pages in a table  
					    */
FUNC_EXTERN ADU_NORM2_FUNC adu_overflow_pages;/* Routine to get total 
					    ** overflow pages in a table  
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_hash;        /* Routine to perform the hash()
                                            ** function on any data value.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_hex;         /* Routine to perform the hex() func
                                            ** on any string data value.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_hxnum;       /* Routine to convert a hex string
                                            ** into an integer; the iihexint()
					    ** function.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_unhex;       /* Routine to convert a hex string
                                            ** back into a string data value.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_ifnull;      /* Routine to perform the ifnull()
                                            ** func on any data type.
                                            */
FUNC_EXTERN ADU_NORM3_FUNC adu_nvl2;		/*Routine to perform the nvl2()
  									        **func on any data type.
  								           	*/         
FUNC_EXTERN ADU_NORM2_FUNC adu_iftrue;      /* Routine to perform the
					    ** ii_iftrue() func on any datatype
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_2int_pow;    /* Routine to raise a number data
                                            ** value (either i or f) to an
                                            ** integer (i) power. 
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_2logkeytologkey;/* This routine converts a 
					    ** logical_key data value into a 
					    ** logical_key data value.  This
					    ** is used for copy a logical_key
					    ** data value into a tuple buffer.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_3logkeytostr;/* This routine converts a 
					    ** logical_key data value into
					    ** various str data values.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_4strtologkey;/* This routine converts various 
					    ** str data values into
					    ** a logical_key data value.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_5logkey_exp; /* Routine that exports a logical 
					    ** key data type to the external 
					    ** world.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_3mny_absu;   /* Routine to give the absolute
                                            ** value of a money data value.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_4mny_addu;   /* Routine to add two money data
                                            ** values.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_2mnytomny;   /* This routine converts a money
                                            ** data value into a money data
                                            ** value.  This is used for copying
                                            ** a money data value into a tuple
                                            ** buffer.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_7mny_divf;   /* Routine to divide either an i or
                                            ** an f data value by a money data
                                            ** value.  The result is an f data
                                            ** value.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_8mny_divu;   /* Routine to divide a money data
                                            ** value by either an i, f, or money
                                            ** data value.  The result is a
                                            ** money data value. 
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_1mnytonum;   /* This routine converts a money
                                            ** data value into an f data value.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_10mny_multu; /* Routine to multiply an i, f, or
                                            ** money data value by a money data
                                            ** value.  The result is a money
                                            ** data value. 
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_numtomny;    /* This routine converts a number
                                            ** data value (either i or f) into
                                            ** a money data value.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_12mny_subu;  /* This routine subtract two money
					    ** values.
					    */
FUNC_EXTERN ADU_NORM1_FUNC adu_posmny;      /* Routine to perform unary "+" on
                                            ** a money value.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_negmny;      /* Routine to perform unary "-" on
                                            ** a money value.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_cpmnytostr;  /* Routine to coerce a money data
                                            ** value into a string data value,
                                            ** for the copy command.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_cpn_dump;    /* Dump blob coupon */

FUNC_EXTERN ADU_NORM1_FUNC adu_1cvrt_date;  /* Routine to perform the _date()
                                            ** func.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_1cvrt_date4; /* Routine to perform the _date4()
                                            ** func.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_2cvrt_time;  /* Routine to perform the _time()
                                            ** func.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_4strconcat;  /* Routine to concatenate a string
                                            ** data value with another string
                                            ** data value.  If the first string
                                            ** is a c, all trailing blanks are
                                            ** removed (unless it is all blanks,
                                            ** in which case one is left) before
                                            ** the second string is
                                            ** concatenated.  The result is text
                                            ** if both strings were text,
					    ** varchar if either string is char
					    ** or varchar, and c otherwise. 
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_6strleft;    /* Routine to return the leftmost
                                            ** so many characters from a string
                                            ** data value. 
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_7strlength;  /* Routine to return the length of
                                            ** a string data value.  If the
                                            ** string is c or char, then the
                                            ** length without trailing blanks is
                                            ** returned.  If the string is a
                                            ** text or varchar, then the actual
                                            ** number of characters in the text
                                            ** or varchar are returned. 
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_8strlocate;  /* Routine to return the location of
                                            ** one string data value within
                                            ** another. 
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_9strlower;   /* Routine to convert all upper case
                                            ** characters in a string data value
                                            ** into lower case. 
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_10strright;  /* Routine to return the rightmost
                                            ** so many characters from a string
                                            ** data value. 
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_11strshift;  /* Routine to shift a string data
                                            ** value to the left or right so
                                            ** many spaces. 
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_12strsize;   /* Routine to return the declared
                                            ** size of a string data value
                                            ** without removing trailing blanks.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_1strtostr;   /* This routine converts a string
                                            ** data value into a string data
					    ** value.  This will truncate the
                                            ** input value if the output value's
                                            ** length is less than the input
                                            ** value's length.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_2alltobyte;  /* This routine converts any data 
					    ** type into a byte/varbyte data
					    ** value. Supports "byte/varbyte"
					    ** functions which are advertised 
					    ** as supporting ALL input types.
					    */
FUNC_EXTERN ADU_NORM2_FUNC adu_3alltobyte;  /* This routine converts any data 
					    ** type into a byte/varbyte data
					    ** value, limiting the output field
					    ** width through a length param.
					    ** Supports "byte/varbyte"
					    ** functions which are advertised 
					    ** as supporting ALL input types.
					    */
FUNC_EXTERN ADU_NORM2_FUNC adu_varbyte_2arg; /* 2 arg version of varbyte */
FUNC_EXTERN ADU_NORM1_FUNC adu_15strupper;  /* Routine to convert all lower case
                                            ** characters in a string data value
                                            ** into upper case. 
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_notrm;       /* This routine converts a string
                                            ** data value into a string data
					    ** value.  The difference between
                                            ** this and adu_1strtostr() is that
                                            ** trailing blanks for c and char
                                            ** will NOT be trimmed if the output
                                            ** is text or varchar.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_xyzzy;       /* Magic function.
					    */
FUNC_EXTERN ADU_NORM2_FUNC adu_16strindex;  /* Routine to return the n'th char-
                                            ** acter in a string as a CHAR(1).
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_17structure; /* Routine to perform the
                                            ** iistructure(i) func.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_18cvrt_gmt;  /* Routine to perform the 
					    ** gmt_timestamp() func.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_lvch_move;   /* Move to/from blob */
FUNC_EXTERN ADU_NORM1_FUNC adu_19strsoundex;/* Routine to perform soundex()
                                            ** on any string data value.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_soundex_dm;  /* Routine to perform soundex_dm()
                                            ** on any string data value.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_strgenerate_digit;
    /* Routine to return the check digit for a string based on a nominated 
    ** encoding scheme.
    */
FUNC_EXTERN ADU_NORM2_FUNC adu_strvalidate_digit;  
    /* Routine to return a integer1 flag (1=OK, 0=Invalid) to indicate if the
    ** check digit contained within a string is valid, based on a nominated 
    ** encoding scheme.
    */
FUNC_EXTERN ADU_NORM2_FUNC adu_tb2di;	    /* Routine to return the `DI' file-
                                            ** name given the table ID.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_di2tb;	    /* Routine to return the unique
					    ** portion of the table ID given
                                            ** `DI' filename.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_typename;    /* Routine to perform the
                                            ** iitypename() func.
                                            */
FUNC_EXTERN ADU_NORM1_FUNC adu_dvdsc;       /* Routine to format a string
					    ** describing the DB_DATA_VALUE's
					    ** datatype and user specified
                                            ** length.  This is the ii_dv_desc()
					    ** func.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_exttype;     /* Find ext. type name for	    */
					    /* unexportable type	    */
FUNC_EXTERN ADU_NORM2_FUNC adu_extlength;   /* Ditto for length */
FUNC_EXTERN ADU_NORM2_FUNC adu_userlen;     /* Routine to perform the
                                            ** iiuserlen() func.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_bit_add;	    /* Routine to return bitwise add */
FUNC_EXTERN ADU_NORM2_FUNC adu_bit_and;	    /* Routine to return bitwise and */
FUNC_EXTERN ADU_NORM1_FUNC adu_bit_not;	    /* Routine to return bitwise not */
FUNC_EXTERN ADU_NORM2_FUNC adu_bit_or;	    /* Routine to return bitwise or  */
FUNC_EXTERN ADU_NORM2_FUNC adu_bit_xor;	    /* Routine to return bitwise xor */
FUNC_EXTERN ADU_NORM1_FUNC adu_ipaddr;	    /* Routine to perform ii_ipaddr()
                                            ** on any string data value.
                                            */
FUNC_EXTERN ADU_NORM2_FUNC adu_intextract;  /* Routine to extract a single 
                                            ** byte from a byte operand
                                            */
FUNC_EXTERN ADU_NORM0_FUNC adu_random;	    /* Returns random int */
FUNC_EXTERN ADU_NORM0_FUNC adu_randomf;	    /* Return random float */
FUNC_EXTERN ADU_NORM2_FUNC adu_random_range;/* Returns random int in range */
FUNC_EXTERN ADU_NORM2_FUNC adu_randomf_range;/* Returns random float in range */
FUNC_EXTERN ADU_NORM2_FUNC adu_randomf_rangef;/* Returns random float in range */
FUNC_EXTERN ADU_NORM0_FUNC adu_uuid_create; /* Returns a new uuid */
FUNC_EXTERN ADU_NORM1_FUNC adu_uuid_to_char;/* Return char form of this uuid */
FUNC_EXTERN ADU_NORM1_FUNC adu_uuid_from_char;/* Returns uuid from char form */
FUNC_EXTERN ADU_NORM2_FUNC adu_uuid_compare;/* Routine to compare two uuids */

FUNC_EXTERN ADU_NORM2_FUNC adu_20substr;	/* Routine to substr(c,i) */
FUNC_EXTERN ADU_NORM3_FUNC adu_21substrlen;	/* Routine to substr(c,i,i) */
FUNC_EXTERN ADU_NORM1_FUNC adu_22charlength;	/* ANSI char_length function */
FUNC_EXTERN ADU_NORM1_FUNC adu_23octetlength;	/* ANSI octet_length function */
FUNC_EXTERN ADU_NORM1_FUNC adu_24bitlength;	/* ANSI bit_length function */
FUNC_EXTERN ADU_NORM2_FUNC adu_25strposition;	/* ANSI position function */
FUNC_EXTERN ADU_NORM3_FUNC adu_26positionfrom;  /* ANSI position FROM */
FUNC_EXTERN ADU_NORM0_FUNC adu_last_id;   	/* last identity in session */

FUNC_EXTERN ADU_NORM1_FUNC adu_strtopt;         /* This routine converts a */
                                                /* string to a point.      */
FUNC_EXTERN ADU_NORM1_FUNC adu_pttostr;         /* This routine converts a */
                                                /* point to a string.      */
FUNC_EXTERN ADU_NORM2_FUNC adu_2flt_to_pt;      /* Returns point from      */
                                                /* point(float,float).     */
FUNC_EXTERN ADU_NORM1_FUNC adu_point_x;         /* Routine to x(point)     */
FUNC_EXTERN ADU_NORM1_FUNC adu_point_y;         /* Routine to y(point)     */
 
FUNC_EXTERN ADU_NORM1_FUNC adu_pt_to_pt;    /* This routine converts a point
                                            ** data value into a point data
                                            ** value.  This is used for copying
                                            ** a point data value into a tuple
                                            ** buffer. */

/* The following functions will verify well known text and conver to blob.*/
FUNC_EXTERN ADU_NORM1_FUNC adu_point_fromText;
FUNC_EXTERN ADU_NORM1_FUNC adu_point_fromWKB;
FUNC_EXTERN ADU_NORM1_FUNC adu_linestring_fromText;
FUNC_EXTERN ADU_NORM1_FUNC adu_linestring_fromWKB;
FUNC_EXTERN ADU_NORM1_FUNC adu_polygon_fromText;
FUNC_EXTERN ADU_NORM1_FUNC adu_polygon_fromWKB;
FUNC_EXTERN ADU_NORM1_FUNC adu_multipoint_fromText;
FUNC_EXTERN ADU_NORM1_FUNC adu_multipoint_fromWKB;
FUNC_EXTERN ADU_NORM1_FUNC adu_multilinestring_fromText;
FUNC_EXTERN ADU_NORM1_FUNC adu_multilinestring_fromWKB;
FUNC_EXTERN ADU_NORM1_FUNC adu_multipolygon_fromText;
FUNC_EXTERN ADU_NORM1_FUNC adu_multipolygon_fromWKB;
FUNC_EXTERN ADU_NORM1_FUNC adu_geometry_fromText;
FUNC_EXTERN ADU_NORM1_FUNC adu_geometry_fromWKB;

FUNC_EXTERN ADU_NORM2_FUNC adu_point_srid_fromText;
FUNC_EXTERN ADU_NORM2_FUNC adu_point_srid_fromWKB;
FUNC_EXTERN ADU_NORM2_FUNC adu_linestring_srid_fromText;
FUNC_EXTERN ADU_NORM2_FUNC adu_linestring_srid_fromWKB;
FUNC_EXTERN ADU_NORM2_FUNC adu_polygon_srid_fromText;
FUNC_EXTERN ADU_NORM2_FUNC adu_polygon_srid_fromWKB;
FUNC_EXTERN ADU_NORM2_FUNC adu_multipoint_srid_fromText;
FUNC_EXTERN ADU_NORM2_FUNC adu_multipoint_srid_fromWKB;
FUNC_EXTERN ADU_NORM2_FUNC adu_multilinestring_srid_fromText;
FUNC_EXTERN ADU_NORM2_FUNC adu_multilinestring_srid_fromWKB;
FUNC_EXTERN ADU_NORM2_FUNC adu_multipolygon_srid_fromText;
FUNC_EXTERN ADU_NORM2_FUNC adu_multipolygon_srid_fromWKB;
FUNC_EXTERN ADU_NORM2_FUNC adu_geometry_srid_fromText;
FUNC_EXTERN ADU_NORM2_FUNC adu_geometry_srid_fromWKB;

/* The following functions act on all geospatial types. */
FUNC_EXTERN ADU_NORM1_FUNC adu_geom_asText; /* Convert a geometry to WKT. */
FUNC_EXTERN ADU_NORM1_FUNC adu_geom_asTextRaw; /* Convert a geometry to WKT
                                                  *without* trim/rounding */
FUNC_EXTERN ADU_NORM2_FUNC adu_geom_asTextRound; /* Convert a geometry to WKT
                                                    with specified rounding */
FUNC_EXTERN ADU_NORM1_FUNC adu_geom_asBinary; /* Convert a geometry to WKB. */

FUNC_EXTERN ADU_NORM2_FUNC adu_geom_nbr; /* Extract an nbr for rtree indexing */
FUNC_EXTERN ADU_NORM1_FUNC adu_geom_hilbert; /* Find the hilbert number for rtree indexing */
FUNC_EXTERN ADU_NORM1_FUNC adu_geom_perimeter; /* Get the perimeter of a spatial objext. */
FUNC_EXTERN ADU_NORM2_FUNC adu_geom_union; /* Get the union of two spatial objects */
FUNC_EXTERN ADU_NORM2_FUNC adu_geom_overlaps; /* Does one geometry overlap another */
FUNC_EXTERN ADU_NORM2_FUNC adu_geom_inside; /* Is one geometry inside the other */

/*
 * These act on generic geometries
 */
FUNC_EXTERN ADU_NORM1_FUNC adu_dimension; /* OGC 1.1 dimension SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_geometry_type; /* OGC 1.1 geometrytype SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_boundary; /* OGC 1.1 boundary SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_envelope; /* OGC 1.1 envelope SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_equals; /* OGC 1.1 equals SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_disjoint; /* OGC 1.1 disjoint SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_intersects; /* OGC 1.1 intersects SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_touches; /* OGC 1.1 touches SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_crosses; /* OGC 1.1 crosses SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_within; /* OGC 1.1 within SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_contains; /* OGC 1.1 contains SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_inside; /* OGC 1.1 inside SQL function */
FUNC_EXTERN ADU_NORM3_FUNC adu_relate; /* OGC 1.1 relate SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_distance; /* OGC 1.1 distance SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_intersection; /* OGC 1.1 intersection SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_difference; /* OGC 1.1 difference SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_sym_difference; /* OGC 1.1 symdifference SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_buffer_one; /* OGC 1.1 buffer SQL function */
FUNC_EXTERN ADU_NORM3_FUNC adu_buffer_two; /* OGC 1.1 buffer SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_convexhull; /* OGC 1.1 convexhull SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_srid; /* OGC 1.1 SRID SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_isempty; /* OGC 1.1 ISEMPTY SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_issimple; /* OGC 1.1 ISEMPTY SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_transform; /* OGC 1.1 transform SQL function */

/*
 * Curve functions
 */
FUNC_EXTERN ADU_NORM1_FUNC adu_startpoint; /* OGC 1.1 startpoint SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_endpoint; /* OGC 1.1 endpoint SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_isclosed; /* OGC 1.1  isclosed SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_isring; /* OGC 1.1 isring SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_stlength; /* OGC 1.1 st_length SQL function */

/*
 * Surface functions
 */
FUNC_EXTERN ADU_NORM1_FUNC adu_centroid; /* OGC 1.1 centroid SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_pointonsurface; /* OGC 1.1 pointonsurface SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_area; /* OGC 1.1 area SQL function */

/*
 * Polygon functions
 */
FUNC_EXTERN ADU_NORM1_FUNC adu_exteriorring; /* OGC 1.1 exteriorring SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_numinteriorring; /* OGC 1.1 numinteriorring SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_interiorringn; /* OGC 1.1 interiorringn SQL function */

/*
 * GeometryCollection functions
 */
FUNC_EXTERN ADU_NORM1_FUNC adu_numgeometries; /* OGC 1.1 numgeometries SQL function */
FUNC_EXTERN ADU_NORM2_FUNC adu_geometryn; /* OGC 1.1 geometryn SQL function */

/*
 * LineString only functions
 */
FUNC_EXTERN ADU_NORM2_FUNC adu_pointn; /* OGC 1.1 pointn SQL function */
FUNC_EXTERN ADU_NORM1_FUNC adu_numpoints; /* OGC 1.1 numpoints SQL function */

/* The following are used by iigeometries */
FUNC_EXTERN ADU_NORM1_FUNC adu_geom_name;
FUNC_EXTERN ADU_NORM1_FUNC adu_geom_dimensions;



/*
**  Forward and/or External function references for aggregate functions.
*/

typedef DB_STATUS ADU_AGBEGIN_FUNC (ADF_CB         *adf_scb,
				    ADF_AG_STRUCT  *ag_struct);

typedef DB_STATUS ADU_AGNEXT_FUNC (ADF_CB         *adf_scb,
				   DB_DATA_VALUE  *dv_next,
				   ADF_AG_STRUCT  *ag_struct);

typedef DB_STATUS ADU_AGNEXT2_FUNC (ADF_CB         *adf_scb,
				   DB_DATA_VALUE  *dv1,
				   DB_DATA_VALUE  *dv2,
				   ADF_AG_OLAP    *olapag_struct);

typedef DB_STATUS ADU_AGEND_FUNC (ADF_CB         *adf_scb,	
				  ADF_AG_STRUCT  *ag_struct,
				  DB_DATA_VALUE  *dv_result);

typedef DB_STATUS ADU_OLAPAGEND_FUNC (ADF_CB     *adf_scb,	
				  ADF_AG_OLAP    *olapag_struct,
				  DB_DATA_VALUE  *dv_result);

FUNC_EXTERN ADU_AGBEGIN_FUNC adu_B0a_noagwork;   /* ag-begin for all aggrs 
						 ** that do not need a 
						 ** workspace DB_DATA_VALUE. 
						 */
FUNC_EXTERN ADU_AGEND_FUNC adu_E0a_anycount;     /* ag-end for all any() or 
						 ** count() aggregates. 
						 */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N2a_count;       /* ag_next for all count() 
						 ** aggregates 
						 */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N1a_any;         /* ag-next for all any() 
						 ** aggregates. 
						 */
FUNC_EXTERN ADU_AGBEGIN_FUNC adu_B0d_minmax_date;/* ag-begin for min(date) or 
						 ** max(date). 
						 */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N0d_tot_date;    /* ag-next for avg(date) and
						 ** sum(date) 
						 */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N5d_min_date;    /* ag-next for min(date). */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N6d_max_date;    /* ag-next for max(date). */
FUNC_EXTERN ADU_AGEND_FUNC adu_E3d_avg_date;     /* ag-end for avg(date). */
FUNC_EXTERN ADU_AGEND_FUNC adu_E4d_sum_date;     /* ag-end for sum(date). */
FUNC_EXTERN ADU_AGEND_FUNC adu_E0d_minmax_date;  /* ag-end for min(date) or 
						 ** max(date). 
						 */
FUNC_EXTERN ADU_AGBEGIN_FUNC adu_B0n_agg_dec;   /* ag-begin for all decimal 
						** aggregates
						*/
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N3n_avg_dec;    /* ag-next for avg(decimal) 
						** and avgu(decimal). 
						*/
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N4n_sum_dec;    /* ag-next for sum(decimal) 
						** and sumu(decimal). 
						*/
FUNC_EXTERN ADU_AGEND_FUNC adu_E3n_avg_dec;     /* ag-end for avg(decimal) and
						** avgu(decimal). 
						*/
FUNC_EXTERN ADU_AGEND_FUNC adu_E4n_sum_dec;     /* ag-end for sum(decimal) and
					        ** sumu(decimal).
						*/
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N5n_min_dec;    /* ag-next for min(decimal). */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N6n_max_dec;    /* ag-next for max(decimal). */
FUNC_EXTERN ADU_AGEND_FUNC adu_E0n_minmax_dec;  /* ag-end for min(decimal) and
						** max(decimal) 
						*/
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N3f_avg_f;      /* ag-next for avg(f). */
FUNC_EXTERN ADU_AGEND_FUNC adu_E3f_avg_f;       /* ag-end for avg(f). */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N4f_sum_f;      /* ag-next for sum(f). */
FUNC_EXTERN ADU_AGEND_FUNC adu_E4f_sum_f;       /* ag-end for sum(f). */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N5f_min_f;      /* ag-next for min(f). */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N6f_max_f;      /* ag-next for max(f). */
FUNC_EXTERN ADU_AGEND_FUNC adu_E0f_minmax_f;    /* ag-end for min(f) or max(f)
						*/
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N3i_avg_i;      /* ag-next for avg(i). */
FUNC_EXTERN ADU_AGEND_FUNC adu_E3i_avg_i;       /* ag-end for avg(i). */
FUNC_EXTERN ADU_AGEND_FUNC adu_E4i_sum_i;       /* ag-end for sum(i). */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N4i_sum_i;      /* ag-next for sum(i). */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N5i_min_i;      /* ag-next for min(i). */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N6i_max_i;      /* ag-next for max(i). */
FUNC_EXTERN ADU_AGEND_FUNC adu_E0i_minmax_i;    /* ag-end for min(i) or max(i)
					        */
FUNC_EXTERN ADU_AGBEGIN_FUNC adu_B0l_minmax_logk;/* ag-begin for min(table_key)
						 ** min(object_key), 
						 ** max(object_key),
						 ** max(table_key). 
						 */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N5l_min_logk;   /* ag-next for min(table_key)
						** min(object_key)
						*/
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N6l_max_logk;   /* ag-next for max(table_key)
						** max(object_key), 
						*/
FUNC_EXTERN ADU_AGEND_FUNC adu_E0l_minmax_logk; /* ag-end for min(table_key)
						** min(object_key), 
						** max(object_key),
						** max(table_key). 
						*/
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N3m_avg_mny;    /* ag-next for avg(money). */
FUNC_EXTERN ADU_AGEND_FUNC adu_E3m_avg_mny;     /* ag-end for avg(money). */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N4m_sum_mny;    /* ag-next for sum(money). */
FUNC_EXTERN ADU_AGEND_FUNC adu_E4m_sum_mny;     /* ag-end for sum(money). */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N5m_min_mny;    /* ag-next for min(money). */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N6m_max_mny;    /* ag-next for max(money). */
FUNC_EXTERN ADU_AGEND_FUNC adu_E0m_minmax_mny;  /* ag-end for min(money) or 
					        ** max(money). 
						*/
FUNC_EXTERN ADU_AGBEGIN_FUNC adu_B0o_minmax_sec;/* ag-begin for min(security),
						** max(security). 
						*/
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N5o_min_sec;    /* ag-next for min(security) */
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N6o_max_sec;    /* ag-next for max(secid) */
FUNC_EXTERN ADU_AGEND_FUNC adu_E0o_minmax_sec;  /* ag-end for min(security), 
						** max(security). 
						*/
FUNC_EXTERN ADU_AGBEGIN_FUNC adu_B0s_minmax_str;/* ag-begin for min(c), max(c)
						** min(char), max(char), 
						** min(text), max(text), 
						** min(varchar), max(varchar)
						*/
FUNC_EXTERN ADU_AGEND_FUNC adu_E0s_minmax_str;  /* ag-begin for min(c), max(c)
						** min(char), max(char), 
						** min(text), max(text), 
						** min(varchar), max(varchar)
						*/
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N5s_min_str;    /* ag-next for min(c),
						** min(char), min(text), 
						** min(varchar)
						*/
FUNC_EXTERN ADU_AGNEXT_FUNC adu_N6s_max_str;    /* ag-next for max(c),
						** max(char), max(text), 
						** max(varchar)
						*/
FUNC_EXTERN ADU_AGNEXT2_FUNC adu_N7a_corrr2;	/* ag-next for corr(),
						** regr_r2()
						*/
FUNC_EXTERN ADU_AGNEXT2_FUNC adu_N8a_covsxy;	/* ag-next for covar_pop(),
						** covar_samp() and regr_sxy()
						*/
FUNC_EXTERN ADU_AGNEXT2_FUNC adu_N9a_ravgx;	/* ag-next for regr_avgx() 
						*/
FUNC_EXTERN ADU_AGNEXT2_FUNC adu_N10a_ravgy;	/* ag-next for regr_avgy() 
						*/
FUNC_EXTERN ADU_AGNEXT2_FUNC adu_N11a_rcnt;	/* ag-next for regr_count()
						*/
FUNC_EXTERN ADU_AGNEXT2_FUNC adu_N12a_rintsl;	/* ag-next for regr_intercept(),
						** regr_slope
						*/
FUNC_EXTERN ADU_AGNEXT2_FUNC adu_N13a_rsxx;	/* ag-next for regr_sxx()
						*/
FUNC_EXTERN ADU_AGNEXT2_FUNC adu_N14a_rsyy;	/* ag-next for regr_syy()
						*/
FUNC_EXTERN ADU_OLAPAGEND_FUNC adu_E7a_corr;	/* ag-end for corr()
						*/
FUNC_EXTERN ADU_OLAPAGEND_FUNC adu_E8a_cpop;	/* ag-end for covar_pop()
						*/
FUNC_EXTERN ADU_OLAPAGEND_FUNC adu_E9a_csamp;	/* ag-end for covar_samp()
						*/
FUNC_EXTERN ADU_OLAPAGEND_FUNC adu_E10a_ravgx;	/* ag-end for regr_avgx()
						*/
FUNC_EXTERN ADU_OLAPAGEND_FUNC adu_E11a_ravgy;	/* ag-end for regr_avgy()
						*/
FUNC_EXTERN ADU_OLAPAGEND_FUNC adu_E12a_rcnt;	/* ag-end for regr_count()
						*/
FUNC_EXTERN ADU_OLAPAGEND_FUNC adu_E13a_rint;	/* ag-end for regr_intercept()
						*/
FUNC_EXTERN ADU_OLAPAGEND_FUNC adu_E14a_rr2;	/* ag-end for regr_r2()
						*/
FUNC_EXTERN ADU_OLAPAGEND_FUNC adu_E15a_rslope;	/* ag-end for regr_slope()
						*/
FUNC_EXTERN ADU_OLAPAGEND_FUNC adu_E16a_rsxx;	/* ag-end for regr_sxx()
						*/
FUNC_EXTERN ADU_OLAPAGEND_FUNC adu_E17a_rsxy;	/* ag-end for regr_sxy()
						*/
FUNC_EXTERN ADU_OLAPAGEND_FUNC adu_E18a_rsyy;	/* ag-end for regr_syy()
						*/

/*
**  Forward and/or External function references for keybuild functions.
*/

typedef DB_STATUS ADU_KEYBLD_FUNC (ADF_CB       *adf_scb,
				   ADC_KEY_BLK  *adc_kblk);

FUNC_EXTERN ADU_KEYBLD_FUNC adu_bbldkey;    /* Routine to build a key for the 
					    ** logical_key datatype.
					    */
FUNC_EXTERN ADU_KEYBLD_FUNC adu_cbldkey;    /* Routine to build a key for the
					    ** char, varchar, text, or c 
					    ** datatypes.
					    */
FUNC_EXTERN ADU_KEYBLD_FUNC adu_dbldkey;    /* Routine to build a key for the
					    ** date datatype.
                                            */
FUNC_EXTERN ADU_KEYBLD_FUNC adu_decbldkey;  /* Routine to build a key for the
					    ** decimal datatype.
					    */
FUNC_EXTERN ADU_KEYBLD_FUNC adu_fbldkey;    /* Routine to build a key for the
					    ** float datatype.
					    */
FUNC_EXTERN ADU_KEYBLD_FUNC adu_ibldkey;    /* Routine to build a key for the
					    ** int datatype.
					    */
FUNC_EXTERN ADU_KEYBLD_FUNC adu_mbldkey;    /* Routine to build a key for the
					    ** money datatype.
					    */
FUNC_EXTERN ADU_KEYBLD_FUNC adu_bitbldkey;  /* Create [V]BIT keys */

FUNC_EXTERN ADU_KEYBLD_FUNC adu_patcomp_kbld;  /* Create 'HELEM' PAT keys */
FUNC_EXTERN ADU_KEYBLD_FUNC adu_patcomp_kbld_uni;  /* Create 'HELEM' PAT keys */

FUNC_EXTERN ADU_KEYBLD_FUNC adu_nvchbld_key; /* Routine to build a key for the
                                             ** nchar and nvarchar datatype.
                                             */


/*
**  Forward and/or External function references for compare functions.
*/

typedef DB_STATUS ADU_COMPARE_FUNC (ADF_CB         *adf_scb,
				    DB_DATA_VALUE  *dv1,
				    DB_DATA_VALUE  *dv2,
				    i4             *cmp);

FUNC_EXTERN ADU_COMPARE_FUNC adu_4date_cmp; /* Compare two date data values.
                                            */
FUNC_EXTERN ADU_COMPARE_FUNC adu_1logkey_cmp;/* Compare two logical_key data
					     ** values.
					     */
FUNC_EXTERN ADU_COMPARE_FUNC adu_6mny_cmp;  /* Compare two money data values.
                                            */

FUNC_EXTERN ADU_COMPARE_FUNC adu_ccmp;  /* Routine that compares two c data
                                        ** values for equality.  No pattern-
                                        ** matching allowed.  Blanks are
                                        ** ignored.  Shorter strings are
                                        ** less than longer strings.
                                        */
FUNC_EXTERN ADU_COMPARE_FUNC adu_textcmp;/* Routine that compares two text data
                                         ** values for equality.  No pattern-
                                         ** matching allowed.  Blanks are
                                         ** significant.  Shorter strings are
                                         ** less than longer strings.
                                         */
FUNC_EXTERN ADU_COMPARE_FUNC adu_varcharcmp;

FUNC_EXTERN ADU_COMPARE_FUNC adu_lvarcharcmp;
                                        /* Routine that compares two char or
                                        ** varchar data values for equality.
                                        ** No pattern- matching allowed.  Blanks
                                        ** are significant.  Shorter strings are
                                        ** effectively padded with blanks to the
                                        ** length of longer strings.
                                        */

FUNC_EXTERN ADU_COMPARE_FUNC adu_longcmp;
                                        /* Routine that walks through the basic
                                        ** mechanics of comparing lob types
                                        */

FUNC_EXTERN ADU_COMPARE_FUNC adu_nvchrcomp;
					/* This routing compares between nchar
					** and  nvarchar data types that are
					** used for unicode string data types.
					*/
FUNC_EXTERN ADU_NORM3_FUNC adu_tableinfo;  /* Routine to do the tableinfo()
                                           ** func.
					   */
FUNC_EXTERN ADU_COMPARE_FUNC adu_1pt_cmp;  /* Compare 2 point data values. 
                                           */

FUNC_EXTERN ADU_COMPARE_FUNC adu_rtree_cmp; /* Compare geometries for rtree*/

/*
**  Forward and/or External function references for other ADU routines.
*/


/* convert a date value into its corresponding histogram value. */
FUNC_EXTERN ADU_NORM1_FUNC adu_3datehmap; 
FUNC_EXTERN ADU_NORM1_FUNC adu_5mnyhmap;    


FUNC_EXTERN bool adu_1monthcheck(char    *input,
				 i4	*output);

FUNC_EXTERN i4 adu_2monthsize(i4  month,
			      i4  year);

FUNC_EXTERN DB_STATUS adu_dbconst(ADF_CB         *adf_scb,
				  ADK_MAP        *kmap,
				  DB_DATA_VALUE  *rdv);

/* Compare two strings data values, with optional pattern matching. */
FUNC_EXTERN DB_STATUS adu_lexcomp(ADF_CB          *adf_scb,
				  i4		use_quel_pm,
				  DB_DATA_VALUE	*dv1,
				  DB_DATA_VALUE	*dv2,
				  i4		*rcmp);

/* Compare two data values in 'numeric' mode, with optional pattern matching. */
FUNC_EXTERN DB_STATUS adu_lexnumcomp(ADF_CB     *adf_scb,
				  i4		use_quel_pm,
				  DB_DATA_VALUE	*dv1,
				  DB_DATA_VALUE	*dv2,
				  i4		*rcmp);

/* Compare data values for 'integer' data. */
FUNC_EXTERN DB_STATUS adu_isinteger(ADF_CB     *adf_scb,
				  DB_DATA_VALUE	*dv1,
				  i4		*rcmp);

/* Compare data values for 'decimal' data. */
FUNC_EXTERN DB_STATUS adu_isdecimal(ADF_CB     *adf_scb,
				  DB_DATA_VALUE	*dv1,
				  i4		*rcmp);

/* Compare data values for 'float' data. */
FUNC_EXTERN DB_STATUS adu_isfloat(ADF_CB     *adf_scb,
				  DB_DATA_VALUE	*dv1,
				  i4		*rcmp);

/* Compare two unicode strings with quel pattern matching symantics. */
FUNC_EXTERN DB_STATUS adu_ulexcomp(ADF_CB          *adf_scb,
				  DB_DATA_VALUE   *str_dv,
				  DB_DATA_VALUE   *pat_dv,
				  i4              *rcmp,
				  bool            bcomp);

/* Compare two string data values using the LIKE operator pattern matching 
** characters.
*/
FUNC_EXTERN DB_STATUS adu_like(ADF_CB		*adf_scb,
			       DB_DATA_VALUE	*str_dv,
			       DB_DATA_VALUE	*pat_dv,
			       u_char		*eptr,
			       i4		*rcmp);
FUNC_EXTERN DB_STATUS adu_like_all(ADF_CB	*adf_scb,
				DB_DATA_VALUE	*str_dv,
				DB_DATA_VALUE	*pat_dv,
				DB_DATA_VALUE	*esc_dv,
				u_i4		pat_flags,
				i4		*rcmp);
FUNC_EXTERN DB_STATUS adu_like_comp(ADF_CB	*adf_scb,
				DB_DATA_VALUE	*pat_dv,
				DB_DATA_VALUE	*ret_dv,
				DB_DATA_VALUE	*esc_dv,
				u_i4		pat_flags);
FUNC_EXTERN DB_STATUS adu_like_comp_uni(ADF_CB	*adf_scb,
				DB_DATA_VALUE	*pat_dv,
				DB_DATA_VALUE	*ret_dv,
				DB_DATA_VALUE	*esc_dv,
				u_i4		pat_flags);

VOID adu_pat_MakeCEchar(	ADF_CB		*adf_scb,
				const UCS2	**rdp,
				const UCS2	*rdend,
				UCS2		**wrtp,
				u_i4		sea_flags);
DB_STATUS adu_pat_MakeCElen(	ADF_CB		*adf_scb,
				const UCS2	*rdp,
				const UCS2	*rdend,
				u_i4		*reslen);


FUNC_EXTERN char *adu_13strtocstr(DB_DATA_VALUE  *dv1,
				  char           *buf);

/* get the length & address of an Ingres character string data value */
FUNC_EXTERN DB_STATUS	adu_lenaddr(ADF_CB         *adf_scb,
				    DB_DATA_VALUE  *dv,
				    i4		   *len,
				    char           **addr);

/* move a string into a DB_DATA_VALUE making sure that the result length
** isn't exceeded and convert non-printing chars to blanks if result
** is the c datatype. 
*/
FUNC_EXTERN DB_STATUS	adu_movestring(ADF_CB         *adf_scb,
				       u_char         *source,
				       i4	      sourcelen,
				       DB_DT_ID	      src_type,
				       DB_DATA_VALUE  *dest);

/* move a string into a DB_DATA_VALUE this version of movestring takes 
** a UCS2 as input and it will be used when we know that the 
** input string type is either DB_NCHR_TYPE or DB_NVCHR_TYPE
** This is used to facilitate coercion of unicode types.
*/
FUNC_EXTERN DB_STATUS   adu_moveunistring(ADF_CB         *adf_scb,
                                       UCS2         *source,
                                       i4        sourcelen,
                                       DB_DATA_VALUE  *dest);

/* get the size of an Ingres character string (without trailing blanks for
** c and char).
*/
FUNC_EXTERN DB_STATUS	adu_size(ADF_CB         *adf_scb,
				 DB_DATA_VALUE  *dv,
				 i4        *size);

/* Compute collation_weight() of a non-Unicode string value. */
FUNC_EXTERN DB_STATUS adu_collweight(ADF_CB	*adf_scb,
				DB_DATA_VALUE	*src_dv,
				DB_DATA_VALUE	*res_dv);

/* find the address of the character string for an Ingres character string
** data value.
*/
FUNC_EXTERN DB_STATUS	adu_3straddr(ADF_CB         *adf_scb,
				     DB_DATA_VALUE  *dv,
				     char           **str_ptr); 


/* get the size of an Ingres character string data value (including trailing
** blanks if the datatype is c or char).
*/
FUNC_EXTERN DB_STATUS   adu_5strcount(ADF_CB         *adf_scb,
				      DB_DATA_VALUE  *dv,
				      i4             *str_len);

/* find the address of the unicode character string for an Ingres 
** unicode character string data value.
*/
FUNC_EXTERN DB_STATUS   adu_7straddr(ADF_CB         *adf_scb,
                                     DB_DATA_VALUE  *dv,
                                     UCS2           **str_ptr);


/* unicode functions for size, convert, dbtoev and concat. */

/* Routine to find the size for DB_NVCHR_STRING */
FUNC_EXTERN DB_STATUS adu_nvchr_size(ADF_CB		*scb,
				     DB_DATA_VALUE	*dv_1,
				     DB_DATA_VALUE	*dv_result);

/* Routine to convert to/from DB_NVCHR_STRING */
FUNC_EXTERN DB_STATUS adu_nvchr_convert(ADF_CB		*scb,
					DB_DATA_VALUE	*dv_in,
					DB_DATA_VALUE	*dv_out);

/* Routine which external data type to convert to DB_NVCHR_STRING */
FUNC_EXTERN DB_STATUS adu_nvchr_dbtoev(ADF_CB		*scb,
				       DB_DATA_VALUE	*db_value,
				       DB_DATA_VALUE	*dv_result);

/* Routine to concatenate two DB_NVCHR_STRING */
FUNC_EXTERN DB_STATUS adu_nvchr_concat(ADF_CB		*scb,
				       DB_DATA_VALUE	*dv_1,
				       DB_DATA_VALUE	*dv_2,
				       DB_DATA_VALUE	*dv_result);

/* Compare two string data values using the LIKE operator pattern matching
** characters.
*/
FUNC_EXTERN DB_STATUS adu_ulike(ADF_CB		*adf_scb,
				DB_DATA_VALUE	*str_dv,
				DB_DATA_VALUE	*pat_dv,
				UCS2		*eptr,
				i4		*rcmp);

/* LIKE on c/text/char/varchar in UTF8 server. */
FUNC_EXTERN DB_STATUS adu_nvchr_utf8like(ADF_CB	*adf_scb,
				DB_DATA_VALUE	*str_dv,
				DB_DATA_VALUE	*pat_dv,
				char		*eptr,
				i4		*rcmp);

/* Compute collation_weight() of a Unicode value. */
FUNC_EXTERN DB_STATUS adu_ucollweight(ADF_CB	*adf_scb,
				DB_DATA_VALUE	*src_dv,
				DB_DATA_VALUE	*res_dv);

/* Compute collation_weight() of a Unicode value, providing override
** collation ID. */
FUNC_EXTERN DB_STATUS adu_ucollweightn(ADF_CB	*adf_scb,
				DB_DATA_VALUE	*src_dv,
				DB_DATA_VALUE	*col_dv,
				DB_DATA_VALUE	*res_dv);

/* Routine for left function for DB_NVCHR_STRING */
FUNC_EXTERN DB_STATUS adu_nvchr_left(ADF_CB           *scb,
                                       DB_DATA_VALUE    *dv_1,
                                       DB_DATA_VALUE    *dv_2,
                                       DB_DATA_VALUE    *dv_result);

/* Routine for right functions for  DB_NVCHR_STRING */
FUNC_EXTERN DB_STATUS adu_nvchr_right(ADF_CB           *scb,
                                       DB_DATA_VALUE    *dv_1,
                                       DB_DATA_VALUE    *dv_2,
                                       DB_DATA_VALUE    *dv_result);

/* Routine for length function for  DB_NVCHR_STRING */
FUNC_EXTERN DB_STATUS adu_nvchr_length(ADF_CB           *scb,
                                       DB_DATA_VALUE    *dv_1,
                                       DB_DATA_VALUE	*dv_result);

/* Routine for trim function for DB_NVCHR_STRING */
FUNC_EXTERN DB_STATUS adu_nvchr_trim(ADF_CB           *scb,
                                       DB_DATA_VALUE    *dv_1,
                                       DB_DATA_VALUE    *dv_result);

/* Routine for coercing nchar and nvarchar types */
FUNC_EXTERN DB_STATUS adu_nvchr_coerce(ADF_CB           *scb,
                                       DB_DATA_VALUE    *dv_1,
                                       DB_DATA_VALUE    *dv_2);

/* Support the two argument version of nvarchar() */
FUNC_EXTERN ADU_NORM2_FUNC adu_nvchr_2args;

/* Routine for locate function for nchar and nvarchar types */
FUNC_EXTERN DB_STATUS adu_nvchr_locate(ADF_CB           *scb,
                                       DB_DATA_VALUE    *dv_1,
                                       DB_DATA_VALUE    *dv_2,
                                       DB_DATA_VALUE    *dv_result);

/* Routine for shift function for  nchar and nvarchar type */
FUNC_EXTERN DB_STATUS adu_nvchr_shift(ADF_CB           *scb,
                                       DB_DATA_VALUE    *dv_1,
                                       DB_DATA_VALUE    *dv_2,
                                       DB_DATA_VALUE    *dv_result);

/* Routine for charextract functions for  nvhar and nvarchar type */
FUNC_EXTERN DB_STATUS adu_nvchr_charextract(ADF_CB           *scb,
                                       DB_DATA_VALUE    *dv_1,
                                       DB_DATA_VALUE    *dv_2,
                                       DB_DATA_VALUE    *dv_result);

/* Routine for substr1 functions for  nvhar and nvarchar type */
FUNC_EXTERN DB_STATUS adu_nvchr_substr1(ADF_CB           *scb,
                                       DB_DATA_VALUE    *dv_1,
                                       DB_DATA_VALUE    *dv_2,
                                       DB_DATA_VALUE    *dv_result);

/* Routine for substr2 functions for  nvhar and nvarchar type */
FUNC_EXTERN DB_STATUS adu_nvchr_substr2(ADF_CB           *scb,
                                       DB_DATA_VALUE    *dv_1,
                                       DB_DATA_VALUE    *dv_2,
                                       DB_DATA_VALUE    *dv_3,
                                       DB_DATA_VALUE    *dv_result);

/* Routine for pad function for nchar and nvarchar types */
FUNC_EXTERN DB_STATUS adu_nvchr_pad(ADF_CB           *scb,
                                    DB_DATA_VALUE    *dv_1,
                                    DB_DATA_VALUE    *dv_result);

/* Routine for sqeezewhite function for nchar and nvarchar types */
FUNC_EXTERN DB_STATUS adu_nvchr_squeezewhite
				   (ADF_CB           *scb,
                                    DB_DATA_VALUE    *dv_1,
                                    DB_DATA_VALUE    *dv_result);

/* Routine for coercing unicode nchar and nvarchar to char and varchar types */
FUNC_EXTERN DB_STATUS adu_nvchr_unitochar(ADF_CB           *scb,
                                          DB_DATA_VALUE    *dv_1,
                                          DB_DATA_VALUE    *dv_2);

/* Routine for coercing unicode nchar and nvarchar to char and varchar types */
FUNC_EXTERN DB_STATUS adu_nvchr_strutf8conv(ADF_CB           *scb,
                                          DB_DATA_VALUE    *dv_1,
                                          DB_DATA_VALUE    *dv_2);

FUNC_EXTERN VOID adu_prdv(DB_DATA_VALUE  *db_dv);

FUNC_EXTERN VOID adu_prtype(DB_DATA_VALUE  *db_dv);

FUNC_EXTERN VOID adu_prvalue(DB_DATA_VALUE  *db_dv);

FUNC_EXTERN VOID adu_2prvalue(i4	     (*fcn)(char *, ...),
			      DB_DATA_VALUE  *db_dv);

FUNC_EXTERN VOID adu_prinstr(ADI_FI_ID  *instr);

FUNC_EXTERN DB_STATUS adu_lolk(ADF_CB             *adf_scb,
			       DB_DATA_VALUE      *cpn_dv,
			       DB_DATA_VALUE      *res_dv);

FUNC_EXTERN DB_STATUS      adu_10secid_rawcmp(ADF_CB          *adf_scb,
				         DB_DATA_VALUE	*seclid1,
				         DB_DATA_VALUE	*seclid2,
				         i4		op,
				         i4		*cmp);

FUNC_EXTERN ADU_NORM2_FUNC adu_13secid_mac;  /*  iimacaccess */
FUNC_EXTERN ADU_NORM1_FUNC adu_sesspriv;     /*  session_priv */
FUNC_EXTERN ADU_NORM1_FUNC adu_iitblstat;    /*  iitblstat */
FUNC_EXTERN DB_STATUS      adu_10seclbl_rawcmp(ADF_CB          *adf_scb,
				         DB_DATA_VALUE	*seclbl1,
				         DB_DATA_VALUE	*seclbl2,
				         i4		op,
				         i4		*cmp);

FUNC_EXTERN ADU_NORM1_FUNC adu_11seclbl_byte;
FUNC_EXTERN ADU_NORM1_FUNC adu_12byte_seclbl;
FUNC_EXTERN ADU_NORM1_FUNC adu_14secid_byte;
FUNC_EXTERN ADU_NORM1_FUNC adu_15byte_secid;
FUNC_EXTERN ADU_NORM2_FUNC adu_13seclbl_disjoint;
FUNC_EXTERN ADU_NORM2_FUNC adu_perm_type;
FUNC_EXTERN ADU_NORM2_FUNC adu_greatest;
FUNC_EXTERN ADU_NORM2_FUNC adu_least;
FUNC_EXTERN DB_STATUS adu_nvchr_toupper( ADF_CB              *scb,
					DB_DATA_VALUE       *dv1,
					DB_DATA_VALUE       *rdv);
FUNC_EXTERN DB_STATUS adu_nvchr_tolower( ADF_CB              *scb,
					DB_DATA_VALUE       *dv1,
					DB_DATA_VALUE       *rdv);
FUNC_EXTERN DB_STATUS adu_nvchr_utf8comp( ADF_CB              *scb,
					i4		    usepmquel,
					DB_DATA_VALUE       *dv1,
					DB_DATA_VALUE       *dv2,
					i4		    *rcmp);
FUNC_EXTERN DB_STATUS adu_utf8cbldkey( ADF_CB          *adf_scb,
					i4              semantics,
					ADC_KEY_BLK     *key_block);
FUNC_EXTERN DB_STATUS adu_nvchr_utf8_bldkey( ADF_CB          *scb,
					i4              semantics,
					ADC_KEY_BLK     *key_block);
FUNC_EXTERN ADU_NORM1_FUNC adu_numeric_norm;
FUNC_EXTERN ADU_NORM2_FUNC adu_aesdecrypt;
FUNC_EXTERN ADU_NORM2_FUNC adu_aesencrypt;
