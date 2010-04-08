/*
**	 Copyright (c) 2004 Ingres Corporation
*/

/*	static char	Sccsid[] = "@(#)rbfglob.h	30.1	11/16/84";	*/
/*
** HISTORY:
**	elein 9/21/89 (PC integration)
**		Remove i4  cast of FUNCT_EXTERN for vfrbf
**	9/22/89 (elein) UG changes
**		ingresug change #90045
**		changed <rbftype.h> and <rbfglob.h> to
**		"rbftype.h" and "rbfglob.h"
**	22-nov-89 (cmr)	added Curaggregate for editable aggregates in RBF.  
**			Also added extern declaration for rFagg().
**	11/29/89 (martym) Took out the extern definition of rFag_sel().
**	12/27/89 (elein) Added definition of Rbf_from_ABF
**	1/16/90  (martym) Changed the size of Rbf_orname from FE_MAXNAME to 
**			FE_MAXTABNAME.
**	1/23/90 (martym) Changed the type of rFdisplay() and rfedit() 
** 		from VOID to bool.
**	22-feb-90 (sylviap)
**		Added yn_tbl for listpick.
**	19-apr-90 (cmr)
**		Added declarations for rFagu_append() and rFagu_remove().
**	11-sep-90 (sylviap)
**		Added En_union. #32851.
**	30-oct-1992 (rdrane)
**		Added declarations for rFm_decl(), rFm_sort(), rFm_rhead(),
**		rFm_data(), rFm_sec(), rFm_head(), rFm_rfoot(), rFm_foot(),
**		rFm_ptop(), rFm_pbot(), and r_GenLabDetSec().
**	8-dec-1992 (rdrane)
**		Add Rbf_xns_given boolean.
**	9-mar-1993 (rdrane)
**		Add Rbf_var_seq and declaration of rFm_vnm().
**	1-jul-1993 (rdrane)
**		Add declaration of rFgdf_getdeffmts() to support
**		user override of hdr/ftr timestamp and page # formatting.
**	13-sep-1993 (rdrane)
**		Add declaration of r_SectionBreakUp() to further support
**		user override of hdr/ftr timestamp and page # formatting.
**	26-Aug-2009 (kschendel) b121804
**	    Bool prototypes to keep gcc 4.3 happy.
**	24-Feb-2010 (frima01) Bug 122490
**	    Add function prototypes as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	 <er.h> 
# include	 "errf.h" 
# include	 <sglob.h> 
# include	 <vifrbf.h> 

/*
**	Global Constants for RBF.
*/

	GLOBALREF	i4	Cury;		/* current line on frame */
	GLOBALREF	i4	Curx;		/* current column on frame */
	GLOBALREF	i4	Cursect;	/* current section on frame */
	GLOBALREF	i4	Curtop;		/* top line of current section*/
	GLOBALREF	i4	Curbot;		/* bot line of current section*/
	GLOBALREF	i4	Curordinal;	/* current column ordinal */
	GLOBALREF	i4	Curaggregate;	/* current aggregate */
	GLOBALREF	OPT	Opt;		/* struct for Options Form */


	GLOBALREF	bool	Rbf_noload;	/* TRUE if -e specified */
	GLOBALREF	i4	Rbf_ortype;	/* origin type */
	GLOBALREF	char	Rbf_orname[FE_MAXTABNAME + 1];	
						/* original name */
        GLOBALREF       bool    Rbf_from_ABF;   /* The -X flag AND a report
	                                        ** name were passed in on
						** the command line.  This
						** will cause special
						** behaviour in the rbf
						** catalog
						*/
	GLOBALREF	bool	Rbf_xns_given;	/*
						** TRUE if delimited identifiers
						** are enabled.
						*/
	GLOBALREF	char	*yn_tbl[2];	/* used for y/n listpick */
	GLOBALREF	char	*En_union;	/* UNION SELECT clause */

						/*
						** Format strings for hdr/ftr
						** date/time and page #
						*/
/*
** Test stuff
*/
	GLOBALREF	char	*Rbf_iname;	/* name of input test file */
	GLOBALREF 	char	*Rbf_oname;	/* name of output test file */
	GLOBALREF	FILE	*Rbf_ofile;	/* output test file */
	GLOBALREF	char	*Test_ionames;	/* points to test i/o filenames
						** specified by -Z flag */
	GLOBALREF	i4	Rbf_var_seq;	/*
						** Sequence number to postpend
						** to default/collapsed attr
						** name when constructing RW
						** variable name.
						*/
/*
**	Declarations of Routines.
*/

	FUNC_EXTERN	LIST	*lalloc();	/* allocate a list element */
	FUNC_EXTERN	i4	omnilin();	/* check for omnilinearity */
	FUNC_EXTERN		vfrbf();	/* run the VIFRED form */

/*
**	Declaration of RBF routines.
*/

	FUNC_EXTERN	STATUS	r_GenLabDetSec();
	FUNC_EXTERN	STATUS	r_SectionBreakUp();
	FUNC_EXTERN	bool	r_IndOpts(i4 *);
	FUNC_EXTERN	bool	r_JDLoad(char *, bool *);
	FUNC_EXTERN	bool	r_JDrFget(char *, i2);
	FUNC_EXTERN	bool	rfedit();	/* "Second" process module */
	FUNC_EXTERN	TRIM	*rFatrim();	/* add a piece of trim */
	FUNC_EXTERN	FIELD	*rFafield();	/* add a field */
	FUNC_EXTERN	FRAME	*rFallfrm();	/* allocate a form */
	FUNC_EXTERN	bool	rFbrk_scn();	/* scan HEADER/FOOTER blocks */
	FUNC_EXTERN	bool	rFbreak(i4, i4 *, char *);
	FUNC_EXTERN	VOID	rFcatalog();	/* catalog for the reports */
	FUNC_EXTERN	COPT	*rFclr_copt();	/* clear out COPT structure */
	FUNC_EXTERN	bool	rFchk_dat();	/* get the data relation */
	FUNC_EXTERN	bool	rFchk_agg();	/* check to see if any aggs */
	FUNC_EXTERN	i4	rFcoptions();	/* run the coptions form */
	FUNC_EXTERN	bool	rFdisplay();	/* display the form */
	FUNC_EXTERN	i4 	rFagg();
	FUNC_EXTERN	VOID	rFagu_append(); /* add aggu fld to copt list */
	FUNC_EXTERN	VOID	rFagu_remove(); /* remove fld from copt list */
	FUNC_EXTERN	VOID	rF_bsdbl();
	FUNC_EXTERN	VOID	rFgdf_getdeffmts();
	FUNC_EXTERN	bool	rFget();	/* get a report */
	FUNC_EXTERN	CS	*rFgt_cs();	/* get a CS structure */
	FUNC_EXTERN	COPT	*rFgt_copt();	/* get COPT given ordinal */
	FUNC_EXTERN	i4	rFloc_sec();	/* section in report given Y */
	FUNC_EXTERN	VOID	rFm_aggs();
	FUNC_EXTERN	STATUS	rFm_data();
	FUNC_EXTERN	STATUS	rFm_decl();
	FUNC_EXTERN	STATUS	rFm_foot();
	FUNC_EXTERN	STATUS	rFm_head();
	FUNC_EXTERN	STATUS	rFm_ptop();
	FUNC_EXTERN	STATUS	rFm_pbot();
	FUNC_EXTERN	STATUS	rFm_rfoot();
	FUNC_EXTERN	STATUS	rFm_rhead();
	FUNC_EXTERN	STATUS	rFm_sec();
	FUNC_EXTERN	STATUS	rFm_sort();
	FUNC_EXTERN	VOID	rFm_vnm();
	FUNC_EXTERN	bool	rFmlayout();	/* set up report layout form */
	FUNC_EXTERN	char	*rFnamfld();	/* name a field on struct form*/
	FUNC_EXTERN	char	*rFprompt();	/* prompt on the form */
	FUNC_EXTERN	bool	rFqur_set();	/* check the query */
	FUNC_EXTERN	i4	rFrbf();	/* controlling routine */
	FUNC_EXTERN	i4	rFroptions();	/* run the ROPTIONS form */
	FUNC_EXTERN	i4	rFstructure();	/* run the STRUCTURE form */
	FUNC_EXTERN	bool	rFeditlayout();	/* run the LAYOUT form */
	FUNC_EXTERN	char	rFscan();	/* scan TCMD lists */
	FUNC_EXTERN	i4	rFwrite();	/* write out a report */
	FUNC_EXTERN	void	rf_att_dflt(i4, char **);
	FUNC_EXTERN	VOID	rfLast();
	FUNC_EXTERN	VOID	rFscn_col();
	FUNC_EXTERN	STATUS	IIRFcbk_CloseBlock();
	FUNC_EXTERN	STATUS 	IIRFtty_TableType();
	FUNC_EXTERN	VOID 	IIRFctt_CenterTitle();
	FUNC_EXTERN	VOID	rFinit();
	FUNC_EXTERN	VOID	rFcrack_cmd();
	FUNC_EXTERN	VOID	rFagset();
	FUNC_EXTERN	VOID	rFset_aggarrs();
	FUNC_EXTERN	VOID	rFskip();
	FUNC_EXTERN	VOID 	rFcmp_cs();
	FUNC_EXTERN	VOID 	rFmstruct();
	FUNC_EXTERN	VOID 	rFfld_rng();
	FUNC_EXTERN	VOID 	rf_rep_init();
	FUNC_EXTERN	VOID 	rFag_scn();
	FUNC_EXTERN	i4 	rfframe();
	FUNC_EXTERN	i4 	MapToAggNum();
	FUNC_EXTERN	STATUS	rFgetforms();

/*
** Utility Routines
*/
	FUNC_EXTERN	Sec_node *	sec_node_alloc();
	FUNC_EXTERN	VOID 		sec_list_append();
	FUNC_EXTERN	VOID 		sec_list_remove_all();
	FUNC_EXTERN	Sec_node *	sec_list_find();

