/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	Initialized Variables used in communicating between VIFRED
**		and RBF.
**      03-Feb-96 (fanra01)
**          Changed externs to GLOBALREFs.
**
*/

	GLOBALREF	CS	*Cs_top;		/* start of array of CS structs
						** for regular simple fields
						*/
	GLOBALREF	i2	Cs_tag;
	GLOBALREF	CS	Other;			/* unassociated trim/fields */
	GLOBALREF	COPT	*Copt_top;		/* start array of COPT structs */
	GLOBALREF	i2	Copt_tag;		/* COPT structure memory tag */
	GLOBALREF	Sec_List Sections;		/* list of Layout Sections */
	GLOBALREF	i2	Cs_length;		/* number of attributes */
	GLOBALREF	i2	Agg_count;		/* number of aggregates */
	GLOBALREF	i2	RbfAggSeq;		/* unique seq # for FDnewfld */
	GLOBALREF	bool	Rbfchanges;		/* TRUE if changes made */
	GLOBALREF	FRAME	*Rbf_frame;		/* ptr to RBF frame */
	GLOBALREF	i2	Rbfrm_tag;		/* Tag for the RBF frame */
	GLOBALREF	char	*Nrdetails; 			/* name */
	GLOBALREF	char	*Nroptions; 			/* name */
	GLOBALREF	char	*Nstructure;			/* name */
	GLOBALREF	char	*Ncoloptions;			/* name */
	GLOBALREF	char	*Naggregates;			/* name */
	GLOBALREF	char	*Nlayout;			/* Edit layout form name */
	GLOBALREF	char	*Nrsave;			/* Save Report form name */
	GLOBALREF	char	*Nindtopts;			/* name */
	GLOBALREF	char	*Nrfbrkopt;			/* name */
	GLOBALREF	char	*Nrfnwpag;			/* name */
