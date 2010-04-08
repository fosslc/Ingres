/*
**	Copyright (c) 2004 Ingres Corporation
*/
/* Definition of datatype IDs for user-defined datatypes	*/

#define		ORP_TYPE	((II_DT_ID) II_LOW_USER)
#define		INTLIST_TYPE	((II_DT_ID) II_LOW_USER+1)
#define		COMPLEX_TYPE	((II_DT_ID) II_LOW_USER+2)
#define		ZCHAR_TYPE	((II_DT_ID) II_LOW_USER+3)
#define		PNUM_TYPE	((II_DT_ID) II_LOW_USER+4)
#define         LZCHAR_TYPE     ((II_DT_ID) II_LOW_USER+5)
#define         VARUCS2_TYPE    ((II_DT_ID) II_LOW_USER+6)


/*}
** Name: ORD_PAIR - An ordered pair
**
** Description:
**      This structure defines an ordered pair in our world.  The two elements,
**	both double's, represent the x & y coordinates of the ordered pair. 
**
** History:
**      8-Feb-1989 (fred)
**          Created.
**	11-Jan-1999 (andbr08)
**	    Added II_PREC_TO_SC_MACRO().
[@history_template@]...
*/
typedef struct _ORD_PAIR
{
    double              op_x;		/* The x coordinate */
    double		op_y;		/* The y coordinate */
} ORD_PAIR;


/*}
** Name: COMPLEX - A complex number
**
** Description:
**      This structure defines a complex number in our world.  The two elements,
**	both double's, represent the real and imaginary parts of a complex 
**	number. 
**
** History:
**	  July-89 (T.)  Created.
[@history_template@]...
*/
typedef struct _COMPLEX
{
    double              real;		/* The real part      */
    double		imag;		/* The imaginary part */
} COMPLEX;


/*}
** Name: intlist - A list of i4's
**
** Description:
**      This structure defines an INTLIST in our world.  The first member of the
**	struct contains the number of valid elements in the list, the remaining
**	N contain the actual elements.
**
## History:
##      14-Jun-1989 (fred)
##          Created.
##	22-mar-1994 (johnst)
##	    Bug #60764
##	    Change explicit long declarations to use i4 type defined
##	    in iiadd.h, which is conditionally defined to be either long
##	    or int as necessary to ensure type correctness and correct 
##	    alignment on platforms such as axp_osf where long's are 64-bit.
##	27-feb-1996 (smiba01)
##	    Changed history comments to ## in order to remove them from
##	    distribution.
[@history_template@]...
*/
typedef struct _INTLIST
{
    i4		element_count;	/* Number of valid elements */
    i4		element_array[1];   /* The elements themselves */
} INTLIST;


/*}
** Name: zchar - A list of characters
**
** Description:
**      This structure defines a ZCHAR in our world.  The first member of the
**	struct contains the number of valid elements in the list, the remaining
**	N contain the actual elements.
**
** History:
##      Aug-1989 (T.)
##          Created.
##	22-mar-1994 (johnst)
##	    Bug #60764
##	    Change explicit i4 declarations to use i4 type defined
##	    in iiadd.h, which is conditionally defined to be either long
##	    or int as necessary to ensure type correctness and correct 
##	    alignment on platforms such as axp_osf where long's are 64-bit.
[@history_template@]...
*/
typedef struct _ZCHAR
{
    i4	count;		    /*  Number of valid elements  */
    char	element_array[1];   /*  The elements themselves   */
} ZCHAR;

/*
** Name: PNUM - Packed Numeric Datatype
**
** Description:
**      These symbols are defined to support the PNUM datatype.
**
** History:
**      23-mar-92 (stevet)
**         Created.
**	20-apr-1999 (somsa01)
**	    Added macro to extract scale from a given db_prec value.
[@history_template@]...
*/
#define     II_LN_FRM_PR_MACRO(prec)    ((int) ((prec)/2+1))
#define     II_PREC_TO_LEN_MACRO(prec)  ((int) ((prec)/2+1))
#define     II_MAX_NUMPREC              31
#define	    II_MAX_CURR	    	    	5
#define     II_MAX_NUMLEN               II_MAX_NUMPREC + 2
#define     II_PS_TO_PRN_MACRO(pr, sc)  ((pr)+1+((sc)>0)+((pr)==(sc)))
#define	    II_LEN_TO_PR_MACRO(len)	((int) 2*len - 1)
#define     II_S_DECODE_MACRO(pr)	((short) ((pr) % 256))
#define	    II_MAX_NUMBYTE  	    	II_PREC_TO_LEN_MACRO(II_MAX_NUMPREC)
#define	    II_MAX_NUMCURR    	        II_MAX_NUMPREC + II_MAX_NUMPREC/3 + II_MAX_CURR + 2  
#define	    II_PREC_TO_SC_MACRO(prec)	((short) ((prec) % 256))

typedef struct
{
   double x;  /* the x coordinate */
   double y;  /* the y coordinate */
} POINT;


typedef struct
{
   long  npoints;    /* number of points */
   POINT points[1];  
} LINE;

/*}
** Name: varucs2 - A list of Unicode code points
**
** Description:
**      This structure defines a VARUCS2 in our world.  The first member of the
**      struct contains the number of valid elements in the list, the remaining
**      N contain the actual elements.
**
** History:
**      08-mar-1996 (tsale01)
**              Created.
[@history_template@]...
*/

typedef       unsigned short  UCS2;
typedef       unsigned long   UCS4;
typedef       unsigned char   UTF8;

typedef       struct _VARUCS2
{
      short   count;                  /*  Number of valid elements  */
      UCS2    element_array[1];       /*  The elements themselves   */
} VARUCS2;



/*
** Function declarations
**
** History:
##      15-sep-1993 (stevet)
##         Changed extern to globalref so that it works with VMS share images.
##	2-mar-1995 (shero03)
##	    Bug #67208
##	    add seglen and xform routines to LONG ZCHAR.
##	29-mar-1995 (wolf) 
##	    Add non-prototyped definitions for xform and seglen routines.
[@history_template@]...
*/
#ifdef __STDC__

void byte_copy(char	*c_from ,
               int	length ,
               char	*c_to );


void us_error(II_SCB	   *scb ,
              long	   error_code ,
              char	   *error_string );

GLOBALREF II_INIT_FILTER  *usc_setup_workspace;
GLOBALREF II_LO_FILTER    *usc_lo_filter;
GLOBALREF II_LO_HANDLER   *usc_lo_handler;
GLOBALREF II_ERROR_WRITER *usc_error;

II_COMPARE_FUNC	    uscpx_compare;
II_LENCHK_FUNC	    uscpx_lenchk;
II_KEYBLD_FUNC      uscpx_keybld;
II_GETEMPTY_FUNC    uscpx_getempty;
II_VALCHK_FUNC      uscpx_valchk;
II_HASHPREP_FUNC    uscpx_hashprep;
II_HELEM_FUNC       uscpx_helem;
II_HMIN_FUNC        uscpx_hmin;
II_HMAX_FUNC        uscpx_hmax;
II_DHMIN_FUNC       uscpx_dhmin;
II_DHMAX_FUNC       uscpx_dhmax;
II_HG_DTLN_FUNC     uscpx_hg_dtln;
II_MINMAXDV_FUNC    uscpx_minmaxdv;
II_TMLEN_FUNC       uscpx_tmlen;
II_TMCVT_FUNC       uscpx_tmcvt;
II_DBTOEV_FUNC      uscpx_dbtoev;
II_1ARG_FI_FUNC     uscpx_convert;
II_2ARG_FI_FUNC     uscpx_add;
II_2ARG_FI_FUNC     uscpx_sub;
II_2ARG_FI_FUNC     uscpx_mul;
II_2ARG_FI_FUNC     uscpx_div;
II_2ARG_FI_FUNC	    uscpx_cmpnumber;
II_1ARG_FI_FUNC     uscpx_real;
II_1ARG_FI_FUNC     uscpx_imag;
II_1ARG_FI_FUNC     uscpx_conj;
II_1ARG_FI_FUNC     uscpx_negate;
II_1ARG_FI_FUNC     uscpx_abs;
II_1ARG_FI_FUNC     uscpx_rtheta;
II_1ARG_FI_FUNC     usop2cpx;
II_1ARG_FI_FUNC     uscpx2op;
II_STATUS           uscpx_bad_op(II_SCB *scb);
II_COMPARE_FUNC	    usi4_compare;
II_LENCHK_FUNC	    usi4_lenchk;
II_KEYBLD_FUNC      usi4_keybld;
II_GETEMPTY_FUNC    usi4_getempty;
II_VALCHK_FUNC      usi4_valchk;
II_HASHPREP_FUNC    usi4_hashprep;
II_HELEM_FUNC       usi4_helem;
II_HMIN_FUNC        usi4_hmin;
II_HMAX_FUNC        usi4_hmax;
II_DHMIN_FUNC       usi4_dhmin;
II_DHMAX_FUNC       usi4_dhmax;
II_HG_DTLN_FUNC     usi4_hg_dtln;
II_MINMAXDV_FUNC    usi4_minmaxdv;
II_TMLEN_FUNC       usi4_tmlen;
II_TMCVT_FUNC       usi4_tmcvt;
II_DBTOEV_FUNC      usi4_dbtoev;
II_1ARG_FI_FUNC     usi4_convert,
                    usi4_size, usi4_total;
II_2ARG_FI_FUNC     usi4_concatenate,
                    usi4_element,
                    usi4_locate;

II_COMPARE_FUNC	    usop_compare;
II_LENCHK_FUNC	    usop_lenchk;
II_KEYBLD_FUNC      usop_keybld;
II_GETEMPTY_FUNC    usop_getempty;
II_VALCHK_FUNC      usop_valchk;
II_HASHPREP_FUNC    usop_hashprep;
II_HELEM_FUNC       usop_helem;
II_HMIN_FUNC        usop_hmin;
II_HMAX_FUNC        usop_hmax;
II_DHMIN_FUNC       usop_dhmin;
II_DHMAX_FUNC       usop_dhmax;
II_HG_DTLN_FUNC     usop_hg_dtln;
II_MINMAXDV_FUNC    usop_minmaxdv;
II_TMLEN_FUNC       usop_tmlen;
II_TMCVT_FUNC       usop_tmcvt;
II_DBTOEV_FUNC      usop_dbtoev;
II_1ARG_FI_FUNC     usop_convert,
                    usop_xcoord,
                    usop_ycoord;
II_2ARG_FI_FUNC     usop_concatenate,
    	    	    usop_add,
                    usop_distance,
                    usop_slope,
                    usop_midpoint,
                    usop_ordpair;

II_COMPARE_FUNC	    usz_compare, usz_like;
II_LENCHK_FUNC	    usz_lenchk;
II_KEYBLD_FUNC      usz_keybld;
II_GETEMPTY_FUNC    usz_getempty;
II_VALCHK_FUNC      usz_valchk;
II_HASHPREP_FUNC    usz_hashprep;
II_HELEM_FUNC       usz_helem;
II_HMIN_FUNC        usz_hmin;
II_HMAX_FUNC        usz_hmax;
II_DHMIN_FUNC       usz_dhmin;
II_DHMAX_FUNC       usz_dhmax;
II_HG_DTLN_FUNC     usz_hg_dtln;
II_MINMAXDV_FUNC    usz_minmaxdv;
II_TMLEN_FUNC       usz_tmlen;
II_TMCVT_FUNC       usz_tmcvt;
II_DBTOEV_FUNC      usz_dbtoev;
II_1ARG_FI_FUNC     usz_convert,
                    uslz_move,
                    usz_size;
II_1ARG_W_FI_FUNC   uslz_lmove;
II_2ARG_W_FI_FUNC   uslz_concat;
II_XFORM_FUNC	    uslz_xform;
II_SEGLEN_FUNC	    uslz_seglen;

II_COMPARE_FUNC	    usnum_compare;
II_LENCHK_FUNC	    usnum_lenchk;
II_KEYBLD_FUNC      usnum_keybld;
II_GETEMPTY_FUNC    usnum_getempty;
II_VALCHK_FUNC      usnum_valchk;
II_HASHPREP_FUNC    usnum_hashprep;
II_HELEM_FUNC       usnum_helem;
II_HMIN_FUNC        usnum_hmin;
II_HMAX_FUNC        usnum_hmax;
II_DHMIN_FUNC       usnum_dhmin;
II_DHMAX_FUNC       usnum_dhmax;
II_HG_DTLN_FUNC     usnum_hg_dtln;
II_MINMAXDV_FUNC    usnum_minmaxdv;
II_TMLEN_FUNC       usnum_tmlen;
II_TMCVT_FUNC       usnum_tmcvt;
II_DBTOEV_FUNC      usnum_dbtoev;
II_1ARG_FI_FUNC     usnum_convert;
II_2ARG_FI_FUNC     usnum_add;
II_2ARG_FI_FUNC     usnum_sub;
II_2ARG_FI_FUNC     usnum_mul;
II_2ARG_FI_FUNC     usnum_div;
II_2ARG_FI_FUNC     usnum_numeric;
II_2ARG_FI_FUNC     usnum_currency;
II_2ARG_FI_FUNC     usnum_fraction;
II_1ARG_FI_FUNC     usnum_numeric_1arg;
II_1ARG_FI_FUNC     usnum_currency_1arg;
II_1ARG_FI_FUNC     usnum_fraction_1arg;
II_1ARG_FI_FUNC	    usop_sum;

/* VARUCS2 - 16 bits Unicode code point data type */

II_COMPARE_FUNC           vucs2_compare, vucs2_like;
II_LENCHK_FUNC            vucs2_lenchk;
II_KEYBLD_FUNC      vucs2_keybld;
II_GETEMPTY_FUNC    vucs2_getempty;
II_VALCHK_FUNC      vucs2_valchk;
II_HASHPREP_FUNC    vucs2_hashprep;
II_HELEM_FUNC       vucs2_helem;
II_HMIN_FUNC        vucs2_hmin;
II_HMAX_FUNC        vucs2_hmax;
II_DHMIN_FUNC       vucs2_dhmin;
II_DHMAX_FUNC       vucs2_dhmax;
II_HG_DTLN_FUNC     vucs2_hg_dtln;
II_MINMAXDV_FUNC    vucs2_minmaxdv;
II_TMLEN_FUNC       vucs2_tmlen;
II_TMCVT_FUNC       vucs2_tmcvt;
II_DBTOEV_FUNC      vucs2_dbtoev;
II_1ARG_FI_FUNC     vucs2_convert,
                    vucs2_size;
II_2ARG_FI_FUNC     vucs2_concatenate;


#else /* __STDC__ */

GLOBALREF II_STATUS  (*usc_setup_workspace)();
GLOBALREF II_STATUS  (*usc_lo_filter)();
GLOBALREF II_STATUS  (*usc_lo_handler)();
GLOBALREF II_STATUS  (*usc_error)();

extern II_STATUS	usi4_lenchk();
extern II_STATUS	usi4_compare();
extern II_STATUS	usi4_keybld();
extern II_STATUS	usi4_getempty();
extern II_STATUS	usi4_valchk();
extern II_STATUS	usi4_hashprep();
extern II_STATUS	usi4_helem();
extern II_STATUS	usi4_hmin();
extern II_STATUS	usi4_hmax();
extern II_STATUS	usi4_dhmin();
extern II_STATUS	usi4_dhmax();
extern II_STATUS	usi4_hg_dtln();
extern II_STATUS	usi4_minmaxdv();
extern II_STATUS	usi4_tmlen();
extern II_STATUS	usi4_tmcvt();
extern II_STATUS	usi4_dbtoev();
extern II_STATUS	usi4_concatenate();
extern II_STATUS	usi4_size();
extern II_STATUS	usi4_element();
extern II_STATUS	usi4_convert();
extern II_STATUS	usi4_locate();
extern II_STATUS	usi4_total();

/* Externs for the complex number datatype */

extern II_STATUS	uscpx_lenchk();
extern II_STATUS	uscpx_compare();
extern II_STATUS	uscpx_keybld();
extern II_STATUS	uscpx_getempty();
extern II_STATUS	uscpx_valchk();
extern II_STATUS	uscpx_hashprep();
extern II_STATUS	uscpx_helem();
extern II_STATUS	uscpx_hmin();
extern II_STATUS	uscpx_hmax();
extern II_STATUS	uscpx_dhmin();
extern II_STATUS	uscpx_dhmax();
extern II_STATUS	uscpx_hg_dtln();
extern II_STATUS	uscpx_minmaxdv();
extern II_STATUS	uscpx_tmlen();
extern II_STATUS	uscpx_tmcvt();
extern II_STATUS	uscpx_dbtoev();
extern II_STATUS	uscpx_convert();
extern II_STATUS	uscpx_add();
extern II_STATUS	uscpx_sub();
extern II_STATUS	uscpx_mul();
extern II_STATUS	uscpx_div();
extern II_STATUS	uscpx_cmpnumber();
extern II_STATUS	uscpx_abs();
extern II_STATUS	uscpx_conj();
extern II_STATUS	uscpx_rtheta();
extern II_STATUS	uscpx_real();
extern II_STATUS	uscpx_imag();
extern II_STATUS	usop2cpx();
extern II_STATUS	usop_sum();
extern II_STATUS	uscpx2op();
extern II_STATUS	uscpx_bad_op();
extern II_STATUS	uscpx_negate();


/* ordered pair functions */
II_COMPARE_FUNC     usop_compare;
II_LENCHK_FUNC      usop_lenchk;
II_KEYBLD_FUNC      usop_keybld;
II_GETEMPTY_FUNC    usop_getempty;
II_VALCHK_FUNC      usop_valchk;
II_HASHPREP_FUNC    usop_hashprep;
II_HELEM_FUNC       usop_helem;
II_HMIN_FUNC        usop_hmin;
II_HMAX_FUNC        usop_hmax;
II_DHMIN_FUNC       usop_dhmin;
II_DHMAX_FUNC       usop_dhmax;
II_HG_DTLN_FUNC     usop_hg_dtln;
II_MINMAXDV_FUNC    usop_minmaxdv;
II_TMLEN_FUNC       usop_tmlen;
II_TMCVT_FUNC       usop_tmcvt;
II_DBTOEV_FUNC      usop_dbtoev;
II_1ARG_FI_FUNC     usop_convert,
                    usop_xcoord,
                    usop_ycoord;
II_2ARG_FI_FUNC     usop_concatenate,
                    usop_add,
                    usop_distance,
                    usop_slope,
                    usop_midpoint,
                    usop_ordpair;
/* Externs for the case-insensitive characters datatype */

extern II_STATUS	usz_lenchk();
extern II_STATUS	usz_compare();
extern II_STATUS	usz_like();
extern II_STATUS	usz_keybld();
extern II_STATUS	usz_getempty();
extern II_STATUS	usz_valchk();
extern II_STATUS	usz_hashprep();
extern II_STATUS	usz_helem();
extern II_STATUS	usz_hmin();
extern II_STATUS	usz_hmax();
extern II_STATUS	usz_dhmin();
extern II_STATUS	usz_dhmax();
extern II_STATUS	usz_hg_dtln();
extern II_STATUS	usz_minmaxdv();
extern II_STATUS	usz_tmlen();
extern II_STATUS	usz_tmcvt();
extern II_STATUS	usz_dbtoev();
extern II_STATUS	usz_convert();
extern II_STATUS	usz_size();
extern II_STATUS        uslz_move();
extern II_STATUS        usz_size();
extern II_STATUS        uslz_lmove();
extern II_STATUS        uslz_concat();
extern II_STATUS        uslz_xform();
extern II_STATUS        uslz_seglen();

/* Externs for the numeric datatype */

extern II_STATUS	usnum_lenchk();
extern II_STATUS	usnum_compare();
extern II_STATUS	usnum_keybld();
extern II_STATUS	usnum_getempty();
extern II_STATUS	usnum_valchk();
extern II_STATUS	usnum_hashprep();
extern II_STATUS	usnum_helem();
extern II_STATUS	usnum_hmin();
extern II_STATUS	usnum_hmax();
extern II_STATUS	usnum_dhmin();
extern II_STATUS	usnum_dhmax();
extern II_STATUS	usnum_hg_dtln();
extern II_STATUS	usnum_minmaxdv();
extern II_STATUS	usnum_tmlen();
extern II_STATUS	usnum_tmcvt();
extern II_STATUS	usnum_dbtoev();
extern II_STATUS	usnum_convert();
extern II_STATUS	usnum_add();
extern II_STATUS	usnum_sub();
extern II_STATUS	usnum_mul();
extern II_STATUS	usnum_div();
extern II_STATUS	usnum_numeric();
extern II_STATUS	usnum_numeric_1arg();
extern II_STATUS	usnum_currency();
extern II_STATUS	usnum_currency_1arg();
extern II_STATUS	usnum_fraction();
extern II_STATUS	usnum_fraction_1arg();

/* VARUCS2 - 16 bits Unicode code point data type */

extern II_STATUS      vucs2_lenchk();
extern II_STATUS      vucs2_compare();
extern II_STATUS      vucs2_like();
extern II_STATUS      vucs2_keybld();
extern II_STATUS      vucs2_getempty();
extern II_STATUS      vucs2_valchk();
extern II_STATUS      vucs2_hashprep();
extern II_STATUS      vucs2_helem();
extern II_STATUS      vucs2_hmin();
extern II_STATUS      vucs2_hmax();
extern II_STATUS      vucs2_dhmin();
extern II_STATUS      vucs2_dhmax();
extern II_STATUS      vucs2_hg_dtln();
extern II_STATUS      vucs2_minmaxdv();
extern II_STATUS      vucs2_tmlen();
extern II_STATUS      vucs2_tmcvt();
extern II_STATUS      vucs2_dbtoev();
extern II_STATUS      vucs2_convert();
extern II_STATUS      vucs2_size();
extern II_STATUS      vucs2_concatenate();

#endif /* __STDC__ */

