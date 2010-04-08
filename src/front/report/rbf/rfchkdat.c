/*	
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h> 
# include	<qg.h>    
# include	<mqtypes.h>
# include	<ug.h>
# include	<ui.h>
# include	<st.h>
# include	"rbftype.h"
# include	"rbfglob.h"
# include	"rbfcons.h"

/*{
** Name:    rFchk_dat() -    Check For Report Data.
**
** Description:
**   RFCHK_DAT - check to see if relations to be reported exist.
**    If no relation is specified by the .DATA command,
**    then check to see that a query has been specified.  If that
**    fails, then return.  
**
**    This is a badly overloaded routine, and not all table name
**    validations go through here anyway (specifially CREATE on
**    TABLE).  This routine is called by rFget() and r_JDrFget()
**    which only set NumRels to non-zero in the case of a JoinDefs, a
**    default report, and an existing report which includes a .DATA
**    or .TABLE command.
**
** Parameters:
**    NumRels - a pointer to the number of relations to validate.
**    RelList - an array of pointers to relation names to be validated.
**
** Returns:
**    TRUE    if successful query or data relation found.
**    FALSE   if any error found.
**
** Side Effects:
**    Sets a value for En_n_attribs and En_ferslv.owner_dest.
**
** Called by:
**    rFget.
**    r_JDrFget.
**
** Error messages:
**    4, 5.
**
** Trace Flags:
**    200.
**
** History:
**    9/8/82 (ps) - written (modified r_chk_dat).
**    7/17/85 (drh) - Modified to accept a tablename in a FROM
**            clause for sql.
**    26-nov-1986 (yamamoto)
**        Modified for double byte characters.
**    26-Jun-1987 (rdesmond)
**        Removed calls to FEgetowner and Feconnect with FErel...
**    22-aug-88 (sylviap)
**        An error message is displayed when an invalid tablename
**        is entered when creating a report.
**    24-aug-88 (sylviap)
**        Changed error 4 to display En_relation instead of En_report.
**    07/24/89 (martym)
**        >> GARNET <<
**        Modified the interface so that the relation name(s) can be 
**        passed in. And multiple relations can be supported. This routine 
**        now parses SQL FROM stmts to get relation names.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**	 1/9/89 (martym)
**		Added rfNextQueryLine().
**	2/7/90 (martym)
**		Coding standards cleanup.
**	2/15/90 (martym)
** 		Changed to use r_JDMaintTabList() to handle cases of 
**		multiple occurrences of the same table name in JoinDef's.
**	6/5/91 (steveh)
**		Fixed part of bug 37667.  White space now scanned following
**		a comma.  See rjoindef.c for the second half of the fix.
**	31-aug-1992 (rdrane)
**		Prelim fixes for 6.5 - add NULL owner for FErel_ffetch().
**	9-oct-1992 (rdrane)
**		Replace reference to En_dat_owner with reference to
**		En_ferslv.owner_dest.  Converted r_error() err_num value to
**		hex to facilitate lookup in errw.msg file.
**	5-nov-1992 (rdrane)
**		Use 6.5 routines to validate relation names, which may now
**		be owner.tablename and/or be delimited identifier(s).
**		Remove local variables relinfo, relqblk, and ret_value since
**		they're now obsolete.  The setting of En_ferslv.owner_dest
**		(previously En_dat_owner) is no longer useful, since even in
**		6.4 it was only being set here and never referred to again!
**		Removed the external GLOBALREFs - they were either not used or
**		already in included hdr files.
**	11-nov-1992
**		Rework aquisition of the query lines, and only effect
**		wholesale casing if QUEL.  Rework parsing of the FROM
**		statements to support owner.tablename and delimited
**		identifiers.
**	8-dec-1992 (rdrane)
**		Use RBF specific global to determine if delimited identifiers
**		are enabled.
**	21-jan-1993 (rdrane)
**		Since we use FE_decompose directly, ensure that the name and
**		and owner buffers will handle an unnormalized identifier!
**	22-sep-1993 (rdrane)
**		Extract the FE_decompose stuff into a static routine.  Correct
**		parameterization of r_JDMaintTabList(), which changed to
**		address bug 54949 in particular and 6.5 owner.tablename in
**		general.  This involves owner resolution and passing rangevar
**		as unnormalized - otherwise it will be added in a manner which
**		will preclude matches.  Re-introduce a ret_val variable for
**		use by r_JDMaintTabList() (better than having it core dump).
**		Note that we now verify the accesability of the relations as
**		we find them, and not as a group at the end.
**	17-nov-1993 (mgw) Bug #56730
**		Handle case where a report/table was specified on the
**		command line that does not exist.
**	22-nov-1993 (rdrane)
**		This is a re-implementation of mgw's 17-nov-1993 change.  It's
**		bit cleaner, and plugs a few minor memory leaks.  Note that
**		bug 56730 resulted from the fact that the 22-sep-93 change
**		forgot that this routine is also called when table names are
**		explicitly included on the command line.  So, allow for this
**		and validate the table's existence.  Otherwise, we wound up
**		calling rf_att_dflt(), got zero attrs, tried to allocate zero
**		space for the attr array, and failed.  Note, howvever, that if
**		the table disappears between here and rf_att_dflt(), the
**		symptom will be the same.
**	4-jan-1994 (rdrane)
**		Use OpenSQL level to determine 6.5 syntax features support,
**      28-mar-1995 (newca01)
**              Added check for STAR when checking for 6.5 standard catalog
**              interface.  This will allow 6.5 STAR to work when an owner 
**              name is specified.  THere may be problems when or if connected
**              to a pre-6.5 STAR database, but since star std catalog interface
**              is at the 6.2 level, too many problems happen with star 1.1
**              and functionality is degraded.  Bug 67724.  Note:  this will 
**              be fixed correctly with Oping2.0 when star and local catalogs 
**              are merged.
**      20-oct-1995 (stoli02/thaju02)
**              bug #70910 - unable to create report based on table owned
**              by "$ingres".
**      02-mar-1996 (morayf)
**              Use of IIUIdcd_dist as a boolean variable is incorrect.
**              It is declared as a pointer to a boolean-returning function,
**              and hence needs to be _called_ not just referenced, as indeed
**              it is elsewhere.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


FUNC_EXTERN bool r_JDMaintTabList();
VOID rfNextQueryLine();

static	VOID rfc_decompose();


bool
rFchk_dat(NumRels, RelList)
i4  *NumRels;
char *RelList[];
{
    /* 
    ** internal declarations:
    */

#define	RFC_NO_STMT	0
#define	RFC_RANGE_STMT	1
#define	RFC_FROM_STMT	2

#define	RFC_REL_WANTED	10
#define	RFC_RVAR_WANTED	11

    i4		i;
    i4		type;
    i4		ret_val;
    bool	rfc_error;
    i4		rfc_parse_state;
    char	*tmp_ptr = NULL;
    char	*rel_ptr = NULL;

    FE_RSLV_NAME rfc_ferslv;


    /* 
    ** start of routine:
    */
#    ifdef xRTR1
    if(TRgettrace(200,0))
    {
        SIprintf(ERx("rFchk_dat: entry.\r\n"));
    }
#    endif
    
    /*
    ** Trim trailing blanks from the relation names.  Note that at worst,
    ** this is a waste of time.  We should only have trailing blanks in the
    ** case of a report name not being found, but which matches a table.
    ** Note also that blanks are not significant in delmited identifiers
    ** anyway ...
    */
    for (i = 0; i < *NumRels; i++)
    {
        if (RelList [i] != NULL)
	{
            STtrmwhite(RelList [i]);
	}
    }


    if (En_sql_from != NULL)
    {
    	_VOID_ MEfree((PTR) En_sql_from);
        En_sql_from = NULL;
    }

    if  ((rfc_ferslv.name_dest = (char *)MEreqmem((u_i4)0,
		(u_i4)(FE_UNRML_MAXNAME+1),TRUE,(STATUS *)NULL)) == NULL)
    {
	IIUGbmaBadMemoryAllocation(ERx("rFchk_dat - name_dest"));
    }

    if  ((rfc_ferslv.owner_dest = (char *)MEreqmem((u_i4)0,
		(u_i4)(FE_UNRML_MAXNAME+1),TRUE,(STATUS *)NULL)) == NULL)
    {
	IIUGbmaBadMemoryAllocation(ERx("rFchk_dat - owner_dest"));
    }

    /*
    ** If we have got a query yank the relations out of it:
    */
    if (En_qcount > 0)
    {
        register QUR    *qur;        /* fast ptr to QUR array */
        register i4     sequence;    /* sequence in QUR array */

        /*
        ** have a query.  Pull out the relation from RANGE statement 
        ** or FROM statement if sql
        */
        for(sequence=1,qur=Ptr_qur_arr; sequence<=En_qcount; qur++,sequence++)
        {    
            /* 
            ** Next line in query:
            */
            CVlower(qur->qur_section);
            if  (STcompare(qur->qur_section,NAM_RANGE) == 0)
	    {
		i = RFC_RANGE_STMT;
		Rbf_xns_given = FALSE;
		St_xns_given = FALSE;
		IIUGdbo_dlmBEobject(qur->qur_text,FALSE);
	    }
	    else if (STcompare(qur->qur_section,NAM_FROM) == 0)
	    {
		i = RFC_FROM_STMT;
	    }
	    else
	    {
		i = RFC_NO_STMT;
	    }
            if  (i != RFC_NO_STMT)
            {    
		_VOID_ rfNextQueryLine(&En_sql_from, qur->qur_text);
            }
        }
    }


    i = 0;
    if (En_sql_from != NULL)
    {
    	/*
    	** Now parse the FROM stmt. There are 3 possibilities here.
    	** The report is based on a single table and the query is 
    	** an SQL query. The report is based on a single table and 
    	** the query is a QUEL query. The report is based on a Join-
    	** Def and the query is an SQL query.
    	*/
    	_VOID_ r_JDMaintTabList(JD_TAB_INIT, NULL, NULL, NULL);
    	tmp_ptr = STalloc(En_sql_from); 
	r_g_set(tmp_ptr);
	rfc_error = FALSE;
	rfc_parse_state = RFC_REL_WANTED;
	while ((!rfc_error) && ((type = r_g_skip()) != TK_ENDSTRING))
	{
		switch(type)
		{
		case(TK_ALPHA):
		case(TK_QUOTE):
			if  ((type == TK_QUOTE) && (!Rbf_xns_given))
			{
				rfc_error = TRUE;
				break;
			}
			rel_ptr = r_g_ident(TRUE);
			if  (rfc_parse_state == RFC_REL_WANTED)
			{
                		RelList[i] = STalloc(rel_ptr);
    				_VOID_ MEfree((PTR)rel_ptr);
				rfc_parse_state = RFC_RVAR_WANTED;
				break;
			}
			rfc_decompose(RelList[i],&rfc_ferslv,&rfc_error);
			if  ((rfc_error) ||
			     (IIUGdlm_ChkdlmBEobject(rel_ptr,rel_ptr,
				    		FALSE) == UI_BOGUS_ID) ||
			     (!r_JDMaintTabList(JD_TAB_ADD,rel_ptr, 
						rfc_ferslv.name_dest,
						rfc_ferslv.owner_dest,
						&ret_val)))
			{
				rfc_error = TRUE;
    				_VOID_ MEfree((PTR)rel_ptr);
				break;
			}
    			_VOID_ MEfree((PTR)rel_ptr);
                	i++;
			break;

		case(TK_COMMA):
			rfc_parse_state = RFC_REL_WANTED;
			CMnext(Tokchar);
			break;

		default:
			/*
			** We really should put out some kind of error
			** message ...
			*/
			rfc_error = TRUE;
			break;
		}
	}

	_VOID_ MEfree((PTR)tmp_ptr);
	if  (rfc_error)
	{
		MEfree((PTR)rfc_ferslv.name_dest);
		MEfree((PTR)rfc_ferslv.owner_dest);
		return(FALSE);
	}
    }

    if (*NumRels == 0)
    {
	if (i > 0)
	{
       		*NumRels = i;
		rfc_error = FALSE;
	}
	else
	{
		*NumRels = 1;
		RelList[0] = STalloc(En_sql_from);
		rfc_decompose(RelList[0],&rfc_ferslv,&rfc_error);
	}
    }
    else
    {
	/*
	** If we're here, we've got an explicit table name which was
	** passed on the command line, and we never went through the
	** above validations (yes, this routine is overloaded ...).
	** NumRels should never be more than one, but why take chances.
	*/
	for (i = 0; i < *NumRels; i++)
	{
		rfc_decompose(RelList[i],&rfc_ferslv,&rfc_error);
		if  (rfc_error)
		{
    			break;
		}
	}
    }

    MEfree((PTR)rfc_ferslv.name_dest);
    MEfree((PTR)rfc_ferslv.owner_dest);

    if  (rfc_error)
    {
    	return(FALSE);
    }

    return(TRUE);
}



/*
** Name:    rfNextQueryLine()
**
** Description:
**   	Allocates new space for a query section and 
**	attaches a new line to it.
**
** Parameters:
** 	section   -- The pointer to the query section.
** 	QueryLine -- Line of query to be added.
**
** Returns:
**    	None.
**
** Side Effects:
**    	section gets expanded.
**
** Called by:
**    	rFchk_dat().
**    	r_JDLoadQuery().
**    	rFm_data().
**    	rFqur_set().
**
** Error messages:
**    	None.
**
** Trace Flags:
**    	None.
**
** History:
**    1/9/90 (martym) 	- Created for RBF.
**    3/13/90 (martym)	- Changed to handle NULL strings.
*/

VOID rfNextQueryLine(section, QueryLine)
char **section;
char *QueryLine;
{

	char *TmpPtr;

	if (QueryLine == NULL || *QueryLine == EOS)
		return;

	if (*section == NULL)
		/*
		** Make it a null string so that STlength won't core dump:
		*/
		*section = STalloc(ERx(""));

	if ((TmpPtr = (char *)MEreqmem((u_i4)0,
			(u_i4)(STlength(*section) + STlength(QueryLine) +1), 
			TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("rfNextQueryLine"));
	}

	_VOID_ STcopy(*section, TmpPtr);
	_VOID_ STcat(TmpPtr, QueryLine);
    	_VOID_ MEfree((PTR) *section);
	*section = TmpPtr;

	return;

}



static
VOID
rfc_decompose(rel_list,rfc_ferslv,rfc_error)
	char		*rel_list;
	FE_RSLV_NAME	*rfc_ferslv;
    	bool		*rfc_error;
{
	bool		dmy;	/*
				** Dummy loop variable to avoid goto's and
				** explicit test of rfc_error every step of
				** the way.
				*/

	rfc_ferslv->name = rel_list;
	rfc_ferslv->is_nrml = FALSE;
	*rfc_error = FALSE;

	/*
	** Decompose any owner.tablename, validate the components,
	** and normalize them for later use in FE_resolve().  We don't
	** use FE_fullresolve() because it's embedded call to FE_resolve()
	** will lead us to believe that we always have an explicit owner
	** specification.
	*/

	FE_decompose(rfc_ferslv);

	dmy = TRUE;
	while (dmy)
	{
		dmy = FALSE;
		if  ((rfc_ferslv->owner_spec) &&
		     (((STcompare(IIUIcsl_CommonSQLLevel(),UI_LEVEL_65) < 0)
		     && IIUIdcd_dist() == FALSE)) 
		     || (IIUIdcd_dist() == TRUE &&
			(STcompare(IIUIcsl_CommonSQLLevel(),
		     UI_LEVEL_61) < 0)))
		{
			*rfc_error = TRUE;
			break;
		}
		if  ((IIUGdlm_ChkdlmBEobject(rfc_ferslv->name_dest,
					    rfc_ferslv->name_dest,
					    FALSE) == UI_BOGUS_ID) ||
		     ((rfc_ferslv->owner_spec) &&
		      ((IIUGdlm_ChkdlmBEobject(rfc_ferslv->owner_dest,
					      rfc_ferslv->owner_dest,
					      FALSE) == UI_BOGUS_ID) &&
		      STcompare(rfc_ferslv->owner_dest, UI_FE_CAT_ID_65))))
		{
			*rfc_error = TRUE;
			break;
		}
		if  (!FE_resolve(rfc_ferslv,rfc_ferslv->name_dest,
				 rfc_ferslv->owner_dest))
		{
			*rfc_error = TRUE;
			break;
		}
	}

	if  (*rfc_error)
	{
	    /* 
	    ** data relation does not exist, is not accessable,
	    ** or specified as owner.tablename for pre-6.5.
	    */
            if (En_rtype == RT_DEFAULT)
            {    
                /* 
                ** default report:
                */
                r_error(0x04,NONFATAL,rel_list,NULL);
            }
            else
            {    
                /* 
                ** report data table:
                */
                r_error(0x05,NONFATAL,rel_list,En_report,NULL);
            }
	}

	return;
}
