/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**	23-jul-93 (robf)
**	   Added ADH_EXTLEN_SEC since the default size is no
**	   longer available through SL
**	28-jun-96 (thoda04)
**	   Added more function prototypes
**	13-feb-1998 (walro03)
**	    aduadtdate.h and adudate.h are redundant.  aduadtdate.h deleted and
**	    references (such as here) changed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-jun-2005 (gupsh01)
**	    Added adh_ucolinit.
**	19-sep-2007 (kibro01) b119154
**	    Correct external length of ansidate to 10 characters
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes: change adh-ucolinit prototype.
*/

/*
**	The following constants define the lengths of the internal and
**	external representations of dates.  These definitions are based
**	on similar definitions in adudate.h
*/

#define		ADH_INTLEN_DTE		12 	/* sizeof(AD_DATENTRNL)       */
#define		ADH_EXTLEN_DTE		25	/* same as AD_1DTE_OUTLENGTH  */
#define		ADH_INTLEN_ADTE		4 	/* size of ADF_TIME_LEN       */
#define		ADH_EXTLEN_ADTE		10	/* same as AD_2ADTE_OUTLENGTH */
#define		ADH_INTLEN_TIME		10 	/* sizeof ADF_TIME_LEN        */
#define		ADH_EXTLEN_TMWO		21	/* same as AD_3TMWO_OUTLENGTH */
#define		ADH_EXTLEN_TMW		31	/* same as AD_4TMW_OUTLENGTH  */
#define		ADH_EXTLEN_TME		31	/* same as AD_5TMW_OUTLENGTH  */
#define		ADH_INTLEN_TSTMP	14 	/* size of ADF_TSTMP_LEN      */
#define		ADH_EXTLEN_TSWO		39	/* same as AD_6TSWO_OUTLENGTH */
#define		ADH_EXTLEN_TSW		49	/* same as AD_7TSW_OUTLENGTH  */
#define		ADH_EXTLEN_TSTMP	49	/* same as AD_8TSTMP_OUTLENGTH */
#define		ADH_INTLEN_INTYM	3 	/* same as ADF_INTYM_LEN      */
#define		ADH_EXTLEN_INTYM	15	/* same as AD_9INYM_OUTLENGTH */
#define		ADH_INTLEN_INTDS	12 	/* same as ADF_INTDS_LEN      */
#define		ADH_EXTLEN_INTDS	45	/* same as AD_10INDS_OUTLENGTH */

/*
**	The following constant represents the protocol level for
**	which decimal became a valid type and could be sent to the server.
*/

#define		ADH_DECIMAL_LEVEL	60	/* GCA protocol level for 6.5 */

/*
**	The following constant is used to tell adh_losegcvt whether it's
**	being called as part of a standard SQL method select or from a
**	datahandler.
*/

# define	ADH_STD_SQL_MTHD	0x01	/* Standard SQL Method */

/*
**      This group of constants and macros define useful things regarding null
**      values represented in DB_DATA_VALUEs.  They parallel defines and
**	constants found in adfint.h.  Should the equivalent definitions in
**	adfint.h change, these should change, too.
*/

#define		ADH_NVL_BIT	((u_char) 0x01)	/* The null value bit.  If    */
						/* this bit is set in the     */
						/* null value byte, then the  */
						/* value of the piece of data */
						/* is null. 		      */

#define  ADH_ISNULL_MACRO(v)  ((v)->db_datatype > 0 ? FALSE : (*(((u_char *)(v)->db_data) + (v)->db_length - 1) & ADH_NVL_BIT ? TRUE : FALSE))
    /* This macro tells if the data value pointed to by `v' contains the null
    ** value.  If it does, the macro evaluates to TRUE, and if it does not, the
    ** macro evaluates to FALSE.  It assumes that `v' is of a legal datatype,
    ** nullable or not.
    */

#define	 ADH_NULLABLE_MACRO(v)	((v)->db_datatype > 0 ? FALSE : TRUE)
    /* This macro tells if the data value pointed to by 'v' is nullable.
    */

#define  ADH_SETNULL_MACRO(v)  (*(((u_char *)(v)->db_data) + (v)->db_length - 1) |= ADH_NVL_BIT)
    /* This macro sets a nullable data value to the null value.  It assumes that
    ** the data value pointed to by `v' is of a legal nullable type.
    */

#define  ADH_UNSETNULL_MACRO(v)  (*(((u_char *)(v)->db_data) + (v)->db_length - 1) &= ~ADH_NVL_BIT)
    /* This macro unsets the null value in a nullable data value.
    */

#define ADH_BASETYPE_MACRO(v)	(abs(v->db_datatype))
    /* This macro evaluates to the base datatype of the data value pointed
    ** to by 'v'.
    */

/* Prototypes */
FUNC_EXTERN 	void 	adh_charend(DB_DT_ID type, char *db_data, 
					i4 inlen, i4 outlen);
FUNC_EXTERN	STATUS	adh_chkeos(char *addr, i4 len);

/* II_LBQ_CB isn't always defined at this point, do it this way: */
typedef struct IILQ_ *II_LBQ_CB_PTR;
FUNC_EXTERN	STATUS	adh_ucolinit(II_LBQ_CB_PTR IIlbqcb, i4 setnfc);

#ifdef WIN16
FUNC_EXTERN void  	adh_losegcvt(DB_DT_ID hosttype, i4 hostlen, 
                  	             DB_DATA_VALUE *dbv, i4 len, i4  flag);
FUNC_EXTERN DB_STATUS adh_dbcvtev(ADF_CB *adf_scb, 
                  	              DB_DATA_VALUE *adh_dv,
                  	              DB_EMBEDDED_DATA	*adh_ev);
FUNC_EXTERN DB_STATUS adh_dbtoev(ADF_CB           *adf_scb,
                  	             DB_DATA_VALUE    *adh_dv,
                  	             DB_EMBEDDED_DATA *adh_ev);
FUNC_EXTERN DB_STATUS adh_evtodb(ADF_CB           *adf_scb, 
                  	             DB_EMBEDDED_DATA *adh_ev,
                  	             DB_DATA_VALUE    *adh_dv,
                  	             DB_DATA_VALUE    *adh_dvbuf,
                  	             bool             adh_anychar);
FUNC_EXTERN DB_STATUS adh_evcvtdb(ADF_CB           *adf_scb,
                  	              DB_EMBEDDED_DATA *adh_ev,
                  	              DB_DATA_VALUE    *adh_dv);
#endif /* WIN16 */
