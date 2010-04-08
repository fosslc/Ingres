/*
** Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include    <fe.h>
# include    <ug.h>
# include    <st.h>
# include    "rbftype.h"
# include    "rbfglob.h"


# define	RF_TRIM_OBJ 	0
# define	RF_FIELD_OBJ 	1
# define 	RF_TRIM_LIST 	0
# define	RF_FIELD_LIST	1
# define 	RF_SORT_BY_XY 	0
# define	RF_SORT_BY_Y	1

 
/*
**   r_GenLabDetSec - Generate the ReportWriter commands for the 
**                    detail section of a labels style report, as well 
**                    as calculating its dimensions (such as the maxi-
**                    mum number of labels per line, etc.).
**
**	This routine assumes:
**		1. Label report cannot have column headings, so ALL trim is
**		   unassociated trim.
**		2. In a detail section, there is NO omilinear blocks, since
**		   blocks are used in printing the labels across the report.
**		3. Aggregates are only found in footers, not in detail section.
**
**	If any of these assumptions change, r_GenLabDetSec will also need to 
**	be modified.
**
**    Parameters:
**      tline   - top line of section.
**      bline   - bottom line of section.
**      utype   - type of underlining.
**                ULS_NONE - none.
**                ULS_LAST - last line only.
**                ULS_ALL - all.
**      GenCode - if TRUE code is generated, otherwise only 
**                the dimensions are calculated.
**
**    Returns:
**        none.
**
**    Called by:
**        rFsave()
**
**    Side Effects:
**        Global struct LabelInfo gets updated.
**
**    Trace Flags:
**        none.
**
**    History:
**        12/12/89 (martym)  written for RBF.
**	  2/20/90 (martym)   Changed to return STATUS passed back by the 
**			     SREPORTLIB routines.
**	  5/2/90 (martym)    Changed the skeleton of labels reports to 
**			     accomidate wrapping fields.
**	  5/7/90 (martym)    Modified to use sorted lists.
**	06-jun-90 (cmr)
**			Make this routine consistent with rFm_sec().
**			Put back in support for the B format.
**			Added MapToAggNum() to calculate the correct rbf agg #;
**			used to index into the Cs_top array on edit.
**	21-jul-90 (cmr) fix for bug 31731
**			[Making this routine consistent with rFm_sec()]
**			[These routines should be merged!]
**			Make underlining work properly.  Make maxline a single
**			i4 (there's no need for an array).  Also, minline
**			removed - not needed.
**	06-aug-90 (sylviap) 
**		Generates new code for label reports so the detail block is
**		closed at the beginning of the page hdr, break hdr, break
**		footer, page footer and report footer. (#32085)
**			
**	16-aug-90 (sylviap) 
**		Takes the trim into consideration when figuring out how wide
**		a single label is (LabelSize). (#32132)
**      29-aug-90 (sylviap)
**              Added the parameter, right_margin. #32701.
**      14-sep-90 (sylviap)
**		Took out unneccessary code. Changed IIRFPrntTrmFld in the
**		manner it decides what to print next. (#32705)
**	08-oct-90 (sylviap)
**		Fixed bug #33771.  Puts extra newline at the end of a detail
**		section.  Problem was CurY was getting reset back to CurTop
**		so lost track of which line it was on.
**	30-oct-1992 (rdrane)
**		Ensure that unsupported datatype fields cannot be rendered.
**		Ensure that all string constants are generated with single,
**		not double quotes.  Remove declaration of s_w_action() since
**		already declared in included hdr files.
**	15-jan-1993 (rdrane)
**		Generate the attribute name as unnormalized.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-Mar-2003 (hanje04)
**	    Cast last '0' in calls to s_w_action() with (char *) so that 
**	    the pointer is completely nulled out on 64bit platforms.
*/


FUNC_EXTERN VOID   	IIUGqsort();
STATUS 			IIRFPrntTrmFld();
VOID 			IIRFTrmFldSort();
i4  			IIRFCmpXYField();
i4  			IIRFCmpYField();
i4  			IIRFCmpXYTrim();
i4  			IIRFCmpYTrim();


STATUS r_GenLabDetSec(tline, bline, utype, GenCode, right_margin)
i4  tline;
i4  bline;
char utype;
bool GenCode;
i4   right_margin;
{

    i4  i;
    i4  trmi, fldi, ord;
    LIST *TrimList = NULL; 
    LIST *FieldList = NULL; 
    FLDHDR *FDgethdr();
    ATT *att;      
    FIELD *f;                       /* field                                 */
    TRIM *t;                        /* trim ptr                              */
    i4  nskip;                      /* number of lines to skip               */
    i4  maxline; 		/* largest line in section  for each att */
    LIST *curf[DB_GW2_MAX_COLS+1];  /* current field for each section        */
    LIST *curt[DB_GW2_MAX_COLS+1];  /* current trim for each section         */
    char trim[2*fdTRIMLEN + 1];     /* trim with quotes and backslashes      */
    char ibuf[20];
    i4  CurX, CurY, CurTop;
    i4  LabelSize;
    STATUS stat = OK;
    STATUS prntstat = OK;
    i4  rows, cols;
    i4  newlnsreq = 0;
    char agindex[16];
    bool TrimDone, FieldDone;
    i4  FirstLn;
    i4  TmpY;
    i4  tmplen, RightField, RightTrim;
    char tmp_buf1[(FE_UNRML_MAXNAME + 1)];

    

    rFfld_rng(&maxline,&curf[0],&curt[0],tline,bline);

    /* 
    ** underline everything:
    */
    if ((utype == ULS_ALL) && (GenCode))
    	RF_STAT_CHK(s_w_action(ERx("underline"),(char *)0));

    if (GenCode)
    {
	/* Open the block if not already done so */
	RF_STAT_CHK(s_w_action(ERx("if"), ERx("$"),
		ERx(LABST_IN_BLK), ERx("='no'"),(char *)0));

	RF_STAT_CHK(s_w_action(ERx("then"), (char *)0));
	RF_STAT_CHK(s_w_action(ERx("let"), ERx(LABST_IN_BLK),
		ERx("='yes'"),(char *)0));
	RF_STAT_CHK(s_w_action(ERx("endlet"), (char *)0));
	RF_STAT_CHK(s_w_action(ERx("begin"), ERx("block"),(char *)0));
	RF_STAT_CHK(s_w_action(ERx("endif"), (char *)0));

        RF_STAT_CHK(s_w_action(ERx("top"),(char *)0));
    }

    TmpY = CurY = 0;
    FirstLn = CurTop = (tline + 1);
    nskip = CurX = LabelSize = RightTrim = RightField = 0;

    if (GenCode)
    {
    	for (i = 0; i <= Cs_length; i++)
    	{
		IIRFTrmFldSort(RF_TRIM_LIST, RF_SORT_BY_XY, &curt[i]);
		IIRFTrmFldSort(RF_FIELD_LIST, RF_SORT_BY_XY, &curf[i]);
    	}
    }

    while(CurTop < bline)
    {

        /* 
        ** write out the newline commands:
        */
        if (nskip > 0)
        {    
            if (GenCode)
	    {
	    	if (newlnsreq > 0)
		    nskip += (newlnsreq - 1);
            	CVna(nskip, ibuf);
                RF_STAT_CHK(s_w_action(ERx("newline"), ibuf, (char *)0));
	    }
            nskip = newlnsreq = 0;
        }

        /* 
        ** Go through the CS linked lists, both associated
        ** and unassociated.
        */

        /* 
        ** Next attribute, (if 0, unassociated):
        */
	trmi = fldi = 0;

	/* Reset CurY OUTSIDE the while loop.  #33771 */
        CurY = CurTop;
	while (trmi <= Cs_length && fldi <= Cs_length)
        {    
	    if (TmpY <= CurY)
		TmpY = CurY;

	    if (trmi <= Cs_length)
            	TrimList = curt [trmi]; 
	    if (fldi <= Cs_length)
            	FieldList = curf [fldi]; 
	    TrimDone = FieldDone = FALSE;

	    /*
	    ** While we still have trim and field to process for 
	    ** this section:
	    */
	    while (!TrimDone || !FieldDone)
	    {

            	/* 
            	** scan the trim structures: 
            	*/
            	while (TrimList != NULL)
            	{    
                	t = TrimList->lt_trim;

			/*
			** no more in this list:
			*/
                	if (t->trmy > bline)
                	{    
                    		/* 
                    		** no more in section:
                    		*/
                        	curt[trmi] = NULL;
		    		TrimDone = TRUE;
                    		break;
                	}

			/* 
			** Should this trim be printed?  Since there are
			** no blocks in the detail section, RBF orders the
			** trim and fields in the way they should be 
			** sequentially printed.
			*/
			prntstat = IIRFPrntTrmFld(RF_TRIM_OBJ, 
					&TrimList, &FieldList, &trmi, 
					&fldi, curt, curf, FieldDone);

			/* If field should be printed first, then break out */
			if (prntstat != OK)
				break;

			/*
                	** go to a new line:
                	*/

			/* Figure out the rightmost trim position */
			if ((tmplen=t->trmx+STlength(t->trmstr)) > RightTrim)
				RightTrim = tmplen;

                	if (t->trmy > CurY)
                	{
				/* 
				** Trim is not on the same line, so need 
				** NEWLINE command
				*/
                    		if (GenCode)
                    		{
                    			CVna((t->trmy - CurY), ibuf);
                        		RF_STAT_CHK(s_w_action(
						ERx("newline"), 
						ibuf, (char *)0));
                    		}   
                    		CurY = t->trmy;
                    		CurX = 0;
                	}

			if (CurX < t->trmx)
			{
				/* 
				** Trim is not at current position (CurX), so 
				** need TAB command
				*/
				if (GenCode)
				{
					CVna(t->trmx - CurX, ibuf);
					RF_STAT_CHK(s_w_action(ERx("tab"), 
						ERx("+"), ibuf, (char *)0));
				}
				CurX = t->trmx;
			}

			/* Check if this should be underlined */
                	if ((utype==ULS_LAST) && (CurY >= maxline) && (GenCode))
                       		RF_STAT_CHK(s_w_action(ERx("underline"),(char *)0));

                	rF_bstrcpy(trim, t->trmstr);

			/* Generate the print statement */
                	if (GenCode)
                	{
                    		RF_STAT_CHK(s_w_action(ERx("print"), 
					ERx("\'"), trim, ERx("\'"), (char *)0));
                    		RF_STAT_CHK(s_w_action(ERx("endprint"),(char *)0));
                	}

			/* Increment current position to AFTER trim */
                    	CurX += STlength(trim);

			/* End underlining if set */
                	if ((utype==ULS_LAST) && (CurY >= maxline) && (GenCode))
                       		RF_STAT_CHK(s_w_action(ERx("nounderline"),(char *)0));

		 	TrimList = TrimList->lt_next;
            	}

            	/* 
            	** no more in this section:
            	*/
            	if (TrimList == NULL)
            	{    
                	curt[trmi] = NULL;
			TrimDone = TRUE;
            	}

	
            	/* 
            	** Now go through the fields:
            	*/
	

            	/* 
            	** scan the field structures: 
            	*/
            	while (FieldList != NULL) 
            	{    
                	f = FieldList->lt_field;

                	/* 
                	** no more in this list:
                	*/
                	if (f->flposy > bline)
                	{    
                    		/* 
                    		** no more in section:
                    		*/
                        	curf[fldi] = NULL;
		    		FieldDone = TRUE;
                    		break;
                	}

			if (fldi > 0)
			{
				/*
				** Assume the worst, and fail for
				** unsupported datatypes.  But can we have a
				** FIELD structure for an unsupported datatype
				** in the first place?
				*/
				prntstat = FAIL;
				ord = r_mtch_att(f->fldname);
				att = r_gt_att(ord);
				if  (att == (ATT *)NULL)
				{
					break;
				}
				prntstat = IIRFPrntTrmFld(RF_FIELD_OBJ, 
					&TrimList, &FieldList, &trmi, &fldi, 
					curt, curf, FieldDone);
			}
			else
			{
				prntstat = OK;
			}

			if (prntstat != OK)
				break;

                	/* 
               		** go to a new line:
               		*/
               		if (f->flposy > CurY)
               		{    
               			if (GenCode)
	   			{
               				CVna((f->flposy - CurY), ibuf);
                      			RF_STAT_CHK(s_w_action(ERx("newline"), 
						ibuf, (char *)0));
	   			}

				if (newlnsreq > 0)
				{
					newlnsreq -= (f->flposy - CurY);
				}

                 		CurY = f->flposy;
                    		CurX = 0;
                	}


                	if (fldi <= 0)
                	{    
                    		CurX += f->flposx;
                	}
			else
			{
                    		if (f->flposx > att->att_position)
                    		{
                        		if (GenCode)
                        		{
                        			CVna((f->flposx - CurX), ibuf);
                            			RF_STAT_CHK(s_w_action( 
							ERx("tab"), ERx("+"), 
							ibuf, (char *)0));
                        		}
                        		CurX += (f->flposx - CurX);
                    		}

                    		if (CurX < att->att_position)
                    		{
                        		if (GenCode)
                        		{
                        			CVna((att->att_position-CurX), 
							ibuf);
                            			RF_STAT_CHK(s_w_action( 
							ERx("tab"), ERx("+"), 
							ibuf, (char *)0));
					}
                        		CurX += (att->att_position - CurX);
                    		}

			}


                	if ((utype==ULS_LAST) && (CurY >= maxline) && (GenCode))
                        	RF_STAT_CHK(s_w_action(ERx("underline"),(char *)0));

                   	if (GenCode)
		    	{
				_VOID_ IIUGxri_id(f->fldname,&tmp_buf1[0]);
                        	RF_STAT_CHK(s_w_action(
					ERx("print"),&tmp_buf1[0],(char *)0));
                       		RF_STAT_CHK(s_w_action(ERx("endprint"),(char *)0));
			}			

			/*
			** See if we are dealing with a wrapping format 
			** and if we are add up the NEWLINES needed to 
			** ensure that this column does not get written 
			** over:
			*/
	    	       	fmt_size(Adf_scb, att->att_format, NULL, 
				&rows, &cols);
	    	        if (rows == 0) 
			{
			  	rows = (att->att_value.db_length)/cols;
			   	if (((att->att_value.db_length)%cols) > 0)
					rows ++;
			    	rows -= 2;
			    	if (rows > 0)
			    		rows += 2;
			    	else
					rows = 2;
			}
			
			if (rows > newlnsreq)
				newlnsreq = rows;

                	CurX += f->fldataln;

			/* Figure out the rightmost field position */
			if (CurX > RightField)
				RightField = CurX;


                	if ((utype==ULS_LAST) && (CurY >= maxline) && (GenCode))
                        	RF_STAT_CHK(s_w_action(ERx("nounderline"),(char *)0));
				
			curf[fldi] = NULL;
			if (fldi < Cs_length)
			{
				fldi ++;
				FieldList = curf[fldi];
			}
			else
			{
				FieldList = NULL;
			}
            	}

            	/* 
            	** no more in this section:
            	*/
            	if (FieldList == NULL)
            	{    
                	curf[fldi] = NULL;
			FieldDone = TRUE;
            	}
	
	    }

	    if (TrimDone)
		trmi ++; 
	    if (FieldDone)
		fldi ++;

        }

        CurTop = CurY + 1;
        nskip ++;

	/* Compare both the trim and the field (#32132) */
	i = max(RightField, RightTrim);
        if (i > LabelSize)
            LabelSize = i;
        CurX = 0;
    }

    if (GenCode)
        /* 
        ** write out any buffered newline commands 
        */
        if (nskip > 0)
        {   
	    if (newlnsreq > 0)
		nskip += (newlnsreq - 1);
            CVna(nskip, ibuf);
            RF_STAT_CHK(s_w_action(ERx("newline"), ibuf, (char *)0));
        }


    /*
    ** Write out the standard commands needed for labels at the end 
    ** of the detail section:
    */
    if (GenCode)
    {
        RF_STAT_CHK(s_w_action(ERx("let"), ERx(LABST_LINECNT), ERx("="), 
                ERx("$"), ERx(LABST_LINECNT), ERx("+1"), (char *)0));

        RF_STAT_CHK(s_w_action(ERx("endlet"), (char *)0));

        RF_STAT_CHK(s_w_action(ERx("if"), ERx("$"), ERx(LABST_LINECNT), 
		ERx("="), ERx("$"), ERx(LABST_MAXPERLIN), (char *)0));

        RF_STAT_CHK(s_w_action(ERx("then"), (char *)0));
        RF_STAT_CHK(s_w_action(ERx("let"), ERx(LABST_LINECNT), ERx("=0"), (char *)0));
        RF_STAT_CHK(s_w_action(ERx("endlet"), (char *)0));

        CVna(LabelInfo. VerticalSpace, ibuf);

        RF_STAT_CHK(s_w_action(ERx("newline"), ibuf, (char *)0));
        RF_STAT_CHK(s_w_action(ERx("end"), ERx("block"), (char *)0));
        RF_STAT_CHK(s_w_action(ERx("lm"), ERx("0"), (char *)0));
        RF_STAT_CHK(s_w_action(ERx("let"), ERx(LABST_IN_BLK), 
		ERx("='no'"),(char *)0)); 

        RF_STAT_CHK(s_w_action(ERx("endlet"), (char *)0));
        RF_STAT_CHK(s_w_action(ERx("else"), (char *)0));

	RF_STAT_CHK(s_w_action(ERx("lm"), ERx("+($"), ERx(LABST_MAXFLDSIZ), 
		ERx(")"), (char *)0));

        RF_STAT_CHK(s_w_action(ERx("top"), (char *)0));
	RF_STAT_CHK(s_w_action(ERx("linestart"), (char *)0));
        RF_STAT_CHK(s_w_action(ERx("endif"), (char *)0));
    }

    if ((utype == ULS_ALL) && (GenCode))
        RF_STAT_CHK(s_w_action(ERx("end"), ERx("underline"), (char *)0));

    /* 
    ** Recalculate the Labels report dimensions:
    */

    LabelInfo. LineCnt = 0;
    LabelInfo. MaxFldSiz = LabelSize + LabelInfo. HorizSpace;  

    if (LabelInfo. MaxFldSiz > right_margin)
	right_margin = LabelInfo. MaxFldSiz;

    LabelInfo. MaxPerLin = (i4)right_margin / (i4)LabelInfo. MaxFldSiz;

    return(OK);
}



/*
**   IIRFPrntTrmFld - Checks to see if a TRIM or FIELD object can be 
**		      printed now or it has to wait for other objects 
**		      to be printed first.
**
**    Parameters:
**      objtyp   - The type of object. TRIM or FIELD.
**	Tlist    - The current list of trim.
**	Flist    - The current list of fields.
**	Tindex   - The current index of curt [].
**	Findex   - The current index of curf [].
**	curf	 - Array of FIELD lists.
**	curt     - Array of TRIM lists.
**	fdone    - If TRUE the field list for this block processed.
**
**    Returns:
**        OK   - If the object can be printed now.
**	  FAIL - If the object has to wait for some other object to 
**		 be printed 1st.
**
**    Called by:
**        r_GenLabDetSec().
**
**    Side Effects:
**        None.
**
**    Trace Flags:
**        none.
**
**    History:
**        5/7/90 (martym)  written for RBF.
**	16-aug-90 (sylviap)
**		Deleted parameters curx, cury, GenCode and tdone since they were
**		not used.
*/


STATUS IIRFPrntTrmFld(objtyp, TList, FList, Tindex, Findex, curt, curf, fdone)
i4  objtyp;
LIST **TList;
LIST **FList;
i4  *Tindex;
i4  *Findex;
LIST *curf[];
LIST *curt[];
bool fdone;
{

	TRIM *trm;
	FIELD *f;
	STATUS stat = OK;
    	LIST *tmplist;
	i4 i;



	if (objtyp == RF_TRIM_OBJ)
	{
		if (*TList != NULL)
        		trm = (*TList)->lt_trim;

		if ((*FList == NULL) && (*Findex <= 0))
		{
			for (i = 0; i <= Cs_length; i++)
			{
            			tmplist = curf[i]; 
				if (tmplist != NULL)
					break;
			}
				
			if (tmplist == NULL)
				return(OK);

                	f = tmplist->lt_field;
                		
			if ((f->flposy < trm->trmy) || 
			   	((f->flposy == trm->trmy) &&
				(f->flposx < trm->trmx)))
			{
				*Findex = i;
				*FList = tmplist;
				return(FAIL);
			}
			else
			{
				return(OK);
			}
		}
		else
		if (*FList != NULL)
		{
			f = (*FList)->lt_field;
			if ((f->flposy < trm->trmy) || 
			   	((f->flposy == trm->trmy) &&
				(f->flposx < trm->trmx)))
				return(FAIL);	
			else
				return(OK);
		}
		else
		{
			return(OK);
		}
	}
	else if (objtyp == RF_FIELD_OBJ)
	{
		if (*FList == NULL)
			return(FAIL);

		tmplist = NULL;
		trm = NULL;
		f = (*FList)->lt_field;
		tmplist = *TList;

		if (tmplist == NULL) 
		{
			return(OK);
		}
		else
		{
			trm = tmplist->lt_trim;
			if (trm == NULL)
				return (OK);
		}

		if ((f->flposy > trm->trmy) || 
			((f->flposy == trm->trmy) && (f->flposx > trm->trmx)))
		{
			return(FAIL);
		}
		return(OK);

	}
	else
	{
		return(FAIL);
	}

}




/*
**   IIRFTrmFldSort - Sort a list of TRIM or FIELD objects.
**
**    Parameters:
**        ListType - The type of list; TRIM or FIELD.
**	  SortBy   - The position to sort by, X or Y.
**	  list     - The list to be sorted.
**
**    Returns:
**        none.
**
**    Called by:
**        r_GenLabDetSec().
**
**    Side Effects:
**        The list gets sorted.
**
**    Trace Flags:
**        none.
**
**    History:
**        5/7/90 (martym)  written for RBF.
*/


VOID IIRFTrmFldSort(ListType, SortBy, list)
i4  ListType;
i4  SortBy;
LIST **list;
{

	TAGID memtag;
	LIST **Plist, *l;
	i4   i, numelements = 0;


	/*
	** Stuff an array of pointers with pointers the contents of 
	** of the linked list:
	*/
	for (numelements = 0, l = *list; l != NULL; l = l->lt_next)
		numelements ++;

	if (numelements < 2)
		return;

	memtag = FEgettag();
	if ((Plist = (LIST **)FEreqmem((u_i4)memtag, 
			(u_i4) ((numelements+1)*(sizeof(LIST *))),
			TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("IIRFTrmFldSort"));
	}
        for (i = 0, l = *list; i < numelements; i ++, l = l->lt_next)
		Plist [i] = l;


	/*
	** Sort the list:
	*/
	if (ListType == RF_TRIM_LIST)
	{
		if (SortBy == RF_SORT_BY_XY)
		{
			IIUGqsort((char *)Plist, numelements, sizeof(LIST *), 
				IIRFCmpXYTrim);
		}
		else
		{
			IIUGqsort((char *)Plist, numelements, sizeof(LIST *), 
				IIRFCmpYTrim);
		}
	}
	else
	if (ListType == RF_FIELD_LIST)
	{
		if (SortBy == RF_SORT_BY_XY)
		{
			IIUGqsort((char *)Plist, numelements, sizeof(LIST *), 
				IIRFCmpXYField);
		}
		else
		{
			IIUGqsort((char *)Plist, numelements, sizeof(LIST *), 
				IIRFCmpYField);
		}
	}
	else
	{
		/*
		** Non-sensical sort:
		*/
		FEfree(memtag);
		return;
	}

	/*
	** Re-arrange the linked list:
	*/
	for (i = 0, Plist [numelements] = NULL; i < numelements; i++)
	{
		l = Plist [i];
		l->lt_next = Plist [i+1];
		l = l->lt_next;
	}
	*list = Plist[0];

	FEfree(memtag);
	return;
}






/*
**   IIRFCmpXYField - Call back for the sorting routine to compare 
**		      two fields based on X and Y positions.
**
**    Parameters:
**        item1 - The 1st field.
**	  item2 - The 2nd field.
**
**    Returns:
**        -1 if item1 less than item2.
**	   0 if item1 same as item2.
**	   1 if item1 greater than item2.
**
**    Called by:
**        IIUGqsort().
**
**    Side Effects:
**        The list gets sorted.
**
**    Trace Flags:
**        none.
**
**    History:
**        5/7/90 (martym)  written for RBF.
*/


i4  IIRFCmpXYField(item1, item2)
char *item1;
char *item2;
{

	FIELD *f1 = (*(LIST **)item1)->lt_field;
	FIELD *f2 = (*(LIST **)item2)->lt_field;

	if (f1->flposy < f2->flposy)
		return(-1);
	else
	if (f1->flposy > f2->flposy)
		return(1);
	else
	{
		if (f1->flposx < f2->flposx)
			return(-1);
		else
		if (f1->flposx == f2->flposx)
			return(0);
		else
			return(1);
	}
}





/*
**   IIRFCmpYField - Call back for the sorting routine to compare 
**		     two fields based on Y positions.
**
**    Parameters:
**        item1 - The 1st field.
**	  item2 - The 2nd field.
**
**    Returns:
**        -1 if item1 less than item2.
**	   0 if item1 same as item2.
**	   1 if item1 greater than item2.
**
**    Called by:
**        IIUGqsort().
**
**    Side Effects:
**        The list gets sorted.
**
**    Trace Flags:
**        none.
**
**    History:
**        5/7/90 (martym)  written for RBF.
*/

i4  IIRFCmpYField(item1, item2)
char *item1;
char *item2;
{

	FIELD *f1 = (*(LIST **)item1)->lt_field;
	FIELD *f2 = (*(LIST **)item2)->lt_field;

	if (f1->flposy < f2->flposy)
		return(-1);
	else
	if (f1->flposy == f2->flposy)
		return(0);
	else
		return(1);
}





/*
**   IIRFCmpXYTrim -  Call back for the sorting routine to compare 
**		      two trim objects based on X and Y positions.
**
**    Parameters:
**        item1 - The 1st trim.
**	  item2 - The 2nd trim.
**
**    Returns:
**        -1 if item1 less than item2.
**	   0 if item1 same as item2.
**	   1 if item1 greater than item2.
**
**    Called by:
**        IIUGqsort().
**
**    Side Effects:
**        The list gets sorted.
**
**    Trace Flags:
**        none.
**
**    History:
**        5/7/90 (martym)  written for RBF.
*/

i4  IIRFCmpXYTrim(item1, item2)
char *item1;
char *item2;
{

	TRIM *t1 = (*(LIST **)item1)->lt_trim;
	TRIM *t2 = (*(LIST **)item2)->lt_trim;

	if (t1->trmy < t2->trmy)
		return(-1);
	else
	if (t1->trmy > t2->trmy)
		return(1);
	else
	{
		if (t1->trmx < t2->trmx)
			return(-1);
		else
		if (t1->trmx == t2->trmx)
			return(0);
		else
			return(1);
	}
}





/*
**   IIRFCmpYTrim -  Call back for the sorting routine to compare 
**		     two trim objects based on Y positions.
**
**    Parameters:
**        item1 - The 1st trim.
**	  item2 - The 2nd trim.
**
**    Returns:
**        -1 if item1 less than item2.
**	   0 if item1 same as item2.
**	   1 if item1 greater than item2.
**
**    Called by:
**        IIUGqsort().
**
**    Side Effects:
**        The list gets sorted.
**
**    Trace Flags:
**        none.
**
**    History:
**        5/7/90 (martym)  written for RBF.
*/

i4  IIRFCmpYTrim(item1, item2)
char *item1;
char *item2;
{

	TRIM *t1 = (*(LIST **)item1)->lt_trim;
	TRIM *t2 = (*(LIST **)item2)->lt_trim;

	if (t1->trmy < t2->trmy)
		return(-1);
	else
	if (t1->trmy == t2->trmy)
		return(0);
	else
		return(1);
}
