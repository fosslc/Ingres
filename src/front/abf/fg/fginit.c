/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<me.h>
# include	<lo.h>
# include	<ug.h>
# include	<st.h>
# include	<ooclass.h>
# include	<abclass.h>
# include	<metafrm.h>
# include	<erfg.h>
# include	"framegen.h"


/**
** Name:	fginit.c -	perform frame generator initialization.
**
** Description:
**	This file defines:
**
**	IIFGinit()	call other initialization routines.
**	tok_init()	initialize Iitokarray.
**	meta_init()	set utility PTRs in METAFRAME.
**	setnul_utilptrs() initialize utility PTRs in METAFRAME to NULL.
**	setlk_hid	create hidden fields in METAFRAME for lookup join field
**	setpk_hid()	create hidden fields in METAFRAME for PK columns.
**	setjn_hid()	create hidden fields in METAFRAME for join fields.
**	setseq_hid()	create hidden fields in METAFRAME for non-displayed
**				sequenced fields.
**	setol_hid()	create hidden fields in METAFRAME for optimistic
**				locking columns
**	setsrt_hid()	set names of hiddens for non-displayed sort columns.
**	IIFGgdrGetDefRetVal()	determine default return value for a frame.
**
** History:
**	4/1/89 (pete)	created file by extracting from framegen.c
**	11/15/89 (pete) Changed $afd_top_frame to $default_start_frame.
**	27-feb-1992 (pete)
**	    Initialize new IITOKINFO.inttoktran attributes.
**	    Remove routine "get_tfname()" and always start processing in
**	    "iimain.tf".
**	24-jul-92 (blaise)
**		Added new substitution variables $timeout_code_exists and
**		$dbevent_code_exists.
**	6-dec-92 (blaise)
**		Added support for optimistic locking: added new substitution
**		variable, $locks_held; and added set_olhid() to create
**		hidden field names for optimistic locking columns.
**	16-feb-94 (blaise)
**		The substitution variables $nullable_master_key and
**		$nullable_detail_key are now set to true if there are
**		nullable key columns or nullable optimistic locking columns.
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch for c89
**      24-sep-96 (hanch04)
**          Global data moved to data.c
**      02-jan-97 (mcgem01)
**          Iitokarray should be an array not a pointer when GLOBALREF'ed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


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
GLOBALREF IITOKINFO Iitokarray[];
#define     DBEVENT_CODE_EXISTS 0
#define     DEFAULT_RETURN_VALUE 1
#define     DEFAULT_START_FRAME 2
#define     DELETE_ALLOWED 3
#define     DELETE_CASCADES 4
#define     DELETE_DBMSRULE 5
#define     DELETE_DETAIL_ALLOWED 6
#define     DELETE_MASTER_ALLOWED 7
#define     DELETE_RESTRICTED 8
#define     DETAIL_TABLE_NAME 9
#define     FORM_NAME 10
#define     FRAME_NAME 11
#define     FRAME_TYPE 12
#define     INSERT_DETAIL_ALLOWED 13
#define     INSERT_MASTER_ALLOWED 14
#define     JOIN_FIELD_DISPLAYED 15
#define     JOIN_FIELD_USED 16
#define LOCKS_HELD		17
#define LOOKUP_EXISTS		18
#define     MASTER_DETAIL_FRAME 19
#define     MASTER_IN_TBLFLD 20
#define     MASTER_SEQFLD_CNAME 21
#define     MASTER_SEQFLD_DISPLAYED 22
#define     MASTER_SEQFLD_EXISTS 23
#define     MASTER_SEQFLD_FNAME 24
#define     MASTER_SEQFLD_USED 25
#define     MASTER_TABLE_NAME 26
	/*
	** true if detail tbl has nullable key OR nullable optimistic locking
	** col(s)
	*/
#define     NULLABLE_DETAIL_KEY 27
	/*
	** true if master tbl has nullable key OR nullable optimistic locking
	** col(s)
	*/
#define     NULLABLE_MASTER_KEY 28
	/*
	** controls # commits in code. No longer used;
	** $optimize_concurrency = true is equivalent to $locks_held != "dbms"
	*/
#define     OPTIMIZE_CONCURRENCY 29
#define     SHORT_REMARK 30
#define     SINGLETON_SELECT 31
#define     SOURCE_FILE_NAME 32
#define     TABLEFIELD_MENU 33
#define     TBLFLD_NAME 34
#define     TEMPLATE_FILE_NAME 35
#define     TIMEOUT_CODE_EXISTS	36
#define     UPDATE_CASCADES 37
#define     UPDATE_DBMSRULE 38
#define     UPDATE_RESTRICTED 39
#define	    USER_QUALIFIED_QUERY 40

GLOBALREF i4  Nmbriitoks ;

/* extern's */
GLOBALREF char  *Tfname ;  /* start template processing here*/
GLOBALREF i4	IifgMstr_NumNulKeys;
GLOBALREF i4	IifgDet_NumNulKeys;
GLOBALREF i4	IIFGhc_hiddenCnt;	/* used to uniquely name hidden fields*/

/* globalref */
GLOBALREF DECLARED_IITOKS *IIFGdcl_iitoks;       /* DECLAREd $vars */

/* functions */
FUNC_EXTERN MFTAB *IIFGpt_pritbl();
FUNC_EXTERN char  *IIFGhn_hiddenname();
FUNC_EXTERN char  *IIFGft_frametype();
FUNC_EXTERN PTR	  IIFGgm_getmem();

/* # define's */
FUNC_EXTERN char  *IIFGgdrGetDefRetVal();
FUNC_EXTERN VOID  IIFGinit();

/* static's */
static bool   lookup_exists();
static bool   sequence_field();
static bool   join_displayed();
static VOID   tok_init();
static VOID   meta_init();

static VOID   setnul_utilptrs();
static VOID   setpk_hid();
static VOID   setlk_hid();
static VOID   setjn_hid();
static VOID   setol_hid();
static VOID   setseq_hid();
static VOID   setcor_nms();
static VOID   setsrt_hid();
static VOID   cntnullkeys();
static i4     cntnk();

static char  *S_true  = ERx("1");
static char  *S_false = ERx("0");
static char  *S_null  = ERx("");
static char  *S_none  = ERx("none");
static char  *S_dbms  = ERx("dbms");
static char  *S_optim = ERx("optimistic");

/*{
** Name:	IIFGinit - main initialization routine. calls others.
**
** Description:
**	call other frame generation initialization routines.
**
** Inputs:
**	
**      METAFRAME *mf   Pointer to meta frame structure from VQ to init from.
**
** Outputs:
**	initialized METAFRAME and Iitokarray.
**
**	Returns:
**		VOID
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	26-may-1989 (pete)	written.
**      20-nov-1989 (pete)	Switch order to call meta_init BEFORE tok_init,
**				because tok_init needs to know name of hidden
**				sequenced field (if any).
**	16-mar-1990 (pete)	Add code to count number nullable key fields.
*/
VOID
IIFGinit(mf)
METAFRAME *mf;
{
	MFTAB *p_mftab;		/* primary table in master section */
	MFTAB *p_mfDetTab;	/* primary table in detail section */

	IIFGhc_hiddenCnt = 0;

	/* find primary Master table in METAFRAME */
	if ( (p_mftab = IIFGpt_pritbl(mf, TAB_MASTER)) != NULL)
		;
	else	/* master table name not found */
	{
	    USER_FRAME *p_uf = (USER_FRAME *) mf->apobj;

	    if (p_uf->class != OC_MUFRAME)
	    {
		/* It's ok for a Menuframe to NOT have a primary
		** master table.  Other frame types we generate
		** code for MUST have a primary master table in METAFRAME.
		*/
		IIFGerr (E_FG0025_Missing_MasterTbl, FGFAIL | FGSETBIT, 0);
	    }
	}

	/* find primary Detail table in METAFRAME */
	if ((mf->mode & MFMODE) == MF_MASDET)
	{
	    /* locate primary table in detail section */
	    if ( (p_mfDetTab  = IIFGpt_pritbl(mf, TAB_DETAIL)) != NULL)
		;
	    else	/* Master-Detail VQ has no primary Detail table */
	    {
		IIFGerr (E_FG0024_Missing_DetailTbl, FGFAIL | FGSETBIT, 0);
	    }
	}
	else
	    p_mfDetTab = NULL;

	/* initialize utility PTRs in METAFRAME's MFTAB and MFCOL structs. */
	meta_init (mf, p_mftab, p_mfDetTab) ;

	/*
	** count nullable key columns/optimistic locking cols in primary
	** table(s)
	*/
	cntnullkeys (mf, p_mftab, p_mfDetTab,
		  &IifgMstr_NumNulKeys, &IifgDet_NumNulKeys);

	/* initialize $variable values */
	tok_init (mf, p_mftab, p_mfDetTab);
}

/*{
** Name:	cntnullkeys - count number of nullable keys in primary tables.
**
** Description:
**	count number of nullable keys in primary tables of UPDATE frames.
**
** Inputs:
**	METAFRAME *mf
**	MFTAB *p_mftab		primary table in master section 
**	MFTAB *p_mfDetTab	primary table in detail section 
**	i4   *mstrcnt		number nullable master key cols
**	i4   *detcnt		number nullable detail key cols
**
** Outputs:
**	Writes values to last two arguments.
**
**	Returns:
**		VOID
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	3/16/90	(pete)	Initial Version.
**	16-feb-94 (blaise)
**		Function now counts number of nullable key cols + number of
**		nullable optimistic locking columns.
*/
static VOID
cntnullkeys (mf, p_mftab, p_mfDetTab, mstrcnt, detcnt)
METAFRAME *mf;
MFTAB *p_mftab;		/* primary table in master section */
MFTAB *p_mfDetTab;	/* primary table in detail section */
i4    *mstrcnt;		/* number nullable master key cols */
i4    *detcnt;		/* number nullable detail key cols */
{
    USER_FRAME *p_uf = (USER_FRAME *) mf->apobj;
    bool optimistic = ((mf->mode & MF_OPTIM) ? TRUE : FALSE);

    /* we only care about the Null key count for UPDATE frames */
    if ( p_uf->class == OC_UPDFRAME)
    {
	*mstrcnt = cntnk (p_mftab, optimistic);

	if (p_mfDetTab != (MFTAB *)NULL)
	{
	    *detcnt = cntnk (p_mfDetTab, optimistic);
	}
	else
	    *detcnt = 0;
    }
    else
	*mstrcnt = *detcnt = 0;

    return;
}

/*{
** Name:	cntnk - count nullable key columns for a table.
**
** Description:
**	walk through the MFCOL array looking for nullable key columns.
**
** Inputs:
**	MFTAB *p_tab	count null key cols for this table.
**	bool optimistic - true if frame uses optimistic locking.
**
** Outputs:
**
**	Returns:
**		count of number nullable key columns.
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	3/16/90	(pete)	Initial Version.
**	16-feb-94 (blaise)
**		Added optimistic parameter.
*/
static i4
cntnk (p_tab, optimistic)
MFTAB *p_tab;
{
	MFCOL **p_mfcol = p_tab->cols;
	i4 i =0;
	i4 nullkeycnt =0;

	/* find nullable key columns and nullable optimistic locking columns */
	for ( ; i < p_tab->numcols; i++, p_mfcol++)
	{
		if ((*p_mfcol)->type.db_datatype < 0) /* if col is nullable */
		{
			/*
			** if ((this is a key column) OR
			** (frame uses optimistic locking AND
			** column is being used for optimistic locking))...
			*/
			if ( ((*p_mfcol)->flags & COL_UNIQKEY) ||
			(optimistic && ((*p_mfcol)->flags & COL_OPTLOCK)) )
			{
		 		nullkeycnt++;
			}
		}
	}

	return nullkeycnt;
}

/*{
** Name:	meta_init - initialize METAFRAME Utility PTRs.
**
** Description:
**	Set names of correlation names and hidden fields by setting
**	utility pointers in MFCOL and MFTAB.
**	(Note that utility pointers in MFCOL are not set for Lookup tables)
**
** Inputs:
**	mf	pointer to METAFRAME.
**	p_mftab address of primary master table in METAFRAME.
**
** Outputs:
**	initialized METAFRAME.
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	26-may-1989 (pete)	written.
*/
static VOID
meta_init(mf, p_mftab, p_mfDetTab)
METAFRAME *mf;
MFTAB *p_mftab;	/* pointer to primary table in master section */
MFTAB *p_mfDetTab; /* pointer to primary table in detail section */
{
    USER_FRAME *p_uf = (USER_FRAME *) mf->apobj;

    /* loop thru METAFRAME & init all MFTAB and MFCOL utility PTRs to NULL*/
    setnul_utilptrs(mf);

    /* create correlation names for every table in METAFRAME. */
    setcor_nms(mf);

    /* All frames -- create hiddens for lookup join fields */
    setlk_hid(mf);

    /* UPDATE frames only: */
    if ( p_uf->class == OC_UPDFRAME)
    {
	/* Create hidden field names for master table PK columns.  */
	setpk_hid( p_mftab);

	/* Create hidden column names for Detail table PK columns. */
	if (p_mfDetTab != NULL)
	    setpk_hid(p_mfDetTab);

	/*
	** Create hidden field names for master & detail table optimistic
	** locking columns
	*/
	setol_hid(p_mftab);
	if (p_mfDetTab != NULL)
	    setol_hid(p_mfDetTab);
    }

    /* APPEND frames only: */
    if ( p_uf->class == OC_APPFRAME )
    {
	setseq_hid(p_mftab);	/* Create hidden field name for any
				** non-displayed sequenced fields in
				** primary Master table.
				*/
	/* NOTE: append frames don't need hidden fields declared for
	** join fields or key fields, whether displayed or not.
	** In Append frames, non-displayed fields in the primary
	** tables are assigned a value in the INSERT statement = to what
	** was entered by the user into the MFCOL.info field in the VQ.
	** Thus, even if a hidden field existed, it couldn't be used in
	** the INSERT statement.
	*/
    }

    /* BROWSE and UPDATE frames */
    if ( ( p_uf->class == OC_UPDFRAME ) || (p_uf->class == OC_BRWFRAME))
    {
	/* Create hidden field names for non-displayed Sort fields for
	** in Primary Master and Detail tables.
	*/
	setsrt_hid(mf);

	/* Create hidden field names for Master/Detail join columns */
	setjn_hid(mf, (bool)(p_uf->class == OC_UPDFRAME));
    }
}

/*{
** Name:	setsrt_hid - set names of hiddens for non-displayed sort columns
**
** Description:
**	Users are allowed to specify a sort on a non-displayed field. However,
**	in SQL you cannot do an ORDER BY on a column that is not in the target
**	list, so to make this work, we have to add the "non-displayed" column
**	to the target list and select it into a hidden field that is never
**	used.
**
** Inputs:
**	mf	pointer to METAFRAME.
**
** Outputs:
**	METAFRAME with utility PTRs set in MFCOL for non-displayed columns
**	with a sort specified.
**
**	Returns:
**		VOID
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	11/29/89	(pete)	Initial version.
*/
static VOID
setsrt_hid(mf)
METAFRAME *mf;
{
	register MFTAB **p_mftab = mf->tabs;
        register MFCOL **p_mfcol;
	register i4  i, j;


        /*
        ** Loop thru all MFTABs in METAFRAME & set hidden field names.
        ** Skip LookUp tables, since they don't have Sort entries.
        */
        for (i=0; i < mf->numtabs; i++, p_mftab++)
        {
            if ((*p_mftab)->usage != TAB_LOOKUP)
            {
		p_mfcol = (*p_mftab)->cols;

        	/* loop thru all columns for this table */
        	for (j=0; j < (*p_mftab)->numcols ; j++, p_mfcol++)
		{
		    /* 
		    ** see if column is not used or in a variable and has 
		    ** a sort order 
		    */
	   	    if ( !((*p_mfcol)->flags & (COL_USED|COL_VARIABLE))
			 && ((*p_mfcol)->corder != 0))
	    	    {
			/* create a hidden field, unless one is already set */
                	if ((*p_mfcol)->utility == NULL)
                	{
                   	    (*p_mfcol)->utility =
				IIFGhn_hiddenname( (*p_mfcol)->alias);
                	}
			else
		   	    ;	/*
			        ** Someone already set the Utility PTR.
				** Leave it alone. (e.g. this column may be
				** part of Master PK and was already set by 
				** setpk_hid).
			        */
	    	    }
        	}
	    }
        }
	return;
}

/*{
** Name:	setlk_hid - Create hiddens for lookup join fields
**
** Description:
**	We need to pass hidden fields to a look_up, to avoid the putforms
**	generated by the BYREF call, in case the user cancels.  
**	We generate them here.  It's needed for the subordinate join column.
**
** Inputs:
**	mf		METAFRAME *	metaframe
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	9/90 (Mike S)	Initial version
*/
static VOID
setlk_hid(mf)
METAFRAME *mf;
{
	register MFTAB *tabptr;
	register MFCOL *colptr;
	register i4  i, j;

	for (i = 0; i < mf->numtabs; i++)
	{
    	    tabptr = mf->tabs[i];
    	    if (tabptr->usage == TAB_LOOKUP)
    	    {
    		for (j = 0; j < tabptr->numcols; j++)
    		{
    		    colptr = tabptr->cols[j];
    		    if ((colptr->flags & COL_SUBJOIN) != 0)
    		       	colptr->utility = IIFGhn_hiddenname(colptr->alias);
    	        }
    	    }
        }
}

/*{
** Name:	setjn_hid - set names of hidden versions of Master join fields.
**
** Description:
**	For the join between the Master and Detail table, set the name for
**	a hidden field for the master part and set the detail part to
**	an empty string.
**
** Inputs:
**	mf	pointer to METAFRAME.
**
** Outputs:
**	METAFRAME with utility PTRs set in MFCOL for join columns, for joins
**	between Master and Detail.
**
**	Returns:
**		VOID
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	5/30/89	(pete)	Written.
**	7/91 (Mike S) Added is_update logic
*/
static VOID
setjn_hid(mf, is_update)
METAFRAME *mf;
bool	is_update;
{
	MFTAB **p_mftab = mf->tabs;
	register MFJOIN **p_mfjoin = mf->joins;
	register i4  i;

	for (i=0; i < mf->numjoins; i++, p_mfjoin++)
	{
	    if ( (*p_mfjoin)->type == JOIN_MASDET)
	    {
		MFTAB *tmp_mftab;
		MFCOL **tmp_mfcol;

		/* pointer to master table in join (MFJOIN.tab_1 is for mstr).*/
		tmp_mftab = *(p_mftab + (*p_mfjoin)->tab_1);

		/* pointer to MFCOLS for above master table in join */
		tmp_mfcol = tmp_mftab->cols + (*p_mfjoin)->col_1 ;

		if ( (*tmp_mfcol)->utility == NULL)
		{
		    /*
		    ** Hasn't been set yet (could have been set earlier if
		    ** this column is also a primary key).
		    ** For update frames, we always need this.  For browse
		    ** frames, we only need it if the column is non-displayed.
		    */
		    if (is_update ||
			(((*tmp_mfcol)->flags & (COL_USED|COL_VARIABLE)) == 0))
		    {
			(*tmp_mfcol)->utility =
				IIFGhn_hiddenname((*tmp_mfcol)->alias);
		    }
		}
		else
		    ;	/*
			** Master utility PTR already set. leave it alone.
			** (this column may be part of Master PK and was already
			** set by setpk_hid).
			*/

		/* pointer to detail table in join (MFJOIN.tab_2 is for detl).*/
		tmp_mftab = *(p_mftab + (*p_mfjoin)->tab_2);

		/* pointer to MFCOLS for above detail table in join */
		tmp_mfcol = tmp_mftab->cols + (*p_mfjoin)->col_2 ;

		/*
		** for Detail part of MD join, never want the utility pointer to
		** be NULL, but don't want it set to a real string either, so
		** set it to point to empty string. This tells subsequent
		** initialization routines (s.a. setpk_hid, which sets
		** names of hidden fields for table PKs -- remember that
		** this detail column may also be a PK) that this utility
		** pointer has already been set.
		*/
		(*tmp_mfcol)->utility = (char *)S_null ;
	    }
	}
}

/*{
** Name:  setcor_nms - set correlation names for tables in SELECT/FROM clause.
**
** Description:
**	loop thru MFTAB structures in METAFRAME and set utility
**	PTR = correlation name for each table. Correlation names:
**	"m" for master table; "ml1", "ml2", etc for master lookup tables;
**	"d" for detail table; "dl1", "dl2", etc for detail lookup tables.
**
** Inputs:
**	mf	pointer to METAFRAME.
**
** Outputs:
**	METAFRAME with filled in 'MFTAB.utility'.
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	5/30/89 (pete)	Written.
*/
static VOID
setcor_nms(mf)
METAFRAME *mf;
{
	register MFTAB **p_mftab = mf->tabs;
	register i4  i;
	i4 mlcnt =1;	/* master lookup table count */
	i4 dlcnt =1;	/* detail lookup table count */
	char buf[5];	/* working buffer */

	for (i=0; i < mf->numtabs; i++, p_mftab++)
	{
	    if ((*p_mftab)->tabsect == TAB_MASTER)
	    {
		if ((*p_mftab)->usage <= TAB_DISPLAY)	/*primary master table*/
		{
		    (*p_mftab)->utility = (char *)ERx("m") ;
		}
		else	/* master section lookup table */
		{
		    STprintf (buf, ERx("ml%d"), mlcnt++);
		    (*p_mftab)->utility =
		    	(char *)IIFGgm_getmem((u_i4)(STlength(buf)+1), FALSE);
		    STcopy (buf, (*p_mftab)->utility);
		}
	    }
	    else	/* assume TAB_DETAIL */
	    {
		if ((*p_mftab)->usage <= TAB_DISPLAY)	/*primary detail table*/
		{
		    (*p_mftab)->utility = (char *)ERx("d") ;
		}
		else	/* detail section lookup table */
		{
		    STprintf (buf, ERx("dl%d"), dlcnt++);
		    (*p_mftab)->utility =
		    	(char *)IIFGgm_getmem((u_i4)(STlength(buf)+1), FALSE);
		    STcopy (buf, (*p_mftab)->utility);
		}
	    }
	}
}

/*{
** Name:	setnul_utilptrs - set METAFRAME MFTAB/MFCOL utility PTRs = NULL.
**
** Description:
**	loop through MFTAB and MFCOL entries in METAFRAME and set utility
**	pointers to NULL. Do for all frame types.
**
** Inputs:
**	mf	pointer to a METAFRAME.
**
** Outputs:
**	METAFRAME with utility PTRs initialized correctly.
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		NONE.
**
** Side Effects:
**
** History:
**	5/30/89 (pete)	Written.
*/
static VOID
setnul_utilptrs(mf)
METAFRAME *mf;
{
	register i4  colcnt, tabcnt;
	register MFTAB **p_mftab = mf->tabs;
	register MFCOL **p_mfcol;

	for (tabcnt=0; tabcnt < mf->numtabs; tabcnt++, p_mftab++)
	{
	    (*p_mftab)->utility = NULL;
	    p_mfcol = (*p_mftab)->cols;

	    for (colcnt=0; colcnt < (*p_mftab)->numcols; colcnt++, p_mfcol++)
	    {
		(*p_mfcol)->utility = NULL;
	    }
	}
}

/*{
** Name:	setpk_hid - create hidden field names for table Primary Key.
**
** Description:
**	Traverse through a table's
**	MFCOLs and, for its PK columns, set the Utility PTR to point to a
**	unique name, to be used later as the name of a hidden field to load
**	that column into (in SELECT statement).
**	Note that the name of the hidden field for some columns will already
**	have been filled in (for example, a PK column that is also one
**	of the join fields). If it's already filled in, then this routine
**	won't mess with it.
**
** Inputs:
**	p_mftab		address of MFTAB for a table.
**
** Outputs:
**	same METAFRAME, with utility PTR in MFCOL filled in for PK fields.
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		NONE
**
** Side Effects:
**	allocates memory (IIFGhn_hiddenname).
**
** History:
**	5/30/89	(pete)	Written.
*/
static VOID
setpk_hid(p_mftab)
MFTAB *p_mftab;
{
	register MFCOL **p_mfcol = p_mftab->cols;
	register i4  i;

	/* loop thru every column in table ...*/
	for (i=0; i < p_mftab->numcols; i++, p_mfcol++)
	{
		/* ... and see if the column is part of a unique key ...*/
		if ( (*p_mfcol)->flags & COL_UNIQKEY )
		{
		    /* ... and, if it is, and its utility PTR is still NULL
		    ** (which means it hasn't already been set by another 
		    ** initialization routine) then set
		    ** name of the hidden field for that column.
		    */
		    if ( (*p_mfcol)->utility == NULL)
		    {
		        (*p_mfcol)->utility =
				IIFGhn_hiddenname( (*p_mfcol)->alias);
		    }
		}
	}
}

/*{
** Name:  setseq_hid - Create hidden fld names for non-displayed sequenced field
**
** Description:
**	If the primary master table contains a non-displayed sequenced
**	field, then create a hidden field for it. (sequenced fields are
**	not supported in the Detail tables).
**
** Inputs:
**	p_mftab		address of MFTAB for primary table in master section.
**
** Outputs:
**	mf with utility pointer set, if needed.
**
**	Returns:
**		NONE
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	5/30/89 (pete)	Written.
*/
static VOID
setseq_hid(p_mftab)
MFTAB *p_mftab;	 	/* primary table in master section */
{
	MFCOL **p_mfcol = p_mftab->cols;
	register i4  i;

	/* loop thru every column in primary master table ...*/
	for (i=0; i < p_mftab->numcols; i++, p_mfcol++)
	{
		/* 
		** ...and see if the col is a non-displayed, not in a variable 
		** sequenced field 
		*/
		if (    ((*p_mfcol)->flags & COL_SEQUENCE)
		    && !((*p_mfcol)->flags & (COL_USED|COL_VARIABLE))
		   )
		{
		    /* ...and, if so, set name of hidden field for that col.*/
		    (*p_mfcol)->utility = IIFGhn_hiddenname( (*p_mfcol)->alias);
		}
	}
}

/*{
** Name:	setol_hid - create hidden field names for opt locking cols
**
** Description:
**	Traverse through a table's MFCOLs and, for any columns which are
**	being used for optimistic locking, set the utility PTR to point to a
**	unique name, to be used later as the name of a hidden field to load
**	that column into (in SELECT statement).
**	If the name of a hidden field is already filled in, we do nothing.
**
** Inputs:
**	p_mftab		address of MFTAB for a table.
**
** Outputs:
**	same METAFRAME, with utility PTR in MFCOL filled in for PK fields.
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		NONE
**
** Side Effects:
**	allocates memory (IIFGhn_hiddenname).
**
** History:
**	4-dec-92 (blaise)
**		Initial version, cloned from Pete's setpk_hid().
*/
static VOID
setol_hid(p_mftab)
MFTAB *p_mftab;
{
	register MFCOL **p_mfcol = p_mftab->cols;
	register i4  i;

	/* loop thru every column in table ...*/
	for (i=0; i < p_mftab->numcols; i++, p_mfcol++)
	{
		/* ... see if the column is used for optimistic locking ... */
		if ( (*p_mfcol)->flags & COL_OPTLOCK )
		{
			/* ... and, if it is, and its utility PTR is still
			** NULL (which means it hasn't already been set by
			** another initialization routine) then set the
			** name of the hidden field for that column.
			*/
			if ( (*p_mfcol)->utility == NULL)
			{
				(*p_mfcol)->utility =
					IIFGhn_hiddenname( (*p_mfcol)->alias);
			}
		}
	}
}

/*{
** Name:	tok_init()	- initialize values in Iitokarray based on VQ.
**
** Description:
**	Initialize the array of $token translations using VQ info.
**	Using meta structure from VQ, initialize info in Iitokarray[],
**	  which has info on the $tokens setup by VQ.
**	Boolean "$" variables are set to "1" if TRUE, or "0" if FALSE
**	  or undefined.
**	String "$" variables are set to their value in VQ, or "" (Empty
**	  string) if undefined in VQ.
**
** Inputs:
**	METAFRAME *mf	metaframe to initialize $vars from.
**	MFTAB *p_mftab	pointer to primary table in master section
**	MFTAB *p_mfDetTab pointer to primary table in detail section
**
** Outputs:
**	correct translation values assigned to Iitokarray[i].toktran.
**
**	Returns:
**		NONE
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	4/1/89 (pete)	written
**	1/31/91 (pete)
**		Add support for $master_in_tblfld.
**	27-feb-1992 (pete)
**	    initialize both integer and character versions of integer
**	    and boolean $vars. Added new $var: $master_detail_frame; changed
**	    initialization of $template_file_name.
*/
static VOID
tok_init(mf, p_mftab, p_mfDetTab)
METAFRAME *mf;	/* mf was checked for NULL when built output file spec */
MFTAB	*p_mftab; /* pointer to primary table in master section */
MFTAB	*p_mfDetTab; /* pointer to primary table in detail section */
{
	USER_FRAME *p_uf = (USER_FRAME *) mf->apobj ;
	i4	i;

    /* $delete_master_allowed */
    /* $insert_master_allowed */
    /* $join_field_displayed */
    /* $join_field_used */
    /* $master_seqfld_displayed */
    /* $master_seqfld_used */
    /* $master_seqfld_exists*/
    /* $master_seqfld_name */
    /* $master_table_name */
	if (p_mftab != NULL)
	{
	    bool join_used;

	    Iitokarray[DELETE_MASTER_ALLOWED].toktran =
		(p_mftab->flags & TAB_DELFLG ) ? S_true : S_false ;

	    Iitokarray[INSERT_MASTER_ALLOWED].toktran =
		(p_mftab->flags & TAB_INSFLG ) ? S_true : S_false ;

	    /* 'sequence_field()' both sets the name of '$master_seqfld_name',
	    ** '$master_seqfld_displayed', and "$master_seqfld_used and 
	    ** returns TRUE or FALSE based
	    ** on if a sequence field exists.
	    */
	    Iitokarray[MASTER_SEQFLD_EXISTS].toktran = 
	      sequence_field(p_mftab, 
				&(Iitokarray[MASTER_SEQFLD_CNAME].toktran),
			        &(Iitokarray[MASTER_SEQFLD_FNAME].toktran),
	      			&(Iitokarray[MASTER_SEQFLD_DISPLAYED].toktran),
	      			&(Iitokarray[MASTER_SEQFLD_USED].toktran) )
	      ? S_true : S_false ;

	    Iitokarray[MASTER_TABLE_NAME].toktran = p_mftab->name;

	    Iitokarray[JOIN_FIELD_DISPLAYED].toktran =
		join_displayed(p_mftab, &join_used) ? S_true : S_false;
	    Iitokarray[JOIN_FIELD_USED].toktran = join_used ? S_true : S_false;
	}
	else
	{
	    Iitokarray[DELETE_MASTER_ALLOWED].toktran =
	        Iitokarray[INSERT_MASTER_ALLOWED].toktran =
	        Iitokarray[MASTER_SEQFLD_DISPLAYED].toktran =
	        Iitokarray[MASTER_SEQFLD_EXISTS].toktran =
	        Iitokarray[JOIN_FIELD_USED].toktran = 
	        Iitokarray[JOIN_FIELD_DISPLAYED].toktran = S_false;

	    Iitokarray[MASTER_SEQFLD_CNAME].toktran =
	    Iitokarray[MASTER_SEQFLD_FNAME].toktran =
	        Iitokarray[MASTER_TABLE_NAME].toktran = S_null;
	}


    /* $delete_detail_allowed */
    /* $insert_detail_allowed */
    /* $delete_cascades */
    /* $delete_restricted */
    /* $delete_dbmsrule */
    /* $detail_table_name */
    /* $update_cascades */
    /* $update_restricted */
    /* $update_dbmsrule */
	if (p_mfDetTab != NULL)
	{
		/* this is a Master-Detail METAFRAME */
		Iitokarray[DELETE_DETAIL_ALLOWED].toktran =
		    (p_mfDetTab->flags & TAB_DELFLG ) ? S_true : S_false ;

		Iitokarray[INSERT_DETAIL_ALLOWED].toktran =
		    (p_mfDetTab->flags & TAB_INSFLG ) ? S_true : S_false ;

		Iitokarray[DELETE_CASCADES].toktran =
		    (p_mfDetTab->flags & TAB_DELRULE ) ? S_false : S_true ;

		Iitokarray[DELETE_RESTRICTED].toktran =
		    (p_mfDetTab->flags & TAB_DELRULE ) ? S_true : S_false ;

		Iitokarray[DELETE_DBMSRULE].toktran =
		    (p_mfDetTab->flags & TAB_DELNONE ) ? S_true : S_false ;

	        Iitokarray[DETAIL_TABLE_NAME].toktran = p_mfDetTab->name;

		Iitokarray[UPDATE_CASCADES].toktran =
		    (p_mfDetTab->flags & TAB_UPDRULE ) ? S_false : S_true ;

		Iitokarray[UPDATE_RESTRICTED].toktran =
		    (p_mfDetTab->flags & TAB_UPDRULE ) ? S_true : S_false ;

		Iitokarray[UPDATE_DBMSRULE].toktran =
		    (p_mfDetTab->flags & TAB_UPDNONE ) ? S_true : S_false ;
	}
	else
	{
	    Iitokarray[DELETE_DETAIL_ALLOWED].toktran = 
	        Iitokarray[INSERT_DETAIL_ALLOWED].toktran =
	        Iitokarray[DELETE_CASCADES].toktran   =
	        Iitokarray[DELETE_RESTRICTED].toktran =
	        Iitokarray[UPDATE_CASCADES].toktran   =
	        Iitokarray[UPDATE_RESTRICTED].toktran = S_false;

	    Iitokarray[DETAIL_TABLE_NAME].toktran     = S_null;
	}


    /* $delete_allowed */
	if ( (Iitokarray[DELETE_DETAIL_ALLOWED].toktran == S_true)
	  || (Iitokarray[DELETE_MASTER_ALLOWED].toktran == S_true))
		Iitokarray[DELETE_ALLOWED].toktran = S_true ;
	else
		Iitokarray[DELETE_ALLOWED].toktran = S_false ;
	

    /* $default_return_value */
	Iitokarray[DEFAULT_RETURN_VALUE].toktran = IIFGgdrGetDefRetVal(mf);


    /* $tblfld_name */
	Iitokarray[TBLFLD_NAME].toktran = TFNAME;


    /* $default_start_frame */
	Iitokarray[DEFAULT_START_FRAME].toktran =
		STequal(p_uf->name, p_uf->appl->start_name)
		? S_true : S_false ;

    /* $form_name */
        Iitokarray[FORM_NAME].toktran = p_uf->form->name;

    /* $frame_name */
	Iitokarray[FRAME_NAME].toktran = p_uf->name ;

    /* $short_remark */
	Iitokarray[SHORT_REMARK].toktran = p_uf->short_remark ; 

    /* $source_file_name */
	Iitokarray[SOURCE_FILE_NAME].toktran = p_uf->source ; 

    /* $frame_type */
    	Iitokarray[FRAME_TYPE].toktran = IIFGft_frametype(p_uf->class);

	
    /* $lookup_exists */
	Iitokarray[LOOKUP_EXISTS].toktran = lookup_exists(mf) ? S_true :S_false;

    /* $locks_held */
    /* $optimize_concurrency (no longer used) */
	if (mf->mode & MF_OPTIM)
	{
		Iitokarray[LOCKS_HELD].toktran = S_optim;
		Iitokarray[OPTIMIZE_CONCURRENCY].toktran = S_true;
	}
	else
	{
		Iitokarray[LOCKS_HELD].toktran =
			(mf->mode & MF_DBMSLOCK) ? S_dbms : S_none;
		Iitokarray[OPTIMIZE_CONCURRENCY].toktran =
			(mf->mode & MF_DBMSLOCK) ? S_false : S_true;
	}

    /* $singleton_select */
	Iitokarray[SINGLETON_SELECT].toktran =
		    (mf->mode & MF_NONEXT) ? S_true : S_false ;

    /* $user_qualified_query */
	Iitokarray[USER_QUALIFIED_QUERY].toktran =
		    (mf->mode & MF_NOQUAL) ? S_false : S_true ;

    /* $template_file_name */
	Iitokarray[TEMPLATE_FILE_NAME].toktran = Tfname ;

    /* $master_detail_frame */
	Iitokarray[MASTER_DETAIL_FRAME].toktran = 
		((mf->mode & MFMODE) == MF_MASDET) ? S_true : S_false ;

    /* $nullable_detail_key */
    /* $nullable_master_key */
	Iitokarray[NULLABLE_DETAIL_KEY].toktran = (IifgDet_NumNulKeys > 0)
						  ? S_true : S_false ;
	Iitokarray[NULLABLE_MASTER_KEY].toktran = (IifgMstr_NumNulKeys > 0)
						  ? S_true : S_false ;

    /* $master_in_tblfld */
	Iitokarray[MASTER_IN_TBLFLD].toktran =
		    ((mf->mode & MFMODE) == MF_MASTAB) ? S_true : S_false ;

    /* $tablefield_menu */
	Iitokarray[TABLEFIELD_MENU].toktran = 
		    ((p_uf->flags & APC_MS_MASK) == APC_MSTAB)
		    ? S_true : S_false ;

    /* $timeout_code_exists */
    /* $dbevent_code_exists */
	Iitokarray[DBEVENT_CODE_EXISTS].toktran = S_false;
	Iitokarray[TIMEOUT_CODE_EXISTS].toktran = S_false;
	for (i = 0; i < mf->numescs &&
		(Iitokarray[DBEVENT_CODE_EXISTS].toktran == S_false ||
		Iitokarray[TIMEOUT_CODE_EXISTS].toktran == S_false); i++)
	{
		if (mf->escs[i]->type == ESC_ON_DBEVENT)
		{
			Iitokarray[DBEVENT_CODE_EXISTS].toktran = S_true;
			continue;
		}
		else if (mf->escs[i]->type == ESC_ON_TIMEOUT)
		{
			Iitokarray[TIMEOUT_CODE_EXISTS].toktran = S_true;
		}
	}

    /* initialize the linked list of DECLARED_IITOKS */
    IIFGdcl_iitoks = (DECLARED_IITOKS *)NULL;
}

/*{
** Name:	IIFGgdrGetDefRetVal - get default return value for a frame.
**
** Description:
**	Look at the frame's return type and determine a correct default
**	return value for that return type.
**	<comments>
**
** Inputs:
**	METAFRAME *mf;	metaframe to locate frame return type in.
**
** Outputs:
**	Returns:
**		pointer to string return value.
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	6/2/89 (pete)	Written.
**	8/6/90 (pete)	Write out "0" for all numeric types, rather than
**			"0.0" for float/money (& "0,0" if II_DECIMAL is set).
**			This is simpler, and equivalent.
*/
char
*IIFGgdrGetDefRetVal(mf)
METAFRAME *mf;
{
	USER_FRAME *p_uf = (USER_FRAME *) mf->apobj;
	i4	type;

	/* calc absolute value of p_uf->return_type.db_datatype */
	if (p_uf->return_type.db_datatype < 0)
	    type = -(p_uf->return_type.db_datatype);
	else
	    type = p_uf->return_type.db_datatype;

	switch (type)
	{
		case DB_INT_TYPE:
		case DB_DEC_TYPE:
		case DB_FLT_TYPE:
		case DB_MNY_TYPE:
		    return ERx("0");

		case DB_NODT:	  /* return type of NONE */
		    return ERx("");

		default:
		    return ERx("''");  /*character types return empty string */
	}
}

/*{
** Name:	lookup_exists - tell if VQ has a lookup table.
**
** Description:
** 	Return TRUE if a lookup table exists in VQ; FALSE if none exists.
**
** Inputs:
**	METAFRAME	*mf
**
**	Returns:
**		bool
**
** History:
**	5/22/89 (pete)	written.
*/
static bool
lookup_exists (mf)
METAFRAME	*mf;
{
        MFTAB **tptr = mf->tabs;
	i4 i=0;

	for ( ;i < mf->numtabs; i++, tptr++ )
	    if ( (*tptr)->usage == TAB_LOOKUP)
		return (TRUE);

	return (FALSE);
}

/*{
** Name:	sequence_field - tell if master table in VQ has sequenced field.
**
** Description:
**  Return TRUE if the primary table in VQ master section has a sequenced
**  field; FALSE otherwise. Also, set the value of the second argument to
**  be either an empty string (if no sequenced fields) or the name
**  of the sequenced field (if one exists).
**
** Inputs:
**	MFTAB	*p_mftab	Table structure for primary MASTER table.
**	char  **p_scname	Set this to name of sequenced database column.
**	char  **p_sfname	Set this to name of sequenced form field.
**	char  **p_sfdisp	Set this to S_true if sequence field exists & is
**				displayed. Set to S_false otherwise.
**	char  **p_sfused	Set this to S_true if sequence field exists & is
**				either on the form or in a local variable. Set 
**				to S_false otherwise.
** Outputs:
**	Set second argument to column name of sequence field (or empty string).
**	Set third argument to alias name of sequence field (or empty string).
**	Set fourth argument according to if sequenced field is displayed.
**	Set fifth argument according to if sequenced field is displayed or in 
**		a variable.
**
**	Returns:
**		bool
**
** History:
**	5/22/89 (pete)		written.
**	5/26/89 (pete)		add second argument.
**	20-nov-89 (pete)	allow sequenced field name to be a hidden field.
*/
static bool
sequence_field(p_mftab, p_scname, p_sfname,  p_sfdisp, p_sfused)
MFTAB *p_mftab;
char  **p_scname;
char  **p_sfname;
char  **p_sfdisp;
char  **p_sfused;
{
	MFCOL **p_mfcol = p_mftab->cols;
	i4 i=0;

	for ( ; i < p_mftab->numcols; i++, p_mfcol++)
	{
		if ( (*p_mfcol)->flags & COL_SEQUENCE)
		{
			/* primary master table has a sequenced field */

			*p_sfname = ( (*p_mfcol)->utility == NULL)
			    ? (*p_mfcol)->alias		  /* displayed field */
			    : (char *) (*p_mfcol)->utility    /* hidden field */
			    ;
/*			*p_sfname = (*p_mfcol)->alias; */

			*p_scname = (*p_mfcol)->name;

			*p_sfdisp = ((*p_mfcol)->flags & COL_USED ) ?
			    S_true : S_false;

			*p_sfused = ((*p_mfcol)->flags & 
						(COL_USED|COL_VARIABLE)) ?
			    S_true : S_false;

			return (TRUE);
		}
	}

	*p_sfname = S_null;
	*p_sfdisp = S_false;
	*p_sfused = S_false;

	return (FALSE);
}

/*{
** Name:  join_displayed - check if any master/detail join fields are displayed.
**
** Description:
**  Return TRUE if any of the join fields between the primary master and
**  detail tables have a join field that is displayed. Return FALSE if none
**  of the join fields are displayed. Note that only the master copy of
**  the join field(s) may be displayed (detail copy of join field(s) may
**  never be displayed, thus those aren't even checked here).
**
** Inputs:
**	MFTAB	*p_mftab	Table structure for primary MASTER table.
**
** Outputs:
**	bool	*join_used	Are any join columns used at all 
**				(field or variable)
**
**	Returns:
**		bool
**
** History:
**	10/3/89 (pete)	Initial version.
*/
static bool
join_displayed(p_mftab, join_used)
MFTAB *p_mftab;
bool  *join_used;
{
	MFCOL **p_mfcol = p_mftab->cols;
	i4 i=0;
	bool displayed = FALSE, used = FALSE;

	/* Loop thru master table and check if any columns are displayed
	** or variable-generating join fields.
	*/
	for ( ; i < p_mftab->numcols; i++, p_mfcol++)
	{
		if ( ((*p_mfcol)->flags & COL_DETJOIN) != 0)
		{
		    	if ( ((*p_mfcol)->flags & COL_USED) != 0)
		    	{
				/* at least one join field is displayed */
				displayed = used = TRUE;	
				/* That's all we need to know */
				break;	
			}
			else if ( ((*p_mfcol)->flags & COL_VARIABLE) != 0)
			{
				used = TRUE;
			}
		}
	}

	*join_used = used;
	return (displayed);
}
