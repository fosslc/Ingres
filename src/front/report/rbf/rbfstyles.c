/*
** Copyright (c) 2004 Ingres Corporation
*/
# include    <compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include    <fe.h>
# include    <st.h>
# include    <rcons.h>
# include    <errw.h>
# include    "rbftype.h"
# include    "rbfglob.h"



/*
** Name:    rbfstyles.c - Set up a default template report style.
**
** Description:
**      This file contains the modules that RBF utilizes to set up 
**      default template report styles for the Indented and Labels
**      Styles.
**
** History:
**    11/2/89 (martym)    Written for RBF.
**    11/29/89 (martym)   Changed all calls to the sections list handling 
**            routines to follow the new interface.    
**    12/11/89 (martym)  Removed rbfstyle.h and included rcons.h.
**    2/7/90 (martym) Coding standards cleanup. Got rid of wrapping lines.
**    3/26/90 (martym) Fixed so that the value of X (character position going 
**            across the page) for indented reports is calculated correctlly.
**    4/3/90 (martym) Modified "r_ChkMaxX()" to use "IIRFwid_WideReport()".
**    4/30/90 (martym) Added "IIRFFmtTitles()".
**	17-aug-90 (sylviap)
**		Added IIRFctt_CenterTitle to center the report title for
**		indented and master/detail reports. (#31446)
**	19-sep-90 (sylviap)
**		Changed how indented reports are set up. Necessary because users
**		can set the sort sequence from the break columns popup, rather
**		than just selecting yes/no.
**	21-nov-90 (sylviap)
**		Checks for folding formats in the detail section. #34522, #34523
**	12/09/92 (dkh) - Added Choose Columns support for rbf.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**/



/*
** # defines:
*/
#define  R_MIN_Y_CNT  2



/*
** Functions declared in this file in order:
*/
bool r_IndentedSetUp();
bool r_LabelsSetUp();
bool r_InitFrameInfo();
bool r_InsertSection();
bool r_MoveTrimField();
bool r_PropagateYMods();
bool r_ChkMaxX();
bool r_SemiColLineUp();
VOID IIRFFmtTitles();




/*
** Imports:
*/
FUNC_EXTERN STATUS IIRFwid_WideReport();
FUNC_EXTERN char   *IIFDftFormatTitle();




/*
** Name:    r_IndentedSetUp - Set up a default Indented Style report.
**
** Description:
**         Sets up a default Indented Style report.
**
** Parameters:
**           None.
**
** Returns: 
**           TRUE if successful, FALSE if not.
**
** Called By:
**           rfedit().
**
** Trace Flags:
**           None.
**
** Side Effects:
**           Global data gets modified.
**
** History:
**         11/2/89 (martym)    Written for RBF.
**	30-oct-1992 (rdrane)
**		Ensure that we ignore unsupported datatypes (of which there
**		shouldn't be any).

**
*/
bool r_IndentedSetUp()
{

    i4  i = 0;
    i4  j = 0;
    TRIM *t;
    CS *cs;
    COPT *copt;
    Sec_node *n = (Sec_node *)NULL;
    i4  PointerY = 0;
    i4  PointerX = 0;
    register ATT *att;
    register LIST *list;
    Sec_node *TmpNode = (Sec_node *)NULL;
    Sec_node *LastSec = (Sec_node *)NULL;
    i4  rows = 0;
    i4  cols = 0;
    i4  MaxX1st = 0;
    i4  sort_seq[DB_GW2_MAX_COLS + 1];		
    bool fold_fmt = FALSE;


    /*
    ** Let the user choose the field to break on:
    */
    for (i=0; i <= DB_GW2_MAX_COLS; i++)
	sort_seq[i] = 0;

    if (!r_IndOpts(sort_seq))
        return(FALSE);

    if (!r_InitFrameInfo(SEC_DETAIL, 0, &PointerY, &PointerX))
        return(FALSE);

    /*
    ** Insert all newlly created sections, adjusting things as we go along:
    */
    for (j = 1; j <= DB_GW2_MAX_COLS; j++)
    {
	/* 
	** Check for non-zero sort sequence.  sort_seq has the index to the
	** column that has a sort sequence set.
	*/

	if ((i = sort_seq[j]) != 0)
	{
        	if  (((att = r_gt_att(i)) == (ATT *)NULL) || att->pre_deleted)
		{
			continue;
		};
        	copt = rFgt_copt(i);

            	n = sec_list_find(SEC_BRKHDR, copt->copt_sequence, &Sections);
            	LastSec = n;

            	if (!r_PropagateYMods(PointerY, R_SEGMENT_SIZE))
                	return(FALSE);

            	if (!r_InsertSection(n, &PointerY))
                	return(FALSE);

            	if (!r_MoveTrimField(att, n, &PointerY, &PointerX, R_TRIM_ONLY,
			R_X_Y_POS, TRUE, FALSE))
                	return(FALSE);

            	if (!r_MoveTrimField(att, n, &PointerY, &PointerX, R_FIELD_ONLY,
			R_X_Y_POS, FALSE, TRUE))
                	return(FALSE);
            
	    	fmt_size(Adf_scb, att->att_format, NULL, &rows, &cols);

        }
    }

    MaxX1st = PointerX + cols;

    /*
    ** Find out the last inserted section and move all of the column 
    ** headings from the Detail Section into that section:
    */
    for (i = 1; i <= En_n_attribs; i++)
    {
        if  (((att = r_gt_att(i)) == (ATT *)NULL) || att->pre_deleted)
	{
		continue;
	}
        copt = rFgt_copt(i);

        if (copt->copt_brkhdr == 'y')
        {
            n = sec_list_find(SEC_BRKHDR, copt->copt_sequence, &Sections);

            if (n == LastSec)
            {
                if (!r_PropagateYMods(n->sec_y+n->sec_y_count+1, 1))
                    return(FALSE);
                (n->sec_y_count) ++;

                TmpNode = sec_list_find(SEC_DETAIL, 0, &Sections);
                TmpNode->sec_y_count = 1;

                for (j = 1; j <= En_n_attribs; j++)
                {
        	    if  (((att = r_gt_att(j)) == (ATT *)NULL) ||
			att->pre_deleted)
		    {
			continue;
		    }
                    copt = rFgt_copt(j);
                    if (copt->copt_brkhdr != 'y')
                    {
                        if (!r_MoveTrimField(att, n, &PointerY, &PointerX, 
                                R_TRIM_ONLY, R_X_Y_POS, FALSE, FALSE))
                            return(FALSE);

                        if (!r_MoveTrimField(att, TmpNode, &PointerY, 
                                &PointerX, R_FIELD_ONLY, R_X_Y_POS, TRUE, 
                                FALSE))
                            return(FALSE);
                    }
                }
            }
        }
	else if ((att->att_format->fmt_type == F_CF) || 
		(att->att_format->fmt_type == F_CFE))
	{
		/* folding format in the detail section #34522, #34523 */
		fold_fmt = TRUE;
	}
    }

    PointerX -= SPC_DFLT;

    if (PointerX < MaxX1st)
	PointerX = MaxX1st;

    if (r_ChkMaxX(PointerX) == FALSE)
       return(FALSE);

    /*
    ** Since we started with a block style report, take the semicolons 
    ** out of the trim of non-indented fields:
    */
    for (i = 1; i <= En_n_attribs; i++)
    {
        copt = rFgt_copt(i);
        cs = rFgt_cs(i);
        if (cs == NULL)
            continue;

        if (copt->copt_brkhdr != 'y')
            for (list = cs->cs_tlist; list != NULL; list = list->lt_next)
            {
                if ((t = list->lt_trim) == NULL)
                    continue;

                if (t->trmstr)
                    t->trmstr[STlength(t->trmstr)-1] = EOS;
            }
    }

    /*
    ** Take out the extra blank lines:
    */

    if (fold_fmt == TRUE)
	/* need extra row in detail section for folding format #34522, #34523*/
	rows = 1;
    else
	rows = 0;

    n = sec_list_find(SEC_DETAIL, 0, &Sections);
    while (LastSec->sec_y+LastSec->sec_y_count+R_MIN_Y_CNT < n->sec_y)
        if (!r_PropagateYMods(LastSec->sec_y+LastSec->sec_y_count+R_MIN_Y_CNT, 
                -1))
            return(FALSE);

    LastSec = n;
    n = sec_list_find(SEC_END, 0, &Sections);
    while (LastSec->sec_y+LastSec->sec_y_count+R_MIN_Y_CNT+rows < n->sec_y)
        if (!r_PropagateYMods(LastSec->sec_y+LastSec->sec_y_count+R_MIN_Y_CNT,
                -1))
            return(FALSE);

    /* Center title based on the new report format/right margin (#31446) */ 
    IIRFctt_CenterTitle(PointerX);	
    return(TRUE);

}






/*
** Name:    r_LabelsSetUp - Set up a default Labels Style report.
**
** Description:
**         Sets up a default Labels Style report.
**
** Parameters:
**           None.
**
** Returns: 
**           TRUE if successful, FALSE if not.
**
** Called By:
**           rfedit().
**
** Trace Flags:
**           None.
**
** Side Effects:
**           Global data gets modified.
**
** History:
**         11/2/89 (martym)    Written for RBF.
**
*/
bool r_LabelsSetUp()
{

    Sec_node *n = (Sec_node *)NULL;
    i4  PointerY = 0;
    i4  PointerX = 0;

    /*
    ** Create a report footer section, if we don't have one yet:
    */
    if ((n = sec_list_find(SEC_REPFTR, 0, &Sections)) == (Sec_node *)NULL)
    {
        if (!r_InitFrameInfo(SEC_END, 0, &PointerY, &PointerX))
            return(FALSE);

        if (!r_PropagateYMods(PointerY, R_SEGMENT_SIZE))
            return(FALSE);

        n = sec_node_alloc(SEC_REPFTR, 0, PointerY);
        if (n == (Sec_node *)NULL)
            return(FALSE);

        n->sec_name = ERget(F_RF0039_Report_Footer);
        sec_list_insert(n, &Sections);

        if (!r_InsertSection(n, &PointerY))
            return(FALSE);
    }

    return(TRUE);

}






/*
** Name:    r_InitFrameInfo - Initialize frame pointers.
**
** Description:
**         Initialize the frame pointers to point to a given section.
**
** Parameters:
**           section  -- A section of report.
**           seq      -- Sort sequence.
**           Y        -- Y position.
**           X        -- X position.
**
** Returns: 
**           TRUE if successful, FALSE if not.
**           Initialized X & Y positions.
**
** Called By:
**           r_IndentedSetUp().
**           r_LabelsSetUp().
**
** Trace Flags:
**           None.
**
** Side Effects:
**           Global data gets modified.
**
** History:
**         11/2/89 (martym)    Written for RBF.
*/
bool r_InitFrameInfo(section, seq, Y, X)
i4  section;
i4  seq;
i4  *Y;
i4  *X;
{

    Sec_node *n = (Sec_node *)NULL;

    n = sec_list_find(section, seq, &Sections);
    if (n == (Sec_node *)NULL)
        return(FALSE);

    *Y = n->sec_y;
    *X = 0;

    return(TRUE);

}





/*
** Name:    r_InsertSection - Insert a new section into the form.
**
** Description:
**          Inserts a new section into RBF's current field definition.
**            The section should nomarly be 'Break-Header' section.
**             This routine will insert in between the 'Report-Header' and 
**          the 'Detail' sections.
**
** Parameters:
**           n  -- Section to be inserted.
**           Y  -- The begining Y position of the Section.
**
** Returns: 
**           TRUE if successful, FALSE if not.
**           The Adjusted Y position.
**
** Called By:
**           r_IndentedSetUp().
**           r_LabelsSetUp().
**
** Trace Flags:
**           None.
**
** Side Effects:
**           Global data gets modified.
**
** History:
**         11/2/89 (martym)    Written for RBF.
*/
bool r_InsertSection(n, Y)
Sec_node *n;
i4  *Y;
{

    n->sec_y = *Y;
    n->sec_y_count = R_MIN_Y_CNT;
    (*Y) += R_SEGMENT_SIZE;

    return(TRUE);

}





/*
** Name:    r_MoveTrimField - Move column info to a given section.
**
** Description:
**         Given a section and a column ATT struct, adjust the 
**         CS structs to move the column's trim (heading) and/or 
**         field into a given section.
**
** Parameters:
**           att  -- The attribute of the item(s) to move.
**           n    -- The section to move the item(s) to.
**           Y    -- The Y position.
**           X    -- The X position.
**           ItemsToMove -- TRIM, FIELD or BOTH.
**           PosToMove   -- Move X pos, Y pos, or BOTH.
**           advX        -- Advance the X position or not.
**           advY        -- Advance the Y position or not.
**
** Returns: 
**           TRUE if successful, FALSE if not.
**           Adjusted X & Y positions.
**
** Called By:
**           r_IndentedSetUp().
**           r_MasterDetailSetUp().
**
** Trace Flags:
**           None.
**
** Side Effects:
**           Global data gets modified.
**
** History:
**         11/2/89 (martym)    Written for RBF.
*/
bool r_MoveTrimField(att, n, Y, X, ItemsToMove, PosToMove, advX, advY)
ATT *att;
Sec_node *n;
i4  *Y;
i4  *X;
i4  ItemsToMove;
i4  PosToMove;
bool advX;
bool advY;
{

    CS *cs;
    i4  i = 0;
    FIELD *f;
    TRIM *t;
    register LIST *list;
    i4  TmpPosY = 0;
    i4  rows = 0;
    i4  cols = 0;
    i4  len = 0;

    for (i = 1; i <= En_n_attribs; i++)
    {
        cs = rFgt_cs(i);
        if (cs == NULL)
            continue;

        if (STcompare(cs->cs_name, att->att_name) == 0)
        {

            for (list = cs->cs_tlist; list != NULL; list = list->lt_next)
            {
                if ((t = list->lt_trim) == NULL)
                    continue;
                break;
            }

            for (list = cs->cs_flist; list != NULL; list = list->lt_next)
            {
                if ((f = list->lt_field) == NULL)
                    continue;
                break;
            }

            if (ItemsToMove == R_TRIM_ONLY || ItemsToMove == R_TRIM_FIELD)
            {
                if (PosToMove == R_Y_POS_ONLY || PosToMove == R_X_Y_POS)
                {
                    TmpPosY = n->sec_y + n->sec_y_count + 1;
                    if (!r_PropagateYMods(TmpPosY, 1))
                        return(FALSE);
                    t->trmy = n->sec_y + n->sec_y_count;
                }

                if (PosToMove == R_X_POS_ONLY || PosToMove == R_X_Y_POS)
                {
                    t->trmx = *X;

                    if (advX)
		    {
                        (*X) += (STlength(t->trmstr) + 1);
		    }
                }
            }

            if (ItemsToMove == R_FIELD_ONLY || ItemsToMove == R_TRIM_FIELD)
            {
                if (PosToMove == R_Y_POS_ONLY || PosToMove == R_X_Y_POS)
                {
                    TmpPosY = n->sec_y + n->sec_y_count + f->flmaxy;

                    if (!r_PropagateYMods(TmpPosY, f->flmaxy))
                        return(FALSE);

                    f->flposy = n->sec_y + n->sec_y_count;
                }

                if (PosToMove == R_X_POS_ONLY || PosToMove == R_X_Y_POS)
                {
                    f->flposx = *X;

                    if (advX)
		    {
			fmt_size(Adf_scb, att->att_format, NULL, 
					&rows, &cols);

			if (ItemsToMove == R_TRIM_FIELD)
			{
			    len = cols;
			}
			else	/* R_FIELD_ONLY */
			{
			    len = (cols > STlength(t->trmstr)) ? 
					cols : STlength(t->trmstr); 

			    len += SPC_DFLT;
			}

			(*X) += len;
		    }
                }
            }
    
            if (advY)
            {
                /*
                ** When advancing the Y position we must also expand 
                ** the current section:
                */
                (n->sec_y_count) += (f->flmaxy);
                (*Y) += f->flmaxy;
            }

            break;

        }
    }

    return(TRUE);

}




/*
** Name:    r_PropagateYMods - Propagate changes to the Y positions.
**
** Description:
**        This module will propagate changes of the Y position to all 
**        the form and special trim segments.
**
** Parameters:
**           from  -- Starting Y position.
**           by    -- Number to increment/decrement by.
**
** Returns: 
**           TRUE if successful, FALSE if not.
**
** Called By:
**           r_IndentedSetUp().
**
** Trace Flags:
**           None.
**
** Side Effects:
**           Global data gets modified.
**
** History:
**         11/2/89 (martym)    Written for RBF.
*/
bool r_PropagateYMods(from, by)
i4  from;
i4  by;
{

    i4  i = 0;
    FIELD *f;
    TRIM *t;
    Sec_node *n;
    register LIST *list;
    CS *cs;


    for (i = 1; i <= En_n_attribs; i++)
    {
        cs = rFgt_cs(i);
        if (cs == NULL)
            continue;

        /*
        ** Adjust all of the field  'Y'  positions:
        */
        for (list = cs->cs_flist; list != NULL; list = list->lt_next)
        {
            if ((f = list->lt_field) == NULL)
                continue;
            if (f->flposy >= from)
                f->flposy += by;
        }

        /*
        ** Adjust all of the trim  'Y'  positions:
        */
        for (list = cs->cs_tlist; list != NULL; list = list->lt_next)
        {
            if ((t = list->lt_trim) == NULL)
                continue;
            if (t->trmy >= from)
                t->trmy += by;
        }
    }
    
    /*
    ** Adjust all of the special trim (sections) 'Y'  positions:
    */
    for (n = Sections. head; n != NULL; n = n->next)
    {
        if (n->sec_y == 0)
            continue;

        if (n->sec_y >= from)
            (n->sec_y) += by;
    }

    Cury += by;

    return(TRUE);

}




/*
** Name:    r_ChkMaxX - Checks to make sure we haven't gone beyond Max X.
**                    
**
** Description:
**        This module will check the current max X against the number allowed 
**        En_lmax, and if we have exceeded it will put up a message.
**
** Parameters:
**           CurrX  -- Current Max X.
**
** Returns: 
**           TRUE if we haven't exceeded, FALSE if we have.
**
** Called By:
**           r_IndentedSetUp().
**           r_JDMasterDetailSetUp().
**
** Trace Flags:
**           None.
**
** Side Effects:
**           None.
**
** History:
**         1/11/90 (martym)    Written for RBF.
*/
bool r_ChkMaxX(CurrX)
i4  CurrX;
{

    if (!St_lspec)
	return(TRUE);

    if (CurrX > En_lmax)
    {
    	if (IIRFwid_WideReport(CurrX, En_lmax) != OK)
		return(FALSE);
    }

    return(TRUE);

}




/*
** Name:    r_SemiColLineUp - Line up the semicolons in the report.
**                    
**
** Description:
**        This module will line up the semicolons in the report layout.
**
** Parameters:
**           None.
**
** Returns: 
**           TRUE if successful, FALSE if exceeded Max X.
**
** Called By:
**           r_JDMasterDetailSetUp().
**
** Trace Flags:
**           None.
**
** Side Effects:
**           None.
**
** History:
**         1/11/90 (martym)    Written for RBF.
**	30-oct-1992 (rdrane)
**		Ensure that we ignore unsupported datatypes.
*/
bool r_SemiColLineUp()
{

    i4  i = 0;
    i4  LargField = 0;
    CS *cs;
    TRIM *t;
    FIELD *f;
    ATT *att;
    register LIST *list;
    i4  rows = 0;
    i4  cols = 0;

    LargField = 0;
    for (i = 1; i <= En_n_attribs; i++)
    {
        cs = rFgt_cs(i);
        if (cs == (CS *)NULL)
            continue;

        for (list = cs->cs_tlist; list != NULL; list = list->lt_next)
        {
            if ((t = list->lt_trim) == NULL)
                continue;
            break;
        }

        if (STindex(t->trmstr, ERx(":"), 0) == NULL)
            continue;

        if (STlength(t->trmstr) > LargField)
            LargField = STlength(t->trmstr);
    }

    for (i = 1; i <= En_n_attribs; i++)
    {
        cs = rFgt_cs(i);
        att = r_gt_att(i);
        if ((cs == (CS *)NULL) || (att == (ATT *)NULL) || att->pre_deleted)
	{
            continue;
	}

        for (list = cs->cs_tlist; list != NULL; list = list->lt_next)
        {
            if ((t = list->lt_trim) == NULL)
                continue;
            break;
        }

        if (STindex(t->trmstr, ERx(":"), 0) == NULL)
            continue;

        if (STlength(t->trmstr) < LargField)
        {
            for (list = cs->cs_flist; list != NULL; list = list->lt_next)
            {
                if ((f = list->lt_field) == NULL)
                    continue;
                break;
            }

            t->trmx += (LargField - STlength(t->trmstr));

            f->flposx += (LargField - STlength(t->trmstr));

	    fmt_size(Adf_scb, att->att_format, NULL, &rows, &cols);

        }
    }

    return(TRUE);

}





/*
** Name:    IIRFFmtTitles - Convert field titles to conform with other
**			    frontend tools.
**                    
**
** Description:
**        This module will call "IIFDftFormatTitle()" to format each 
**	  column title.
**
** Parameters:
**           None.
**
** Returns: 
**           None. 
**
** Called By:
**           rfedit().
**
** Trace Flags:
**           None.
**
** Side Effects:
**           None.
**
** History:
**         4/30/90 (martym)    Written for RBF.
*/
VOID IIRFFmtTitles()
{

    i4  i = 0;
    CS *cs = NULL;
    TRIM *t = NULL;
    LIST *list = NULL;

    for (i = 1; i <= En_n_attribs; i++)
    {
        cs = rFgt_cs(i);
        if (cs == NULL)
            continue;

	t = NULL;
        for (list = cs->cs_tlist; list != NULL; list = list->lt_next)
        {
            if ((t = list->lt_trim) == NULL)
                continue;
            break;
        }

	if ((list != NULL) && (t != NULL))
	{
        	_VOID_ IIFDftFormatTitle(t->trmstr);
	}

    }

    return;

}

/*
** Name:    IIRFctt_CenterTitle
**
** Description:
**     	Centers the report title based on the right margin.  Since this should
**	only be called when creating a report, it assumes the first trim in 
**	Other, is the report title (unassociated trim).  
**
** Called by: 
**	r_IndentedSetUp
**	r_JDMasterDetailSetUp
**
*/

VOID
IIRFctt_CenterTitle(right_margin)
i4	right_margin;
{
	TRIM    *tr;
	i4	width;
	i4	tmp_pos;

	/* Get the report title */
	tr = (TRIM *) Other.cs_tlist->lt_trim;
	width = STlength (tr->trmstr);

	/* 
	** Only center if a valid position, otherwise leave the title at 
	** the default position that RW (r_m_rtitle) figured out.
	*/
	if ((tmp_pos = (right_margin/2) - (width/2)) >= 0)
		tr->trmx = tmp_pos;
}
