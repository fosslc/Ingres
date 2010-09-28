/*
** Copyright (c) 2004 Ingres Corporation 
*/

#include <stdio.h>
#include <string.h>

/**
** Name: IIADD.H - User Defined Abstract DataType Definition Header
**
** Description:
**	This file contains those special symbols which are used only for the
**	dynamic adding of datatypes to the INGRES system.  The datatypes
**	defined in this file are used primarily at program [of whatever
**	variety] startup time.  That is, these structures are used to pass
**	datatype definition information to the INGRES system at the time the
**	program, be it a DBMS server, /STAR server, gateway, or application,
**	starts.
**
**
** History: 
**      24-may-89 (fred)
**          Created from internal file add.h
**      09-dec-1992 (stevet)
**          Release 7.0 modifications. Replace er_generic_err with 
**          er_sqlstate_err in II_ERR_STRUCT to support SQLSTATE. 
**          Added lenspec_routine to IIADD_FI_DFN to support user defined
**          result length calculations.
**      10-jan-1993 (stevet)
**          Change II_MAXNAME to 32.
**       4-May-1993 (fred)
**          Added support for large objects.
**	17-jun-1993 (shailaja)
**	    Fixed compiler warnings.
**      01-sep-1993 (stevet)
**          Added symbols for INGRES class objects.  Added GLOBALDEF and
**          GLOBALREF.
**      13-Oct-1993 (fred)
**          Added seglen function for objects whose size requires more
**          granularity than just the byte.
**      08-nov-1993 (stevet)
**          Added symbols for LONG POLYGON and LONG LINE.  Added
**          declaration for IIclsadt_register().  Also added II_ALL_TYPE.
**      05-nov-1993 (smc/swm)
**          Bug #58635
**          Bug #58879
**          Made structure elements type compatable with those defined
**          within the standard header so that we are size and alignment
**	    equivalent. 
**	    The add_l_reserved and add_owner elements are PTR in the DMF
**	    standard header, so these are now char * here.
**	    The add_length is i4 in the DMF standard header, so this needs
**	    to be a scalar type of equivalent size in the <iiadd.h> given
**	    to customers. So make add_length int for axp_osf and continue
**	    to default to long on other platforms.
**	    Similarly use sizeof(int) rather than sizeof(long) in the
**	    I4ASSIGN_MACRO which also assumes that a long is 4 bytes.
**	    (Is there some obscure reason why the literal value 4 is not
**	    used?)
**	05-feb-1994 (stevet)
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.  In this instance owner was missing from 
**          the 'standard header'.  This change was made to ADP_POP_CB 
**          but not made to II_POP_CB. (B59747)
**      16-feb-1994 (stevet)
**          Added function declarations without prototypes. On machine like
**          HP/UX, ANSI C compiler is optional so function prototype
**          cannot be used.
**      21-mar-1994 (johnst)
**          Bug #60764
**          Made structure elements type equivalent to those defined
**          in the 'standard header' and other internal header files, 
**	    so that struct members declared in this file correctly match
**	    the size and alignment of the corresponding internal members.
**	    In particular, all variables that were previously declared as type
**	    long must be declared as int's for axp_osf (DEC Alpha/OSF1, where
**	    int's are 32-bit and long's are 64-bit), but will continue to 
**          default to long on other platforms. But rather than add masses of
**	    porting #ifdef's for conditional long/int declarations, I changed 
**	    the long declarations to use the new types i4/u_i4 and 
**	    conditionally typedef'd them to be either long/int as appropriate,
**	    so the default situation remains unchanged for all other platforms.
**	12-apr-1994 (swm)
**	    Bug #62658
**	    Added typedef declarations for for non-prototyped builds
**	    (where __STDC__ is not defined).
**	16-feb-1995 (shero03)
**	    Bug #65565
**	    Support xform routine for copy out and into
** 	11-apr-1995 (wonst02)
**	    Bug #68008
** 	    Add the II_DECIMAL type, which looks just like
** 	    the PNUM user-defined type.
**	21-feb-1996 (toumi01)
**	    Change "axp_osf" to "__alpha" so conditionality depends only on
**	    standard DEC headers, not on the CA ingres build environment,
**	    so that the condition will work correctly in adf/ads.
**	28-mar-1996 (toumi01)
**	    Increased size of II_COUPON from 20 bytes to 24 bytes for
**	    __alpha (axp_osf) to allow for 8-byte pointer.  Added dummy
**	    long "Align" variable to II_PERIPHERAL union to force alignment
**	    to accommodate pointer in coupon.
**	16-may-1996 (shero03)
**	    Support variable length UDTs.
**	3-jul-96 (inkdo01)
**	    Add new operation type code for predicate functions.
**	07-jan-1998 (shero03)
**	    Add BYTE_SWAP directive.
**	26-mar-1998 (shust01)
**	    changed all references of __alpha to 
**	    #if defined(__alpha) && defined(__osf__) 
**	    since the changes inside the #if statements all involve a 
**	    64 bite platform, which is Alpha/OSF.  However, it should not 
**	    be for Alpha/VMS (which also has __alpha set). bug 87302.
**	08-oct-1998 (shero03)
**	    
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	17-Feb-1999 (shero03)
**	    Support 4 operands
**	21-apr-1999 (somsa01)
**	    Added pop_info to II_POP_CB.
**      25-may-1999 (hweho01)
**          Added support for ris_u64 (AIX 64-bit platform):
**          1) defined appropriate storage type to hold 4-byte integer,
**          2) force alignment for coupon PTR in data structure
**             II_PERIPHERAL.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-apr-1999 (hanch04)
**	    Added INT_MIN, INT_MAX, UINT_MAX 
**	    Blob coupon for LP64 needs to be bigger.
**	08-Sep-2000 (hanje04)
**	    Added support for Alpha Linux (axp_lnx).
**	20-feb-2001 (gupsh01)
**	    Added constants for new data type nvarchar for unicode 
**	    variable length text type. 
**      16-Aug-2001 (hweho01)
**          Avoid compiler error redefining INT_MIN, INT_MAX and UINT_MAX
**          on ris_u64 and rs4_us5 platforms.
**	08-jan-2002 (somsa01)
**	    Changes for Win64.
**	09-Sep-2002 (hanch04)
**	    i4 should be an int for all platforms
**      13-Sep-2002 (hanch04)
**          Blob coupon does not need to be bigger anymore.
**	    Structure _II_SCB needs to match 64-bit ADF_SCB
**	    include stdio to make sure system defines such as LP64 are set
**      20-Sep-2002 (hweho01)
**          The hybrid build on AIX will handle the 32 - 64 bit upgrade 
**          (platform string settings : config=rs4_us5 config64=r64_us5),  
**          the ADP_COUPON (in adp.h) must be 5 words, so the struct        
**          II_COUPON in this file needs to match with it. 
**	28-Mar-2003 (hanje04)
**	    Add LP64 to __alpha defines so that changes are taken for all	**	    64bit platforms.
**      01-Apr-2003 (hanje04)
**          Remove LP64 and use __x86_64__ instead.
**      12-Apr-2004 (stial01)
**          Define add_length, pop_length, adw_length as size_t.
**      29-Aug-2004 (kodse01)
**          BUG 112917
**          Added i8 datatype.
**	19-Jan-2005 (drivi01)
**	    Added a db_collID to II_DATA_VALUE to be consistent with
**	    DB_DATA_VALUE in iicommon.h
**	28-Mar-2005 (lakvi01)
**	    Added changes for Windows 64-bit.
**	25-Oct-2005 (hanje04)
**	    Include <string.h> to quiet compiler errors with GCC 4.0
**      13-Feb-2007 (hanal04) Bug 117665
**          Added typedefs for i2 and u_i2. i2 is needed by the DECIMAL
**          MACROs appended from iicommon.h
**      19-Feb-2007 (hanal04) Bug 117665
**          iipk.h may have already defined u_i2. As iiadd.h and iipk.h
**          are supplied files we should only typedef u_i2 if iipk.h has not
**          already done so.
**	29-Feb-2008 (kiria01) SIR 117944
**	    Updated to fuller list and improved the comments
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
**      24-nov-2009 (joea)
**          Change II_BOOLEAN to 38.  Add missing operators.
**      17-Aug-2010 (thich01)
**          Add spatial II types and spatial II ops.
**  19-Aug-2010 (thich01)
**      Left a space at 60 for GCA SECL TYPE.
*/
/*
**	11-may-2006
**	    Modified by DATAllegro.com
*/

/*}
** Name: PTR -- Generic pointer -- placeholder for void *
*/

#ifdef __STDC__
typedef void *	II_PTR;
#else
typedef char *  II_PTR;
#endif

/*}
** Name: GLOBALDEF and GLOBALREF
*/
#ifdef VMS
#define     GLOBALDEF           globaldef
#define	    GLOBALREF		globalref
#else
#define	    GLOBALDEF
#define	    GLOBALREF		extern
#endif

/*}
** Name: i1 - data type large enough to hold 1-byte integers.
*/
typedef char		i1;
#if !defined(u_i1)
typedef unsigned char	u_i1;
#endif

/*}
** Name: i2 - data type large enough to hold 2-byte integers.
*/
typedef short		i2;
#if !defined(u_i2)
typedef unsigned short	u_i2;
#endif

/*}
** Name: i4 - data type large enough to hold 4-byte integers.
*/
typedef int 		i4;
typedef unsigned int 	u_i4;

/*}
** Name: i8 - data type large enough to hold 8-byte integers.
*/
#ifdef LP64
#ifdef WIN64
    typedef LONG64 i8;
    typedef unsigned LONG64 u_i8;
#else
    typedef long i8;
    typedef unsigned long u_i8;
#endif
#else /* LP64 */
#ifdef WNT
    typedef __int64 i8;
    typedef unsigned __int64 u_i8;
#else
    typedef long long i8;
    typedef unsigned long long u_i8;
#endif
#endif  /* LP64 */

/*}
** Values defined from <limits.h>
*/
#if !defined(any_aix)
#ifndef INT_MIN
#define INT_MIN         (-2147483647-1) /* min value of an "int" */
#endif
#ifndef INT_MAX
#define INT_MAX         2147483647      /* max value of an "int" */
#endif
#ifndef UINT_MAX
#define UINT_MAX        4294967295U     /* max value of an "unsigned int" */
#endif
#endif   /* not aix */

#define I4_MIN		INT_MIN
#define I4_MAX		INT_MAX
#define UI4_MAX		UINT_MAX

/*}
** Name: II_STATUS - Status value to return to INGRES
**
** Description:
**      This type is used to return success/failure indicators to internal
**	INGRES code.
**
** History:
**      24-may-89 (fred)
**          Created.
[@history_template@]...
*/

typedef int II_STATUS;
#define                 II_OK		((II_STATUS) 0)
#define			II_INFO		((II_STATUS) 2)
#define                 II_ERROR        ((II_STATUS) 5)
#define                 II_TARGET_EXHAUSTED     ((II_STATUS) 7)
#define                 II_SOURCE_EXHAUSTED     ((II_STATUS) 8)

/*}
** Name: II_DT_NAME - Name of an INGRES datatype or operator.
**
** Description:
**      This structure contains the name for an INGRES datatype or operator.
**	The structure is an array of 24 characters.  If the name is less than 24
**	characters long, the character position after the last character should
**	contain a null byte (byte with integral value 0).  Thus, the name CHAR
**	would be represented as 'c','h','a','r','\000'.
**
** History:
**      24-may-89 (fred)
**	    Created.
[@history_template@]...
*/
typedef struct _IIDT_NAME
{
#define			II_MAXNAME	    32	/* Size of Name for	    */
						/* internal objects	    */
    char            name[II_MAXNAME];		/* Name of object being defined. */
}   II_DT_NAME;

/*}
** Name: II_DT_ID - Datatype id
**
** Description:
**      This type is used to define the internal identifier for INGRES datatypes.
**
** History:
**      23-may-89 (fred)
**          Created.
**      01-sep-1993 (stevet)
**          Added INGRES class objects symbols.
** 	11-apr-1995 (wonst02)
**	    Bug #68008
** 	    Add the II_DECIMAL type, which looks just like
** 	    the PNUM user-defined type.
**	04-apr-1997 (shero03)
**	    Add NBR3D
**	29-Feb-2008 (kiria01) SIR 117944
**	    Updated to fuller list and improved the comments
**
*/
typedef short II_DT_ID;
#define                 II_NODT         ((II_DT_ID) 0)
#define                 II_LOW_CLSOBJ   ((II_DT_ID) 8192)
#define			II_HIGH_CLSOBJ	((II_DT_ID) 8192+128)

#define                 II_LOW_USER     ((II_DT_ID) 16384)
#define			II_HIGH_USER	((II_DT_ID) 16384+128)

/*
**  INGRES Internal Datatype Values
*/
#define			II_BOOLEAN	((II_DT_ID)  38)
    /*  Booleans are stored as a single byte that can only take the values
    **  0 or 1.
    */
#define			II_MONEY	((II_DT_ID)  5)
    /* Money is stored as a single f8
    */
#define			II_DECIMAL	((II_DT_ID) 10)
    /* Packed decimals are stored as a sequence of 4-bit decimal digits
    ** whose sign is encoded in the first 4 bits and length and scale are in
    ** the precision field.
    */
#define			II_CHAR		((II_DT_ID) 20)
    /* Char is stored to full length with spaces padded if need
    */
#define			II_VARCHAR	((II_DT_ID) 21)
    /* Varchar is stored as a 2 byte integer at the head of the data value.
    ** The actual data then follows the byte length
    */
#define			II_LVCH		((II_DT_ID) 22)
    /* Long varchar is stored as a peripheral data type whoes implementation
    ** is hidden behind the coupon that represents it.
    */
#define			II_BYTE		((II_DT_ID) 23)
    /* Byte is stored to full length with ASCII NUL padded if need
    */
#define			II_VBYTE	((II_DT_ID) 24)
    /* Varbyte is stored as a 2 byte integer at the head of the data value.
    ** The actual data then follows the length
    */
#define			II_LBYTE	((II_DT_ID) 25)
    /* Long varbyte is stored as a peripheral data type whoes implementation
    ** is hidden behind the coupon that represents it.
    */
#define			II_NCHAR	((II_DT_ID) 26)
    /* Nchar is stored to full length with UTF-16 spaces padded if needed
    */
#define			II_NVARCHAR	((II_DT_ID) 27)
    /* Nvarchar is stored as a 2 byte integer at the head of the data value.
    ** The actual data then follows the length. Note that the data will be in
    ** UTF-16 code points and the length field is a count of these and is not
    ** a lenghth in bytes.
    */
#define			II_LNVCH	((II_DT_ID) 28)
    /* Long nvarchar is stored as a peripheral data type whoes implementation
    ** is hidden behind the coupon that represents it.
    */
#define			II_INTEGER	((II_DT_ID) 30)
    /* Integer is stored as either 1, 2, 4 or 8 bytes as determined by the
    ** length field
    */
#define			II_FLOAT	((II_DT_ID) 31)
    /* Float is stored as either 4 or 8 bytes as determined by the
    ** length field
    */
#define			II_C		((II_DT_ID) 32)
    /* C type is stored to full length with spaces padded if needed. If
    ** compares are performed in the context of C type then all spaces
    ** are ignored.
    */
#define			II_TEXT		((II_DT_ID) 37)
    /* Text is stored as a 2 byte integer at the head of the data value.
    ** The actual data then follows the byte length. Note that trailing
    ** spaces are significant compared to varchar.
    */
#define			II_ALL_TYPE	((II_DT_ID) 39)
    /* All type is used to specify function parameters that will be type
    ** check internally and coerced as needed.
    */
#define                 II_LONGTEXT     ((II_DT_ID) 41)
    /* Long text is an internal datatype which is stored like varchar. All
    ** datatypes must be coercable to/from this datatype. This type is also
    ** used for representing a typeless NULL. This data type may never be
    ** stored in the database.
    */
#define 		II_GEOM 	((II_DT_ID) 56)
#define 		II_POINT 	((II_DT_ID) 57)
#define 		II_MPOINT 	((II_DT_ID) 58)
#define 		II_LINE 	((II_DT_ID) 59)
#define 		II_MLINE 	((II_DT_ID) 61)
#define 		II_POLY 	((II_DT_ID) 62)
#define 		II_MPOLY 	((II_DT_ID) 63)
#define 		II_NBR 		((II_DT_ID) 64)
#define 		II_GEOMC 	((II_DT_ID) 65)
    /* New spatial datatypes.  These are internal types and are all LBYTE data
    ** under the 'covers.'
    */

   /* Spatial datatypes */
#define	    PNT_TYPE	     II_LOW_CLSOBJ
#define     BOX_TYPE        (II_LOW_CLSOBJ + 1)
#define     POLY_TYPE       (II_LOW_CLSOBJ + 2)
#define     LSEG_TYPE       (II_LOW_CLSOBJ + 3)
#define     LINE_TYPE       (II_LOW_CLSOBJ + 4)
#define     CIRCLE_TYPE     (II_LOW_CLSOBJ + 5)
#define     LLINE_TYPE      (II_LOW_CLSOBJ + 6)
#define     LPOLY_TYPE      (II_LOW_CLSOBJ + 7)
#define     NBR_TYPE        (II_LOW_CLSOBJ + 8)
#define     NBR3D_TYPE      (II_LOW_CLSOBJ + 9)
#define	    IPNT_TYPE	    (II_LOW_CLSOBJ + 10)
#define     IBOX_TYPE       (II_LOW_CLSOBJ + 11)
#define     IPOLY_TYPE      (II_LOW_CLSOBJ + 12)
#define     ILSEG_TYPE      (II_LOW_CLSOBJ + 13)
#define     ILINE_TYPE      (II_LOW_CLSOBJ + 14)
#define     ICIRCLE_TYPE    (II_LOW_CLSOBJ + 15)
#define     LILINE_TYPE     (II_LOW_CLSOBJ + 16)
#define     LIPOLY_TYPE     (II_LOW_CLSOBJ + 17)

#define			II_LEN_UNKNOWN	-1

/*}
** Name: II_OP_ID - Operator Identifier
**
** Description:
**      This type describes the internal numeric identifier used to identify an
**	operator or function.  Once in use, this identifier should not change
**	for a particular operator.
**
** History:
**      23-may-89 (fred)
**          Created.
**      01-sep-1993 (stevet)
**          Added INGRES class objects symbols.
**	23-oct-05 (inkdo01)
**	    Added II_COUNT_OP and II_SUM_OP.
**	29-Feb-2008 (kiria01) SIR 117944
**	    Updated to fuller list and improved the comments
*/
typedef short II_OP_ID;
#define                 II_CLSOBJ_OPSTART      ((II_OP_ID) 8192)
#define                 II_OPSTART             ((II_OP_ID) 16384)
#define                 II_NOOP                ((II_OP_ID) -1)
/*
**  INGRES supplied operator ID's
*/
#define II_NE_OP             (II_OP_ID)  0  /* !=                      */
#define II_MUL_OP            (II_OP_ID)  1  /* *                       */
#define II_POW_OP            (II_OP_ID)  2  /* **                      */
#define II_ADD_OP            (II_OP_ID)  3  /* +                       */
#define II_SUB_OP            (II_OP_ID)  4  /* -                       */
#define II_DIV_OP            (II_OP_ID)  5  /* /                       */
#define II_LT_OP             (II_OP_ID)  6  /* <                       */
#define II_LE_OP             (II_OP_ID)  7  /* <=                      */
#define II_EQ_OP             (II_OP_ID)  8  /* =                       */
#define II_GT_OP             (II_OP_ID)  9  /* >                       */
#define II_GE_OP             (II_OP_ID) 10  /* >=                      */
#define II_BNTIM_OP          (II_OP_ID) 11  /* _bintim                 */
#define II_BIOC_OP           (II_OP_ID) 12  /* _bio_cnt                */
#define II_CHRD_OP           (II_OP_ID) 13  /* _cache_read             */
#define II_CHREQ_OP          (II_OP_ID) 14  /* _cache_req              */
#define II_CHRRD_OP          (II_OP_ID) 15  /* _cache_rread            */
#define II_CHSIZ_OP          (II_OP_ID) 16  /* _cache_size             */
#define II_CHUSD_OP          (II_OP_ID) 17  /* _cache_used             */
#define II_CHWR_OP           (II_OP_ID) 18  /* _cache_write            */
#define II_CPUMS_OP          (II_OP_ID) 19  /* _cpu_ms                 */
#define II_BDATE_OP          (II_OP_ID) 20  /* _date                   */
#define II_DIOC_OP           (II_OP_ID) 21  /* _dio_cnt                */
#define II_ETSEC_OP          (II_OP_ID) 22  /* _et_sec                 */
#define II_PFLTC_OP          (II_OP_ID) 23  /* _pfault_cnt             */
#define II_PHPAG_OP          (II_OP_ID) 24  /* _phys_page              */
#define II_BTIME_OP          (II_OP_ID) 25  /* _time                   */
#define II_VERSN_OP          (II_OP_ID) 26  /* _version                */
#define II_WSPAG_OP          (II_OP_ID) 27  /* _ws_page                */
#define II_ABS_OP            (II_OP_ID) 28  /* abs                     */
#define II_ANY_OP            (II_OP_ID) 29  /* any                     */
#define II_ASCII_OP          (II_OP_ID) 30  /* ascii or c              */
#define II_ATAN_OP           (II_OP_ID) 31  /* atan                    */
#define II_AVG_OP            (II_OP_ID) 32  /* avg                     */
#define II_DBMSI_OP          (II_OP_ID) 33  /* dbmsinfo                */
#define II_CHAR_OP           (II_OP_ID) 34  /* char                    */
#define II_CNCAT_OP          (II_OP_ID) 35  /* concat                  */
#define II_TYNAM_OP          (II_OP_ID) 36  /* typename                */
#define II_COS_OP            (II_OP_ID) 37  /* cos                     */
#define II_CNT_OP            (II_OP_ID) 38  /* count                   */
#define II_CNTAL_OP          (II_OP_ID) 39  /* count*                  */
#define II_DATE_OP           (II_OP_ID) 40  /* date                    */
#define II_DPART_OP          (II_OP_ID) 41  /* date_part               */
#define II_DTRUN_OP          (II_OP_ID) 42  /* date_trunc              */
#define II_DBA_OP            (II_OP_ID) 43  /* dba                     */
#define II_DOW_OP            (II_OP_ID) 44  /* dow                     */
#define II_ISNUL_OP          (II_OP_ID) 45  /* is null                 */
#define II_NONUL_OP          (II_OP_ID) 46  /* is not null             */
#define II_EXP_OP            (II_OP_ID) 47  /* exp                     */
#define II_F4_OP             (II_OP_ID) 48  /* float4                  */
#define II_F8_OP             (II_OP_ID) 49  /* float8                  */
#define II_I1_OP             (II_OP_ID) 50  /* int1                    */
#define II_I2_OP             (II_OP_ID) 51  /* int2                    */
#define II_I4_OP             (II_OP_ID) 52  /* int4                    */
#define II_INTVL_OP          (II_OP_ID) 53  /* interval                */
#define II_LEFT_OP           (II_OP_ID) 54  /* left                    */
#define II_LEN_OP            (II_OP_ID) 55  /* length                  */
#define II_LOC_OP            (II_OP_ID) 56  /* locate                  */
#define II_LOG_OP            (II_OP_ID) 57  /* log                     */
#define II_LOWER_OP          (II_OP_ID) 58  /* lowercase               */
#define II_MAX_OP            (II_OP_ID) 59  /* max                     */
#define II_MIN_OP            (II_OP_ID) 60  /* min                     */
#define II_MOD_OP            (II_OP_ID) 61  /* mod                     */
#define II_MONEY_OP          (II_OP_ID) 62  /* money                   */
#define II_PAD_OP            (II_OP_ID) 63  /* pad                     */
#define II_RIGHT_OP          (II_OP_ID) 64  /* right                   */
#define II_SHIFT_OP          (II_OP_ID) 65  /* shift                   */
#define II_SIN_OP            (II_OP_ID) 66  /* sin                     */
#define II_SIZE_OP           (II_OP_ID) 67  /* size                    */
#define II_SQRT_OP           (II_OP_ID) 68  /* sqrt                    */
#define II_SQZ_OP            (II_OP_ID) 69  /* squeeze                 */
#define II_SUM_OP            (II_OP_ID) 70  /* sum                     */
#define II_EXIST_OP          (II_OP_ID) 71  /* exists                  */
#define II_TEXT_OP           (II_OP_ID) 72  /* text or vchar           */
#define II_TRIM_OP           (II_OP_ID) 73  /* trim                    */
#define II_PLUS_OP           (II_OP_ID) 74  /* u+                      */
#define II_MINUS_OP          (II_OP_ID) 75  /* u-                      */
#define II_UPPER_OP          (II_OP_ID) 76  /* uppercase               */
#define II_USRLN_OP          (II_OP_ID) 77  /* iiuserlen               */
#define II_USRNM_OP          (II_OP_ID) 78  /* username                */
#define II_VARCH_OP          (II_OP_ID) 79  /* varchar                 */
#define II_LIKE_OP           (II_OP_ID) 80  /* like                    */
#define II_NLIKE_OP          (II_OP_ID) 81  /* not like                */
#define II_HEX_OP            (II_OP_ID) 82  /* hex                     */
#define II_IFNUL_OP          (II_OP_ID) 83  /* ifnull                  */
#define II_STRUC_OP          (II_OP_ID) 84  /* iistructure             */
#define II_DGMT_OP           (II_OP_ID) 85  /* date_gmt                */
#define II_CHA12_OP          (II_OP_ID) 86  /* iichar12                */
#define II_CHEXT_OP          (II_OP_ID) 87  /* charextract             */
#define II_NTRMT_OP          (II_OP_ID) 88  /* ii_notrm_txt            */
#define II_NTRMV_OP          (II_OP_ID) 89  /* ii_notrm_vch            */
#define II_NXIST_OP          (II_OP_ID) 90  /* not exists              */
#define II_XYZZY_OP          (II_OP_ID) 91  /* xyzzy                   */
#define II_HXINT_OP          (II_OP_ID) 92  /* iihexint                */
#define II_TB2DI_OP          (II_OP_ID) 93  /* ii_tabid_di             */
#define II_DI2TB_OP          (II_OP_ID) 94  /* ii_di_tabid             */
#define II_DVDSC_OP          (II_OP_ID) 95  /* ii_dv_desc              */
#define II_DEC_OP            (II_OP_ID) 96  /* decimal                 */
#define II_ETYPE_OP          (II_OP_ID) 97  /* ii_ext_type             */
#define II_ELENGTH_OP        (II_OP_ID) 98  /* ii_ext_length           */
#define II_LOGKEY_OP         (II_OP_ID) 99  /* object_key              */
#define II_TABKEY_OP         (II_OP_ID)100  /* table_key               */
#define II_101_OP            (II_OP_ID)101  /*   Unused                */
#define II_IFTRUE_OP         (II_OP_ID)102  /* ii_iftrue               */
#define II_RESTAB_OP         (II_OP_ID)103  /* resolve_table           */
#define II_GMTSTAMP_OP       (II_OP_ID)104  /* gmt_timestamp           */
#define II_CPNDMP_OP         (II_OP_ID)105  /* ii_cpn_dump             */
#define II_BIT_OP            (II_OP_ID)106  /* bit                     */
#define II_VBIT_OP           (II_OP_ID)107  /* varbit                  */
#define II_ROWCNT_OP         (II_OP_ID)108  /* ii_row_count            */
#define II_SESSUSER_OP       (II_OP_ID)109  /* session_user            */
#define II_SYSUSER_OP        (II_OP_ID)110  /* system_user             */
#define II_INITUSER_OP       (II_OP_ID)111  /* initial_user            */
#define II_ALLOCPG_OP        (II_OP_ID)112  /* iitotal_allocated_pages */
#define II_OVERPG_OP         (II_OP_ID)113  /* iitotal_overflow_pages  */
#define II_BYTE_OP           (II_OP_ID)114  /* byte                    */
#define II_VBYTE_OP          (II_OP_ID)115  /* varbyte                 */
#define II_LOLK_OP           (II_OP_ID)116  /* ii_lolk(large obj)      */
#define II_NULJN_OP          (II_OP_ID)117  /* a = a or null=null      */
#define II_PTYPE_OP          (II_OP_ID)118  /* ii_permit_type(int)     */
#define II_SOUNDEX_OP        (II_OP_ID)119  /* soundex(char)           */
#define II_BDATE4_OP         (II_OP_ID)120  /* _date4(int)             */
#define II_INTEXT_OP         (II_OP_ID)121  /* intextract              */
#define II_ISDST_OP          (II_OP_ID)122  /* isdst()                 */
#define II_CURDATE_OP        (II_OP_ID)123  /* CURRENT_DATE            */
#define II_CURTIME_OP        (II_OP_ID)124  /* CURRENT_TIME            */
#define II_CURTMSTMP_OP      (II_OP_ID)125  /* CURRENT_TIMESTAMP       */
#define II_LCLTIME_OP        (II_OP_ID)126  /* LOCAL_TIME              */
#define II_LCLTMSTMP_OP      (II_OP_ID)127  /* LOCAL_TIMESTAMP         */
#define II_SESSION_PRIV_OP   (II_OP_ID)128  /* session_priv            */
#define II_IITBLSTAT_OP      (II_OP_ID)129  /* iitblstat               */
#define II_130_OP            (II_OP_ID)130  /*   Unused                */
#define II_LVARCH_OP         (II_OP_ID)131  /* long_varchar            */
#define II_LBYTE_OP          (II_OP_ID)132  /* long_byte               */
#define II_BIT_ADD_OP        (II_OP_ID)133  /* bit_add                 */
#define II_BIT_AND_OP        (II_OP_ID)134  /* bit_and                 */
#define II_BIT_NOT_OP        (II_OP_ID)135  /* bit_not                 */
#define II_BIT_OR_OP         (II_OP_ID)136  /* bit_or                  */
#define II_BIT_XOR_OP        (II_OP_ID)137  /* bit_xor                 */
#define II_IPADDR_OP         (II_OP_ID)138  /* ii_ipaddr(c)            */
#define II_HASH_OP           (II_OP_ID)139  /* hash(all)               */
#define II_RANDOMF_OP        (II_OP_ID)140  /* randomf                 */
#define II_RANDOM_OP         (II_OP_ID)141  /* random                  */
#define II_UUID_CREATE_OP    (II_OP_ID)142  /* uuid_create()           */
#define II_UUID_TO_CHAR_OP   (II_OP_ID)143  /* uuid_to_char(u)         */
#define II_UUID_FROM_CHAR_OP (II_OP_ID)144  /* uuid_from_char(c)       */
#define II_UUID_COMPARE_OP   (II_OP_ID)145  /* uuid_compare(u,u)       */
#define II_SUBSTRING_OP      (II_OP_ID)146  /* substring               */
#define II_UNHEX_OP          (II_OP_ID)147  /* unhex                   */
#define II_CORR_OP           (II_OP_ID)148  /* corr()                  */
#define II_COVAR_POP_OP      (II_OP_ID)149  /* covar_pop()             */
#define II_COVAR_SAMP_OP     (II_OP_ID)150  /* covar_samp()            */
#define II_REGR_AVGX_OP      (II_OP_ID)151  /* regr_avgx()             */
#define II_REGR_AVGY_OP      (II_OP_ID)152  /* regr_avgy()             */
#define II_REGR_COUNT_OP     (II_OP_ID)153  /* regr_count()            */
#define II_REGR_INTERCEPT_OP (II_OP_ID)154  /* regr_intercept()        */
#define II_REGR_R2_OP        (II_OP_ID)155  /* regr_r2()               */
#define II_REGR_SLOPE_OP     (II_OP_ID)156  /* regr_slope()            */
#define II_REGR_SXX_OP       (II_OP_ID)157  /* regr_sxx()              */
#define II_REGR_SXY_OP       (II_OP_ID)158  /* regr_sxy()              */
#define II_REGR_SYY_OP       (II_OP_ID)159  /* regr_syy()              */
#define II_STDDEV_POP_OP     (II_OP_ID)160  /* stddev_pop()            */
#define II_STDDEV_SAMP_OP    (II_OP_ID)161  /* stddev_samp()           */
#define II_VAR_POP_OP        (II_OP_ID)162  /* var_pop()               */
#define II_VAR_SAMP_OP       (II_OP_ID)163  /* var_samp()              */
#define II_TABLEINFO_OP      (II_OP_ID)164  /* iitableinfo()           */
#define II_POS_OP            (II_OP_ID)165  /* ANSI position()         */
#define II_ATRIM_OP          (II_OP_ID)166  /* ANSI trim()             */
#define II_CHLEN_OP          (II_OP_ID)167  /* ANSI char_length()      */
#define II_OCLEN_OP          (II_OP_ID)168  /* ANSI octet_length()     */
#define II_BLEN_OP           (II_OP_ID)169  /* ANSI bit_length()       */
#define II_NCHAR_OP          (II_OP_ID)170  /* nchar()                 */
#define II_NVCHAR_OP         (II_OP_ID)171  /* nvarchar()              */
#define II_UTF16TOUTF8_OP    (II_OP_ID)172  /* ii_utf16_to_utf8()      */
#define II_UTF8TOUTF16_OP    (II_OP_ID)173  /* ii_utf8_to_utf16()      */
#define II_I8_OP             (II_OP_ID)174  /* int8                    */
#define II_COWGT_OP          (II_OP_ID)175  /* collation_weight()      */
#define II_ADTE_OP           (II_OP_ID)176  /* ansidate()              */
#define II_TMWO_OP           (II_OP_ID)177  /* time_wo_tz()            */
#define II_TMW_OP            (II_OP_ID)178  /* time_with_tz()          */
#define II_TME_OP            (II_OP_ID)179  /* time_local()            */
#define II_TSWO_OP           (II_OP_ID)180  /* timestamp_wo_tz()       */
#define II_TSW_OP            (II_OP_ID)181  /* timestamp_with_tz()     */
#define II_TSTMP_OP          (II_OP_ID)182  /* timestamp_local()       */
#define II_INDS_OP           (II_OP_ID)183  /* interval_dtos()         */
#define II_INYM_OP           (II_OP_ID)184  /* interval_ytom()         */
#define II_YPART_OP          (II_OP_ID)185  /* year()                  */
#define II_QPART_OP          (II_OP_ID)186  /* quarter()               */
#define II_MPART_OP          (II_OP_ID)187  /* month()                 */
#define II_WPART_OP          (II_OP_ID)188  /* week()                  */
#define II_WIPART_OP         (II_OP_ID)189  /* week_iso()              */
#define II_DYPART_OP         (II_OP_ID)190  /* day()                   */
#define II_HPART_OP          (II_OP_ID)191  /* hour()                  */
#define II_MIPART_OP         (II_OP_ID)192  /* minute()                */
#define II_SPART_OP          (II_OP_ID)193  /* second()                */
#define II_MSPART_OP         (II_OP_ID)194  /* microsecond()           */
#define II_NPART_OP          (II_OP_ID)195  /* nanosecond()            */
#define II_ROUND_OP          (II_OP_ID)196  /* round()                 */
#define II_TRUNC_OP          (II_OP_ID)197  /* trunc[ate]()            */
#define II_CEIL_OP           (II_OP_ID)198  /* ceil[ing]()             */
#define II_FLOOR_OP          (II_OP_ID)199  /* floor()                 */
#define II_CHR_OP            (II_OP_ID)200  /* chr()                   */
#define II_LTRIM_OP          (II_OP_ID)201  /* ltrim()                 */
#define II_LPAD_OP           (II_OP_ID)202  /* lpad()                  */
#define II_RPAD_OP           (II_OP_ID)203  /* rpad()                  */
#define II_REPL_OP           (II_OP_ID)204  /* replace()               */
#define II_ACOS_OP           (II_OP_ID)205  /* acos()                  */
#define II_ASIN_OP           (II_OP_ID)206  /* asin()                  */
#define II_TAN_OP            (II_OP_ID)207  /* tan()                   */
#define II_PI_OP             (II_OP_ID)208  /* pi()                    */
#define II_SIGN_OP           (II_OP_ID)209  /* sign()                  */
#define II_ATAN2_OP          (II_OP_ID)210  /* atan2()                 */
#define II_BYTEXT_OP         (II_OP_ID)211  /* byteextract()           */
#define II_ISINT_OP          (II_OP_ID)212  /* is integer              */
#define II_NOINT_OP          (II_OP_ID)213  /* is not integer          */
#define II_ISDEC_OP          (II_OP_ID)214  /* is decimal              */
#define II_NODEC_OP          (II_OP_ID)215  /* is not decimal          */
#define II_ISFLT_OP          (II_OP_ID)216  /* is float                */
#define II_NOFLT_OP          (II_OP_ID)217  /* is not float            */
#define II_NUMNORM_OP        (II_OP_ID)218  /* numeric norm            */
#define II_UNORM_OP          (II_OP_ID)219  /* unorm                   */
#define II_PATCOMP_OP        (II_OP_ID)220  /* patcomp                 */
#define II_LNVCHR_OP         (II_OP_ID)221  /* long_nvarchar           */
#define II_ISFALSE_OP        (II_OP_ID)222  /* is false                */
#define II_NOFALSE_OP        (II_OP_ID)223  /* is not false            */
#define II_ISTRUE_OP         (II_OP_ID)224  /* is true                 */
#define II_NOTRUE_OP         (II_OP_ID)225  /* is not true             */
#define II_ISUNKN_OP         (II_OP_ID)226  /* is unknown              */
#define II_NOUNKN_OP         (II_OP_ID)227  /* is not unknown          */
#define II_LASTID_OP         (II_OP_ID)228  /* last_identity           */
#define II_BOO_OP            (II_OP_ID)229  /* boolean                 */
#define II_CMPTVER_OP        (II_OP_ID)230  /* iicmptversion           */ 
#define II_NVL2_OP           (II_OP_ID)235  /* NVL2                    */

#define II_X_OP              (II_OP_ID)245  /* x(point)                 */
#define II_Y_OP              (II_OP_ID)246  /* y(point)                 */
#define II_BPOINT_OP         (II_OP_ID)247  /* blob point operators     */
#define II_LINE_OP           (II_OP_ID)248  /* Line operators           */
#define II_POLY_OP           (II_OP_ID)249  /* Polygon operators        */
#define II_MPOINT_OP         (II_OP_ID)250  /* multi point operators    */
#define II_MLINE_OP          (II_OP_ID)251  /* multi Line operators     */
#define II_MPOLY_OP          (II_OP_ID)252  /* multi Polygon operators  */
#define II_GEOMWKT_OP        (II_OP_ID)253  /* Well known text ops      */
#define II_GEOMWKB_OP        (II_OP_ID)254  /* Well known binary ops    */
#define II_NBR_OP            (II_OP_ID)255  /* Spatial nbr op           */
#define II_HILBERT_OP        (II_OP_ID)256  /* Spatial hilbert op       */
#define II_OVERLAPS_OP       (II_OP_ID)257  /* overlaps(geom, geom      */
#define II_INSIDE_OP         (II_OP_ID)258  /* inside(geom, geom)       */
#define II_PERIMETER_OP      (II_OP_ID)259  /* perimeter(geom)          */
#define II_GEOMNAME_OP       (II_OP_ID)260  /* iigeomname()             */
#define II_GEOMDIMEN_OP      (II_OP_ID)261  /* iigeomdimensions()       */
#define II_DIMENSION_OP      (II_OP_ID)262  /* dimension(geom)          */
#define II_GEOMETRY_TYPE_OP  (II_OP_ID)263  /* geometrytype(geom)       */
#define II_BOUNDARY_OP       (II_OP_ID)264  /* boundary(geom)           */
#define II_ENVELOPE_OP       (II_OP_ID)265  /* envelope(geom)           */
#define II_EQUALS_OP         (II_OP_ID)266  /* equals(geom, geom)       */
#define II_DISJOINT_OP       (II_OP_ID)267  /* disjoint(geom, geom)     */
#define II_INTERSECTS_OP     (II_OP_ID)268  /* intersects(geom, geom)   */
#define II_TOUCHES_OP        (II_OP_ID)269  /* touches(geom, geom)      */
#define II_CROSSES_OP        (II_OP_ID)270  /* crosses(geom, geom)      */
#define II_WITHIN_OP         (II_OP_ID)271  /* within(geom, geom)       */
#define II_CONTAINS_OP       (II_OP_ID)272  /* contains(geom, geom)     */
#define II_RELATE_OP         (II_OP_ID)273  /* relate(geom, geom, char) */
#define II_DISTANCE_OP       (II_OP_ID)274  /* distance(geom, geom)     */
#define II_INTERSECTION_OP   (II_OP_ID)275  /* intersection(geom, geom) */
#define II_DIFFERENCE_OP     (II_OP_ID)276  /* difference(geom, geom)   */
#define II_SYMDIFF_OP        (II_OP_ID)277  /* symdifference(geom, geom)*/
#define II_BUFFER_OP         (II_OP_ID)278  /* buffer()                 */
#define II_CONVEXHULL_OP     (II_OP_ID)279  /* convexhull(geom)         */
#define II_POINTN_OP         (II_OP_ID)280  /* pointn(linestring)       */
#define II_STARTPOINT_OP     (II_OP_ID)281  /* startpoint(curve)        */
#define II_ENDPOINT_OP       (II_OP_ID)282  /* endpoint(curve)          */
#define II_ISCLOSED_OP       (II_OP_ID)283  /* isclosed(curve)          */
#define II_ISRING_OP         (II_OP_ID)284  /* isring(curve)            */
#define II_STLENGTH_OP       (II_OP_ID)285  /* st_length(curve)         */
#define II_CENTROID_OP       (II_OP_ID)286  /* centroid(surface)        */
#define II_PNTONSURFACE_OP   (II_OP_ID)287  /* pointonsurface(surface)  */
#define II_AREA_OP           (II_OP_ID)288  /* area(surface)            */
#define II_EXTERIORRING_OP   (II_OP_ID)289  /* exteriorring(polygon)    */
#define II_NINTERIORRING_OP  (II_OP_ID)290  /* numinteriorring(polygon) */
#define II_INTERIORRINGN_OP  (II_OP_ID)291  /* interiorringn(polygon, i)*/
#define II_NUMGEOMETRIES_OP  (II_OP_ID)292  /* numgeometries(geomcoll)  */
#define II_GEOMETRYN_OP      (II_OP_ID)293  /* geometryn(geomcoll)      */
#define II_ISEMPTY_OP        (II_OP_ID)294  /* ISEMPTY(geom)            */
#define II_ISSIMPLE_OP       (II_OP_ID)295  /* ISSIMPLE(geom)           */
#define II_UNION_OP          (II_OP_ID)296  /* union(geom, geom)        */
#define II_SRID_OP           (II_OP_ID)297  /* SRID(geom)               */
#define II_NUMPOINTS_OP      (II_OP_ID)298  /* NUMPOINTS(linestring)    */
#define II_TRANSFORM_OP      (II_OP_ID)299  /* TRANSFORM(geom)          */
#define II_GEOMWKTRAW_OP     (II_OP_ID)300  /* AsTextRaw(geom)          */
#define II_GEOMC_OP          (II_OP_ID)301  /* GeomColl operators       */


/*}
** Name: II_FI_ID - Function Instance Identifier
**
** Description:
**      This type describes the internal numeric identifier use to identify a
**	particular instance of an operator or function.  Once in use, this
**	identifier cannot be changed without losing data in databases.
**
** History:
**      23-may-89 (fred)
**          Created.
**      01-sep-1993 (stevet)
**          Added INGRES class objects symbols.
[@history_template@]...
*/
typedef short II_FI_ID;
#define                 II_NO_FI                ((II_FI_ID) -1)
#define                 II_CLSOBJ_FISTART	((II_FI_ID) 8192)
#define                 II_FISTART	        ((II_FI_ID) 16384)

/*}
** Name: II_ERROR_BLOCK - Mirror iicommon.h DB_ERROR structure.
**
** Description:
**      This structure contains information about a particular error.
**	Note that "err_file" and "err_line", while defined, are not
**	used.
**
** History:
**	18-Sep-2008 (jonj)
**	    Sir 120874: Match up II_ERROR_BLOCK with DB_ERROR, add these
**	    comments/history.
[@history_template@]...
*/
typedef struct _II_ERROR_BLOCK
{
    i4	            err_code;
    i4	            err_data;
    II_PTR	    err_file;
    i4		    err_line;
} II_ERROR_BLOCK;

/*}
** Name: II_ERR_STRUCT - struct to be used in all datatype error processing.
**
** Description:
**	II_ERR_STRUCT defines the structure to be used for returning errors and
**	formatted messages to the caller.
**
**	er_errcode will contain the code which identifies the error to the
**	calling facility.
**
**	er_class should contain a value of 3 (II_EXTERNAL_ERROR).  This
**	identifies the error as originating in user defined ADT code, and
**	ensures the correct processing and formatting of the error message.
**
**	If there is an error,
**	the user error code will be placed into the er_usererr and er_errcode
**	field.  There are two fields for use internally.
**	
**	A SQLSTATE error code will be placed into the er_sqlstate_error field.
**	Any valid SQLSTATE error number may be used here;  the #define for
**	miscellaneous is provided below.  See INGRES error documentation and/or
**	release notes for further details.
**	
**	er_ebuflen is sent by the caller to specify how large the buffer
**	pointed to by er_errmsgp is.
**
**	er_emsglen is sent back to the caller (i.e. set be the called routine)
**	to tell the caller the length of the resulting formatted error message
**	which was placed in the buffer pointed to by ad_errmsgp.
**
**	er_errmsgp contains a pointer to a buffer where a formatted message can
**	be placed.  If this pointer is NULL (0), then no message may be
**	provided.
**
**	History:
**	    09-jun-89 (fred)
**		Adapted from internal files.
**          25-nov-1992 (stevet)
**              Changed generic error to SQLSTATE.
**      21-mar-1994 (johnst))
**          Bug #60764
**          Made struct members type equivalent to those defined
**          in struct ADF_ERROR in adf.h.
**	18-Sep-2008 (jonj)
**	    SIR 120874: Add er_dberror to match ADF_ERROR in adf.h
**
**
*/
typedef struct _II_ERR_STRUCT
{
    i4	er_errcode;	    /* The error being returned.  When an
				    ** internal error occurs, this will contain
				    ** an error code that maps to ADF.
				    ** When a user error occurs this should
				    ** contain the appropriate user error
				    ** code which may or may not map to
				    */
    int		er_class;	    /* Error Class */
#define			II_EXTERNAL_ERROR   3
    i4	er_usererr;	    /* The same as err_errcode
				    ** corresponds to ad_errcode.
				    */
#define		II_SQLSTATE_STRING_LEN	5
    char	er_sqlstate_err[II_SQLSTATE_STRING_LEN];	    
                                    /* The SQLSTATE error code that corresponds
				    ** to ad_errcode.  Applicable numbers for
				    ** sqlstate errors can be used here;  the
				    ** following define specifies the 
				    ** "miscellaneous" error.
				    */
#define         II_SQLSTATE_MISC_ERROR          "50000"
    int		er_ebuflen;	    /* Length of buffer, in bytes, pointed
				    ** to by ad_errmsgp.  If this is equal
				    ** to zero, then no error message will be
				    ** formatted and returned to the user.
				    */
    int		er_emsglen;	    /* Length of a formatted message pointed
				    ** to by ad_errmsgp.
				    */
    char	*er_errmsgp;	    /* A ptr to a buffer where the error mess-
				    ** age corresponding to ad_errcode can be
				    ** formatted and placed.  If this pointer
				    ** is NULL, then no error message will
				    ** be formatted and returned to user.
				    */
    II_ERROR_BLOCK er_dberror;	    /* Full information about er_errcode */
}   II_ERR_STRUCT;

/*}
** Name: II_SCB - Session Control Information
**
** Description:
**      This structure defines various pieces of session control information
**	which is used by the INGRES datatype subsystem.  At this time, only the
**	error information should be used.  All other fields are reserved for
**	internal use only, and should not be altered in any way.
**
** History:
**      09-jun-89 (fred)
**          Adapted from Ingres internal Header files.
**      21-mar-1994 (johnst))
**          Bug #60764
**	    Increased size of scb_rsvd_1[] padding for axp_osf to align the
**          scb_error field with the corresponding adf_errcb element of  
**          ADF_CB in adf.h.
**          Made II_ERROR_BLOCK struct members type equivalent to those defined
**          in struct DB_ERROR in iicommon.h.
**	18-Sep-2008 (jonj)
**	    Sir 120874: Match up II_ERROR_BLOCK with DB_ERROR.
[@history_template@]...
*/
typedef struct _II_SCB
{
#if (defined (__alpha) && defined(__osf__)) || \
    (defined(_AIX) && defined(__64BIT__)) || \
    (defined(__alpha) && defined(__linux__)) || \
    (defined(__x86_64__) && defined(__linux__)) || \
    defined(WIN64) || defined(_LP64) 
  /* axp_osf, axp_lnx or AIX 64-bit */
    int		    scb_rsvd_1[22];	/* Reserved to RTI */
# else
    int		    scb_rsvd_1[20];	/* Reserved to RTI */
# endif
    II_ERR_STRUCT   scb_error;		/* Place error information here */
}   II_SCB;

/*}
** Name: II_DATA_VALUE - A data element descriptor
**
** Description:
**      This structure is used throughout INGRES to describe data elements.  It
**	consists of a length of the data element, the type identifier used, a
**	pointer to the data element itself, and a precision field to be used later.
**
** History:
**      12-jun-89 (fred)
**          Created.
**      21-mar-1994 (johnst))
**          Bug #60764
**          Made struct members type equivalent to those defined
**          in struct DB_DATA_VALUE in iicommon.h.
[@history_template@]...
*/
typedef struct _II_DATA_VALUE
{
    char	    *db_data;		/* Ptr to the actual data.
                                        ** Often, this will be a ptr
                                        ** into a tuple.
                                        */
    i4		    db_length;          /* Number of bytes of storage
                                        ** reserved for the actual data.
                                        */
    II_DT_ID        db_datatype;        /* The datatype of the data
                                        ** value represented here.
                                        */
    short           db_prec;            /* Precision of the data value.
					*/
    short 	    db_collID;		/* Collation ID for this
					** character field */
}   II_DATA_VALUE;

/*}
** Name: II_KEY_BLK - The function call block used to call the
**                     adc_keybld() routine.
**
** Description:
**      A pointer to one of these structures will be supplied to the
**      "keybld()" routine.  This structure will contain the type
**      of key that is being built, and three II_DATA_VALUES.  One of
**      these will be the value to build the key from, and the other
**      two will be where the key (low, high, or both) will be placed.
**      The type of key that was built will also be returned via this
**      block.
**
** History:
**	12-jun-89 (fred)
**	    Adapted from internal files.
**	25-Jun-2008 (kiria01) SIR120473
**	    Move adc_isescape into new adc_pat_flags.
*/

typedef struct _II_KEY_BLK
{
    II_DATA_VALUE   adc_kdv;            /* This is the data value to
                                        ** build a key for.
                                        */
    II_OP_ID	    adc_opkey;		/* Key operator used to tell
                                        ** adc_keybld() what kind of
                                        ** comparison operation this key
					** is being built for.
                                        */
    int             adc_tykey;          /* Type of key created by
                                        ** adc_keybld().  Note, that the
					** following ordering is required
					** by the OPF module for it to
					** work correctly.
                                        */

#define                 II_KNOMATCH     1   /* The value given can't possibly*/
                                            /* match any value of the type   */
                                            /* and length being keyed, so no */
                                            /* scan of the relation should   */
                                            /* be done.  (No low or high key */
					    /* was built.)                   */

#define                 II_KEXACTKEY    2   /* The key formed can match      */
                                            /* exactly one value from the    */
                                            /* relation.  The execution      */
                                            /* phase should seek to this     */
                                            /* point and scan forward until  */
                                            /* it is sure that it has        */
                                            /* exhausted all possible        */
                                            /* matching values.  (The key    */
					    /* formed was placed in the low  */
					    /* key slot.)                    */

#define			II_KRANGEKEY    3   /* Two keys were formed:  One    */
					    /* representing the lowest value */
					    /* in the relation that can be   */
					    /* matched, and the other which  */
					    /* represents the highest value  */
					    /* that can be matched.  The     */
					    /* execution phase should seek   */
					    /* to the point matching the low */
					    /* key, then scan forward until  */
					    /* it has exhausted all values   */
					    /* that might be less than or    */
					    /* equal to the high key.  (Both */
					    /* low and high keys were        */
					    /* formed.)                      */

#define                 II_KHIGHKEY     4   /* The key formed can match all  */
					    /* values in the relation that   */
					    /* are less than or equal to the */
					    /* key; that is, the key         */
					    /* represents the highest value  */
					    /* that can be matched.  The     */
					    /* execution phase should start  */
					    /* at the beginning of the       */
					    /* relation and seek forward     */
					    /* until it has exhausted all    */
					    /* values that might be less     */
					    /* than or equal to the key.     */
					    /* (Only the high key was        */
					    /* formed.)                      */

#define                 II_KLOWKEY      5   /* The key formed can match all  */
                                            /* values in the relation that   */
                                            /* are greater than or equal to  */
					    /* the key; that is, the key     */
					    /* represents the lowest value   */
					    /* that can be matched.  The     */
					    /* execution phase should seek   */
					    /* to this point and scan forward*/
					    /* from there.  (Only the low key*/
					    /* was formed.)                  */

#define			II_KNOKEY	6   /* The boolean expression was too*/
					    /* complex for the optimizer to  */
					    /* use.  Note, this value is     */
					    /* never  returned by _keybld()  */
					    /* routine.  It is used only by  */
					    /* the optimizer.		     */

#define                 II_KALLMATCH   7    /* The value given will match    */
                                            /* any value of the type and     */
                                            /* length being keyed, so a full */
                                            /* scan of the relation should   */
                                            /* be done.  (No low or high key */
					    /* was built.)                   */

    II_DATA_VALUE    adc_lokey;	        /* If adc_tykey = II_KEXACTKEY or
                                        ** II_KLOWKEY, this will be the key
                                        ** built.  If adc_tykey = II_KRANGEKEY,
                                        ** this will be the low key built.
                                        ** In all other cases, this will be
                                        ** undefined.
                                        */
    II_DATA_VALUE    adc_hikey;		/* If adc_tykey = II_KHIGHKEY, this
                                        ** will be the key built.  If
                                        ** adc_tykey = II_KRANGEKEY, this
                                        ** will be the high key built.
                                        ** In all other cases, this will be
                                        ** undefined.
                                        */
    unsigned short   adc_escape;	/* adc_escape is the escape char that
					** the user has specified for the LIKE
					** family or operators.  adc_escape
					** has valid info in it *ONLY* if bit
					** AD_PAT_HAS_ESCAPE set in adc_pat_flags.
                                        */
    i4		    adc_pat_flags;	/* Search flags == like_modifiers */
#define AD_PAT_ISESCAPE_MASK    0x00000003 /* Mask for checking escape bits */
#define	AD_PAT_NO_ESCAPE	0   /* LIKE op, no escape char given    */
#define	AD_PAT_HAS_ESCAPE	1   /* valid escape char in pst_escape  */
#define	AD_PAT_DOESNT_APPLY	2   /* not a LIKE operator		    */
                                        /* If HAS_ESCAPE, then adc_escape holds the escape
					** char that the user specified for the LIKE
					** operator or one of its variants. If this is
					** not set, then the user either has not given an
					** escape clause, or this is not a LIKE op. */
#define AD_PAT_WO_CASE		0x00000004
					/* If set the LIKE is to be forced to be
					** WITHOUT CASE. */
#define AD_PAT_WITH_CASE	0x00000008
					/* If set the LIKE is to be forced to be
					** WITH CASE. If neither AD_PAT_WO_CASE or
					** AD_PAT_WITH_CASE set, defer to collation
					** setting. If both set, AD_PAT_WO_CASE taken. */
#define AD_PAT_DIACRIT_OFF	0x00000010
					/* If set, ignore diacriticals in unicode. */
#define AD_PAT_BIGNORE		0x00000020
					/* If set ignore blanks ala Quel. Usually,
					** derived from the C datatype being present. */
#define AD_PAT_UNUSED1		0x00000040
#define AD_PAT_UNUSED2		0x00000080
#define AD_PAT_RESERVED		0xFFFFFF00
					
}   II_KEY_BLK;

/*}
** Name: IIADD_DT_DFN - Information defining a datatype to INGRES
**
** Description:
**      This structure contains the basic information necessary for INGRES to
**	define a new datatype to the system.
**
** History:
**      24-Jan-1989 (fred)
**          Created.
**       4-May-1993 (fred)
**          Added function prototypes.  Also added dtd_attributes
**          field which is used to set keying, sorting, histogram, and
**          peripheral status of a datatype.
**	12-apr-1994 (swm)
**	    Bug #62658
**	    Added typedef declarations for for non-prototyped builds
**	    (where __STDC__ is not defined). Nb. Although the declarations
**	    within this hdr file only use the __STDC__ typedef conditionally
**	    declarations elsewhere do not; for example, spatial type
**	    implementation in common/adf/ads.
[@history_template@]...
*/
#ifdef __STDC__
typedef II_STATUS II_LENCHK_FUNC (II_SCB         *adf_scb,
				  int            adc_is_usr,
				  II_DATA_VALUE  *adc_dv,
				  II_DATA_VALUE  *adc_rdv);   
typedef II_STATUS II_COMPARE_FUNC (II_SCB         *adf_scb,
				   II_DATA_VALUE  *adc_dv1,
				   II_DATA_VALUE  *adc_dv2,
				   int	           *adc_cmp_result);
typedef II_STATUS II_KEYBLD_FUNC  (II_SCB       *adf_scb,
				   II_KEY_BLK *adc_kblk);
typedef II_STATUS II_GETEMPTY_FUNC  (II_SCB         *adf_scb,
				     II_DATA_VALUE  *adc_emptydv); 
typedef II_STATUS II_KLEN_FUNC  (II_SCB         *adf_scb,
				 int            adc_flags,
				 II_DATA_VALUE  *adc_dv,
				 i4        *adc_width);
typedef II_STATUS II_KCVT_FUNC  (II_SCB         *adf_scb,
				 int            adc_flags,
				 II_DATA_VALUE  *adc_dv,
				 char           *adc_buf,
				 i4        *adc_buflen);
typedef II_STATUS II_VALCHK_FUNC  (II_SCB         *adf_scb,
				   II_DATA_VALUE  *adc_dv);
typedef II_STATUS II_HASHPREP_FUNC  (II_SCB         *adf_scb,
				     II_DATA_VALUE  *adc_dvfrom,
				     II_DATA_VALUE  *adc_dvhp);
typedef II_STATUS II_HELEM_FUNC  (II_SCB         *adf_scb,
				  II_DATA_VALUE  *adc_dvfrom,
				  II_DATA_VALUE  *adc_dvhg);
typedef II_STATUS II_HMIN_FUNC  (II_SCB         *adf_scb,
				 II_DATA_VALUE  *adc_fromdv,
				 II_DATA_VALUE  *adc_min_dvdhg);
typedef II_STATUS II_HMAX_FUNC  (II_SCB         *adf_scb,
				 II_DATA_VALUE  *adc_fromdv,
				 II_DATA_VALUE  *adc_max_dvhg);
typedef II_STATUS II_DHMIN_FUNC  (II_SCB         *adf_scb,
				  II_DATA_VALUE  *adc_fromdv,
				  II_DATA_VALUE  *adc_min_dvdhg);
typedef II_STATUS II_DHMAX_FUNC  (II_SCB         *adf_scb,
				  II_DATA_VALUE  *adc_fromdv,
				  II_DATA_VALUE  *adc_min_dvdhg);
typedef II_STATUS II_ISMINMAX_FUNC  (II_SCB         *adf_scb,
				     II_DATA_VALUE  *adc_dv);
typedef II_STATUS II_HG_DTLN_FUNC  (II_SCB         *adf_scb,
				    II_DATA_VALUE  *adc_fromdv,
				    II_DATA_VALUE  *adc_hgdv);
typedef II_STATUS II_MINMAXDV_FUNC  (II_SCB         *adf_scb,
				     II_DATA_VALUE  *adc_mindv,
				     II_DATA_VALUE  *adc_maxdv);
typedef II_STATUS II_TMLEN_FUNC  (II_SCB         *adf_scb,
				  II_DATA_VALUE  *adc_dv,
				  short          *adc_defwid,
				  short          *adc_worstwid);
typedef II_STATUS II_TMCVT_FUNC  (II_SCB         *adf_scb,
				  II_DATA_VALUE  *adc_dv,
				  II_DATA_VALUE  *adc_tmdv,
				  int           *adc_outlen);
typedef II_STATUS II_DBTOEV_FUNC  (II_SCB         *adf_scb,
				   II_DATA_VALUE  *db_value,
				   II_DATA_VALUE  *ev_value);
typedef II_STATUS II_DBCVTEV_FUNC  (II_SCB         *adf_scb,
				    II_DATA_VALUE  *db_value,
				    II_DATA_VALUE  *ev_value);
typedef II_STATUS II_XFORM_FUNC  (II_SCB  *adf_cb,
				  char    *ws);
typedef II_STATUS II_SEGLEN_FUNC (II_SCB        *adf_scb,
				  II_DT_ID      adc_l_dt,
				  II_DATA_VALUE *adc_rdv);
typedef II_STATUS II_COMPR_FUNC  (II_SCB	*adf_scb,
				  II_DATA_VALUE *adc_dv,
				  int		adc_flag,
				  char		*adc_buf,
				  i4	*adc_outlen);
#else
typedef II_STATUS II_LENCHK_FUNC();
typedef II_STATUS II_COMPARE_FUNC();
typedef II_STATUS II_KEYBLD_FUNC();
typedef II_STATUS II_GETEMPTY_FUNC();
typedef II_STATUS II_KLEN_FUNC();
typedef II_STATUS II_KCVT_FUNC();
typedef II_STATUS II_VALCHK_FUNC();
typedef II_STATUS II_HASHPREP_FUNC();
typedef II_STATUS II_HELEM_FUNC();
typedef II_STATUS II_HMIN_FUNC();
typedef II_STATUS II_HMAX_FUNC();
typedef II_STATUS II_DHMIN_FUNC();
typedef II_STATUS II_DHMAX_FUNC();
typedef II_STATUS II_ISMINMAX_FUNC();
typedef II_STATUS II_HG_DTLN_FUNC();
typedef II_STATUS II_MINMAXDV_FUNC();
typedef II_STATUS II_TMLEN_FUNC();
typedef II_STATUS II_TMCVT_FUNC();
typedef II_STATUS II_DBTOEV_FUNC();
typedef II_STATUS II_DBCVTEV_FUNC();
typedef II_STATUS II_XFORM_FUNC();
typedef II_STATUS II_SEGLEN_FUNC();
typedef II_STATUS II_COMPR_FUNC();
#endif

typedef struct _IIADD_DT_DFN
{
    int		    dtd_object_type;	    /* Check that block type is	    */
#define                 II_O_DATATYPE   0x0210
    II_DT_NAME	    dtd_name;		    /* The name of the datatype */
    II_DT_ID	    dtd_id;		    /* Id number for the */
					    /* datatype */
    II_DT_ID        dtd_underlying_id;      /* Underlying type for */
					    /* peripheral datatype */
    int             dtd_attributes;         /* Characteristics of DT */
    /*
    ** The following four constants are added for initial support of
    ** large objects.  These datatypes do not have any inherent sort
    ** order, keyability, or histogram support.  For many cases, these
    ** three attributes overlap.  A datatype which is not sortable
    ** cannot be a key.  However, they are provided as separate
    ** attributes for user defined datatype support.
    **
    ** Furthermore, the converse of the aforementioned implication is
    ** not true: a datatype which is not keyable may very well be sortable.
    ** Very large attributes are also designated as peripheral
    ** datatypes.  The peripheral refers to the fact that they are
    ** actually stored outside of the base table/tuple.
    */
#define                 II_DT_NOBITS    0x0 /* No specific atts */
#define			II_DT_NOKEY	0x20
					    /*
					    ** Datatype cannot be specified as a
					    ** key column in a modify or index
					    ** statement.
					    */
#define			II_DT_NOSORT	0x40
					    /* Datatype cannot be specified as
					    ** the target of a SORT BY or ORDER
					    ** BY clause in a query.  Also, OPF
					    ** is not permitted to require a
					    ** sort on this datatype for correct
					    ** execution of a query plan.
					    ** Datatypes tagged with this bit
					    ** may have no inherent sort order
					    ** -- they will be simply marked as
					    ** "different" by the sort
					    ** comparison routine.
					    */
#define		        II_DT_NOHISTOGRAM  0x80
					    /*
					    ** Histograms are not available for
					    ** this datatype.  Furthermore, they
					    ** cannot (and should not) be
					    ** constructed.  OPF is expected to
					    ** "make do" without such data.  (At
					    ** present, OPF will choose to make
					    ** such datatypes "like" some other
					    ** datatype -- and use that
					    ** datatypes histograms.
					    */
#define                 II_DT_PERIPHERAL   0x100
					    /*
					    ** Datatype is stored outside of
					    ** the basic tuple format.  This
					    ** means that the tuple itself may
					    ** contain either the full data
					    ** element or it may contain a
					    ** `coupon' which may be redeemed
					    ** later to obtain the actual
					    ** datatype.
					    **
					    ** The data representing a
					    ** peripheral datatype is always
					    ** represented by an II_PERIPHERAL
					    ** structure.  This structure
					    ** represents the union of the
					    ** II_COUPON structure and a byte
					    ** stream (an array of 1
					    ** byte/char).  This data structure
					    ** also contains a flag indicating
					    ** whether this is the real thing
					    ** or the coupon.
					    */

#define                 II_DT_VARIABLE_LEN  0x200
					    /*
					    ** Datatype is variable length. 
					    ** This means that it can be
					    ** compressed, and written 
					    ** with less than its fully
					    ** defined length.
					    */

#ifdef __STDC__
    II_LENCHK_FUNC  *dtd_lenchk_addr;       /* Ptr to lenchk function. */
    II_COMPARE_FUNC *dtd_compare_addr;      /* Ptr to compare function. */
    II_KEYBLD_FUNC  *dtd_keybld_addr;       /* Ptr to keybld function. */
    II_GETEMPTY_FUNC *dtd_getempty_addr;    /* Ptr to getempty function. */
    II_KLEN_FUNC    *dtd_klen_addr;         /* Unused -- Should be zero (0) */
    II_KCVT_FUNC    *dtd_kcvt_addr;         /* Unused -- Should be zero (0) */
    II_VALCHK_FUNC  *dtd_valchk_addr;       /* Ptr to valchk function. */
    II_HASHPREP_FUNC *dtd_hashprep_addr;    /* Ptr to hashprep function. */
    II_HELEM_FUNC   *dtd_helem_addr;        /* Ptr to helem function. */
    II_HMIN_FUNC    *dtd_hmin_addr;         /* Ptr to hmin function. */
    II_HMAX_FUNC    *dtd_hmax_addr;         /* Ptr to hmax function. */
    II_DHMIN_FUNC   *dtd_dhmin_addr;        /* Ptr to "default" hmin function */
    II_DHMAX_FUNC   *dtd_dhmax_addr;        /* Ptr to "default" hmax function */
    II_HG_DTLN_FUNC *dtd_hg_dtln_addr;      /* Ptr to hg_dtln function. */
    II_MINMAXDV_FUNC *dtd_minmaxdv_addr;    /* Ptr to minmaxdv function. */
    II_STATUS       (*dtd_tpls_addr)();     /* Unused -- Should be zero (0) */
    II_STATUS	    (*dtd_decompose_addr)();/* Unused -- Should be zero (0) */
    II_TMLEN_FUNC   *dtd_tmlen_addr;        /* Ptr to tmlen function. */
    II_TMCVT_FUNC   *dtd_tmcvt_addr;        /* Ptr to tmcvt function. */
    II_DBTOEV_FUNC  *dtd_dbtoev_addr;       /* Ptr to dbtoev function. */
    II_DBCVTEV_FUNC *dtd_dbcvtev_addr;      /* Unused -- Should be zero (0) */
    II_XFORM_FUNC   *dtd_xform_addr;        /* Large obj. */
					    /* transformation function */
    II_SEGLEN_FUNC  *dtd_seglen_addr;       /* large obj. segment */
					    /* length function */
	II_COMPR_FUNC   *dtd_compr_addr;		/* Ptr to compres/expand func */
#else
    II_STATUS       (*dtd_lenchk_addr)();   /* Ptr to lenchk function. */
    II_STATUS       (*dtd_compare_addr)();  /* Ptr to compare function. */
    II_STATUS       (*dtd_keybld_addr)();   /* Ptr to keybld function. */
    II_STATUS       (*dtd_getempty_addr)(); /* Ptr to getempty function. */
    II_STATUS	    (*dtd_klen_addr)();     /* Unused -- Should be zero (0) */
    II_STATUS	    (*dtd_kcvt_addr)();     /* Unused -- Should be zero (0) */
    II_STATUS       (*dtd_valchk_addr)();   /* Ptr to valchk function. */
    II_STATUS       (*dtd_hashprep_addr)(); /* Ptr to hashprep function. */
    II_STATUS       (*dtd_helem_addr)();    /* Ptr to helem function. */
    II_STATUS       (*dtd_hmin_addr)();     /* Ptr to hmin function. */
    II_STATUS       (*dtd_hmax_addr)();     /* Ptr to hmax function. */
    II_STATUS	    (*dtd_dhmin_addr)();    /* Ptr to "default" hmin function */
    II_STATUS	    (*dtd_dhmax_addr)();    /* Ptr to "default" hmax function */
    II_STATUS	    (*dtd_hg_dtln_addr)();  /* Ptr to hg_dtln function. */
    II_STATUS       (*dtd_minmaxdv_addr)(); /* Ptr to minmaxdv function. */
    II_STATUS	    (*dtd_tpls_addr)();     /* Unused -- Should be zero (0) */
    II_STATUS	    (*dtd_decompose_addr)();/* Unused -- Should be zero (0) */
    II_STATUS	    (*dtd_tmlen_addr)();    /* Ptr to tmlen function. */
    II_STATUS	    (*dtd_tmcvt_addr)();    /* Ptr to tmcvt function. */
    II_STATUS	    (*dtd_dbtoev_addr)();   /* Ptr to dbtoev function. */
    II_STATUS	    (*dtd_dbcvtev_addr)();  /* Unused -- Should be zero (0) */
    II_STATUS       (*dtd_xform_addr)();    /* Large obj. */
					    /* transformation function */
    II_STATUS       (*dtd_seglen_addr)();   /* large obj. segment */
					    /* length function */
	II_STATUS		(*dtd_compr_addr)();	/* Ptr to compres/expand func */
#endif /* __STDC__ */
}   IIADD_DT_DFN;

/*}
** Name: IIADD_FO_DFN - Information defining an operator or function
**
** Description:
**      This structure is analogous to the IIADD_DT_DFN structure above,
**	except it specifies information about functions (F) and operators (O).
**
**	This information includes
**
**	    fod_name -- the name of the function/operator
**	    fod_id   -- the identification number of the function/operator
**	    fod_type -- whether this object is a function or operator
**
** History:
**      24-Jan-1989 (fred)
**          Created.
[@history_template@]...
*/
typedef struct _IIADD_FO_DFN
{
    int		    fod_object_type;	/* Check for correct block type	    */
#define                 II_O_OPERATION  0x0211
    II_DT_NAME      fod_name;           /* Func/Op name */
    II_OP_ID	    fod_id;		/* Func/Op Identifier */
    int		    fod_type;		/* Function or Operator */
#define			II_NORMAL	((char) 4)
}   IIADD_FO_DFN;

/*}
** Name: IIADD_FI_DFN - Information about a function instance.
**
** Description:
**      This data structure, analogous to be two previous structures, provides
**	information to define a function instance (See ADF Specification).  And,
**	once again, the information provided here is only that information which
**	must be preserved across program invocations.
**
**	This information includes
**
**	    fid_id	-- Function instance identification number
**	    fid_opid	-- Operator id associated with this function instance
**	    fid_arg1	-- Datatype of first argument/operand
**	    fid_arg2	-- Datatype of second argument/operand
**	    fid_arg3	-- Datatype of third argument/operand
**	    fid_arg4	-- Datatype of fourth argument/operand
**	    fid_result	-- Datatype of function instance result
**
** History:
**      24-Jan-1989 (fred)
**          Created.
**      09-dec-1992 (stevet)
**          Added lenspec_routine to IIADD_FI_DFN to support user defined
**          result length calculations.
**       4-May-1993 (fred)
**          Added fid_attributes to allow function instances for large
**          objects to be defined.
**      21-mar-1994 (johnst))
**          Bug #60764
**          Made struct IIADD_FI_DFN members type equivalent to those defined
**          in struct ADF_ERROR in adf.h.
**	12-apr-1994 (swm)
**	    Bug #62658
**	    Added typedef declarations for for non-prototyped builds
**	    (where __STDC__ is not defined).
**	17-Feb-1999 (shero03)
**	    Support 4 operands
**	25-Apr-2006 (kschendel)
**	    Add countvec-style lenspec call to support multi-param UDF's.
[@history_template@]...
*/

#ifdef __STDC__
typedef II_STATUS II_0ARG_FI_FUNC (II_SCB   	    *adf_cb,
				   II_DATA_VALUE    *dv_result);
typedef II_STATUS II_1ARG_FI_FUNC (II_SCB   	    *adf_cb,
				   II_DATA_VALUE    *dv_in,
				   II_DATA_VALUE    *dv_result);
typedef II_STATUS II_2ARG_FI_FUNC (II_SCB   	    *adf_cb,
				   II_DATA_VALUE    *dv_in1,
				   II_DATA_VALUE    *dv_in2,
				   II_DATA_VALUE    *dv_result);
typedef II_STATUS II_3ARG_FI_FUNC (II_SCB   	    *adf_cb,
				   II_DATA_VALUE    *dv_in1,
				   II_DATA_VALUE    *dv_in2,
				   II_DATA_VALUE    *dv_in3,
				   II_DATA_VALUE    *dv_result);
typedef II_STATUS II_4ARG_FI_FUNC (II_SCB   	    *adf_cb,
				   II_DATA_VALUE    *dv_in1,
				   II_DATA_VALUE    *dv_in2,
				   II_DATA_VALUE    *dv_in3,
				   II_DATA_VALUE    *dv_in4,
				   II_DATA_VALUE    *dv_result);
typedef II_STATUS II_1ARG_W_FI_FUNC (II_SCB   	    *adf_cb,
				     II_DATA_VALUE  *dv_in,
				     II_DATA_VALUE  *dv_work,
				     II_DATA_VALUE  *dv_result);
typedef II_STATUS II_2ARG_W_FI_FUNC (II_SCB   	      *adf_cb,
				     II_DATA_VALUE    *dv_in1,
				     II_DATA_VALUE    *dv_in2,
				     II_DATA_VALUE    *dv_work,
				     II_DATA_VALUE    *dv_result);
typedef II_STATUS II_3ARG_W_FI_FUNC (II_SCB   	      *adf_cb,
				     II_DATA_VALUE    *dv_in1,
				     II_DATA_VALUE    *dv_in2,
				     II_DATA_VALUE    *dv_in3,
				     II_DATA_VALUE    *dv_work,
				     II_DATA_VALUE    *dv_result);
typedef II_STATUS II_4ARG_W_FI_FUNC (II_SCB   	      *adf_cb,
				     II_DATA_VALUE    *dv_in1,
				     II_DATA_VALUE    *dv_in2,
				     II_DATA_VALUE    *dv_in3,
				     II_DATA_VALUE    *dv_in4,
				     II_DATA_VALUE    *dv_work,
				     II_DATA_VALUE    *dv_result);
#else
typedef II_STATUS II_0ARG_FI_FUNC();
typedef II_STATUS II_1ARG_FI_FUNC();
typedef II_STATUS II_2ARG_FI_FUNC();
typedef II_STATUS II_3ARG_FI_FUNC();
typedef II_STATUS II_4ARG_FI_FUNC();
typedef II_STATUS II_1ARG_W_FI_FUNC();
typedef II_STATUS II_2ARG_W_FI_FUNC();
typedef II_STATUS II_3ARG_W_FI_FUNC();
typedef II_STATUS II_4ARG_W_FI_FUNC();
#endif /* __STDC__ */

typedef struct _IIADD_FI_DFN
{
    int		    fid_object_type;	/* Check to verify block type	    */
#define                 II_O_FUNCTION_INSTANCE 0x0212
    II_FI_ID	    fid_id;             /* FID */
    II_FI_ID	    fid_cmplmnt;	/* FID of compliment */
    II_OP_ID	    fid_opid;		/* Associated operator		    */
    int		    fid_optype;
#define                 II_COMPARISON   1
#define			II_OPERATOR	2
#define			II_AGGREGATE	3
/* See II_NORMAL above */
#define			II_PREDICATE	5
#define			II_COERCION	6
    int             fid_attributes;     /* How does the FI behave */
#define                 II_FID_F0_NOFLAGS      0
#define			II_FID_F2_COUNTVEC     2
						/* Indicates that the FI is
						** called as (cb, count, vec)
						** where count is # vec entries
						** and vec is II_DATA_VALUE *[]
						** vec[0] is the result.
						*/
#define			II_FID_F4_WORKSPACE    4
    	    	    	    	    	    	/*
						** Indicates that the f.i. needs
						** some workspace.  It is
						** primarily for peripheral
						** fi's which require a
						** workspace to operate, which
						** is ``unusual.''
						*/
#define		        II_FID_F8_INDIRECT	8
    	    	    	    	    	    	/*
						** Indicates that this f.i. uses
						** some indirect services via
						** ADF.  Consequently, care must
						** be employed in invoking this
						** f.i. so as not to cause
						** internal deadlock or resource
						** contention.
						*/
#define                 II_FID_F32_NOTVIRGIN  32  
                                                /*
                                                ** Even though this is a parm-less
                                                ** f.i., it should NOT be added
                                                ** to the VIRGIN segment. Functions
                                                ** such as random() should produce
                                                ** distinct values for each row
                                                ** processed by the containing CX.
                                                */
#define			II_FID_F128_VARARGS	128
						/* Set if FI takes a variable
						** number of arguments; if set,
						** fid_numargs is the minimum
						** number of args.
						*/
#define			II_FID_F256_NONULLSKIP	256
						/* Set this if the FI wants to
						** see NULL (like an arbitrary
						** fncall) rather than skip
						** them and return NULL (like
						** an operator).
						*/
#define			II_F512_RES1STARG	512
						/* For dtfamily result is 1st arg
						*/
#define			II_F1024_RES2NDARG	1024
						/* For dtfamily result is 2nd arg
						*/
#define			II_F2048_COMPAT_IDT	2048
						/* 'compatible' input dt ok
						*/
#define			II_F4096_SQL_CLOAKED 4096
						/* Not for SQL FI
						** This FI will not be resolved to
						** when in SQL mode. The resolver
						** will skip this when in SQL but
						** there could be 'legacy' instances
						** in CX data that will map to it.
						*/
#define			II_F8192_QUEL_CLOAKED 8192
						/* Not for Quel FI
						** This FI will not be resolved to
						** when in Quel mode. The resolver
						** will skip this when in Quel but
						** there could be 'legacy' instances
						** in CX data that will map to it.
						*/
    i4         fid_wslength;            /*
    	    	    	    	    	** Length of workspace -- valid only if
					** II_FID_F4_WORKSPACE set above
					*/
    int		    fid_numargs;
    II_DT_ID	    *fid_args;		/* Datatype for arguments */
    II_DT_ID	    fid_result;		/* ...  for result */
					/* Note that the result datatype in
					** particular is normally the NOT NULL
					** version, and the nullability is
					** calculated from the actual args.
					** For NONULLSKIP FI's, though, the
					** result type can be explicitly
					** nullable (negative).
					*/
    int		    fid_rltype;		/* How to compute result length	    */
#define			II_RES_FIRST	1
#define			II_RES_SECOND	2
#define			II_RES_LONGER	3
#define			II_RES_SHORTER	4
#define			II_RES_FIXED	7
#define			II_RES_KNOWN	18
#define                 II_RES_EXTERN   999
#define			II_RES_EXTERN_COUNTVEC 1000

    i4  	    fid_rlength;        /* ... result length itself         */
    int		    fid_rprec;		/* ... Precision and scale	    */
#define			II_PSVALID	0
    II_STATUS	    (*fid_routine)();	/* Routine to perform the FI	    */
    II_STATUS	    (*lenspec_routine)(); /* Routine to perform length calc */
}   IIADD_FI_DFN;

/*}
** Name: IIADD_DEFINTION - Overall structure of definitions for INGRES
**
** Description:
**	This defines the usage of the IIADD_DT_DFN, IIADD_FO_DFN, and
**	IIADD_FI_DFN.  This usage is used to pass in all the datatype
**	definition information at program startup time.
**
**
** History:
**      24-Jan-1989 (fred)
**          Created.
**      01-sep-1993 (stevet)
**          Added INGRES class objects symbols.
**      05-nov-1993 (smc/swm)
**          Bug #58635
**          Bug #58879
**          Made structure elements type compatable with those defined
**          within the standard header so that we are size and alignment
**	    equivalent. 
**	    The add_l_reserved and add_owner elements are PTR in the DMF
**	    standard header, so these are now char * here.
**	    The add_length is i4 in the DMF standard header, so this needs
**	    to be a scalar type of equivalent size in the <iiadd.h> given
**	    to customers. So make add_length int for axp_osf and continue
**	    to default to long on other platforms.
**	    Similarly use sizeof(int) rather than sizeof(long) in the
**	    I4ASSIGN_MACRO which also assumes that a long is 4 bytes.
**	    (Is there some obscure reason why the literal value 4 is not
**	    used?)
**      21-mar-1994 (johnst))
**          Bug #60764
**          Made struct IIADD_DEFINITION members type compatable with those 
**          defined in the standard header so that we are size and alignment
**	    equivalent. 
[@history_template@]...
*/
typedef struct _IIADD_DEFINITION	IIADD_DEFINITION;
struct _IIADD_DEFINITION
{
    IIADD_DEFINITION  *add_next;        /* Next in the list -- std header */
    IIADD_DEFINITION  *add_prev;	/* Previous in the list -- std header */
    size_t  	    add_length;         /* length of this control block */
    short	    add_type;		/* the type of the control block */
#define                 IIADD_DFN2_TYPE   (short) 0x0202
#define                 IIADD_CLSOBJ_TYPE (short) 0x0301
    short           add_s_reserved;
    char	    *add_l_reserved;
    char	    *add_owner;
    i4  	    add_ascii_id;
    int		    add_risk_consistency;
					/* boolean which, if set, says to   */
					/* ignore database consistency	    */
					/* issues and use the datatypes	    */
					/* regardless of whether the	    */
					/* recovery system agrees.	    */
					/* THIS IS VERY DANGEROUS:	    */
					/*   SETTING THIS FLAG REMOVES	    */
					/*   RESPONSIBILITY FROM THE INGRES */
					/*   RECOVERY SYSTEM.		    */
					/* USE THIS FLAG WITH EXTREME	    */
					/* CAUTION.			    */
#define			IIADD_CONSISTENT	    0
#define			IIADD_INCONSISTENT    1
    i4  	    add_major_id;	/* Major identifier, cannot be */
                                        /* more than 0x3fffffffL       */
#define			IIADD_INGRES_ORIGINAL 0x80000000L
#define			IIADD_INGRES_CLSOBJ   0x40000000L
    i4  	    add_minor_id;	/* Minor identifier */
    int		    add_l_ustring;	/* Length of ... */
    char	    *add_ustring;	/* Arbitrary string to be printed   */
					/* in log file when stuff is used.  */
    int		    add_trace;		/* Boolean indicating whether	    */
					/* II_DBMS_LOG tracing is desired   */
#define                 IIADD_T_LOG_MASK	0x1 /* Print trace messages */
#define			IIADD_T_FAIL_MASK	0x2
					/* Server startup fail on error */
    int		    add_count;		/* total number of objects to be    */
					/* added			    */
    int             add_dt_cnt;		/* Count of ADD_DT_DFN's */
    IIADD_DT_DFN    *add_dt_dfn;	/* and Ptr to array of ADD_DT_DFN's */
    int		    add_fo_cnt;		/* Count of ....FO...... */
    IIADD_FO_DFN    *add_fo_dfn;	/* and Ptr to array of ADD_FO_DFN's */
    int		    add_fi_cnt;		/* Count of function instances	    */
    IIADD_FI_DFN    *add_fi_dfn;	/* Ptr to array of their dfn's	    */
};

/*}
** Name: II_VLEN - Variable length INGRES character array
**
** Description:
**      This structure describes the internal format of an INGRES TEXT,
**      VARCHAR, or byte varying type.  These both consist of a 2 byte
**      integral length followed by that number of bytes.  For
**      purposes of this definition, the array length is specified as 1.
**
** History:
**      15-jun-89 (fred)
**          Created.
**	29-Feb-2008 (kiria01) SIR 117944
**	    Updated to improve the comments
*/
typedef struct _II_VLEN
{
    short           vlen_length;	/* Number of characters in */
    char	    vlen_array[1];	/* Array of characters */
}   II_VLEN;

/*}
** Name: II_VLEN2 - Variable length INGRES character array
**
** Description:
**      This structure describes the internal format of an INGRES
**      NVARCHAR. This consists of a 2 byte integral length followed by
**      that number of 2 byte code-points. For purposes of this
**      definition, the array length is specified as 1.
**
** History:
**	29-Feb-2008 (kiria01) SIR 117944
**	    Added to support NVARCHAR access
*/
typedef struct _II_VLEN2
{
    short           vlen2_length;	/* Number of characters in */
    short	    vlen2_array[1];	/* Array of code-points */
}   II_VLEN2;

/*
** The following sections define objects necessary for manipulations
** of large objects.  Those not using/building large objects can
** ignore that which follows.
*/



/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _II_POP_CB      II_POP_CB;
typedef	struct _II_LO_WKSP     II_LO_WKSP;
typedef struct _II_ADC_PRIVATE II_ADC_PRIVATE;
typedef struct _II_FI_PRIVATE  II_FI_PRIVATE;
typedef struct _II_SHARED      II_SHARED;

/*
**  Defines of other constants.
*/

/*
**      Operations defined on peripheral objects.
**
**	These are prefixed with ADF's facility code (2).
*/
#define                 II_INFORMATION 0x0201	    /*
						    ** Obtain information about
						    ** runtime environment.
						    */
#define                 II_GET         0x0202	    /*
						    ** Get the next segment
						    */
#define			II_PUT	       0x0203	    /*
						    ** Put the next segment
						    */
#define			II_COPY	       0x0205	    /*
						    ** Copy a coupon from one
						    ** place to another
						    */

#define                 II_E_NOMORE    0x27001     /* return value for */
						   /* get when no more */
						   /* data */


/*}
** Name: II_CALLBACKS - Set of INGRES call back functions
**
** Description:
**      This structure defines the set of callback functions available from the
**	INGRES system to user defined datatypes.  The name functions listed
**	below may be available, but may not, depending upon the calling context.
**	If the function address provided below is 0, then the function is not
**	available.
**
**	The ii_trace function provides the capability to send trace messages
**	from with user defined datatype code.  It is called as
**	    status = (*fcn)(dispose_mask, length, string);
**	where
**	    length is the length of the string,
**	    string is a pointer to the string, and
**	    dispose_mask indicates how the string is to be sent.
**		II_TRACE_FE_MASK indicates that the string is to be sent
**		    to the frontend,
**		II_TRACE_ELOG_MASK indicates that the string is to be sent
**		    to the INGRES error log, and
**		II_TRACE_TLOG_MASK indicates that the sting is to go only to the
**		    INGRES trace log (II_DBMS_LOG).  Note that if II_DBMS_LOG
**		    is defined, Messages going to the error log will also appear
**		    in the trace log.
**
**	    If the call is valid, the function will return II_OK (0).  Parameter
**	    errors will return II_ERROR.  The dispose_mask must have only valid
**	    bits set, the string address must be valid, and the length must be
**	    larger than 0, but less than 1000.
**
**	NOTE:  After the call to IIudadt_register, the address provided for
**	the II_CALLBACK structure will no longer be valid.  Therefore, the user
**	code MUST copy the information in which it is interested to stable
**	memory within its domain.
**
** History:
**      10-Apr-1990 (fred)
**          Created.
**      22-Jan-1993 (fred)
**          Added call back points for use by large object designers.
**          These callbacks are used to read/write large object
**          segments, and to filter large object segments through a
**          slave function.  Since large objects are composed of
**          segment of some underlying type, many operations on a
**          large object can be replaced by a series of similar
**          operations on the segments.  The filter function
**          facilitates such operations (for example, uppercase'ing a
**          long string).
**	12-apr-1994 (swm)
**	    Bug #62658
**	    Added typedef declarations for for non-prototyped builds
**	    (where __STDC__ is not defined).
[@history_template@]...
*/
#ifdef __STDC__
typedef II_STATUS II_ERROR_WRITER(II_PTR      scb,
				  i4     err_num,
				  char        *text);

typedef II_STATUS II_LO_HANDLER(int      op_code,
				II_PTR   pop_cb);

typedef II_STATUS II_INIT_FILTER(II_PTR    	scb,
				 II_DATA_VALUE *dv_lo,
				 II_DATA_VALUE *dv_work);

typedef II_STATUS II_FILTER_FCN (II_PTR   	  scb,
				 II_DATA_VALUE   *dv_in,
				 II_DATA_VALUE   *dv_out,
				 II_LO_WKSP 	 *work);

typedef II_STATUS II_LO_FILTER(II_PTR            scb,
			       II_DATA_VALUE     *dv_in,
			       II_DATA_VALUE     *dv_out,
			       II_FILTER_FCN     *fcn,
			       II_PTR            work,
			       int               continuation);
#else
typedef II_STATUS II_ERROR_WRITER();

typedef II_STATUS II_LO_HANDLER();

typedef II_STATUS II_INIT_FILTER();

typedef II_STATUS II_FILTER_FCN ();

typedef II_STATUS II_LO_FILTER();
#endif /* __STDC__ */

typedef struct _II_CALLBACKS
{
    int             ii_cb_version;      /* The current version of callbacks */
#define                 II_CB_V1        0x1
    /* v1 had only trace function. */
#define                 II_CB_V2        0x2
#define                 II_CB_VERSION   II_CB_V2
    
#define                 II_CB_NOT_AVAILABLE 0
    II_STATUS       (*ii_cb_trace)();	/* To get trace functionality */
#define                 II_TRACE_FE_MASK    0x1
#define			II_TRACE_ELOG_MASK  0x2
#define			II_TRACE_TLOG_MASK  0x4
#define			II_TRACE_MAXLEN	    1000
/* both prototype and non-prototype version are declared */
#ifdef __STDC__
    II_ERROR_WRITER  *ii_error_fcn;     /*
					 ** Places error information
					 ** in the scb appropriately.
					 ** This function removes
					 ** dependence upon the
					 ** internal ADF_CB format
					 ** from the user's domain.
					 */
    II_LO_HANDLER    *ii_lo_handler_fcn;/*
					 ** handles large object
					 ** segments.
					 */
    II_INIT_FILTER   *ii_init_filter_fcn;/* Set up for filter function */
    II_LO_FILTER     *ii_filter_fcn;
#else
    II_STATUS        (*ii_error_fcn)();  /*
					 ** Places error information
					 ** in the scb appropriately.
					 ** This function removes
					 ** dependence upon the
					 ** internal ADF_CB format
					 ** from the user's domain.
					 */
    II_STATUS        (*ii_lo_handler_fcn)();/*
					    ** handles large object
					    ** segments.
					    */
    II_STATUS        (*ii_init_filter_fcn)();/* Set up for filter function */
    II_STATUS        (*ii_filter_fcn)();
#endif /* __STDC */
}   II_CALLBACKS;

#ifdef __STDC__
II_STATUS
IIudadt_register(IIADD_DEFINITION  **ui_block_ptr, 
		 II_CALLBACKS      *callback_block);
#endif

#ifdef __STDC__
II_STATUS
IIclsadt_register(IIADD_DEFINITION  **ui_block_ptr,
		  II_CALLBACKS      *callback_block);
#endif

/*}
** Name: II_COUPON - Coupon for later exchange for a peripheral type.
**
** Description:
**      This structure is used by the system for the storage of peripheral
**	types.  Peripheral types are not stored, by the DBMS, inline with the
**	tuple.  Instead, this coupon is stored.  Later, when the actual data for
**	the peripheral type is desired, this coupon may be `redeemed' for the
**	actual data.
**	
**      The data in the coupon is really not controlled by ADF, however, ADF
**	must understand its format.  Therefore, there is some cooperation in
**	design between the tuple storer (DMF in the DBMS case) and ADF.
**
**      DMF will use the fields contained herein to store the peripheral
**	location of the data represented by this coupon.  This will be used
**	later, by DMF, to materialize the actual data.
**
** History:
**      07-nov-1989 (fred)
**          Created.
**      21-mar-1994 (johnst))
**          Bug #60764
**          Made struct II_COUPON members type equivalent to those defined
**          in struct ADP_COUPON in adp.h. 
[@history_template@]...
*/
typedef struct _II_COUPON
{
#if (defined (__alpha) && defined(__osf__)) || \
    (defined(__alpha) && defined(__linux__)) || \
    (defined(__x86_64__) && defined(__linux__)) || \
    defined(WIN64)	
    i4       cpn_id[6];		/* Some sort of identification */
#else
    i4       cpn_id[5];		/* Some sort of identification */
#endif
}   II_COUPON;

/*}
** Name: II_PERIPHERAL - Structure describing a peripheral datatype
**
** Description:
**      This data structure "describes" the representation of a peripheral
**	datatype.  A datatype's declaration as peripheral indicates that the
**	data may not be stored in actual tuple on disk.  Instead, there may be
**	an II_COUPON which will indicate that the data is elsewhere.  This
**	data structure represents what will actually be present on the disk in
**	either case.  This structure represents a union between the II_COUPON
**	mentioned above, and a byte stream of indeterminate form.  Also included
**	is enough information to tell the difference.
**
** History:
**      07-nov-1989 (fred)
**          Created.
**      21-mar-1994 (johnst))
**          Bug #60764
**          Made struct II_PERIPHERAL members type equivalent to those defined
**          in struct ADP_PERIPHERAL in adp.h.
[@history_template@]...
*/
typedef struct _II_PERIPHERAL
{
    i4             per_tag;            /* Tag as to whether this is a coupon
					   ** or the real thing */
# define                II_P_DATA      0   /* Actual data */
# define                II_P_COUPON    1   /* Coupon is present */
# define		II_P_GCA       2   /* Data present in GCA format */
# define		II_P_GCA_L_UNK 3   /* 
					    ** Data present, GCA format, but
					    ** unknown length.
					    */
    u_i4    	per_length0;	/* First half of length */
    u_i4    	per_length1;	/* Second half */
    union {
	II_COUPON	    val_coupon;	/* The coupon structure */
	char		    val_value[1];
	int                 val_int;    /* test for segment ind */
#if (defined (__alpha) && defined(__osf__)) || \
    (defined(__alpha) && defined(__linux__))|| \
    (defined(__x86_64__) && defined(__linux__))
 /* axp_osf, axp_lnx  */
	long		    Align;	/* force alignment for coupon PTR */
#endif
#if defined(WIN64)
	double		    Align;	/* force alignment for coupon PTR */
#endif
    }
		       per_value;
# define                II_HDR_SIZE         (sizeof(i4) \
					      + (2 * sizeof(u_i4)))
# define                II_NULL_PERIPH_SIZE (II_HDR_SIZE \
					      + sizeof(int) + 1)
}   II_PERIPHERAL;

/*}
** Name: II_POP_CB - CB used to invoke peripheral operations (POP's)
**
** Description:
**      This control block is used for ADF to invoke the peripheral operations
**	necessary to allow ADF to manipulate peripheral datatypes.  Internally,
**	ADF divides peripheral datatypes into smaller units of some
**	non-peripheral datatype.  By so doing, the DBMS can store the segments
**	as tuples in non-peripheral tables (table extensions in DMF
**	terminology).
**
**	This control block is used to invoke the necessary operations of get,
**	put, and delete.  All of these operations involve the use of a coupon,
**	as described above.  All but delete are transfering data from or to a
**	segment.  Thus, this control block, in addition to the standard headers
**	necessary in the DBMS, defines the two data elements as a segment and a
**	coupon.  Other administrative information is present to describe in more
**	detail the operation being performed.  The operation codes which are
**	described by this CB are described above.
**
**	ADF internally expects to call a routine whose standard interface is
**	
**	    status = (*routine)(int operation, II_POP_CB* pop_cb);
**
**	where status is one of
**	
**		II_INFORMATION	    Obtain information about
**				    runtime environment.
**
**		II_GET		    Get the next segment
**
**		II_PUT		    Put the next segment
**
**              II_COPY             Copy the object.
**
**
** History:
**      23-Jan-1990 (fred)
**          Created.
**       1-Nov-1993 (fred)
**          Added pop_segno{0,1} for Steve.
**	05-feb-1994 (stevet)
**          This change for ADP_POP_CB should be made to II_POP_CB as well
**            05-nov-1993 (smc)
**               Bug #58635
**               Made l_reserved & owner elements PTR for consistency with
**	         DM0M_OBJECT.
**               In this instance owner was missing from the 'standard header'.
**      21-mar-1994 (johnst))
**          Bug #60764
**          Made struct II_POP_CB members type compatable with those defined in
**          struct ADP_POP_CB in adp.h so that we are size and alignment
**	    equivalent. 
**          Made l_reserved & owner elements II_PTR for consistency with the
**          DM0M_OBJECT 'standard header'. In this instance "owner" was missing
**	    from the standard header.
**	21-apr-1999 (somsa01)
**	    Added pop_info.
[@history_template@]...
*/
struct _II_POP_CB
{
    II_POP_CB       *pop_next;     /* Standard header for all DBMS objects */
    II_POP_CB       *pop_prev;
    size_t  	    pop_length;
    short	    pop_type;
#define                 II_POP_TYPE    0x2001
    short	    pop_s_reserved;
    II_PTR	    pop_l_reserved;
    II_PTR	    pop_l_owner;
    i4  	    pop_ascii_id;
#define                 II_POP_ASCII_ID 0
	/* EO Standard header */
    II_ERROR_BLOCK  pop_error;		/* Result of this operation */
    int		    pop_continuation;	/* status of this operation */
#define                 II_C_BEGIN_MASK     0x1 /* First time invoked */
#define			II_C_END_MASK	    0x2 /* Last time */
#define                 II_C_RANDOM_MASK    0x4
    int		    pop_temporary;	/* Is this a real table or temp */
#define			II_POP_SHORT_TEMP  0x1
    i4         pop_segno0;         /* Segment number */
    i4         pop_segno1;
    II_DATA_VALUE   *pop_coupon;	/* data value holding coupon */
    II_DATA_VALUE   *pop_segment;	/* and the segment ... */
    II_DATA_VALUE   *pop_underdv;	/*
					** Data value (no data part)
					** describing the underlying data for
					** table creations.
					*/
    II_PTR          *pop_user_arg;	/* Place for service to store data */
    II_PTR          *pop_info;		/* Extra information usefull to the
					** peripheral operation. for backend
					** storage, this will contain info
					** about the target table */
    
};

/*}
** Name: II_FI_PRIVATE - Function instance's workspace arena
**
** Description:
**      This section is data used by the function instances to
**      manage their state.  This should not be used by adc_()
**      routines.
** History:
**  	24-Nov-1992 (fred)
**          Created as part of restructuring II_LO_WKSP.
**      21-mar-1994 (johnst))
**          Bug #60764
**          Made struct II_FI_PRIVATE members type equivalent to those defined
**          in struct ADP_FI_PRIVATE in adp.h.
[@history_template@]...
*/
struct _II_FI_PRIVATE
{
    int             fip_state;          /* State of operation */
# define               ADW_F_INITIAL           0
# define               ADW_F_STARTING          1
# define               ADW_F_RUNNING           2
# define               ADW_F_DONE_NEED_NULL    3
    int             fip_null_bytes;
    i4         fip_l0_value;       /* Length of the large object */
    i4         fip_l1_value;
    i4         fip_test_mode;      /* are we in test mode ? */
    i4         fip_test_length;
    i4         fip_test_sent;
    int             fip_done;           /* Is fetch operation complete */
    int             fip_seg_ind;        /* Current segment indicator */
    int             fip_seg_needed;     /* bytes needed for above */
    II_DATA_VALUE   fip_other_dv;	/* Free II_DATA_VALUE */
    II_DATA_VALUE   fip_seg_dv;		/* Data Segment */
    II_DATA_VALUE   fip_cpn_dv;		/* Coupon Segment */
    II_DATA_VALUE   fip_under_dv;	/* Underlying datatype for blob */
    II_DATA_VALUE   fip_work_dv; 	/* Unformated workspace provided */
    II_POP_CB	    fip_pop_cb;		/* Periperal Op. Ctl Block */
    II_PERIPHERAL   fip_periph_hdr;     /* Header area */
};

/*}
** Name: II_SHARED - Shared workspace arena
**
** Description:
**      This section is data which is used to communicate
**      between the function instance routines and the adc_()
**      routines.  These areas may be set or examined by either
**      side, typically by agreement between the two parties.
**
** History:
**  	24-Nov-1992 (fred)
**          Created as part of restructuring II_LO_WKSP.
**      21-mar-1994 (johnst))
**          Bug #60764
**          Made struct II_SHARED members type equivalent to those defined
**          in struct ADP_SHARED in adp.h.
[@history_template@]...
*/
struct _II_SHARED
{
    /*
    ** This is the datatype for which the operation is being
    ** performed.
    */

    II_DT_ID       shd_type;
    /*
    ** These two values indicated whether the input or output values
    ** are segmented (respectively).  Segmented means that the values
    ** are to be sequences of legal segments of the underlying data
    ** type.  For large objects in the form of [II_P_] coupons or GCA
    ** representations, these values will be set.  If the data is to
    ** be specified as a the data in [relatively] raw form, these will
    ** be cleared.  Note that for a given call, neither, either, or
    ** both of these values may be set.  The segment instance routines use
    ** these values to determine how to do their jobs.
    **
    ** Note that OME routines can ignore the next four fields.
    */

    int	    	    shd_inp_segmented;
    int	    	    shd_out_segmented;

    i4  	    shd_inp_tag;
    i4   	    shd_out_tag;        /* More detailed ... */

    /*
    ** These two values are the lengths computed by the segment instance
    ** routines.  These will be set by the segment instance routines, and
    ** subsequently used by the function instance routines to set up
    ** the output coupon and/or to check the validity of the object.
    ** Failure to correctly set these values will result in either the
    ** creation or reporting of invalid/corrupted large objects.
    ** Either of these is bad.
    **
    ** The segment instance routine is not necessarily called at the end of
    ** the action (i.e. after the last section has been presented),
    ** and thus is not necessarily aware of when it has been called
    ** for the last time on a particular large object.  Consequently,
    ** it is incumbent upon this routine to keep the shd_l{0,1}_check
    ** variable up to date -- accurate as of the time that the
    ** instance routine returns.
    */

    i4   	    shd_l0_check;
    i4         shd_l1_check; 

    /*
    ** The expected action indicator communicates is used to convey
    ** information between the filter routine and the function
    ** instance routine.  It is set to ADW_START before the first
    ** instance call.  Thereafter, it is examined at the return of
    ** the call to determine the next action necessary
    ** for the filter routine to perform.  If the value is
    ** set (by the instance routiner, on its return) to ADW_GET_DATA, then the
    ** filter routine will obtain the next section of data
    ** to be processed by the instance.  If it is set to
    ** ADW_FLUSH_SEGMENT, then the filter routine is
    ** expected to dispose of the current (presumably "full") output
    ** segment, and provide a new, empty one for the instance
    ** routine to deal with.  Note here that if the instance returns
    ** ADW_GET_DATA, and there is no more data to get, the filter
    ** will 1) dispose of the current segment, and 2)
    ** assume that the work of the instance  is complete.  The
    ** instance routine will not be called again.
    **
    ** This sequencing mandates that the shd_l{0,1}_chk values be
    ** correct (as noted above).  It further mandates that the output
    ** segment always be a valid segment as it stands.
    */

    int	    	    shd_exp_action;
# define                ADW_CONTINUE        0
# define    	        ADW_START   	    1
# define                ADW_GET_DATA        2
# define                ADW_FLUSH_SEGMENT   3
# define                ADW_FLUSH_STOP	    4
# define                ADW_NEXT_SEGMENT    5
# define                ADW_STOP            6

    /*
    ** The adw_i_area is a generic pointer to the area holding
    ** the current input segment;  the adw_o_area similarly
    ** describes the output segment.
    */

    i4  	    shd_i_used;	        /* Number of bytes used so far */
    i4         shd_i_length;
    II_PTR          shd_i_area;
    i4         shd_o_used;         /* Number of bytes used so far */
    i4         shd_o_length;
    II_PTR     	    shd_o_area;
};

/*}
** Name: II_ADC_PRIVATE - Adc workspace arena
**
** Description:
**      The final section is an area of data for use by the adc_()
**      routines.  It will be neither examined nor set by the function
**      instance level routines during the course of processing.  It
**      can be trusted to be preserved across calls bounded by the
**      continuation - completed call sequence.
**
** History:
**  	24-Nov-1992 (fred)
**          Created as part of restructuring II_LO_WKSP.
**      21-mar-1994 (johnst))
**          Bug #60764
**          Made struct II_ADC_PRIVATE members type equivalent to those defined
**          in struct ADP_ADC_PRIVATE in adp.h.
**       7-Oct-2005 (hanal04) Bug 113093 INGSRV2973
**          Added ADW_S_BYTES. In SPuslong_xform() we may copy part of a
**          POINT or IPOINT because of input buffer boundaries. We need
**          to know how many whole points have been transfered and
**          the ADW_S_BYTES short is used to ensure the subsequent call
**          includes the partial (I)POINT's bytes in the amount transferred.
[@history_template@]...
*/
struct _II_ADC_PRIVATE
{
    int		    adc_state;		/* Current state for machine */
    i4  	    adc_need;		/* Number of bytes to complete state */
    i4  	    adc_longs[16];	/* Collection of integers */
# define                ADW_L_COUNTLOC 0 /* Where in o_area is the */
					 /* count for the segment */
# define                ADW_L_LRCOUNT  0 /* Count for left/right ops */
    short           adc_shorts[16];     /* ...both long and short */
# define                ADW_S_COUNT    0 /* varchar count, current seg */
# define 		ADW_S_BYTES    1 /* Count of the last POINT's bytes */
					 /* transferred in previous call */
    char            adc_chars[32];       /* Collection of characters */
# define                ADW_C_HALF_DBL 0 /* Place for 1/2 a double */
					 /* byte character -- used in */
					 /* long varchar */
    char            adc_memory[32];     /* Generic memory buffer for */
					/* forming objects. */
};

/*}
** Name: II_LO_WKSP - Workspace for longish peripheral operations
**
** Description:
**      This data structure is used by peripheral operations to maintain
**	their context across calls.  Since peripheral datatypes tend to be
**	larger than is convenient to store in memory, it is often the case
**	that many passes are required for them to complete.  That is,
**	they will often return before their operation is complete, requesting
**	that their caller obtain more data or buffer space in which they can
**	work.
**
**	Given that this is the case and that there may be more than one
**	peripheral operation going on at once,  these routines require that a
**	workspace be provided.  The definition of the fields in this workspace
**	is public -- it must be since the caller must declare and own this
**	space;  the routine being called may specify a public usage for some or
**	all of these fields.  However, unless the called routine specifies that
**	the field may be altered, the caller is expected to preserve the value
**	of all fields across calls.
**
** History:
**      15-may-1990 (fred)
**          Created.
**  	24-Nov-1992 (fred)
**          Redone.  Separated sections by interface function.  Added
**          explanatory material in support of OME large objects.
**      21-mar-1994 (johnst))
**          Bug #60764
**          Made struct II_LO_WKSP members type equivalent to those defined
**          in struct ADP_LO_WKSP in adp.h.
**	    Add in adw_l_owner field, as it was previously missing from the 
**	    "standard header" defined here.
[@history_template@]...
*/
struct _II_LO_WKSP
{
    II_LO_WKSP      *adw_next;   /* Standard header for all DBMS objects */
    II_LO_WKSP      *adw_prev;
    size_t  	    adw_length;
    short	    adw_type;
#define                 II_ADW_TYPE    0x2002
    short	    adw_s_reserved;
    II_PTR	    adw_l_reserved;
    II_PTR	    adw_l_owner;
    i4  	    adw_ascii_id;
#define                 II_ADW_ASCII_ID 0
	/* EO Standard header */
    II_PTR          adw_filter_space;   /* storage for filter function */
    II_FI_PRIVATE   adw_fip;
    II_SHARED       adw_shared;
    II_ADC_PRIVATE  adw_adc;
};
