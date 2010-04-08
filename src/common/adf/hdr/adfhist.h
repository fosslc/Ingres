/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ADFHIST.H -	contains all constant definitions needed for creating
**			histogram elements.
**
** Description:
**        This file contains all the constant definitions needed for creating
**	histogram elements for RTI known datatypes.  This includes intrinsic
**	and abstract datatypes.
**
** History:
**      19-jun-86 (ericj)
**          Initial creation.
**	24-mar-88 (thurston)
**	    Fixed a bug effecting the optimizer.  The constants AD_DTE2_HMIN_VAL
**	    and AD_DTE3_HMAX_VAL should represent MINI4 and MAXI4, respectively.
**          This is what I have changed them to.  The max value had been set
**          to 2147483647, which was OK, but the min value was erroneously set
**          to 0x1000000.  First, the hex representation of MINI4 should be
**          0x80000000 (that's an 8 followed by 7 zeros, not a 1 followed by 6
**          zeros!).  Secondly, it is more portable to just use the MINI4 and
**          MAXI4 constants found in <compat.h>.
**	28-mar-89 (mikem)
**	    Added logical_key support.  Added AD_LOG1_MAX_HSIZE, 
**	    AD_LOG2_DHMIN_VAL, AD_LOG3_HMIN_VAL, AD_LOG4_DHMAX_VAL, 
**	    AD_LOG5_HMAX_VAL.
**	05-jul-89 (jrb)
**	    Added #defs for decimal.
**	06-jul-89 (jrb)
**	    Changed min values for floats to -FMAX instead of FMIN.    
**	23-aug-89 (jrb)
**	    Changed FMIN to -FMAX since this is really the number we were after
**	    for the minimum histogram element for floats.
**	22-sep-1992 (fred)
**	    Added support for BIT types.  These all correspond to the
**	    same values.  Fixed up some ANSI warnings about AD_SEC{4,5}_[D]HMAX
**	    stuff, too.
**	18-jan-93 (rganski)
**	    Changed maximum histogram value size for character types
**	    (AD_CHR1_MAX_HSIZE, AD_CHA1_MAX_HSIZE, AD_TXT1_MAX_HSIZE,
**	    AD_VCH1_MAX_HSIZE, and AD_LOG1_MAX_HSIZE) from 8 to
**	    DB_MAX_HIST_LENGTH, which is 1950.
**	    Part of Character Histogram Enhancements project. See project spec
**	    for more details.
**       6-Apr-1993 (fred)
**          Added byte string datatypes.
**	23-apr-93 (robf)
**	    Added maximum histogram value size for security label datatype
**	    (AD_SEC_MAX_HSIZE)
**	16-feb-2001 (gupsh01)
**	    Added maximum histogram value for nchar and nvarchar data types. 
**	12-may-04 (inkdo01)
**	    Added constans for bigint.
**	24-apr-06 (dougi)
**	    Added values for new date/time types.
**	26-sep-2006 (dougi)
**	    Changed timestamp to occupy i4, not i8.
**      13-aug-2009 (joea)
**          Add AD_BOO_DHMIN_VAL, AD_BOO_DHMAX_VAL, AD_BOO_HMIN_VAL, and
**          AD_BOO_HMAX_VAL.
**/


/*
**  Defines of other constants.
*/

/*
**  Define histogram sizes and size limits for certain datatypes.
*/

#define                 AD_CHR1_MAX_HSIZE   DB_MAX_HIST_LENGTH
#define                 AD_CHA1_MAX_HSIZE   DB_MAX_HIST_LENGTH
#define			AD_DTE1_HSIZE	    4
#define			AD_TME1_HSIZE	    4
#define			AD_TST1_HSIZE	    4
#define			AD_INT1_HSIZE	    4
#define			AD_MNY1_HSIZE	    4
#define			AD_TXT1_MAX_HSIZE   DB_MAX_HIST_LENGTH
#define			AD_VCH1_MAX_HSIZE   DB_MAX_HIST_LENGTH
#define			AD_LOG1_MAX_HSIZE   DB_MAX_HIST_LENGTH
#define			AD_DEC1_HSIZE	    8
# define		AD_BIT1_MAX_HSIZE   8
# define                AD_BYTE1_MAX_HSIZE  8
/*
**   Define minimum, minimum "default", maximum, and maximum "default"
** values for RTI known datatypes.
*/

#define                 AD_CHR2_DHMIN_VAL  'A'
#define			AD_CHR3_HMIN_VAL   '!'
#define			AD_CHR4_DHMAX_VAL  'z'
#define			AD_CHR5_HMAX_VAL   '~'

#define                 AD_CHA2_DHMIN_VAL  'A'
#define			AD_CHA3_HMIN_VAL   '\0'
#define			AD_CHA4_DHMAX_VAL  'z'
#define			AD_CHA5_HMAX_VAL   '\377'

#define			AD_DTE2_HMIN_VAL   MINI4	/* was -2147483648 */
#define			AD_DTE3_HMAX_VAL   MAXI4	/* was  2147483647 */
#define			AD_TME2_HMIN_VAL   MINI4	/* was -2147483648 */
#define			AD_TME3_HMAX_VAL   MAXI4	/* was  2147483647 */
#define			AD_TST2_HMIN_VAL   MINI4
#define			AD_TST3_HMAX_VAL   MAXI4
#define			AD_INT2_HMIN_VAL   MINI4	/* was -2147483648 */
#define			AD_INT3_HMAX_VAL   MAXI4	/* was  2147483647 */

#define	                AD_BOO_DHMIN_VAL    DB_FALSE
#define	                AD_BOO_DHMAX_VAL    DB_TRUE
#define			AD_I1_1DHMIN_VAL    0
#define			AD_I1_2DHMAX_VAL    100
#define			AD_I2_1DHMIN_VAL    0
#define			AD_I2_2DHMAX_VAL    1000
#define			AD_I4_1DHMIN_VAL    0
#define			AD_I4_2DHMAX_VAL    5000
#define	                AD_BOO_HMIN_VAL     DB_FALSE
#define	                AD_BOO_HMAX_VAL     DB_TRUE
#define			AD_I1_1HMIN_VAL	    MINI1
#define			AD_I1_2HMAX_VAL	    MAXI1
#define			AD_I2_1HMIN_VAL	    MINI2
#define			AD_I2_2HMAX_VAL	    MAXI2
#define			AD_I4_1HMIN_VAL	    MINI4
#define			AD_I4_2HMAX_VAL	    MAXI4
#define			AD_I8_1HMIN_VAL	    MINI8
#define			AD_I8_2HMAX_VAL	    MAXI8

#define			AD_DEC1_DHMIN_VAL   0.0
#define			AD_DEC2_DHMAX_VAL   1000.0
#define			AD_DEC3_HMIN_VAL    -1e31
#define			AD_DEC4_HMAX_VAL    1e31

#define			AD_F4_1DHMIN_VAL    0.0
#define			AD_F4_2DHMAX_VAL    1000.0
#define			AD_F4_1HMIN_VAL	    -FMAX
#define			AD_F4_2HMAX_VAL	    FMAX
#define			AD_F8_1DHMIN_VAL    0.0
#define			AD_F8_2DHMAX_VAL    1000.0
#define			AD_F8_1HMIN_VAL	    -FMAX
#define			AD_F8_2HMAX_VAL	    FMAX

#define			AD_MNY2_HMIN_VAL    -999999999
#define			AD_MNY3_HMAX_VAL    999999999

#define			AD_TXT2_DHMIN_VAL   'A'
#define			AD_TXT3_HMIN_VAL    '\0'
#define			AD_TXT4_DHMAX_VAL   'z'
#define			AD_TXT5_HMAX_VAL    '\177'

#define                 AD_VCH2_DHMIN_VAL  'A'
#define			AD_VCH3_HMIN_VAL   '\0'
#define			AD_VCH4_DHMAX_VAL  'z'
#define			AD_VCH5_HMAX_VAL   '\377'

#define                 AD_LOG2_DHMIN_VAL  '\0'
#define			AD_LOG3_HMIN_VAL   '\0'
#define			AD_LOG4_DHMAX_VAL  '\377'
#define			AD_LOG5_HMAX_VAL   '\377'

#define			AD_SEC2_DHMIN_VAL  '\0'
#define			AD_SEC3_HMIN_VAL   '\0'
#define			AD_SEC4_DHMAX_VAL  '\377'
#define			AD_SEC5_HMAX_VAL   '\377'

#define			AD_BIT2_DHMIN_VAL  '\0'
#define			AD_BIT3_HMIN_VAL   '\0'
#define			AD_BIT4_DHMAX_VAL  '\377'
#define			AD_BIT5_HMAX_VAL   '\377'

#define			AD_BYTE2_DHMIN_VAL  '\0'
#define			AD_BYTE3_HMIN_VAL   '\0'
#define			AD_BYTE4_DHMAX_VAL  '\377'
#define			AD_BYTE5_HMAX_VAL   '\377'

#define			AD_NCHR2_DHMIN_VAL  (UCS2) 0
#define			AD_NCHR3_HMIN_VAL   (UCS2) 0
#define			AD_NCHR4_DHMAX_VAL  ((UCS2) 0xFFFF)
#define			AD_NCHR5_HMAX_VAL   ((UCS2) 0xFFFF)

#define			AD_NVCHR2_DHMIN_VAL  (UCS2) 0
#define			AD_NVCHR3_HMIN_VAL   (UCS2) 0
#define			AD_NVCHR4_DHMAX_VAL  ((UCS2) 0xFFFF)
#define			AD_NVCHR5_HMAX_VAL   ((UCS2) 0xFFFF)

