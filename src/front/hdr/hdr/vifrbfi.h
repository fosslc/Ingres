/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	Initialized Variables used in communicating between VIFRED
**		and RBF.
*/

	GLOBALDEF CS	*Cs_top = {0};		/* start of array of CS structs
						** for regular simple fields
						*/
	GLOBALDEF i2	Cs_tag = -1;		
	GLOBALDEF CS	Other = {0};		/* unassociated trim and fields */
	GLOBALDEF COPT	*Copt_top = {0};	/* start array of COPT structs */
	GLOBALDEF i2	Copt_tag = -1;		/* Copt struct memory tag */
	GLOBALDEF Sec_List Sections = {0};	/* list of layout sections */
	GLOBALDEF i2	Cs_length = {0};	/* number of attributes */
	GLOBALDEF i2	Agg_count = {0};	/* number of aggregates */
	GLOBALDEF i2	RbfAggSeq = {0};	/* unique seq # for FDnewfld */
	GLOBALDEF bool	Rbfchanges = FALSE;	/* TRUE if changes made to form */
	GLOBALDEF FRAME	*Rbf_frame = {0};	/* ptr to RBF frame */
	GLOBALDEF i2	Rbfrm_tag = -1;		/* tag for the RBF frame */
	GLOBALDEF char	*Nroptions;
	GLOBALDEF char	*Nstructure;
	GLOBALDEF char	*Ncoloptions;
	GLOBALDEF char	*Naggregates;
	GLOBALDEF char	*Nlayout;
	GLOBALDEF char	*Nrsave;
	GLOBALDEF char	*Nrdetails;
	GLOBALDEF char	*Nindtopts;
	GLOBALDEF char	*Nrfbrkopt;
	GLOBALDEF char	*Nrfnwpag;
