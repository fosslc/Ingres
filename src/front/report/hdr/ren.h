/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** static char	Sccsid[] = "%W%	%G%";
*/


/*
**	Types for the Report specifier 
*/

/*
**	RSO - structure contains fields for the RSORT relation
**		containing info on the breaks in the report.
*/

typedef struct rso
{
	char	*rso_name;			/* name of sort attribute */
	i4	rso_order;			/* ordinal of sort order */
	char	*rso_direct;			/* direction of sort order */
	i4	rso_sqheader;			/* current seq number of header
						** text */
	i4	rso_sqfooter;			/* current seq number of footer
						** text */
	struct	rso	*rso_below;		/* link to next in list */
}   RSO;


/*
**	REN - structure contains the REPORTS table structure.
*/

/* 1/3/85 renamed repowner to owner to avoid name space conflict */
/* but repowner is never used (azad) */

typedef struct ren
{
	char	*ren_report;			/* report name */
	char	*ren_owner;			/* report owner */
	i4	ren_created;			/* created date */
	i4	ren_modified;			/* modified date */
	char	*ren_type;			/* type of report */
	i4	ren_acount;			/* count of actions */
	i4	ren_scount;			/* count of sort vars */
	i4	ren_qcount;			/* count of query records */
	RSO	*ren_rsotop;			/* start of RSO linked list */
	struct ren	*ren_below;		/* next REN in linked list */
}   REN;
