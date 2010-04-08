/*
**	Copyright (c) 2004 Ingres Corporation
*/
/**
** Name: IIPK.H - User Defined Packed Decimal type.
**
## History: 
##      28-apr-92 (vijay)
##          rs/6000 AIX has u_char typedefed in sys/types.h
##      15-sep-1993 (stevet)
##          Removed GLOBALDEF and GLOBALREF, these are now defined in iiadd.h
##      09-feb-1995 (chech02)
##          Add rs4_us5 for AIX 4.1.
##      27-feb-1996 (smiba01)
##          Changed history comments to ## in order to remove from
##	    distribution.
##      02-Aug-1999 (hweho01)
##          Added ris_u64 (AIX 64-bit) to the platform list that has  
##          u_char typedefed in sys/types.h
##	30-Oct-2002 (hanch04)
##          Added axp_osf to the platform list that has  
##          u_char typedefed in sys/types.h
##	05-Dec-2002 (bonro01)
##	    Added usl_us5 to the platform list that has u_char typedef
##	    in sys/types.h
##	20-Oct-2004 (bonro01)
##	    Added hp2_us5 and hpb_us5 to the platform list that has
##	    u_char typedef in sys/types.h
##	24-dec-2004 (shaha03)
##	    SIR #113754: Added i64_hpu the platform list that has 
##          u_char typedef in sys/types.h
##      19-Feb-2007 (hanal04) Bug 117665
##          iiadd.h may have already typedef'd u_i2. As iiadd.h and iipk.h
##          are supplied files we should only define u_i2 if iiadd.h has
##          not already done so.
##	20-Jun-2009 (kschendel) SIR 122138
##	    Minor revision to config strings.
##	20-Aug-2009 (frima01) SIR 122138
##	    Restore single HP-UX config strings cause the Jamrule depends
##	    on the VERS file.
**/

#define	    VOID		void

# if defined(any_aix) || defined(usl_us5) || defined(axp_osf) || \
     defined(hpb_us5) || defined(i64_hpu) || defined(hp2_us5)
# include <sys/types.h>
# else
#define	    u_char		unsigned char
# endif

#if !defined(u_i2)
#define     u_i2                unsigned short
#endif
#define	    f8			double

typedef	    char		*PTR;

/*	-- Packed Decimal Values --
** For decimal, 0x0 through 0x9 are valid digits; 0xa, 0xc, 0xe, and 0xf
** are valid plus signs (with 0xc being preferred); and 0xb and 0xd are
** valid minus signs (with 0xd being preferred).  Defined below are the
** preferred plus, preferred minus, and the alternate minus values.  No
** more than these are needed.
*/
#define	    IIMH_PK_PLUS		((u_char)0xc)
#define	    IIMH_PK_MINUS		((u_char)0xd)
#define	    IIMH_PK_AMINUS	    	((u_char)0xb)

#define	    IIMHDECOVF		1188
#define	    IIMHDECDIV		(0x00010000 + 0x00000A00 + 0x32)

#define	    FALSE		0
#define	    TRUE		1
#define	    EOS			'\0'
#define	    OK			0
#define	    E_IICL_MASK		0x00010000
#define	    E_IICV_MASK		0x00000500
#define	    IICV_OK		0
#define     IICV_SYNTAX		(E_IICL_MASK + E_IICV_MASK + 0x01)
#define	    IICV_UNDERFLOW	(E_IICL_MASK + E_IICV_MASK + 0x02)
#define	    IICV_OVERFLOW	(E_IICL_MASK + E_IICV_MASK + 0x03)
#define	    IICV_TRUNCATE	(E_IICL_MASK + E_IICV_MASK + 0x04)
#define	    IICV_PKLEFTJUST	0x01
#define	    IICV_PKZEROFILL	0x02
#define	    IICV_PKROUND	0x04
#define     MAXI4           	0x7fffffff
#define     MINI4           	(-MAXI4 - 1)

#define	    min(x,y)		((x) < (y) ? (x) : (y))
#define	    max(x,y)		((x) > (y) ? (x) : (y))





