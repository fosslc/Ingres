/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/

/**
** Name: CUI.H - Header file for CUI subdirectory
**
** Description:
**      This file contains definitions used by routines in the CUI
**	subdirectory of the CUF facility!
**
** History: 
**      10-feb-1993 (ralph)
**          Created as part of the "Delimited Identifier" project.
**      07-mar-1993 (rblumer)
**          Added CUI_MAX_UNORMAL_NAME and prototype for cui_idunorm.
**	25-mar-93 (rickh)
**	    Added cus_trmwhite prototype and expanded the scope of this
**	    header file to embrace the whole CUI subdirectory.  Also
**	    added prototype for cui_idxlate and made the cui_idunorm
**	    prototype a FUNC_EXTERN.
**	25-mar-1993 (rblumer)
**	    added CUI_ID_NOLIMIT to allow unlimited length strings;
**	    used for character defaults (which can be up to 1500 bytes).
**      22-sep-1993 (rblumer)
**          Removed CUI_MAX_UNORMAL_NAME; use DB_MAX_DELIMID instead.
**      19-jan-94 (rblumer)
**          Redefined CU_ID_PARAM_ERROR to new error E_US1A26_CU_ID_PARAM_ERROR.
**      18-feb-94 (rblumer)
**          Added prototype for cui_f_idxlate (faster than cui_idxlate for
**          translating regular identifiers in-place).
**	29-sep-2004 (devjo01)
**	    Add prototypes for cui_chk3_locname, and cui_chk1_dbname.
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Added prototypes for cui_move, cui_compare, cui_trmwhite routines
[@history_template@]...
**/

/*
** Mnemonics used in psl_idxlate() to control identifier translation
**
** History:
**      26-oct-92 (ralph)
**          written
**	10-feb-1993 (ralph)
**	    moved from psfparse.h
**	25-mar-1993 (rblumer)
**	    added CUI_ID_NOLIMIT to allow unlimited length strings;
**	    used for character defaults (which can be up to 1500 bytes).
**	20-sep-1993 (jnash)
**	    Add cui_idlen() FUNC_EXTERN.
*/
# define	CUI_ID_DLM	0x0001L	/* Treat the identifier as delimited */
# define	CUI_ID_DLM_M	0x0002L	/* Don't xlate case of delimited ids */
# define	CUI_ID_DLM_L	0x0004L	/* xlate delimited ids to lower case */
# define	CUI_ID_DLM_U	0x0008L	/* xlate delimited ids to upper case */
# define	CUI_ID_QUO	0x0010L	/* Treat the identifier as quoted    */
# define	CUI_ID_QUO_M	0x0020L	/* Don't xlate case of quoted ids    */
# define	CUI_ID_QUO_L	0x0040L	/* xlate quoted ids to lower case    */
# define	CUI_ID_QUO_U	0x0080L	/* xlate quoted ids to upper case    */
# define	CUI_ID_REG	0x0100L	/* Treat the identifier as regular   */
# define	CUI_ID_REG_M	0x0200L	/* Don't xlate case of regular ids   */
# define	CUI_ID_REG_L	0x0400L	/* xlate regular ids to lower case   */
# define	CUI_ID_REG_U	0x0800L	/* xlate regular ids to upper case   */
# define	CUI_ID_ANSI	0x1000L	/* Check for special ANSI conditions */
# define	CUI_ID_NORM	0x2000L	/* Treat identifier as if normalized */
# define	CUI_ID_STRIP	0x4000L	/* Ignore trailing white space	     */
# define	CUI_ID_NOLIMIT	0x010000L    /* Allow unlimited length	     */
# define	CUI_ID_FIPS	(CUI_ID_DLM_M | CUI_ID_REG_U | CUI_ID_QUO_M)
# define	CUI_ID_INGRES	(CUI_ID_DLM_L | CUI_ID_REG_L | CUI_ID_QUO_M)

/*
** Error messages
*/
# define	E_CU_ID_PARAM_ERROR	E_US1A26_CU_ID_PARAM_ERROR


/*	prototypes in CUID.C	*/

FUNC_EXTERN DB_STATUS
cui_idunorm(
	    u_char	*src,
	    u_i4	*src_len_parm,
	    u_char	*dst,
	    u_i4	*dst_len_parm,
	    u_i4	mode,
	    DB_ERROR	*err_blk);

FUNC_EXTERN	DB_STATUS
cui_idxlate(
u_char		*src,
u_i4		*src_len_parm,
u_char		*dst,
u_i4		*dst_len_parm,
u_i4	mode,
u_i4	*retmode,
DB_ERROR        *err_blk
);

FUNC_EXTERN	i4
cui_f_idxlate(
	      char	*ident,
	      i4	length,
	      u_i4 dbxlate
);
FUNC_EXTERN 	DB_STATUS      	
cui_idlen(
u_char     	*src, 
u_char		separator,
u_i4		*src_len,
DB_ERROR	*err_blk
);

FUNC_EXTERN	DB_STATUS
cui_chk1_dbname(
char   *dbname
);

FUNC_EXTERN     DB_STATUS
cui_chk3_locname(
char   *cp
);

FUNC_EXTERN	i4
cui_compare(
i4	len1,
char	*id1,
i4	len2,
char	*id2);

FUNC_EXTERN	VOID
cui_move(
i4	len1,
char	*id1,
char    pad_char,
i4	len2,
char	*id2);

/*	prototypes in CUSTRING.C	*/

FUNC_EXTERN	i4
cui_trmwhite(
i4	len,
char	*id);

