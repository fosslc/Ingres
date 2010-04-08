/*
**	Copyright (c) 1996, 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <si.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include       <fe.h>
# include       <er.h>
# include       <me.h>
# include       <lo.h>
# include       <ug.h>
# include       <st.h>
# include       <ooclass.h>
# include       <abclass.h>
# include       <metafrm.h>
# include       <erfg.h>
# include       "framegen.h"
/*
** Name:        fgdata.c
**
** Description: Global data for fg facility.
**
** History:
**
**      24-sep-96 (hanch04)
**          Created.
**	19-dec-96 (chech02)
**	    Included si.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**	fginit.c
*/

/* GLOBALDEF's */
/*
** NOTE: Iitokarray[] must be in ALPHA order by "tokname" for BINARY search.
** Also, the symbols following each entry are indexes into the array, & are
** used for initialization.
** To add an entry to this list: add it below, IN CORRECT SORTED ORDER; then 
** add a corresponding symbol for its place in the array (and increment other
** symbols below the new one); then add code to tok_init() to initialize it.
**  
** IBM(EBCDIC) NOTE: "_" character is in different sort order. Rule to use for
** setting new symbol names to make sure this list is in correct alpha order
** for BOTH ASCII and EBCDIC: : "two symbols should not differ for the first
** time on a character where one of them has an underscore ('_')".  E.G.
** $candoit and $can_notdoit will cause a problem, but $can_doit and
** $can_notdoit will sort ok in both.
*/


GLOBALDEF IITOKINFO Iitokarray[] =
{
     /* **SORTed BY** */	
     {ERx("$dbevent_code_exists"), NULL, FG_TOK_BUILTIN}, /* any On-Dbevent escape code? */
#define     DBEVENT_CODE_EXISTS 0
     {ERx("$default_return_value"),NULL,FG_TOK_BUILTIN},	/* return value for frame */
#define     DEFAULT_RETURN_VALUE 1
     {ERx("$default_start_frame"), NULL, FG_TOK_BUILTIN},/* TRUE if startup frame */
#define     DEFAULT_START_FRAME 2
     {ERx("$delete_allowed"),    NULL, FG_TOK_BUILTIN},	/* frame option */
#define     DELETE_ALLOWED 3
     {ERx("$delete_cascades"),   NULL, FG_TOK_BUILTIN},	/* delete rule */
#define     DELETE_CASCADES 4
     {ERx("$delete_dbmsrule"),   NULL, FG_TOK_BUILTIN},	/* delete rule */
#define     DELETE_DBMSRULE 5
     {ERx("$delete_detail_allowed"),  NULL, FG_TOK_BUILTIN},	/* frame option */
#define     DELETE_DETAIL_ALLOWED 6
     {ERx("$delete_master_allowed"),  NULL, FG_TOK_BUILTIN},	/* frame option */
#define     DELETE_MASTER_ALLOWED 7
     {ERx("$delete_restricted"), NULL, FG_TOK_BUILTIN}, /* delete rule */
#define     DELETE_RESTRICTED 8
     {ERx("$detail_table_name"), NULL, FG_TOK_BUILTIN},	/* name of detail DBase table */
#define     DETAIL_TABLE_NAME 9
     {ERx("$form_name"),	 NULL, FG_TOK_BUILTIN},/* name of vifred form */
#define     FORM_NAME 10
     {ERx("$frame_name"),	 NULL, FG_TOK_BUILTIN},  /* frame name */
#define     FRAME_NAME 11
     {ERx("$frame_type"),   NULL, FG_TOK_BUILTIN},  /* VQ type: "update", etc */
#define     FRAME_TYPE 12
     {ERx("$insert_detail_allowed"),NULL, FG_TOK_BUILTIN},  /* frame option */
#define     INSERT_DETAIL_ALLOWED 13
     {ERx("$insert_master_allowed"),  NULL, FG_TOK_BUILTIN},	/* frame option */
#define     INSERT_MASTER_ALLOWED 14
     {ERx("$join_field_displayed"),  NULL, FG_TOK_BUILTIN},/*any join fields displayed?*/
#define     JOIN_FIELD_DISPLAYED 15
     {ERx("$join_field_used"),  NULL, FG_TOK_BUILTIN},  /*any join fields displayed?*/
#define     JOIN_FIELD_USED 16
     {ERx("$locks_held"),  NULL, FG_TOK_BUILTIN},  /* "locks held" behaviour */
#define LOCKS_HELD		17
     {ERx("$lookup_exists"),	   NULL, FG_TOK_BUILTIN},/* any lookup tables in VQ? */
#define LOOKUP_EXISTS		18
     {ERx("$master_detail_frame"),     NULL, FG_TOK_BUILTIN},  /* master/detail frame? */
#define     MASTER_DETAIL_FRAME 19
     {ERx("$master_in_tblfld"), NULL,FG_TOK_BUILTIN},/* is master displayed in tbl fld?*/
#define     MASTER_IN_TBLFLD 20
     {ERx("$master_seqfld_cname"),NULL,FG_TOK_BUILTIN},/* name of sequenced key column.*/
#define     MASTER_SEQFLD_CNAME 21
     {ERx("$master_seqfld_displayed"), NULL,FG_TOK_BUILTIN},/*is sequenced fld displyd?*/
#define     MASTER_SEQFLD_DISPLAYED 22
     {ERx("$master_seqfld_exists"),NULL, FG_TOK_BUILTIN},/* does sequenced field exist?*/
#define     MASTER_SEQFLD_EXISTS 23
     {ERx("$master_seqfld_fname"), NULL,FG_TOK_BUILTIN},/* name of sequenced key field.*/
#define     MASTER_SEQFLD_FNAME 24
     {ERx("$master_seqfld_used"),NULL,FG_TOK_BUILTIN},/* is seq fld on form or in var? */
#define     MASTER_SEQFLD_USED 25
     {ERx("$master_table_name"),  NULL, FG_TOK_BUILTIN},	/* name of master table */
#define     MASTER_TABLE_NAME 26
     {ERx("$nullable_detail_key"),  NULL,FG_TOK_BUILTIN},
	/*
	** true if detail tbl has nullable key OR nullable optimistic locking
	** col(s)
	*/
#define     NULLABLE_DETAIL_KEY 27
     {ERx("$nullable_master_key"),  NULL,FG_TOK_BUILTIN},
	/*
	** true if master tbl has nullable key OR nullable optimistic locking
	** col(s)
	*/
#define     NULLABLE_MASTER_KEY 28
     {ERx("$optimize_concurrency"),NULL,FG_TOK_BUILTIN},
	/*
	** controls # commits in code. No longer used;
	** $optimize_concurrency = true is equivalent to $locks_held != "dbms"
	*/
#define     OPTIMIZE_CONCURRENCY 29
     {ERx("$short_remark"),   NULL, FG_TOK_BUILTIN},/* short remark for frame */
#define     SHORT_REMARK 30
     {ERx("$singleton_select"),	   NULL, FG_TOK_BUILTIN},/* singleton or attached qry? */
#define     SINGLETON_SELECT 31
     {ERx("$source_file_name"), NULL, FG_TOK_BUILTIN}, /* name of source file */
#define     SOURCE_FILE_NAME 32
     {ERx("$tablefield_menu"), NULL, FG_TOK_BUILTIN}, /* Use tablefield menu? */
#define     TABLEFIELD_MENU 33
     {ERx("$tblfld_name"),NULL, FG_TOK_BUILTIN},  /* name of det. table field */
#define     TBLFLD_NAME 34
     {ERx("$template_file_name"),  NULL, FG_TOK_BUILTIN},  /* name of template file */
#define     TEMPLATE_FILE_NAME 35
     {ERx("$timeout_code_exists"), NULL, FG_TOK_BUILTIN},  /* any On-Timeout escape code?" */
#define     TIMEOUT_CODE_EXISTS	36
     {ERx("$update_cascades"),  NULL, FG_TOK_BUILTIN},  /* update rule */
#define     UPDATE_CASCADES 37
     {ERx("$update_dbmsrule"),  NULL, FG_TOK_BUILTIN},  /* update rule */
#define     UPDATE_DBMSRULE 38
     {ERx("$update_restricted"),  NULL,FG_TOK_BUILTIN}, /* update rule */
#define     UPDATE_RESTRICTED 39
     {ERx("$user_qualified_query"), NULL, FG_TOK_BUILTIN}/*user enters qual & then "Go"*/
#define	    USER_QUALIFIED_QUERY 40
};
#define NMBRIITOKS (sizeof(Iitokarray)/sizeof(IITOKINFO))
GLOBALDEF i4  Nmbriitoks = NMBRIITOKS;

/* extern's */
GLOBALDEF char  *Tfname = ERx("iimain.tf");  /* start template processing here*/
GLOBALDEF i4	IifgMstr_NumNulKeys;
GLOBALDEF i4	IifgDet_NumNulKeys;
GLOBALDEF i4	IIFGhc_hiddenCnt;	/* used to uniquely name hidden fields*/

/*
**	fgquery.c
*/

GLOBALDEF i4  G_fgqline;  
GLOBALDEF char *G_fgqfile;

/*
**	fgusresc.c
*/

GLOBALDEF FILE  *fedmpfile;

/*
**	fgvars.c
*/

GLOBALDEF DECLARED_IITOKS *IIFGdcl_iitoks = NULL;       /* DEFINEd $vars */

/*
**	framegen.c
*/

/* GLOBALDEF's */
GLOBALDEF METAFRAME *G_metaframe;
 
GLOBALDEF IFINFO Ii_ifStack[MAXIF_NEST];
GLOBALDEF i4    Ii_tos; /* index for top of "Ii_ifStack" */
 
GLOBALDEF LOCATION IIFGcl_configloc;    /* LOCATION for II_CONFIG (FILES) */
GLOBALDEF char  IIFGcd_configdir[MAX_LOC+1];
GLOBALDEF FILE  *outfp;         /* output FILE to put generated 4gl into */


