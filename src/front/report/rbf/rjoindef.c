/* 
**    Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM= int_lnx int_rpl ibm_lnx
*/

# include    <compat.h>
# include    <cv.h>		/* 6-x_PC_80x86 */
# include    <me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include    <fe.h>
# include    <rcons.h>
# include    "rbftype.h"
# include    "rbfglob.h"
# include    "rbfcons.h"
# include    <cm.h>
# include    <st.h>
# include    <nm.h>
# include    <lo.h>
# include    <uf.h>
# include    <ug.h>
# include    <ui.h>
# include    <uigdata.h>
# include    <fedml.h>
# include    <qg.h>    
# include    <mqtypes.h>
# include    <oodefine.h>
# include    <errw.h>
# include    <rcons.h>
# include    <rglob.h>
# include    <gn.h>



/*
** Name:    rjoindef.c - Use a JoinDef as report data source.
**
** Description:
**      This file contains the modules that RBF utilizes to use a QBF 
**      JoinDef as the data source for a report.
**
** History:
**    07/27/89 (martym)    written for RBF.
**    11/27/89 (martym)    Rewrote 'r_JDMasterDetailSetUp'.
**    11/29/89 (martym)    Changed all calls to the sections list 
**			handling routines to follow the new interface.
**    12/14/89 (elein) replaced include of rbfstyle with rcons.
**    12/27/89 (elein) removed copy into Rbf_orname.
**    1/10/90 (martym) Modified almost all routines.
**    1/13/90 (martym) Changed some instances of FE_MAXNAME to FE_MAXTABNAME.
**    1/16/90 (martym) Add IIGN routines.
**    2/7/90 (martym)  Coding Standards cleanup.
**    2/15/90 (martym) Changed r_JDMaintTabList() to handle cases of multiple 
**			occurrences of the same table name in JoinDef's.
**    3/4/90 (martym) Changed r_JDLoadQuery() to prepare the query 
**			qualification and remainder for selection criteria 
**			modifications.
**	25-jul-90 (sylviap)
**		Added IIRF_adt_AddTable for editing joindef reports.  Need to
**		save information about which tables are master tables and
**		which tables are detail tables.
**	17-aug-90 (sylviap)
**		Centers the report title for master/detail reports. (#31446)
**	10-sep-90 (sylviap)
**		Fix for #33203.  Create the WHERE join clause correctly.
**	11-sep-90 (sylviap)
**		Added En_union. #32851.
**	25-sep-90 (sylviap)
**		Changed to generate OPEN SQL for a master/detail joindef query.
**		Generates "UNION SELECT ...' ' AS colname..." (#33338)
**	12-oct-90 (sylviap)
**		Added #defines to rplace ERx - make object code smaller.
**		Deciphers multiple comments in the query.  May have the
**		  UNION SELECT clause enclosed in comments.  See large comment
**		  block in r_JDLoadQuery for more info. (#32781)
**	08-nov-90 (sylviap)
**		Added code to parse through the comment block:
**			/* DO NOT MODIFY. DOING SO MAY CAUSE REPORT TO 
**			   BE UNUSABLE.
**		Assumes if comments are changed, then possible the code has been
**		also changed.  If the comments are modified, RBF does not edit
**		the report.  It's possible that the comments are NOT changed,
**		but the code IS changed, but RBF does it's best to check on the
**		code.
**    	21-nov-90 (sylviap)
**            	Checks for folding formats in the detail section. #34522, #34523
**    	14-feb-91 (sylviap)
**		Sets sec_brkord when making the break header for the 
**		(master/detail) join column. (#35926)
**	 4-jun-91 (steveh)
**		Fixed bug 37667 by correctly detecting absence of "union"
**		clause in report.
**	13-sep-91 (steveh)
**		Fixed bug 39792.  This problem was caused by an invalid MEfree
**		of union_clause.  TmpPtr2 is initially pointed to union_clause 
**		but is then modified as it is used to scan thru the string past 
**		the leading comment.  union_clause is then reset to TmpPtr2 
**		leaving it an unallocated pointer.
**		Also removed unused variables.
**	8-apr-92 (rdrane) 41871
**		Changed to generate OPEN SQL for the query itself, not just
**		for the UNION SELECT portion (cf. fix for 33338).  Load time
**		parsing of the joindef target list still recognizes the
**		"abc=tbl.col" form for compatibility.
**
**		The parsing of the target list in r_JDLoadQuery() had several
**		problems:
**		a) The collapsing of the target list string via STcopy was 
**		   clumsy and grossly inefficient.  The copy is no longer done;
**		   pointers are now used directly.
**		b) The incrementing of the target list string counter
**		   (CMbyteinc) was always being based upon the string's initial
**		   character, not its current character.
**		c) After finding a pertinent character and skipping over it,
**		   the logic would fall through and increment again.  This
**		   would work, since adjacent pertinent characters is not
**		   legal.  However, it was very confusing.
**		d) The actual DB column name was presented to
**		   IIUGlbo_lowerBEobject() only if it was the last one in the
**		   list.  This is now done for all DB column names.
**
**		The portion of the fix for 37667 which looks for the WIDTH
**		comment will be obsolete post-6.4.  The WIDTH comment was never
**		supposed to be stored in the DB in the first place!
**	31-aug-1992 (rdrane)
**		Prelim fixes for 6.5 - change over from IIUGlbo_lowerBEobject().
**	9-oct-1992 (rdrane)
**		Use new constants in (s/r/rbf)_reset invocations.
**		Replace En_relation with reference to global FE_RSLV_NAME
**		En_ferslv.name.
**	4-nov-1992 (rdrane)
**		Fix up IIUGbmaBadMemoryAllocation() ambiguities.
**		Re-work the usages of FE_MAXTABNAME, since it now reflects
**		un-normalized length.
**		Generate the JoinDef text with unnormalized owner, tablename,
**		correlation, and attribute names (r_JDSQLCreate).
**		Declare r_JDRvToTabName(), r_JDDefSort(), r_JDSQLCreate(),
**		r_JDAttToRvar(), r_JDIsMasTab(), r_JDCreateAttrList(),
**		and IIRF_adt_AddTbl() as static to this module.
**		Declare IIRFckc_CheckComment() as FUNC_EXTERN - it is NOT
**		defined in this module.
**		Modify r_JDRvToTabName() to return a pointer to the RvTabName
**		structure, since we now have to deal with the owner as well,
**		and would rather not limit ourselves to forcing return of
**		the un-resolved tablename.
**		r_JDMaintTabList() now accepts three char * arguments to
**		support setting owner in the TBL structure array.
**	8-dec-1992 (rdrane)
**		Effect conversion of En_tar_list for old-style "c=a.b" lists
**		if we're connected to a 6.5 or above database.  Ensure that
**		temporary dynamically allocated memory is freed before return.
**		Use RBF specific global to determine if delimited identifiers
**		are enabled.  If not pre-6.5, ensure delimited identifiers
**		are enabled.
**	21-jan-1993 (rdrane)
**		Since we use FE_decompose directly, ensure that the name and
**		and owner buffers will handle an unnormalized identifier!
**		Note the implication that they will normalize to a length of
**		DB_GW1_MAXNAME or less.
**	23-jul-1993 (rdrane)
**		Always compare to UI_FE_CAT_ID_65 as case insensitive to
**		support FIPS.
**	20-sep-1993 (rdrane)
**		RelList must point to unnormalized owner.tablename references
**		since that is what rFchk_dat() and rf_att_dflt() expect.
**		This addresses bug 54949.  Also, ensure that the FROM clause
**		is generated with the unnormalized table name - it was only
**		being unnormalized if 6.5 and an explicit owner specification
**		was required!  Note the underlying assumption that all internal
**		data structures (JD_ATTR, RV_TAB, TBL, MQ*, etc.) contain the
**		fully normalized identifier.
**		Use NO_TBL_TAG since tags are really unsigned.  Cast NULL
**		pointer values apropriately.
**	4-jan-1994 (rdrane)
**		Use OpenSQL level to determine 6.5 syntax features support,
**		and check dlm_Case capability directly to determine if
**		delimited identifiers are suppported.
**	29-mar-1996 (schte01)
**		Added NO_OPTIM for axp_osf to get around problem where
**		a master/detail report on a joindef generates a tabular
**		report.
**	01-jul-1998 (toumi01)
**		Added lnx_us5 to NO_OPTIM list to get around problem where
**		a master/detail report comes out tabular (as on axp_osf).
**	15-jan-1999 (schte01)
**		Removed NO_OPTIM for axp_osf to see if problem goes away with
**        new DEC c compiler.
**	06-oct-1999 (toumi01)
**		Change Linux config string from lnx_us5 to int_lnx.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-aug-2000 (somsa01)
**		Added ibm_lnx to NO_OPTIM list to get around problem where
**		a master/detail report comes out tabular (as on axp_osf).
**      13-feb-03 (chash01) x-integrate change#461908
**          in r_JDAttToRvar()
**          local character array rvar[] can not be returned as value.  Make 
**          the return variable static so that caller mwy reference it 
**          properly under any circumstance.
**	22-Jun-2005 (bonro01)
**	    rbf was incorrectly parsing the comment records from reports
**	    and allowing reports to be editted that should not have been.
**	    The parsing was incorrectly running past the end of the comment
**	    string and reading unallocated storage. This resulted in random
**	    errors, or incorrectly identifying that a report can be editted.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	18-Aug-2009 (drivi01)
**	    Cleanup precedence warning in effort to port to 
**	    Visual Studio 2008.
**	26-Aug-2009 (kschendel) b121804
**	    Bool prototypes to keep gcc 4.3 happy.
**	24-Feb-2010 (frima01) Bug 122490
**	    Moved rFreset declaration to rglob.h as neccessary
**	    to eliminate gcc 4.3 warnings.
**	
*/


/*
** # define's:
*/
#define JD_MASTER_DETAIL      ERx("MD")
#define JD_MASTER_MASTER      ERx("MM")
#define JD_DETAIL_MASTER      ERx("DM")
#define JD_DETAIL_DETAIL      ERx("DD")
#define JD_FIELD_NOT_SHOWN    0
#define JD_LNK_NULL           (-1)
#define W_UNION	              ERx("union select ")
#define W_N	              ERx("n")
#define W_Y	              ERx("y")
#define W_SPACE	      	      ERx(" ")
#define W_COMMA 	      ERx(",")
#define W_PERIOD	      ERx(".")
#define W_EQUAL 	      ERx("=")
#define C_EQUAL 	      '='
#define W_AS		      ERx("as")
#define W_SP_AS		      ERx(" as ")
#define W_SP_EQUAL 	      ERx(" = ")
#define W_AND                 ERx("and")
#define W_SP_AND              ERx(" and ")
#define W_SP_WHERE            ERx(" where ")
#define W_SP_FROM             ERx(" from ")
#define W_ASTERISK            ERx("*")
#define W_O_PAREN             ERx("(")
#define W_C_PAREN             ERx(")")
#define W_WIDTH		      ERx("width ")


typedef struct _jd_attr
{
    char attr [FE_MAXNAME + 1];	     /* Name of attribute internal to RBF    */
    char rvar [FE_MAXNAME + 1];	     /* Name of range variable in JoinDef    */
    char field [FE_MAXNAME + 1];     /* Name of attribute in database        */
    i4  OccurCnt;		     /* Number of occurrences of field       */
    bool used;			     /* Accounting flag to be used by cleint */
    struct _jd_attr *link;           /* Link to the next element in list     */
} JD_ATTR;


typedef struct _rvar_tab
{
    char rvar [FE_MAXNAME + 1];	     /* Name of range variable in JoinDef    */
    char tab [FE_MAXNAME + 1];	     /* Name of table in JoinDef             */
    char own [FE_MAXNAME + 1];	     /* Name of owner of table in JoinDef    */
    bool counted;                    /* Accounting flag to be used by cleint */
    i4   link;			     /* Link to the next occurrence in list  */
} RV_TAB;

	


/* 
** GLOBALDEF's 
*/



/* 
** Statics:
*/
static MQQDEF *JoinDef = NULL;		/* The JoinDef                        */
static char   *MasterRvars [MQ_MAXRELS];/* A list of MASTER tables Range Vars */
static i4      NumMasters = 0;		/* Number of MASTER tables in JoinDef */
static bool    IsMasterDetail = FALSE;  /* If true current JD is Master/Detail*/
static i4      NumRels = 0;		/* Number of relations in JoinDef     */

# define NO_TBL_TAG	0
static TAGID   Tbl_tag = NO_TBL_TAG;            /* TBL tag id */

static RV_TAB  RvTabName [MQ_MAXRELS];

static VOID r_JDDefSort();
static VOID r_JDSQLCreate();
static RV_TAB *r_JDRvToTabName();
static char *r_JDAttToRvar();
static bool r_JDIsMasTab();
static VOID r_JDCreateAttrList();
static VOID IIRF_adt_AddTbl();

i4  cnt = 0;
i4  ins = 0;


/*
** Functions declared in this file and referenced by
** other modules:
*/
char *r_JDTabNameToRv();
bool r_JDMaintAttrList();
bool r_JDLoadQuery();
bool r_JDMasterDetailSetUp();
bool r_JDMaintTabList();
bool IIRF_mas_MasterTbl();


/*
** Imports:
*/
FUNC_EXTERN bool r_InitFrameInfo();
FUNC_EXTERN bool r_InsertSection();
FUNC_EXTERN bool r_MoveTrimField();
FUNC_EXTERN bool r_PropagateYMods();
FUNC_EXTERN bool r_ChkMaxX();
FUNC_EXTERN bool r_SemiColLineUp();
FUNC_EXTERN VOID rfNextQueryLine();
FUNC_EXTERN bool IIRFckc_CheckComment();
FUNC_EXTERN bool vfmqbf(char *, bool, MQQDEF **, bool, FRAME **);



/*
** Name:    r_JDLoad - Read in a JoinDef.
**
** Description:
**         Reads in a JoinDef into this module's address space,
**         and calculates the master detail table names. Returns 
**         TRUE if the JoinDef is successfully read in and FALSE
**         otherwise.
**
**	char *JDName;        the JoinDef name passed in
**	bool *JDMasDet;      set to TRUE if JoinDef is master/detail
**
**/
bool r_JDLoad(char *JDName, bool *JDMasDet)
{

    	i4 i;
	bool found = FALSE;


    	if (JoinDef != NULL)
        	_VOID_ MEfree((PTR) JoinDef);

    	if (!vfmqbf(JDName, TRUE, &JoinDef, FALSE, NULL))
        	return(FALSE);

	/*
	** Clean up statics:
	*/
    	NumMasters = 0;
    	IsMasterDetail = FALSE;

	/*
	** Build the relation/range-variable list:
	*/
	_VOID_ r_JDMaintTabList(JD_TAB_INIT, NULL, NULL, NULL, NULL);
    	for (i = 0; i < JoinDef->mqnumtabs; i++)
		if (!r_JDMaintTabList(JD_TAB_ADD, 
				JoinDef->mqtabs [i]->mqrangevar,
				JoinDef->mqtabs [i]->mqrelid,
				JoinDef->mqtabs [i]->mqrelown))
			return(FALSE);

    	/*
    	** If we have got a master/detail JD, record the master table name:
    	*/
    	i = 0;
	found = FALSE;
    	while (i < JoinDef->mqnumjoins && !found)
    	{
        	if (STcompare(JoinDef->mqjoins [i]->jcode, JD_MASTER_DETAIL) ==
				0)
        	{
            		MasterRvars[NumMasters ++] = JoinDef->mqjoins[i]->rv1;
			found = IsMasterDetail = TRUE;
        	}
        	else
        	if (STcompare(JoinDef->mqjoins [i]->jcode, JD_DETAIL_MASTER) ==
				0)
        	{
            		MasterRvars[NumMasters ++] = JoinDef->mqjoins[i]->rv2;
			found = IsMasterDetail = TRUE;
        	}
        	else
            	i ++;
    	}

	/*
	** If we have got a master/detail look for any master/master's and 
	** register the non-recorded master table name:
	*/
	if (IsMasterDetail)
 	    for (i = 0; i < JoinDef->mqnumjoins; i ++)
		if (STcompare(JoinDef->mqjoins [i]->jcode, JD_MASTER_MASTER) ==
				0)
		{
			if (!r_JDIsMasTab(JoinDef->mqjoins[i]->rv1))
				MasterRvars [NumMasters ++] = 
					JoinDef->mqjoins[i]->rv1;

			if (!r_JDIsMasTab(JoinDef->mqjoins[i]->rv2))
				MasterRvars [NumMasters ++] = 
					JoinDef->mqjoins[i]->rv2;
		}

    	*JDMasDet = IsMasterDetail;

	 /* Free TBL linked list - used for joindefs */
	if (Tbl_tag != NO_TBL_TAG)
	{
		FEfree(Tbl_tag);
		Tbl_tag = NO_TBL_TAG;
		Ptr_tbl = NULL;
	}

	/* Fill the TBL linked list of the tables in the joindef */
	for (i = 0; i < JoinDef->mqnumtabs; i ++)
	{
		/* Add table to the TBL linked list */
		IIRF_adt_AddTbl (JoinDef->mqtabs[i]->mqrelid, 
			JoinDef->mqtabs[i]->mqrelown,
			JoinDef->mqtabs[i]->mqrangevar, 
			r_JDIsMasTab(JoinDef->mqtabs[i]->mqrangevar));
	}

    	return(TRUE);

}



/*
** Name:    r_JDrFget - Load the report specification information.
**
** Description:
**      Loads the report specification information into the report 
**      formatter data structures and then frame structures, for a 
**      given JoinDef. Also generates single SQL query to express 
**	the JoinDef.
**         
**	char *JDName;			The name of the JoinDef
**	i2 style;			The report style
**
**/
bool r_JDrFget(char *JDName, i2 style)
{

    i4  i;
    static char *RelList[MQ_MAXRELS];
    static TAGID RelListTag;


    /* 
    ** Reset global variables:
    */
    r_reset(RP_RESET_RELMEM4,RP_RESET_RELMEM5,RP_RESET_OUTPUT_FILE,
		RP_RESET_CLEANUP,RP_RESET_LIST_END);
    s_reset(RP_RESET_SRC_FILE,RP_RESET_REPORT,RP_RESET_LIST_END);
    rFreset(RP_RESET_REPORT,RP_RESET_LIST_END);


    _VOID_ r_JDMaintAttrList(JD_ATT_LIST_FLUSH, NULL, NULL, NULL);

    /*
    ** Start the GN routines:
    */
    if (IIGNssStartSpace(1, ERx(""), FE_MAXNAME, 
		FE_MAXNAME, GN_MIXED, GNA_ACCEPT, (VOID (*)())NULL) != OK)
    {
	return(FALSE);
    }

    if ((En_ferslv.name = (char *)FEreqmem ((u_i4)Rst4_tag,
    (u_i4)(FE_MAXTABNAME), TRUE, (STATUS *)NULL)) == NULL)
    {
	IIUGbmaBadMemoryAllocation(ERx("r_JDrFget - name"));
    }

    if ((En_ferslv.name_dest = (char *)FEreqmem ((u_i4)Rst4_tag,
    (u_i4)(FE_MAXNAME+1), TRUE, (STATUS *)NULL)) == NULL)
    {
	IIUGbmaBadMemoryAllocation(ERx("r_JDrFget - name_dest"));
    }

    if ((En_ferslv.owner_dest = (char *)FEreqmem ((u_i4)Rst4_tag,
    (u_i4)(FE_MAXNAME+1), TRUE, (STATUS *)NULL)) == NULL)
    {
	IIUGbmaBadMemoryAllocation(ERx("r_JDrFget - owner_dest"));
    }

    _VOID_ STcopy(JDName, En_ferslv.name);


    En_report = JDName;
    _VOID_ CVlower(En_report);

    St_style = style;
    Rbf_ortype = OT_JOINDEF;
    En_rtype = RT_DEFAULT;


    _VOID_ r_JDCreateAttrList();


    /*
    ** Make a list of and check for valid data relations:
    */
    RelListTag = MEgettag();
    for (i = 0; i < JoinDef->mqnumtabs; i ++)
    {
        if  ((RelList[i] = (char *)MEreqmem(RelListTag,
                (u_i4)(FE_MAXTABNAME),TRUE,(STATUS *)NULL)) == NULL)
        {
                IIUGbmaBadMemoryAllocation(ERx("rjoindef - RelList"));
        }
	if  (!IIUIxrt_tbl(&(JoinDef->mqtabs[i]->mqrelown[0]),
			 &(JoinDef->mqtabs[i]->mqrelid[0]),
			 RelList[i]))
	{
		IIUGtagFree(RelListTag);
		return(FALSE);
	}
    }


    if(!rFchk_dat(&(JoinDef->mqnumtabs), RelList))        
    {
	IIUGtagFree(RelListTag);
        return(FALSE);
    }


    /*
    ** Set up the ATT structures for each attribute
    ** in the data relation:
    */
    _VOID_ rf_att_dflt(JoinDef->mqnumtabs, RelList);
    IIUGtagFree(RelListTag);

    if (En_rtype == RT_DEFAULT)
    {    
        /* 
        ** set up the default report: 
        */
        if (!St_silent)
        {
            IIUGmsg(ERget(S_RF002C_Setting_up_default_re), (bool) FALSE, 0);
        }

        /* 
        ** Set up the default sort columns, for the JoinDef:
        */
        _VOID_ r_JDDefSort();
    }


    /*
    ** Set up the BRK data structures for each sort
    ** attribute with a RACTION tuple defined for it.
    */
    r_brk_set();        


    /*
    ** Set up the CDAT and DTPL variables for SQL
    ** to read the data relation(s).
    */
    r_JDSQLCreate();

    if (En_rtype == RT_DEFAULT)
    {

	/*
	** Since the ReportWriter does not support any of RBF's template 
	** styles, if  'St_style'  is set to a template style set it to 
	** a generic ReportWriter report type before calling the Report-
	** Writer, and then set it back:
	*/
	i2 SaveStyle = St_style;
	if (St_style == RS_TABULAR)
	    St_style = RS_COLUMN;
	else
        if (St_style == RS_INDENTED || St_style == RS_MASTER_DETAIL)
            St_style = RS_BLOCK;

        r_m_rprt();
	St_style = SaveStyle;
    }
    else
    {
        /*    
        ** Set up the TCMD data structures:
        */
        Tcmd_tag = FEbegintag();
        r_tcmd_set();
        FEendtag();
    }



    /* 
    ** set up the CLR and OP structures for
    ** fast processing of aggregates:
    */
    r_lnk_agg();    


    if (Err_count > 0)
    {   /* 
        ** No errors allowed in report:     
        */
        return(FALSE);
    }

    return(TRUE);

}







/*
**  Name:  r_JDDefSort
**
**  Description:
**      Sets up the default sort list for a joindef.
**
*/
static
VOID
r_JDDefSort()
{
    
    i4  i;
    ATTRIB ordinal;
    i4  SrtOrd = 0;


    /*
    ** Allocate storage for sort data:
    */
    En_scount = JoinDef->mqnumatts;
    if ((Ptr_sort_top = (SORT *)FEreqmem((u_i4)Rst4_tag,
            (u_i4)(((En_scount) * sizeof(SORT))), 
            TRUE, (STATUS *)NULL)) == NULL)
    {
        IIUGbmaBadMemoryAllocation(ERx("r_JDDefSort - Ptr_sort_top"));
    }

    for (i = 0; i < JoinDef->mqnumjoins; i++)
    {
        /*
        ** We only want to sort master columns of Master/Detail -or-
        ** Detail/Master JoinDefs:
        */
        if (STcompare(JoinDef->mqjoins[i]->jcode, JD_MASTER_DETAIL) == 0)
        {
            ordinal = r_mtch_att(JoinDef->mqjoins[i]->col1);
            if (STcompare(JoinDef->mqjoins[i]->col1, NAM_DETAIL) == 0 ||
                    STcompare(JoinDef->mqjoins[i]->col1, NAM_PAGE) == 0 ||
                    STcompare(JoinDef->mqjoins[i]->col1, NAM_REPORT) == 0)
                r_m_sort(SrtOrd+1, ordinal, W_N, O_ASCEND);
            else
                r_m_sort(SrtOrd+1, ordinal, W_Y, O_ASCEND);
            SrtOrd ++;
        }
        else
        if (STcompare(JoinDef->mqjoins[i]->jcode, JD_DETAIL_MASTER) == 0)
        {
            ordinal = r_mtch_att(JoinDef->mqjoins[i]->col2);
            if (STcompare(JoinDef->mqjoins[i]->col2, NAM_DETAIL) == 0 ||
                    STcompare(JoinDef->mqjoins[i]->col2, NAM_PAGE) == 0 ||
                    STcompare(JoinDef->mqjoins[i]->col2, NAM_REPORT) == 0)
                r_m_sort(SrtOrd+1, ordinal, W_N, O_ASCEND);
            else
                r_m_sort(SrtOrd+1, ordinal, W_Y, O_ASCEND);
            SrtOrd ++;
        }
    }

    if (SrtOrd == 0)
    {
	char *att_name;

	att_name = r_gt_att(1)->att_name;
	if (STcompare(att_name,NAM_DETAIL) == 0 ||
		STcompare(att_name,NAM_PAGE) == 0 ||
		STcompare(att_name,NAM_REPORT) == 0)
    	    r_m_sort(1, 1, W_N, O_ASCEND);
	else
    	    r_m_sort(1, 1, W_Y, O_ASCEND);
        SrtOrd ++;
    }

    En_scount = SrtOrd;

    if (!r_srt_set())    
        /*
        ** Bad sort detected:
        */
        r_error(0x0C, FATAL, NULL);

    return;

}



/*
**  Name:  r_JDSQLCreate
**
**  Description:
**      Generate a select and a union subselect to represent the JoinDef.
**      Store the query components in the global vars.
**
**	Note that the table name is NOT generated as owner.tablename if
**	o The connection is to a pre-6.5 database.
**	o The table owner is the catalog owner.
**	o The table owner is the invoker.
**	o The table owner is the DBA and the invoker does NOT own a table
**	  of the same name (this implies that the name was specified as
**	  owner.tablename).
**
**	If not pre-6.5, ensure that delimited identifier recognition
**	is enabled.
**
**  Side Effect:  
**        The following global variables will be reset:
**            En_sql_from
**            En_pre_tar
**            En_tar_list
**            En_q_qual
**            En_union
**  History:
**	28-sep-1993 (rdrane)
**		Don't forget to unnormalize table names in the UNION
**		clause when they don't need an explicit owner.  This
**		addresses bug 54949.
**	17-mar-1994 (rdrane)
**		Correct sense of compare when determining Rbf_/St_xns_given
**		setting (b58671,b60626).
*/
static
VOID
r_JDSQLCreate()
{

    i4  i;
    i4  len;
    i4  master_joins = 0;	/* counts the number of master joins */
    i4  detail_joins = 0;	/* counts the number of detail joins */
    char *master_clause;
    char *detail_clause;
    RV_TAB *rvar_tab_ptr;
    FE_RSLV_NAME rjc_ferslv;
    bool incl_owner[MQ_MAXRELS];
    char ColName [FE_MAXNAME + 1];
    char tmp_tbl_buf[FE_MAXTABNAME];
    char tmp_col_buf[(FE_UNRML_MAXNAME + 1)];


    if  (IIUIdlmcase() != UI_UNDEFCASE)
    {
	Rbf_xns_given = TRUE;
	St_xns_given = TRUE;
    }
    rjc_ferslv.name_dest = NULL;
    rjc_ferslv.owner_dest = NULL;

    if (En_sql_from != NULL)
    {
        _VOID_ MEfree((PTR) En_sql_from);
        En_sql_from = NULL;
    }

    if (En_pre_tar != NULL)
    {
        _VOID_ MEfree((PTR) En_pre_tar);
        En_pre_tar = NULL;
    }

    if (En_tar_list != NULL)
    {
        _VOID_ MEfree((PTR) En_tar_list);
        En_tar_list = NULL;
    }

    if (En_q_qual != NULL)
    {
        _VOID_ MEfree((PTR) En_q_qual);
        En_q_qual = NULL;
    }

    if (En_union != NULL)
    {
        _VOID_ MEfree((PTR) En_union);
        En_union = NULL;
    }


    /* 
    ** Alloc storage for and fill the FROM STMT:
    */
    len = 0;
    for (i = 0; i < JoinDef->mqnumtabs; i ++)
    {
	if  ((STcompare(IIUIcsl_CommonSQLLevel(),UI_LEVEL_65) < 0) ||
	     (STbcompare(JoinDef->mqtabs[i]->mqrelown,0,
			 UI_FE_CAT_ID_65,0,TRUE) == 0) ||
	     (STequal(JoinDef->mqtabs[i]->mqrelown,IIuiUser)) ||
	     ((STequal(JoinDef->mqtabs[i]->mqrelown,IIuiDBA)) &&
	      (!FE_resolve(&rjc_ferslv,JoinDef->mqtabs[i]->mqrelid,
			   IIuiUser))))
	{
	    _VOID_ IIUGxri_id(JoinDef->mqtabs[i]->mqrelid,&tmp_tbl_buf[0]);
	    incl_owner[i] = FALSE;
	}
	else
	{
	    _VOID_ IIUIxrt_tbl(JoinDef->mqtabs[i]->mqrelown,
				JoinDef->mqtabs[i]->mqrelid,
				&tmp_tbl_buf[0]);
	    incl_owner[i] = TRUE;
	}
        len += STlength(&tmp_tbl_buf[0]);
        len += STlength(W_SPACE);
        _VOID_ IIUGxri_id(JoinDef->mqtabs [i]->mqrangevar,
			  &tmp_tbl_buf[0]);
        len += STlength(&tmp_tbl_buf[0]);
        len += STlength(W_COMMA);
    }
    if ((En_sql_from = (char *)MEreqmem((u_i4)0, 
            (u_i4) (len+1),      /* length + terminating 0*/
            TRUE,
            (STATUS *)NULL)) == NULL)
    {
        IIUGbmaBadMemoryAllocation(ERx("r_JDSQLCreate - En_sql_from"));
    }
    En_sql_from [0] = EOS;

    for (i = 0; i < JoinDef->mqnumtabs; i ++)
    {
	if  (incl_owner[i])
	{
	    _VOID_ IIUIxrt_tbl(JoinDef->mqtabs[i]->mqrelown,
				JoinDef->mqtabs[i]->mqrelid,
				&tmp_tbl_buf[0]);
	}
	else
	{
	    _VOID_ IIUGxri_id(JoinDef->mqtabs[i]->mqrelid,&tmp_tbl_buf[0]);
	}
	_VOID_ STcat(En_sql_from, &tmp_tbl_buf[0]);
        _VOID_ STcat(En_sql_from, W_SPACE);
        _VOID_ IIUGxri_id(JoinDef->mqtabs [i]->mqrangevar,
			  &tmp_tbl_buf[0]);
        _VOID_ STcat(En_sql_from, &tmp_tbl_buf[0]);
        _VOID_ STcat(En_sql_from, W_COMMA);
    }
    if (STlength(En_sql_from) >= STlength(W_COMMA))
    {
        En_sql_from [STlength(En_sql_from) - STlength(W_COMMA)] = EOS;
    }


    /* 
    ** No pre TARGET STMT is needed:
    */


    /* 
    ** Alloc storage for and fill the TARGET LIST:
    */
    if ((En_tar_list = (char *)MEreqmem((u_i4)0, 
	(u_i4)((STlength(ERx(".,")) * JoinDef->mqnumatts) +  
		(STlength(W_SP_AS) * JoinDef->mqnumatts) +  
		(FE_UNRML_MAXNAME * JoinDef->mqnumatts * 3) +
		1),
		TRUE,
		(STATUS *)NULL)) == NULL)
    {
        IIUGbmaBadMemoryAllocation(ERx("r_JDSQLCreate - En_tar_list"));
    }

    _VOID_ r_JDMaintAttrList(JD_ATT_INIT_USE_FLAGS, NULL, NULL, NULL);

    /*
    ** Set up the master table's fields first:
    */
    for (i = 0; i < JoinDef->mqnumatts; i++)
    {
        _VOID_ IIUGxri_id(JoinDef->mqatts[i]->rvar,&tmp_tbl_buf[0]);
        if (r_JDIsMasTab(JoinDef->mqatts[i]->rvar))
	{
		if (r_JDMaintAttrList(JD_ATT_GET_FIELD_CNT,
		    JoinDef->mqatts[i]->rvar, JoinDef->mqatts[i]->col, &cnt))
		{
           		_VOID_ STcat(En_tar_list, &tmp_tbl_buf[0]);
           		_VOID_ STcat(En_tar_list, W_PERIOD);
        		_VOID_ IIUGxri_id(JoinDef->mqatts[i]->col,
					  &tmp_col_buf[0]);
           		_VOID_ STcat(En_tar_list, &tmp_col_buf[0]);
			if (cnt > 1)
			{
           			_VOID_ STcat(En_tar_list, W_SP_AS);
				_VOID_ r_JDMaintAttrList(JD_ATT_GET_ATTR_NAME,
					JoinDef->mqatts[i]->rvar, 
					JoinDef->mqatts[i]->col, &ColName [0]);
           			_VOID_ STcat(En_tar_list, ColName);
			}
       			_VOID_ STcat(En_tar_list, W_COMMA);
           		_VOID_ r_JDMaintAttrList(JD_ATT_SET_USE_FLAG, 
					JoinDef->mqatts[i]->rvar,
                       			JoinDef->mqatts[i]->col, NULL);
		}
	}
    }

    /*
    ** Set up the rest of the fields:
    */
    for (i = 0; i < JoinDef->mqnumatts; i++)
    {
        _VOID_ IIUGxri_id(JoinDef->mqatts[i]->rvar,&tmp_tbl_buf[0]);
        if (r_JDMaintAttrList(JD_ATT_GET_FIELD_CNT, 
		JoinDef->mqatts[i]->rvar, JoinDef->mqatts[i]->col, &cnt))
        {
           	_VOID_ STcat(En_tar_list, &tmp_tbl_buf[0]);
           	_VOID_ STcat(En_tar_list, W_PERIOD);
       		_VOID_ IIUGxri_id(JoinDef->mqatts[i]->col,&tmp_col_buf[0]);
           	_VOID_ STcat(En_tar_list, &tmp_col_buf[0]);
		if (cnt > 1)
		{
           		_VOID_ STcat(En_tar_list, W_SP_AS);
			_VOID_ r_JDMaintAttrList(JD_ATT_GET_ATTR_NAME,
					JoinDef->mqatts[i]->rvar, 
					JoinDef->mqatts[i]->col, &ColName [0]);
           		_VOID_ STcat(En_tar_list, ColName);
		}
           	_VOID_ STcat(En_tar_list, W_COMMA);
           	_VOID_ r_JDMaintAttrList(JD_ATT_SET_USE_FLAG, 
				JoinDef->mqatts[i]->rvar,
                       		JoinDef->mqatts[i]->col, NULL);
        }
    }

    if (STlength(En_tar_list) >= STlength(W_COMMA))
    {
        En_tar_list [STlength(En_tar_list) - STlength(W_COMMA)] = EOS;
    }


    /* 
    ** Alloc storage for and fill the QUERY QUALIFICATION:
    */

    /* First count the different types of joins */

    for (i = 0; i < JoinDef->mqnumjoins; i++)
    {

	if (STindex(JoinDef->mqjoins [i]->jcode, ERx("D"), 0) == NULL)
	   master_joins ++;
	else
	   detail_joins ++;
    }

    /* 
    ** Check if there are any joins. It's possible to have a joindef that
    ** is one table w/no joins.  In this case, there is no WHERE clause.
    */
    if (master_joins + detail_joins != 0)
    {
    	if ((master_clause = (char *)MEreqmem((u_i4)0, 
            (u_i4) ((STlength(W_PERIOD) * master_joins * 2) +  
                     (STlength(W_SP_EQUAL) * master_joins) +
                     (STlength(W_SP_AND) * master_joins) +
                     (FE_UNRML_MAXNAME * master_joins * 4) +
                     1),
            TRUE,
            (STATUS *)NULL)) == NULL)
    	{
            IIUGbmaBadMemoryAllocation(ERx("r_JDSQLCreate - master clause"));
    	}

    	if ((detail_clause = (char *)MEreqmem((u_i4)0, 
            (u_i4) ((STlength(W_PERIOD) * detail_joins * 2) +  
                     (STlength(W_SP_EQUAL) * detail_joins) +
                     (STlength(W_SP_AND) * detail_joins) +
                     (FE_UNRML_MAXNAME * detail_joins * 4) +
                     1),
            TRUE,
            (STATUS *)NULL)) == NULL)
    	{
            IIUGbmaBadMemoryAllocation(ERx("r_JDSQLCreate - detail clause"));
    	}

	if ((En_q_qual = (char *)MEreqmem((u_i4)0,
	    (u_i4) ((STlength(W_PERIOD) * JoinDef->mqnumjoins * 2) +
		(STlength(W_SP_EQUAL) * JoinDef->mqnumjoins) +
		(STlength(W_SP_AND) * JoinDef->mqnumjoins) +
		(FE_UNRML_MAXNAME * JoinDef->mqnumjoins * 4) +
		1),
	     TRUE,
	     (STATUS *)NULL)) == NULL)
	{
	     IIUGbmaBadMemoryAllocation(ERx("r_JDSQLCreate - En_q_qual"));
	}

    	for (i = 0; i < JoinDef->mqnumjoins; i++)
    	{
	    if (STindex(JoinDef->mqjoins [i]->jcode, ERx("D"),0) == NULL)
	    {
		/* Build master join clause */
        	_VOID_ IIUGxri_id(JoinDef->mqjoins[i]->rv1,&tmp_col_buf[0]);
                _VOID_ STcat(master_clause, &tmp_col_buf[0]);
                _VOID_ STcat(master_clause, W_PERIOD);
        	_VOID_ IIUGxri_id(JoinDef->mqjoins[i]->col1,&tmp_col_buf[0]);
                _VOID_ STcat(master_clause, &tmp_col_buf[0]);
                _VOID_ STcat(master_clause, W_SP_EQUAL);
        	_VOID_ IIUGxri_id(JoinDef->mqjoins[i]->rv2,&tmp_col_buf[0]);
                _VOID_ STcat(master_clause, &tmp_col_buf[0]);
                _VOID_ STcat(master_clause, W_PERIOD);
        	_VOID_ IIUGxri_id(JoinDef->mqjoins[i]->col2,&tmp_col_buf[0]);
                _VOID_ STcat(master_clause, &tmp_col_buf[0]);
                _VOID_ STcat(master_clause, W_SP_AND);

	    }
	    else
	    {
		/* Build detail join clause */
        	_VOID_ IIUGxri_id(JoinDef->mqjoins[i]->rv1,&tmp_col_buf[0]);
                _VOID_ STcat(detail_clause, &tmp_col_buf[0]);
                _VOID_ STcat(detail_clause, W_PERIOD);
        	_VOID_ IIUGxri_id(JoinDef->mqjoins[i]->col1,&tmp_col_buf[0]);
                _VOID_ STcat(detail_clause, &tmp_col_buf[0]);
                _VOID_ STcat(detail_clause, W_SP_EQUAL);
        	_VOID_ IIUGxri_id(JoinDef->mqjoins[i]->rv2,&tmp_col_buf[0]);
                _VOID_ STcat(detail_clause, &tmp_col_buf[0]);
                _VOID_ STcat(detail_clause, W_PERIOD);
        	_VOID_ IIUGxri_id(JoinDef->mqjoins[i]->col2,&tmp_col_buf[0]);
                _VOID_ STcat(detail_clause, &tmp_col_buf[0]);
                _VOID_ STcat(detail_clause, W_SP_AND);
	    }
    	}
	/* Move STcat outside the for loop. #33203 */
        _VOID_ STcat(En_q_qual, master_clause);
        _VOID_ STcat(En_q_qual, detail_clause);

	/* Get rid of the trailing 'AND' */

    	if (STlength(master_clause) >= STlength(W_SP_AND))
	{
        	master_clause 
		   [STlength(master_clause) - STlength(W_SP_AND)] = EOS;
	}
    	if (STlength(detail_clause) >= STlength(W_SP_AND))
	{
        	detail_clause 
		   [STlength(detail_clause) - STlength(W_SP_AND)] = EOS;
	}
    	if (STlength(En_q_qual) >= STlength(W_SP_AND))
	{
        	En_q_qual [STlength(En_q_qual) - STlength(W_SP_AND)] = EOS;
	}

        /*
        ** Stick a blank space at the end, just in case if we have to add 
        ** additional qualifications later to handle selection criteria:
        */
        _VOID_ STcat(detail_clause, W_SPACE);
        _VOID_ STcat(En_q_qual, W_SPACE);
    }

    /*
    ** If we don't have a master/detail JD "En_union" doesn't 
    ** need to be filled out:
    */
    if (!IsMasterDetail)
        return;

    /* 
    ** Alloc storage for and fill the REMAINDER of the query:
    */
    if ((En_union = (char *)MEreqmem((u_i4)0, 
            (u_i4) (STlength(W_UNION) +  
                    (STlength(ERx("' ' AS ., ")) * JoinDef->mqnumatts) +  
                    (FE_UNRML_MAXNAME * 2 * JoinDef->mqnumatts) +
                    (STlength(W_SP_FROM)) + 
                    (FE_MAXTABNAME * 2) +
                    (STlength(W_SP_WHERE)) +
                    (STlength(master_clause)) +
                    (STlength(ERx(" and not exists (select * from "))) +
                    (FE_MAXTABNAME * JoinDef->mqnumtabs * 2) +
                    (STlength(ERx(" , ")) * JoinDef->mqnumtabs) +  
                    (STlength(W_SP_WHERE)) +
                    (STlength(W_PERIOD) * JoinDef->mqnumjoins * 2) +  
                    (STlength(ERx(" = AND ")) * JoinDef->mqnumjoins) +
                    (FE_MAXTABNAME * JoinDef->mqnumjoins * 2) +
                    (FE_UNRML_MAXNAME * JoinDef->mqnumjoins * 2) +
                    (STlength(W_C_PAREN)) +  
                    1),
            TRUE,
            (STATUS *)NULL)) == NULL)
    {
        IIUGbmaBadMemoryAllocation(ERx("r_JDSQLCreate - En_union"));
    }

    _VOID_ STcopy(W_UNION, En_union);

    _VOID_ r_JDMaintAttrList(JD_ATT_INIT_USE_FLAGS, NULL, NULL, NULL);

    /*
    ** Set up the master table's fields first:
    */
    for (i = 0; i < JoinDef->mqnumatts; i++)
    {
       	_VOID_ IIUGxri_id(JoinDef->mqatts[i]->rvar,&tmp_tbl_buf[0]);
        if (r_JDIsMasTab(JoinDef->mqatts[i]->rvar))
	{
        	if (r_JDMaintAttrList(JD_ATT_GET_FIELD_CNT, 
			JoinDef->mqatts [i]->rvar, JoinDef->mqatts [i]->col, 
			&cnt))
        	{
            		_VOID_ STcat(En_union, &tmp_tbl_buf[0]);
            		_VOID_ STcat(En_union, W_PERIOD);
        		_VOID_ IIUGxri_id(JoinDef->mqatts[i]->col,
					  &tmp_col_buf[0]);
                	_VOID_ STcat(En_union, &tmp_col_buf[0]);
            		_VOID_ STcat(En_union, W_COMMA);
           		_VOID_ r_JDMaintAttrList(JD_ATT_SET_USE_FLAG, 
					JoinDef->mqatts[i]->rvar,
                       			JoinDef->mqatts[i]->col, NULL);
        	}
	}
    }

    /*
    ** Set up the reset of the fields:
    */
    for (i = 0; i < JoinDef->mqnumatts; i++)
        if (r_JDMaintAttrList(JD_ATT_GET_FIELD_CNT, 
		JoinDef->mqatts [i]->rvar, JoinDef->mqatts [i]->col, &cnt))
        {
           	_VOID_ STcat(En_union, ERx("' ' AS "));
        	_VOID_ IIUGxri_id(JoinDef->mqatts[i]->col,
				  &tmp_col_buf[0]);
               	_VOID_ STcat(En_union, &tmp_col_buf[0]);
           	_VOID_ STcat(En_union, W_COMMA);
           	_VOID_ r_JDMaintAttrList(JD_ATT_SET_USE_FLAG, 
				JoinDef->mqatts[i]->rvar,
                   		JoinDef->mqatts[i]->col, NULL);
        }

    if (STlength(En_union) >= STlength(W_COMMA))
        En_union [STlength(En_union) - STlength(W_COMMA)] = EOS;

    _VOID_ STcat(En_union, W_SP_FROM);

    for (i = 0; i < NumMasters; i ++)
    {
    	if  ((rvar_tab_ptr = r_JDRvToTabName(MasterRvars[i])) != (RV_TAB *)NULL)
	{
	    if  ((STcompare(IIUIcsl_CommonSQLLevel(),UI_LEVEL_65) < 0) ||
		 (STbcompare(rvar_tab_ptr->own,0,
			     UI_FE_CAT_ID_65,0,TRUE) == 0) ||
		 (STequal(rvar_tab_ptr->own,IIuiUser)) ||
		 ((STequal(rvar_tab_ptr->own,IIuiDBA)) &&
		  (!FE_resolve(&rjc_ferslv,rvar_tab_ptr->tab,IIuiUser))))
	    {
		_VOID_ IIUGxri_id(rvar_tab_ptr->tab,&tmp_tbl_buf[0]);
	    }
	    else
	    {
		_VOID_ IIUIxrt_tbl(rvar_tab_ptr->own,rvar_tab_ptr->tab,
				   &tmp_tbl_buf[0]);
	    }
	    _VOID_ STcat(En_union,&tmp_tbl_buf[0]);
	    _VOID_ STcat(En_union, W_SPACE);
       	    _VOID_ IIUGxri_id(rvar_tab_ptr->rvar,&tmp_col_buf[0]);
            _VOID_ STcat(En_union, &tmp_col_buf[0]);
            _VOID_ STcat(En_union, W_COMMA);
	}
    }

    if (STlength(En_union) >= STlength(W_COMMA))
    {
        En_union [STlength(En_union) - STlength(W_COMMA)] = EOS;
    }

    if (master_joins == 0)
    {
        _VOID_ STcat(En_union, ERx(" where not exists (select "));
    }
    else
    {
	/* Add master join clause */
        _VOID_ STcat(En_union, W_SP_WHERE);
        _VOID_ STcat(En_union, master_clause);
        _VOID_ STcat(En_union, ERx(" and not exists (select "));
    }
    _VOID_ STcat(En_union, ERx("* from "));

    /*
    ** Make sure that all of instances of the master tables 
    ** are tagged as having been used:
    */
    _VOID_ r_JDMaintTabList(JD_TAB_INIT_INS_CNT, NULL, NULL, NULL, NULL);
    for (i = 0; i < NumMasters; i ++)
	_VOID_ r_JDMaintTabList(JD_TAB_RVAR_ADV_INS_CNT, MasterRvars [i], 
		NULL, NULL, &ins);

    for (i = 0; i < JoinDef->mqnumtabs; i++)
    {
        if (!r_JDIsMasTab(JoinDef->mqtabs[i]->mqrangevar))
        {
		_VOID_ r_JDMaintTabList(JD_TAB_RVAR_ADV_INS_CNT, 
			JoinDef->mqtabs[i]->mqrangevar, NULL, NULL, &ins);
		if  (incl_owner[i])
		{
		    _VOID_ IIUIxrt_tbl(JoinDef->mqtabs[i]->mqrelown,
					JoinDef->mqtabs[i]->mqrelid,
					&tmp_tbl_buf[0]);
		}
		else
		{
		    _VOID_ IIUGxri_id(JoinDef->mqtabs[i]->mqrelid,
				      &tmp_tbl_buf[0]);
		}
		_VOID_ STcat(En_union, &tmp_tbl_buf[0]);
           	_VOID_ STcat(En_union, W_SPACE);
		_VOID_ IIUGxri_id(JoinDef->mqtabs[i]->mqrangevar,
				  &tmp_col_buf[0]);
		_VOID_ STcat(En_union, &tmp_col_buf[0]);
           	_VOID_ STcat(En_union, W_COMMA);
        }
    }

    if (STlength(En_union) >= STlength(W_COMMA))
    {
        En_union [STlength(En_union) - STlength(W_COMMA)] = EOS;
    }

    _VOID_ STcat(En_union, W_SP_WHERE);
    _VOID_ STcat(En_union, detail_clause);

    return;

}




/*
**  Name:  r_JDRvToTabName
**
**  Description:
**      Given a table range variable return the table's full-name.
**      If no such table found, return NULL. Note since rv's are 
**	unique we don't need to check the table's instance.
**
**	For flexibility in the 6.5 namespace environment, return the
**	pointer to the RV_TAB and let the caller
**	un-resolve the owner.tablename construct as required.
*/
static
RV_TAB *
r_JDRvToTabName(rv)
char *rv;			/* Range Variable */
{

    i4  i;

    if (rv == NULL || *rv == EOS)
    {
        return((RV_TAB *)NULL);
    }

    for (i = 0; i < NumRels; i++)
    {
        if (STcompare(rv,RvTabName[i].rvar) == 0)
	{
            return(&RvTabName[i]);
	}
    }

    return((RV_TAB *)NULL);

}



/*
**  Name:  r_JDTabNameToRv
**
**  Description:
**      Given a table name return its range variable.
**      If no such table found, return NULL.
**
**  History:
**	22-sep-1993 (rdrane)
**		Re-define to pass owner as parameter, and include it
**		in the table matching algorithm.
*/
char *r_JDTabNameToRv(tab, own, instance)
char *tab;			/* Table Name */
char *own;			/* Table Owner */
i4  instance;			/* Instance Of Table Name */
{

	i4	i;
	i4	InsCnt;


	if  ((tab == NULL) || (*tab == EOS))
	{
		return(NULL);
	}

	InsCnt = 0;
	for (i = 0; i < NumRels; i++)
	{
		if  ((STcompare(RvTabName[i].tab,tab) == 0) &&
		     ((own == NULL) || (*own == EOS) ||
		      (STcompare(RvTabName[i].own,own) == 0)))
		{
			InsCnt ++;
			if (InsCnt == instance)
			{
				return(RvTabName[i].rvar);
			}
			else
			{
				if  (RvTabName[i].link == JD_LNK_NULL)
				{
					return(NULL);
				}
				else
				{
					i = RvTabName[i].link - 1;
				}

			}
		}
	}

	return(NULL);

}




/*
**  Name:  r_JDMaintAttrList
**
**  Description:
**      Maintains a list of all attributes used.
**
**  History:
**	22-sep-1993 (rdrane)
**		Re-write the routine as a case statement.
**		Cast NULL pointer values apropriately.
*/
bool r_JDMaintAttrList(action, arg1, arg2, ReturnVal)
i4  action;			/* Action to be performed */
char *arg1;			/* Argument 1 */
char *arg2;			/* Argument 2 */
PTR  *ReturnVal;		/* Pointer to return value */
{
	static JD_ATTR	*AttrHead = (JD_ATTR *)NULL;
	static JD_ATTR	*AttrTail = (JD_ATTR *)NULL;

		i4	LastOccur;
		JD_ATTR	*new;
		JD_ATTR	*prev;
		char	*GenName;
		bool	ret_val;


	new = NULL;
	prev = NULL;
	ret_val = FALSE;

	switch(action)
	{
	case JD_ATT_ADD_TO_LIST:
		LastOccur = 0;
		new = AttrHead;
		while (new != (JD_ATTR *)NULL)
		{
			if  (STcompare(new->field,arg2) == 0)
			{
				LastOccur = new->OccurCnt;
			}
			new = new->link;
		}
		if  ((new = (JD_ATTR *)MEreqmem((u_i4)0,
					 sizeof(JD_ATTR),TRUE,
					(STATUS *)NULL)) == (JD_ATTR *)NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("r_JDMaintAttrList - new"));
		}

		new->OccurCnt = LastOccur + 1;
		new->used = FALSE;
		_VOID_ STcopy(arg1, new->rvar);
		_VOID_ STcopy(arg2, new->field);
		if  (new->OccurCnt > 1)
		{
			if  (IIGNgnGenName(1, new->field, &GenName) != OK)
			{
				break;
			}
			_VOID_ STcopy(GenName, new->attr);
		}
		else
		{
			_VOID_ IIGNonOldName(1, new->field);
			_VOID_ STcopy(new->field, new->attr);
		}

		new->link = (JD_ATTR *)NULL;
		if (AttrHead == (JD_ATTR *)NULL)
		{
			AttrHead = new;
			AttrTail = AttrHead;
		}
		else
		{
			AttrTail->link = new;
			AttrTail = new;
		}
		ret_val = TRUE;
    		break;

	case JD_ATT_GET_FIELD_CNT:
		new = AttrHead;
		while (new != (JD_ATTR *)NULL)
		{
			if  ((STcompare(new->field, arg2) == 0) &&
			     (STcompare(new->rvar, arg1) == 0) &&
			     (new->used == FALSE))
			{
				*((i4 *)ReturnVal) = new->OccurCnt;
				ret_val = TRUE;
				break;
			}
			new = new->link;
		}
		break;

	case JD_ATT_GET_RVAR_NAME:
		new = AttrHead;
		while (new != (JD_ATTR *)NULL)
		{
			if (STcompare(new->attr, arg1) == 0)
			{
				_VOID_ STcopy(new->rvar, (char *)ReturnVal);
				ret_val = TRUE;
				break;
			}
			new = new->link;
		}
		break;

	case JD_ATT_DELETE_FROM_LIST:
		new = AttrHead;
		new = AttrHead;
		while (new != (JD_ATTR *)NULL)
		{
			if  ((STcompare(new->field, arg2) == 0) &&
			     (STcompare(new->rvar, arg1) == 0))
			{
				if  (new == AttrHead)
				{
					AttrHead = AttrHead->link;
					if  (AttrHead == (JD_ATTR *)NULL)
					{
						AttrTail = AttrHead;
					}
				}
				else if  (new == AttrTail)
				{
					AttrTail = prev;
					AttrTail->link = (JD_ATTR *)NULL;
				}
				else
				{
					prev->link = new->link;
				}
				_VOID_ MEfree((PTR) new);    
				ret_val = TRUE;
				break;
			}
			prev = new;
			new = new->link;
		}
		break;

	case JD_ATT_LIST_EMPTY:
		if  ((AttrHead == (JD_ATTR *)NULL) &&
		     (AttrTail == (JD_ATTR *)NULL))
		{
			ret_val = TRUE;
			break;
		}
		break;

	case JD_ATT_LIST_FLUSH:
		new = AttrHead;
		prev = AttrHead;
		while (new != (JD_ATTR *)NULL)
		{
			prev = new;
			new = new->link;
			_VOID_ MEfree((PTR) prev);    
		}
		AttrHead = (JD_ATTR *)NULL;
		AttrTail = (JD_ATTR *)NULL;
		_VOID_ IIGNesEndSpace(1);
		ret_val = TRUE;
		break;

	case JD_ATT_INIT_USE_FLAGS:
		new = AttrHead;
		while (new != (JD_ATTR *)NULL)
		{
			new->used = FALSE;
			new = new->link;
		}
		ret_val = TRUE;
		break;

	case JD_ATT_SET_USE_FLAG:
		new = AttrHead;
		while (new != (JD_ATTR *)NULL)
		{
			if  ((STcompare(new->field, arg2) == 0) &&
			     (STcompare(new->rvar, arg1) == 0))
			{
				new->used = TRUE;
				ret_val = TRUE;
				break;
			}
			new = new->link;
		}
		break;

	case JD_ATT_GET_OCCUR_CNT:
		new = AttrHead;
		while (new != (JD_ATTR *)NULL)
		{
			if  ((STcompare(new->field, arg2) == 0) &&
			     (STcompare(new->rvar, arg1) == 0))
			{
				*((i4 *)ReturnVal) = new->OccurCnt;
				ret_val = TRUE;
				break;
			}
			new = new->link;
		}
		break;

	case JD_ATT_GET_ATTR_NAME:
		new = AttrHead;
		while (new != (JD_ATTR *)NULL)
		{
			if  ((STcompare(new->field, arg2) == 0) &&
			     (STcompare(new->rvar, arg1) == 0))
			{
				_VOID_ STcopy(new->attr, (char *)ReturnVal); 
				ret_val = TRUE;
				break;
			}
			new = new->link;
		}
		break;

	case JD_ATT_GET_FIELD_NAME:
		new = AttrHead;
		while (new != (JD_ATTR *)NULL)
		{
			if  ((STcompare(new->rvar, arg1) == 0) &&
			     (STcompare(new->attr, arg2) == 0))
			{
				_VOID_ STcopy(new->field, (char *)ReturnVal);
				ret_val = TRUE;
				break;
			}
			new = new->link;
		}
		break;

	default:
		/*
		** Unknown action:
		*/
		break;
		break;
	}

	return(ret_val);
}




/*
**  Name:  r_JDAttToRvar
**
**  Description:
**      Given an attribute name return its table's range var, using the 
**      JoinDef information. This routine can also be used to see if an 
** 	attribute is used in the JoinDef. Note that the attribute 
**	names passed in are internal to RBF and are unique; therefore, 
**	we don't need to worry about the table instance here.
*/
static
char
*r_JDAttToRvar(AttrName)
char *AttrName;			/* Attribute name */
{

    i4  i;
    static char rvar [FE_MAXNAME + 1];

    if (AttrName == NULL || *AttrName == EOS)
        return(NULL);

    for (i = 0; i < JoinDef->mqnumatts; i ++)
	if (r_JDMaintAttrList(JD_ATT_GET_RVAR_NAME,
		AttrName, NULL, &rvar[0]))
            return(rvar);

    return(NULL);

}





/*
**  Name:  r_JDIsMasTab
**
**  Description:
**      Given a relation's range var returns TRUE if it's master table 
** 	or if not a master/detail joindef (you can have master/master joins
**	but not detail/detail joins).
**
*/
static
bool
r_JDIsMasTab(Rvar)
char *Rvar;			/* Range variable */
{

    i4  i;

    if (!IsMasterDetail)
	return(TRUE);

    for (i = 0; i < NumMasters; i++)
	if (STcompare(Rvar, MasterRvars [i]) == 0)
	    return(TRUE);

    return(FALSE);

}





/*
**  Name:  r_JDLoadQuery
**
**  Description:
**      This is called when we are about to edit a report created 
**      by RBF from a JoinDef. This routine will assign the different 
**      pieces of the report query.
**
*/
bool r_JDLoadQuery()
{

    i4  j = 0;
    char *relptr = NULL;
    char *attname;
    char *rvar;
    char *TmpPtr, *TmpPtr2, *TmpPtr3;
    char *rels;        		    /* types (M or D) of tables in FROM list 
				    ** extra 4 for space, asterisk, slash and 
				    ** NULL 
				    */
    char tbl_name[(FE_UNRML_MAXNAME + 1)]; /* table name */
    char owner_name[(FE_UNRML_MAXNAME + 1)];/* owner name */
    char rvar_name[DB_GW1_MAXNAME+1];/* range variable name */
    i4   token_type;		    /* used for parsing FROM clause */
    bool master_flag;		    /* TRUE = master table */
    bool found_jd = FALSE;	    /* TRUE = found jd comment block  */
    bool comment_out = FALSE;	    /* TRUE = UNION SELECT clause is in comment 
				    **        block  */
    char *union_clause = NULL;
    i4   search_num;
    i4   len1, len2;
    char tmp_char;
    bool as_seen;
    bool as_convert;
    char *tmp_tar_name;
    char *tmp_tar_list;
    FE_RSLV_NAME rjd_ferslv;        /*
				    ** Compound identifier and tablename
				    ** validation work struct
				    */


    if (En_pre_tar != NULL)
    {
        _VOID_ MEfree((PTR) En_pre_tar);
        En_pre_tar = NULL;
    }

    if (En_tar_list != NULL)
    {
        _VOID_ MEfree((PTR) En_tar_list);
        En_tar_list = NULL;
    }

    if (En_q_qual != NULL)
    {
        _VOID_ MEfree((PTR) En_q_qual);
        En_q_qual = NULL;
    }

    if (En_union != NULL)
    {
        _VOID_ MEfree((PTR) En_union);
        En_union = NULL;
    }


    if (En_sql_from != NULL)
    {
        _VOID_ MEfree((PTR) En_sql_from);
        En_sql_from = NULL;
    }


    if (En_qcount > 0)
    {
        register QUR  *qur; 
        register i4   sequence;

        for(sequence=1,qur=Ptr_qur_arr; sequence<=En_qcount; qur++,sequence++)
        {    
            CVlower(qur->qur_section);
            if (STcompare(qur->qur_section, NAM_TLIST) == 0)
	    {
			if (En_tar_list == NULL)
                	    En_tar_list = STalloc(qur->qur_text);
			else
			    _VOID_ rfNextQueryLine(&En_tar_list, qur->qur_text);
	    }
            else
            if (STcompare(qur->qur_section, NAM_FROM) == 0)
            {    
			/* Need the FROM clause to make linked TBL list */
			if (En_sql_from == NULL)
                	    En_sql_from = STalloc(qur->qur_text);
			else
			    _VOID_ rfNextQueryLine(&En_sql_from, qur->qur_text);
	    }
            else
            if (STcompare(qur->qur_section, NAM_WHERE) == 0)
            {    
			if (En_q_qual == NULL)
                	    En_q_qual = STalloc(qur->qur_text);
			else
			    _VOID_ rfNextQueryLine(&En_q_qual, qur->qur_text);
            }
            else
            if (STcompare(qur->qur_section, NAM_REMAINDER) == 0)
            {    
			if (union_clause == NULL)
                	    union_clause = STalloc(qur->qur_text);
			else
			    _VOID_ rfNextQueryLine(&union_clause,qur->qur_text);
            }
        }

	/*
	** Process the TARGET stmt:
	*/
	if (STcompare(En_tar_list, W_ASTERISK) != 0 && 
			STcompare(En_tar_list, ERx("RBF.all")) != 0)
	{
		/*
		** In cases where the report has been generated based 
		** on a JoinDef we need to get the column names from 
		** the TARGET stmt:
		*/

		_VOID_ r_JDMaintAttrList(JD_ATT_LIST_FLUSH, NULL, NULL, NULL);
		
		/*
		** Start the GN routines:
		*/
		if (IIGNssStartSpace(1, ERx(""), FE_MAXNAME, 
				FE_MAXNAME, GN_MIXED, GNA_ACCEPT, 
				(VOID (*)())NULL) != OK)
		{
			return(FALSE);
		}
       			 
		r_g_set(&En_tar_list[0]);
		rjd_ferslv.name = NULL;
		rjd_ferslv.name_dest = &tbl_name[0];
		rjd_ferslv.owner_dest = &owner_name[0];
		rjd_ferslv.is_nrml = FALSE;
    		if  (STindex(Tokchar,W_EQUAL,0) != NULL)
		{
			/*
			** Handle oldstyle "abc=tbl.col" list.  These
			** will never be created for 6.5 and above, so we
			** needn't bother looking for delimited ID's.  If
			** we're connected to 6.5 or above, effect conversion
			** to OPEN/SQL syntax.
			*/ 
			as_convert = FALSE;
			tmp_tar_name = NULL;
			tmp_tar_list = NULL;
    			if  (STcompare(IIUIcsl_CommonSQLLevel(),
				       UI_LEVEL_65) >= 0)
			{
				/*
				** " AS " takes up more space than "=", so count
				** the equals and compute how much more space we
				** need in En_tar_list.  Don't forget to reset
				** Tokchar for the actual parsing pass!
				*/
				j = 0;
				while (*Tokchar != EOS)
				{
					if  (*Tokchar == C_EQUAL)
					{
						j++;
					}
					CMnext(Tokchar);
				}
				j = (STlength(W_SP_AS) -
				     STlength(W_EQUAL)) * j;
				if  ((tmp_tar_list = (char *)MEreqmem((u_i4)0,
            				(u_i4)(STlength(En_tar_list) + j + 1),
            				TRUE,(STATUS *)NULL)) == NULL)
    				{
        				IIUGbmaBadMemoryAllocation(
					    ERx("r_JDLoadQuery - AS convert"));
    				}
				as_convert = TRUE;
				r_g_set(&En_tar_list[0]);
			}
	        	while ((token_type = r_g_skip()) != TK_ENDSTRING)
			{
				switch(token_type)
				{
				case TK_ALPHA:
					rjd_ferslv.name = r_g_ident(TRUE);
					break;
		    		case TK_EQUALS:
					/*
					** Throwaway the W_EQUAL, but keep
					** the target if we're converting
					*/
					CMnext(Tokchar);
					if  (as_convert)
					{
						tmp_tar_name = rjd_ferslv.name;
					}
					break;
				case TK_COMMA:
    					CMnext(Tokchar);
					/*
					** Decompose any rvar.attname, validate
					** the components, and normalize them
					** for later use.  Note that no rvar
					** spec will leave owner_name set to
					** EOS.
					*/
					FE_decompose(&rjd_ferslv);
					if  ((IIUGdlm_ChkdlmBEobject(
						rjd_ferslv.name_dest,
						rjd_ferslv.name_dest,
						rjd_ferslv.is_nrml) ==
							 UI_BOGUS_ID) ||
					     ((rjd_ferslv.owner_spec) &&
			      		      (IIUGdlm_ChkdlmBEobject(
						rjd_ferslv.owner_dest,
						rjd_ferslv.owner_dest,
						rjd_ferslv.is_nrml) ==
							 UI_BOGUS_ID)))
					{
						return(FALSE);
					}
					if (!r_JDMaintAttrList(
							JD_ATT_ADD_TO_LIST,
							rjd_ferslv.owner_dest,
							rjd_ferslv.name_dest,
							NULL))
					{
						return(FALSE);
					}
					if  (as_convert)
					{
						if  (tmp_tar_name != NULL)
						{
							STpolycat(5,
								tmp_tar_list,
								rjd_ferslv.name,
								W_SP_AS,
								tmp_tar_name,
								W_COMMA,
								tmp_tar_list);
							MEfree(
							    (PTR)tmp_tar_name);
						}
						else
						{
							STpolycat(3,
								tmp_tar_list,
								rjd_ferslv.name,
								W_COMMA,
								tmp_tar_list);
						}
						tmp_tar_name = NULL;
					}
					MEfree((PTR)rjd_ferslv.name);
					rjd_ferslv.name = NULL;
					break;
				default:
					/*
					** This really shouldn't happen ...
					*/
					CMnext(Tokchar);
					break;
				}
    			}
			if  (rjd_ferslv.name != NULL)
			{
				/*
				** Do the final rvar and attname.  There
				** should always be one!
				*/
				FE_decompose(&rjd_ferslv);
				if  ((IIUGdlm_ChkdlmBEobject(
						rjd_ferslv.name_dest,
						rjd_ferslv.name_dest,
						rjd_ferslv.is_nrml) ==
						 	UI_BOGUS_ID) ||
				     ((rjd_ferslv.owner_spec) &&
		      		      (IIUGdlm_ChkdlmBEobject(
						rjd_ferslv.owner_dest,
						rjd_ferslv.owner_dest,
						rjd_ferslv.is_nrml) ==
						 	UI_BOGUS_ID)))
				{
					return(FALSE);
				}
				if (!r_JDMaintAttrList(JD_ATT_ADD_TO_LIST,
						rjd_ferslv.owner_dest,
						rjd_ferslv.name_dest,NULL))
				{
					return(FALSE);
				}
				if  (as_convert)
				{
					if  (tmp_tar_name != NULL)
					{
						STpolycat(5,
							tmp_tar_list,
							rjd_ferslv.name,
							W_SP_AS,
							tmp_tar_name,
							W_COMMA,
							tmp_tar_list);
						MEfree((PTR)tmp_tar_name);
					}
					else
					{
						STpolycat(3,
							tmp_tar_list,
							rjd_ferslv.name,
							W_COMMA,
							tmp_tar_list);
					}
					MEfree((PTR)En_tar_list);
					En_tar_list = tmp_tar_list;
				}
				MEfree((PTR)rjd_ferslv.name);
			}
		}
		else
		{
			as_seen = FALSE;
			/*
			** Handle newstyle "tbl.col as abc" list or a list
			** with no result names.  These may or may not
			** contain delimited ID's.
			*/ 
	        	while ((token_type = r_g_skip()) != TK_ENDSTRING)
			{
				switch(token_type)
				{
				case TK_ALPHA:
				case TK_QUOTE:
					TmpPtr = r_g_ident(TRUE);
					if  (as_seen)
					{
						/*
						** End of target w/ a result
						** column.  Ignore the result
						** column itself - we already
						** have the rvar and/or attname
						*/
						MEfree((PTR)TmpPtr);
						break;
					}
	        			if  ((token_type == TK_QUOTE) &&
					     (!Rbf_xns_given))
					{
						/*
						** We should really flag an
						** error here ...
						*/
						MEfree((PTR)TmpPtr);
						break;
					}
					if  (STbcompare(TmpPtr,
						STlength(W_AS),W_AS,
						STlength(W_AS),TRUE) == 0)
					{
						/*
						** Remember that we saw the
						** W_AS and throw it away
						*/
						as_seen = TRUE;
						MEfree((PTR)TmpPtr);
						break;
					}
					/*
					** This is the column name.  Save it
					** until we see a comma or EOS
					*/
					rjd_ferslv.name = TmpPtr;
					break;
				case TK_COMMA:
					CMnext(Tokchar);
					as_seen = FALSE;
					if  (rjd_ferslv.name == NULL)
					{
						/*
						** This should NEVER happen!
						*/
						break;
					}
					/*
					** Decompose any rvar.attname, validate
					** the components, and normalize them
					** for later use.  Note that no rvar
					** spec will leave owner_name set to
					** EOS.
					*/
					FE_decompose(&rjd_ferslv);
					MEfree((PTR)rjd_ferslv.name);
					rjd_ferslv.name = NULL;
					if  ((IIUGdlm_ChkdlmBEobject(
						rjd_ferslv.name_dest,
						rjd_ferslv.name_dest,
						rjd_ferslv.is_nrml) ==
							 UI_BOGUS_ID) ||
					     ((rjd_ferslv.owner_spec) &&
			      		      (IIUGdlm_ChkdlmBEobject(
						rjd_ferslv.owner_dest,
						rjd_ferslv.owner_dest,
						rjd_ferslv.is_nrml) ==
							 UI_BOGUS_ID)))
					{
						return(FALSE);
					}
					if (!r_JDMaintAttrList(
							JD_ATT_ADD_TO_LIST,
							rjd_ferslv.owner_dest,
							rjd_ferslv.name_dest,
							NULL))
					{
						return(FALSE);
					}
					break;
				default:
					/*
					** This really shouldn't happen ...
					*/
					CMnext(Tokchar);
					break;
				}
    			}
			if  (rjd_ferslv.name != NULL)
			{
				/*
				** End of target list.  Do the final rvar
				** and/or attname (there should ALWAYS be
				** one!).
				*/
				FE_decompose(&rjd_ferslv);
				MEfree((PTR)rjd_ferslv.name);
				if  ((IIUGdlm_ChkdlmBEobject(
						rjd_ferslv.name_dest,
						rjd_ferslv.name_dest,
						rjd_ferslv.is_nrml) ==
						 	UI_BOGUS_ID) ||
				     ((rjd_ferslv.owner_spec) &&
		      		      (IIUGdlm_ChkdlmBEobject(
						rjd_ferslv.owner_dest,
						rjd_ferslv.owner_dest,
						rjd_ferslv.is_nrml) ==
						 	UI_BOGUS_ID)))
				{
					return(FALSE);
				}
				if (!r_JDMaintAttrList(JD_ATT_ADD_TO_LIST,
						rjd_ferslv.owner_dest,
						rjd_ferslv.name_dest,NULL))
				{
					return(FALSE);
				}
			}
		}
	}


	/*
  	** Get the comment block describing joindef relationship.
	** All JoinDefs will have En_q_qual (WHERE clause).  RBF joindef 
	** reports will also have a union_clause. 
	**
	** The table types (master/detail or master/master for example)
	** are saved in a comment block in the last section, either En_q_qual
	** or union_clause depending upon how the report was saved.
	** For Joindef reports saved in RBF, union_clause will be the union 
	** select and comment block w/the table type 
	** (master/detail) info.  For MM joins or a joindef w/no joins 
	** (single table joindef) that have been COPYREP'ed in, 
	** the comment block will be tacked to the end of En_q_qual.  
	**
	** It is also possible that the union_clause is surrounded by comments. 
	** These comments are marked by slash and asterisk.  The comment
	** block will be found at the end of En_q_qual.  If the
	** report has a detail selection criteria, the UNION SELECT clause 
	** is not needed, therefore it is commented out.  RBF needs to 
	** strip off these comment marks if they exist.  (#32781)
	*/


	if (union_clause != NULL && *union_clause != EOS)
	{
		/* for master/detail joindefs  */
		TmpPtr2 = union_clause;
		search_num = 1;		/* check the first character
					** if a comment */
	}
	else if (En_q_qual != NULL && *En_q_qual != EOS)
	{
		/* for master/master joindefs (copyrep'ped in) */
		TmpPtr2 = En_q_qual;
		search_num = 0;		/* search full string for
					** comment */
	}
	else 
	{
		/* for single table joindefs (copyrep'ped in) */
		TmpPtr2 = En_sql_from;
		search_num = 0;		/* search full string for
					** comment */
	}


	/* 
	** check if UNION clause is in comments, marked by a slash  
	** and asterisk
	*/
    	if (((TmpPtr = (STindex(TmpPtr2, ERx("/"), search_num))) != NULL) &&
	   (STbcompare (TmpPtr, 2, ERx("/*"), 2, TRUE) == 0))
	{
		if (IIRFckc_CheckComment(TmpPtr, &TmpPtr3) == FALSE)
		{
			/* 
			** Could not parse the comment block: 
			** "DO NOT MODIFY. DOING SO MAY CAUSE REPORT TO 
			** BE UNUSABLE."
			**
			** Assumes if this has been modified, it's possible user
			** also modified code.  
			*/
			IIUGerr(E_RF0099_union_modified, UG_ERR_ERROR, 0);
			return (FALSE);
		}
		*TmpPtr = '\0';			   /* end the string */
		CMnext(TmpPtr);			   /* skip over it */
    		TmpPtr2 = (STindex(TmpPtr, ERx("/"), 0)); /* point to closing 
							   ** comment */
		while (*TmpPtr2 != EOS)
		{
			if (!CMalpha(TmpPtr2))	  /* advance past the comment */
			{
		   	    CMnext(TmpPtr2);	          /* to "union" */
			}
			else
			{
			    /* Look for "width ".  This is an indicator that
			       the search has already passed the possible
			       location of the "union" statement (i.e. no
			       "union select" statement is present. (37667) */
			    if (*TmpPtr2 != EOS && 
				STbcompare(TmpPtr2, 0, W_WIDTH,
					      STlength(W_WIDTH),TRUE) == 0)
			    {
				return TRUE;
			    }

			    comment_out = TRUE;
			    break;
			}
		}
		if (!comment_out)
		{
			/* 
			** since reached EOS on TmpPtr2, then we must
			** have reached the joindef comment block
			*/ 
			rels = STalloc(TmpPtr3);
			found_jd = TRUE;
		}
		/* 
		** If both union_clause and En_q_qual are NULL, then TmpPtr2
		** was set by En_sql_from - in this case the joindef is based
		** on a single table joindef.  Hence, no UNION SELECT
		** clause.
		*/
		if ((union_clause != NULL && *union_clause != EOS) ||
		   (En_q_qual != NULL && *En_q_qual != EOS))
		{
			/* TmpPtr2 must be allocated for union_clause
			   to be freed later (bug 39792).            */
			TmpPtr2 = STalloc(TmpPtr2);
    			union_clause = TmpPtr2;
		}
	}

	/* get comment block describing the joindef */
    	if ((TmpPtr = (STindex(TmpPtr2, ERx("/"), 0))) != NULL)
	{
		if (comment_out)
		{
			*TmpPtr = '\0'; /* end the union_clause at backslash */
		 	CMnext(TmpPtr);	      /* skip over closing backslash */
		}
		
		if (IIRFckc_CheckComment(TmpPtr, &TmpPtr3) == FALSE)
		{
			/* 
			** Could not parse the comment block: 
			** "DO NOT MODIFY. DOING SO MAY CAUSE REPORT TO 
			** BE UNUSABLE."
			**
			** Assumes if this has been modified, it's possible user
			** also modified code.  
			*/
			IIUGerr(E_RF009A_jd_modified, UG_ERR_ERROR, 0);
			return (FALSE);
		}

		if (!comment_out)
			*TmpPtr = '\0';	 /* end the union_clause at backslash */

		rels = STalloc(TmpPtr3);
		_VOID_ STtrmwhite(TmpPtr2);
	}
	else if (!found_jd)
	{
		/* Could not find the comment block */
		IIUGerr(E_RF0093_no_jd_comment, UG_ERR_ERROR, 0);
		return (FALSE);
	}

	/* get rid of trailing asterisk if UNION clause was in comments */
    	if (comment_out) 
	{
		len1 = STlength(TmpPtr2);
		len2 = STlength(W_ASTERISK);

		if (len1 < len2)
		{
			/* Looking for an ASTERISK at the end of the string */
			IIUGerr(E_RF009B_union_asterisk, UG_ERR_ERROR, 0);
			return (FALSE);
		}

		TmpPtr = &TmpPtr2[len1 - len2];
		if (STbcompare(TmpPtr,len2,W_ASTERISK,len2,TRUE) == 0)
		{
			*TmpPtr = EOS;
		}
		else
		{
			/* 
			** Looking for an ASTERISK at the end of the string.
			** Last char is not an asterisk.
			*/
			IIUGerr(E_RF009B_union_asterisk, UG_ERR_ERROR, 0);
			return (FALSE);
		}
	}

	/*
	** Process the QUERY-QUALIFICATION stmt. Find out if there are 
	** any qualifications to handle selection criteria. If there are,
	** they would start by a '(' and go to the end of the query. We 
	** need to have En_q_qual pointing to only the qualifications 
	** needed to express the JoinDef, since the selection criteria 
	** maybe changed by the user during the editing. Do the same for 
	** the REMAINDER of the query as well:
	**
	** It is possible there is no WHERE clause for a joindef that consists
	** of a table by itself.  This joindef has no join columns.  In 
	** this case, En_q_qual is NULL.
	*/
	if (En_q_qual != NULL && *En_q_qual != EOS)
	{
		TmpPtr = STskipblank(En_q_qual, STlength(En_q_qual));
		if (TmpPtr != NULL)
			STcopy(TmpPtr, En_q_qual);

		relptr = En_q_qual;
		if (En_q_qual != NULL && *En_q_qual != EOS)
		{
			while ((CMcmpcase(relptr, W_O_PAREN) != 0) && 
				(*relptr != EOS))
			{
				CMnext(relptr);
			}
			TmpPtr = relptr;
			if (relptr != NULL && *relptr != EOS)
				relptr = STalloc(relptr);
			*TmpPtr = EOS;
		}

	}

	/* strip off the last paren in the remainder clause */
	if (union_clause != NULL && *union_clause != EOS)
	{
		_VOID_ STtrmwhite(union_clause);

		len1 = STlength(union_clause);
		len2 = STlength(W_C_PAREN);
		if (len1 < len2)
		{
			/* 
			** Looking for a close paren at the end of the string.
			** String is too short.
			*/
			IIUGerr(E_RF009C_union_paren, UG_ERR_ERROR, 0);
			return (FALSE);
		}
		TmpPtr = &union_clause[len1 - len2];
		if (STbcompare(TmpPtr,len2, W_C_PAREN,len2,TRUE) == 0)
		{
			*TmpPtr = EOS;
		}
		else
		{
			/* 
			** Looking for a close paren at the end of the string.
			** Last char is not a left paren.
			*/
			IIUGerr(E_RF009C_union_paren, UG_ERR_ERROR, 0);
			return (FALSE);
		}
	}

	if (relptr != NULL && *relptr != EOS)
	{
		if (En_q_qual != NULL && *En_q_qual != EOS)
		{
			_VOID_ STtrmwhite(En_q_qual);
			len1 = STlength(En_q_qual);
			len2 = STlength(W_AND);
			if (len1 < len2)
			{
				/* 
				** Looking for "and" at the end of the string.
				** String is too short.
				*/
				IIUGerr(E_RF009E_union_and, UG_ERR_ERROR,0);
				return (FALSE);
			}
			TmpPtr = &En_q_qual[len1 - len2];
			if (STbcompare(TmpPtr, 0, W_AND, len2, TRUE) == 0)
			{
				*TmpPtr = EOS;
			}
			else
			{
				/* 
				** Looking for "and" at the end of the string.
				** Last word is not "and".
				*/
				IIUGerr(E_RF009E_union_and, UG_ERR_ERROR, 0);
				return (FALSE);
			}
		}

		if (union_clause != NULL && *union_clause != EOS)
		{
			len1 = STlength(union_clause);
			len2 = STlength(relptr);
			if (len1 < len2)
			{
				/* 
				** Looking for the where clause at the end of 
				** the string.  String is too short.
				*/
				IIUGerr(E_RF009D_union_where, UG_ERR_ERROR, 0);
				return (FALSE);
			}
			union_clause [len1 - len2] = EOS; 
			_VOID_ STtrmwhite(union_clause);

			len1 = STlength(union_clause);
			len2 = STlength(W_AND);
			if (len1 < len2)
			{
				/* 
				** Looking for "and" at the end of the string.
				** String is too short.
				*/
				IIUGerr(E_RF009E_union_and, UG_ERR_ERROR, 0);
				return (FALSE);
			}
			TmpPtr = &union_clause[len1 - len2];
			if (STbcompare(TmpPtr, 0, W_AND, len2, TRUE) == 0)
			{
				*TmpPtr = EOS;
			}
			else
			{
				/* 
				** Looking for "and" at the end of the string.
				** Last word is not "and".
				*/
				IIUGerr(E_RF009E_union_and, UG_ERR_ERROR, 0);
				return (FALSE);
			}
			_VOID_ STtrmwhite(union_clause);

			/* 
			** If the selection criteria was on a master column, 
			** there will be an extra paren at the end that needs 
			** to be stripped off.
			*/
    			if ((STindex(union_clause, W_C_PAREN, 0)) != NULL)
			{
				len1 = STlength(union_clause);
				len2 = STlength(W_C_PAREN);
				if (len1 < len2)
				{
					/* 
					** Looking for a close paren at the end
					** of the string.  String is too short.
					*/
					IIUGerr(E_RF009C_union_paren, 
						UG_ERR_ERROR, 0);
					return (FALSE);
				}
				TmpPtr = &union_clause[len1 - len2];
				if (STbcompare(TmpPtr,len2,
				   W_C_PAREN,len2, TRUE) == 0)
				{
					*TmpPtr = EOS;
				}
				else
				{
					/* 
					** Looking for a close paren at the end
					** of the string.  Last char is not a 
					** left paren.
					*/
					IIUGerr(E_RF009C_union_paren, 
						UG_ERR_ERROR, 0);
					return (FALSE);
				}
			}
		}
		_VOID_ MEfree((PTR) relptr);
	}

	/* Copy clause to Global variable, En_union */
	if (union_clause != NULL && *union_clause != EOS)
	{
        	En_union = STalloc(union_clause);
        	_VOID_ MEfree((PTR) union_clause);
	}

	/* Now fill up the linked TBL list */

	r_g_set (En_sql_from);		   /* set up parsing FROM clause */

	TmpPtr2 = rels;
	while (!CMalpha(TmpPtr2))	   /* advance past the comment */
	{
		/* Look for the start of the 'm' or 'd characters */
		/* Report an error if we reach end of comment     */
		if (STbcompare(TmpPtr2,0,ERx("*/"),2,TRUE) == 0 )
		{
		    IIUGerr(E_RF0099_union_modified, UG_ERR_ERROR, 0);
		    return (FALSE);
		}
		CMnext(TmpPtr2);	   /* to the type list */
	}
	if (STbcompare(TmpPtr2,0,ERx("m"),1,TRUE) == 0)
		master_flag = TRUE;
	else
		master_flag = FALSE;

	 /* Free old TBL linked list cause we're going to create it */

	if (Tbl_tag != NO_TBL_TAG)
	{
		FEfree(Tbl_tag);
		Tbl_tag = NO_TBL_TAG;
		Ptr_tbl = NULL;
	}

	rjd_ferslv.name_dest = &tbl_name[0];
	rjd_ferslv.owner_dest = &owner_name[0];
	rjd_ferslv.is_nrml = FALSE;
	for (;;)
	{
		token_type = r_g_skip(); 	/* skip any white space */
		if ((token_type == TK_ALPHA) ||
		    ((token_type == TK_QUOTE) && (Rbf_xns_given)))
		{
			/* should have table name */
			rjd_ferslv.name = r_g_ident(TRUE);
			/*
			** Decompose any owner.tablename, validate the
			** components, and normalize them for later use.
			** Note that no owner spec will leave owner_name
			** set to EOS.
			*/
			FE_decompose(&rjd_ferslv);
			MEfree((PTR)rjd_ferslv.name);
			if  ((rjd_ferslv.owner_spec) &&
			     (STcompare(IIUIcsl_CommonSQLLevel(),
					UI_LEVEL_65) < 0))
			{
				continue;
			}
			if  ((IIUGdlm_ChkdlmBEobject(&tbl_name[0],
					&tbl_name[0],
					rjd_ferslv.is_nrml) == UI_BOGUS_ID) ||
			     ((rjd_ferslv.owner_spec) &&
			      (IIUGdlm_ChkdlmBEobject(&owner_name[0],
				      &owner_name[0],
				      rjd_ferslv.is_nrml) == UI_BOGUS_ID)))
			{
				continue;
			}
				
			token_type = r_g_skip();/* skip white space */
			if ((token_type == TK_ALPHA) ||
			    ((token_type == TK_QUOTE) && (Rbf_xns_given)))
			{
  				/* have range variable */
				TmpPtr = r_g_ident(FALSE);
				if  (IIUGdlm_ChkdlmBEobject(TmpPtr,
					&rvar_name[0],FALSE) == UI_BOGUS_ID)
				{
					continue;
				}
				MEfree((PTR)TmpPtr);
				/* add table w/range variable to linked list */
				IIRF_adt_AddTbl (&tbl_name[0], 
						 &owner_name[0],
						 &rvar_name[0], 
						 master_flag);
			}
			else
			{
				/* couldn't find expected range variable */
				IIUGerr(E_RF0095_bad_range_var,UG_ERR_ERROR,0);
				return (FALSE);
			}
		}
		else if (token_type == TK_COMMA)
		{
			CMnext(Tokchar);	   /* go past comma */
			CMnext(TmpPtr2);	   /* get next type */
			if (!CMalpha(TmpPtr2))
			{
				/* 
				** there more tables in FROM clause, but the
				** comment block doesn't have any more types.
				*/
				IIUGerr(E_RF0096_bad_jd_comment,UG_ERR_ERROR,0);
				return (FALSE);
			}
			if (STbcompare(TmpPtr2,0,ERx("m"),1,TRUE) == 0)
				master_flag = TRUE;
			else
				master_flag = FALSE;

			continue;
		}
		else if (token_type == TK_ENDSTRING)
		{
			CMnext(TmpPtr2);	   /* increment */
			while (CMspace(TmpPtr2))   /* Check for end comment */
				CMnext(TmpPtr2);
			if (CMalpha(TmpPtr2))
			{
				/* 
				** Finished processing tables in FROM clause, 
				** but there are more table types in the
				** comment block.
				*/
				IIUGerr(E_RF0097_bad_jdcomment2,UG_ERR_ERROR,0);
				return (FALSE);
			}
			break;
		}
		else 
		{
			/* 
			** parsing FROM clause, and reached an unexpected 
			** character 
			*/
			IIUGerr(E_RF0098_bad_from_clause,UG_ERR_ERROR,0);
			return (FALSE);
		}
	}
	_VOID_ MEfree((PTR) rels);
    }

    return(TRUE);

}




/*
**  Name:  r_JDMasterDetailSetUp
**
**  Description:
**      This routine will set up a default master/detail style report
**      based on the currently specified JoinDef.
**
*/
bool r_JDMasterDetailSetUp()
{

    TRIM *t;
    CS *cs;
    ATT *att;
    ATTRIB ordinal;
    SORT *srt;
    COPT *copt;
    i4  i = 0;
    i4  sequence;
    i4  PointerY = 0;
    i4  PointerX = 0;
    register LIST *list;
    Sec_node *LastSec, *n, *TmpNode;
    char TmpBuf[(FE_MAXNAME * 2) +1];
    bool fold_fmt = FALSE;


    for(sequence=1,srt=Ptr_sort_top; sequence<=En_n_sorted; srt++,sequence++)
    {
        if ((ordinal = r_mtch_att(srt->sort_attid)) < 0)
        {
            r_error(0x06, NONFATAL, srt->sort_attid, NULL);
            continue;
        }
                                                                
        if  ((att = r_gt_att(ordinal)) == (ATT *)NULL)
        {
            r_error(0x06, NONFATAL, srt->sort_attid, NULL);
            continue;
        }

        if (att->att_brk_ordinal > 0)
        {
            copt = rFgt_copt(ordinal);
            copt->copt_sequence = att->att_brk_ordinal;
            copt->copt_order = srt->sort_direct [0];
            copt->copt_break = srt->sort_break [0];
	    copt->copt_brkhdr = 'y';

	    if (!(n=sec_list_find(SEC_BRKHDR, copt->copt_sequence, &Sections)))
	    {
		n = sec_node_alloc(SEC_BRKHDR, copt->copt_sequence, 0);
		STcopy(att->att_name, TmpBuf);
		STcat(TmpBuf, ERget(F_RF003E_Break_Header));
		n->sec_name = STalloc(TmpBuf);
		/* Set the ordinal number for the break header #35926 */
		n->sec_brkord = ordinal;
		sec_list_insert(n, &Sections);
	    }

	    /*
	    ** FIXME: we're presuming the we only want one break header 
	    **        and that on the first sort column. If this is not 
	    **        correct following break should be removed:
	    */
	    break;
        }
    } 


    if (!r_InitFrameInfo(SEC_DETAIL, 0, &PointerY, &PointerX))
	return(FALSE);


    /*
    ** Insert the break header section(s):
    */
    for (i = 1; i <= En_n_attribs; i++)
    {
        copt = rFgt_copt(i);
        att = r_gt_att(i);
        if (copt->copt_brkhdr == 'y')
        {
            LastSec = n = sec_list_find(SEC_BRKHDR, 
			copt->copt_sequence, &Sections);
            if (!r_PropagateYMods(PointerY, R_SEGMENT_SIZE))
                return(FALSE);
            if (!r_InsertSection(n, &PointerY))
                return(FALSE);
        }
 	else if ((att->att_format->fmt_type == F_CF) ||
		 (att->att_format->fmt_type == F_CFE))
	{
		/* folding format in the detail section #34522, #34523 */
		fold_fmt = TRUE;
	}

    }

    /*
    ** Set up the Master fields:
    */
    for (i = 1; i <= En_n_attribs; i++)
    {
        copt = rFgt_copt(i);
        att = r_gt_att(i);
        if (r_JDIsMasTab(r_JDAttToRvar(att->att_name)))
	{
	    PointerX = 0;
            if (!r_MoveTrimField(att, LastSec, &PointerY,
                        &PointerX, R_TRIM_FIELD, R_X_Y_POS, TRUE, TRUE))
                return(FALSE);
	    if (r_ChkMaxX(PointerX) == FALSE)
		return(FALSE);
	}
    }

    /*
    ** Insert a blank line:
    */
    if (!r_PropagateYMods(LastSec->sec_y+LastSec->sec_y_count+1, 1))
    	return(FALSE);
    (LastSec->sec_y_count) ++;

    /*
    ** Set up the detail fields:
    */
    TmpNode = sec_list_find(SEC_DETAIL, 0, &Sections);
    TmpNode->sec_y_count = 1;
    PointerX = 0;
    for (i = 1; i <= En_n_attribs; i++)
    {
        copt = rFgt_copt(i);
        att = r_gt_att(i);
        if (!r_JDIsMasTab(r_JDAttToRvar(att->att_name)))
	{
            if (!r_MoveTrimField(att, LastSec, &PointerY,
                        &PointerX, R_TRIM_ONLY, R_X_Y_POS, FALSE, FALSE))
                return(FALSE);

            if (!r_MoveTrimField(att, TmpNode, &PointerY,
                        &PointerX, R_FIELD_ONLY, R_X_Y_POS, TRUE, FALSE))
                return(FALSE);
	}
     }

     if (r_ChkMaxX(PointerX) == FALSE)
        return(FALSE);

    /*
    ** Take the Semicolons out of the trim for detail fields:
    */
    for (i = 1; i <= En_n_attribs; i++)
    {
        att = r_gt_att(i);
        cs = rFgt_cs(i);
        if (cs == NULL)
            continue;
 
        if (!r_JDIsMasTab(r_JDAttToRvar(att->att_name)))
            for (list = cs->cs_tlist; list != NULL; list = list->lt_next)
            {
                if ((t = list->lt_trim) == NULL)
                    continue;

                if (t->trmstr)
                    t->trmstr[STlength(t->trmstr)-1] = EOS;
            }
    }

    if (fold_fmt == TRUE)
	/* need extra row in detail section for folding format #34522, #34523*/
    	i = 1;
    else
	i = 0;

    n = sec_list_find(SEC_DETAIL, 0, &Sections);

    while (LastSec->sec_y+LastSec->sec_y_count+2 < n->sec_y)
        if (!r_PropagateYMods(LastSec->sec_y+LastSec->sec_y_count +2, -1))
            return(FALSE);
 
    LastSec = n;
    n = sec_list_find(SEC_END, 0, &Sections);

    while (LastSec->sec_y+LastSec->sec_y_count+2+i < n->sec_y)
        if (!r_PropagateYMods(LastSec->sec_y+LastSec->sec_y_count +2, -1))
            return(FALSE);

    /* Center title based on the new report format/right margin (#31446) */
    IIRFctt_CenterTitle(PointerX);

    return(r_SemiColLineUp());

}





/*
**  Name:  r_JDCreateAttrList
**
**  Description:
**  	Creates a list of attributes used in the JoinDef, making sure 
**	that all of the names are unique.
**
*/
static
VOID
r_JDCreateAttrList()
{
    i4  i;

    for (i = 0; i < JoinDef->mqnumatts; i++)
	/*
	** If the field is in use:
	*/
	if (*(JoinDef->mqatts[i]->formfield) != EOS &&
			JoinDef->mqatts[i]->formfield != (char *)NULL &&
			JoinDef->mqatts[i]->jpart != JD_FIELD_NOT_SHOWN)
	{
        	_VOID_ r_JDMaintAttrList(JD_ATT_ADD_TO_LIST, 
        			JoinDef->mqatts[i]->rvar,
        			JoinDef->mqatts[i]->col, NULL);
	}

    return;

}




/*
**  Name:  r_JDMaintTabList
**
**  Description:
**      Maintains the RvTabName [] list. 
** 	Returns TRUE if successful, and FALSE if not.
**
**  History:
**	22-sep-1993 (rdrane)
**		Include owner in all table matching algorithms.
**		Re-write the routine as a case statement.  Remove
**		the check for JD comment block immediately following
**		the range variable.  All the variables should be in fully
**		normalized form, and the search for the comment introducer
**		can be confused by a delimited abbreviation name!
**
*/
bool r_JDMaintTabList(action, arg1, arg2, arg3, ReturnVal)
i4  action;
char *arg1;
char *arg2;
char *arg3;
PTR  *ReturnVal;
{

	i4	i;
	i4	InsCnt;
	i4	LastLink;
	bool	ret_val;


	InsCnt = 0;
	LastLink = JD_LNK_NULL;
	switch(action)
	{
	case JD_TAB_ADD:
		/*
		** Check for list already full:
		*/
		if (NumRels >= MQ_MAXRELS)
		{
			return(FALSE);
		}
		for (i = 0; i < NumRels; i++)
		{
			if  ((STcompare(RvTabName[i].tab,arg2) == 0) &&
			     ((arg3 == NULL) || (*arg3 == EOS) ||
			      (STcompare(RvTabName[i].own,arg3) == 0)))
			{
				LastLink = i;
				if  (RvTabName[i].link != JD_LNK_NULL)
				{
					i = RvTabName[i].link - 1;
				}
			}
		}

    		_VOID_ STcopy(arg1,RvTabName[NumRels].rvar);
		_VOID_ STcopy(arg2,RvTabName[NumRels].tab);
		_VOID_ STcopy(arg3,RvTabName[NumRels].own);
		RvTabName[NumRels].counted = FALSE;
		RvTabName[NumRels].link = JD_LNK_NULL;
		if (LastLink != JD_LNK_NULL)
		{
			RvTabName [LastLink]. link = NumRels;
		}
		NumRels ++;
		ret_val = TRUE;
		break;

	case JD_TAB_INIT:
		NumRels = 0;
		ret_val = TRUE;
		break;

	case JD_TAB_INIT_INS_CNT:
		for (i = 0; i < NumRels; i++)
		{
			RvTabName[i].counted = FALSE;
		}
		ret_val = TRUE;
		break;

	case JD_TAB_RVAR_ADV_INS_CNT:
		/*
		** Assume we won't find it ...
		*/
		ret_val = FALSE;
		for (i = 0; i < NumRels; i++)
		{
			if  (STcompare(RvTabName[i].rvar,arg1) == 0)
			{
				InsCnt++;
				if  (!RvTabName[i].counted)
				{
					*((i4 *)ReturnVal) = InsCnt;
					RvTabName[i].counted = TRUE;
					ret_val = TRUE;
					break;
				}
				else if (RvTabName[i].link != JD_LNK_NULL)
				{
					i = RvTabName[i].link - 1;
				}
			}
		}
		break;

	case JD_TAB_TAB_ADV_INS_CNT:
		/*
		** Assume we won't find it ...
		*/
		ret_val = FALSE;
		for (i = 0; i < NumRels; i++)
		{
			if  ((STcompare(RvTabName[i].tab,arg2) == 0) &&
			     ((arg3 == NULL) || (*arg3 == EOS) ||
			      (STcompare(RvTabName[i].own,arg3) == 0)))
			{
				InsCnt ++;
				if  (!RvTabName[i].counted)
				{
					*((i4 *)ReturnVal) = InsCnt;
					RvTabName[i].counted = TRUE;
					ret_val = TRUE;
					break;
				}
				else if (RvTabName[i].link != JD_LNK_NULL)
				{
					i = RvTabName[i].link - 1;
				}
			}
		}
		break;

	default:
		/*
		** Unknown action:
		*/
		ret_val = FALSE;
		break;
	}

	return(ret_val);
}
/*
/* Name: IIRF_adt_AddTbl
**
** Description:
**	Adds a table, its range variable (correlation name) and its table
**	type (master/detail) to a linked TBL list.  The order of the tables
**	added corresponds to the order of the tables in the FROM clause.
**	This information is necessary when building the WHERE clauses
**	for selection criteria.  For more information, look at comments
**	in rfmdata.c, under IIRFtty_TableType's description.
**
** History:
**	22-sep-1993 (rdrane)
**		Use NO_TBL_TAG since tags are really unsigned.  Cast NULL
**		pointer values apropriately.
*/

static
VOID
IIRF_adt_AddTbl (tbl_name, tbl_owner, rangevar, master_fl)
char *tbl_name;
char *tbl_owner;
char *rangevar;
bool  master_fl;
{
	TBL	*new_tbl;	/* New table */
	TBL	*tmp_tbl;	/* New table */


	if (Tbl_tag == NO_TBL_TAG)
	{
		Tbl_tag = FEgettag();
		if (Tbl_tag == NO_TBL_TAG)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIRF_adt_AddTbl - Tbl_tag"));
		}
	}
	if ((new_tbl = (TBL *)FEreqmem((u_i4)Tbl_tag,
		(u_i4)(sizeof(TBL)), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("IIRF_adt_AddTbl - new_tbl"));
	}
	STcopy (tbl_name, new_tbl->tbl_name);
	STcopy (tbl_owner, new_tbl->tbl_owner);
	STcopy (rangevar, new_tbl->rangevar);
	if (master_fl)
	{
		new_tbl->type = TRUE;
	}
	else
	{
		new_tbl->type = FALSE;
	}
	new_tbl->next_tbl = NULL;
		
	if (Ptr_tbl == (TBLPTR *)NULL)
	{
    		if ((Ptr_tbl = (TBLPTR *)FEreqmem((u_i4)Tbl_tag,
            		(u_i4)(sizeof(TBLPTR)), TRUE, (STATUS *)NULL)) ==
							(TBLPTR *)NULL)
    		{
        		IIUGbmaBadMemoryAllocation(ERx("IIRF_adt_AddTbl - Ptr_tbl"));
    		}
		Ptr_tbl->first_tbl = new_tbl;
	}
	else
	{
		tmp_tbl = Ptr_tbl->last_tbl;
		tmp_tbl->next_tbl = new_tbl;
	}
	Ptr_tbl->last_tbl = new_tbl;
}

/* Name: IIRF_mas_MasterTbl
**
** Description:
**	Returns TRUE if the table specified by the range variable is a master
**	table.  If it cannot find the table, returns FALSE and err is set
**	FAIL.
*/
bool
IIRF_mas_MasterTbl (rangevar, err)
char	*rangevar;
i4	*err;
{
	TBL	*cur_tbl;


	*err = OK;
	cur_tbl = Ptr_tbl->first_tbl;
	while (cur_tbl != NULL)
	{
		if (STcompare(cur_tbl->rangevar,rangevar) == 0)
		{
			return (cur_tbl->type);
		}
		cur_tbl = cur_tbl->next_tbl;
	}
	*err= FAIL;
	return (FALSE);
}
	
