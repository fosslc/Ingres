/*
**	 Copyright (c) 2004 Ingres Corporation
*/
# include	 <sglobi.h> 
# include	 <vifrbfi.h> 

/*
**	Global Constants for RBF.
**   12/27/89 (elein) Added definition of Rbf_from_ABF
**	 1/5/90 (martym)  Added alloc_indopts.
**	 1/16/90 (martym) Changed the size of Rbf_orname from FE_MAXNAME to 
**			FE_MAXTABNAME.
**	30-jan-90 (cmr)	- There is no need to initialize Opt here.
**      22-feb-90 (sylviap) 
**             Added yn_tbl for listpick. 
**	5/23/90 (martym)
**		Added "alloc_rfbrkopt", and "alloc_rfnwpag".
**	27-aug-90 (cmr/esd)  integration of VM Porting changes
**		Removed GLOBALDEF for alloc_struct because it doesn't seem 
**		to be used anywhere and it clashes with alloc_save (first
**		7 chars are the same). 
**      11-sep-90 (sylviap) 
**		Added En_union. #32851.
**	8-dec-1992 (rdrane)
**		Add Rbf_xns_given boolean.
**	9-mar-1993 (rdrane)
**		Add Rbf_var_seq.
*/

	GLOBALDEF i2    Agg_tag = -1;       /* Memory Tag of ADF_AGNAMES
						** and AGGINFO arrays */

	GLOBALDEF i4	Cury = {0};		/* current line on frame */
	GLOBALDEF i4	Curx = {0};		/* current column on frame */
	GLOBALDEF i4	Cursect = {0};		/* current section on frame */
	GLOBALDEF i4	Curtop = {0};		/* top line of curr section */
	GLOBALDEF i4	Curbot = {0};		/* bot line of curr section */
	GLOBALDEF i4	Curordinal = {0};	/* current column ordinal */
	GLOBALDEF i4	Curaggregate = {0};	/* current aggregate */
	GLOBALDEF bool	Rbf_noload = FALSE;	/* TRUE if -e specified */
	GLOBALDEF i4	Rbf_ortype = 0;		/* origin type */
	GLOBALDEF char	Rbf_orname[FE_MAXTABNAME + 1] = "";	
						/* original name */
	GLOBALDEF char	*Rbf_iname = NULL;	/* name of input test file */
	GLOBALDEF char	*Rbf_oname = NULL;	/* name of output test file */
	GLOBALDEF char	*Rbf_ofile = NULL;	/* output test file */

        GLOBALDEF       bool    Rbf_from_ABF;   /* The -X flag AND a report
                                                ** name were passed in on
                                                ** the command line.  This
                                                ** will cause special
                                                ** behaviour in the rbf
                                                ** catalog
                                                */
	GLOBALDEF	bool	Rbf_xns_given;	/*
						** TRUE if delimited identifiers
						** are enabled.
						*/
	GLOBALDEF char	*Test_ionames = NULL;	/* points to test i/o filenames
						** specified by -Z flag */
	GLOBALDEF	i4	Rbf_var_seq;	/*
						** Sequence number to postpend
						** to default/collapsed attr
						** name when constructing RW
						** variable name.
						*/

	GLOBALDEF	bool	alloc_opts;	/* TRUE if coloptions frame has 
						** been allocated */
	GLOBALDEF	bool	alloc_ropts;	/* TRUE if roptions frame
						** has been allocated */
	GLOBALDEF	bool	alloc_save;	/* TRUE if save frame has been
						** allocated */
	GLOBALDEF	bool	alloc_detail;	/* TRUE if detail frame has been
						** allocated */
	GLOBALDEF	bool	alloc_layout;	/* TRUE if layout frame has been
						** allocated */
	GLOBALDEF	bool	alloc_aggs;
	GLOBALDEF	bool	alloc_indopts;	/* TRUE if F_INDTOPTS frame has 
						** been allocated */

	GLOBALDEF	bool	alloc_rfbrkopt;	/* TRUE if F_BRKOPT frame has 
						** been allocated */

	GLOBALDEF	bool	alloc_rfnwpag;	/* TRUE if F_PAGOPT frame has 
						** been allocated */
	GLOBALDEF 	OPT	Opt;
	GLOBALDEF	char	*yn_tbl[2];	/* used for listpick of 
						** yes/no */
	GLOBALDEF	char	*En_union=NULL; /* UNION SELECT clause */

