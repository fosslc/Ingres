/*
**    Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <st.h>		/* 6-x_PC_80x86 */
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include    <fe.h>
# include    <qg.h>    
# include    <mqtypes.h>
# include    <ug.h>
# include    <ui.h>
# include    "rbftype.h"
# include    "rbfglob.h"
# include    "rbfcons.h"
# include    <errw.h>

/**
** Name:    rfattdflt.c - fill array of ATT structures.
**
** Description:
**    This file defines:
**
**    rfattdflt    fill array of ATT structures.
**
** History:
**	04-sep-1987 (rdesmond)
**	16-dec-88 (sylviap)
**		Changed memory allocation to use FEreqmem.
**    	07/17/89 (martym) 
**		>> GARNET <<
**		Changed to pass in parameters to handle more than
**        	1 relation at a time.
**      9/22/89 (elein) UG changes
**              ingresug change #90045
**		changed <rbftype.h> and <rbfglob.h> to "rbftype.h" 
**		and "rbfglob.h"
**	1/8/89 (martym)
**		Added code to make sure an attribute is being used in 
**		the source of data is a JoinDef, and not allow duplicate 
**		attribute names.
**	2/7/90 (martym)
**		Coding standards cleanup.
**	2/15/90 (martym)
**		Changed to use r_JDMaintTabList() to handle cases of 
**		multiple occurrences of the same table name in JoinDef's.
**	11-jul-90 (sylviap)
**		Changed rf_att_dflt to use table resolution (first look for user
**		as the owner of the table, then the DBA) rather than assuming
**		the owner from En_dat_owner.  Currently rFchk_dat sets 
**		En_dat_owner.  The new behavior, as well as rFchk_dat setting
**		En_dat_owner will have to change when we start supporting
**		owner.table - especially in the case when a joindef may be based
**		on tables of different owners.
**	20-aug-90 (cmr) fix for bug 32152
**		Changed default position for att_position to 0 instead of -1.
**	31-aug-1992 (rdrane)
**		Fix-up parameterization of FEnumatts() and FErel_ffetch() for
**		6.5.  Remove declaration of FEtsalloc() - it's already in fe.h;
**	12/05/92 (dkh) - Added support choose columns support for rbf.
**	26-Aug-1993 (fredv) - included <me.h>.
**	13-sep-1993 (rdrane)
**		Put out the unsupported datatype message for any
**		disqualified columns whenever we initially set-up the
**		list.  Note that if ChooseColumns is in effect, then we've
**		already handled it there.
**      28-mar-1995 (newca01)
**              Added additional check for STAR at the 6.5 level--since 
**              STAR std catalog interface is at the 6.2 level, nothing 
**              works any more!!  B67724.   This will be fixedin Oping 2.0
**              when star and local catalogs are merged.
**	20-oct-1995 (stoli02/thaju02)
**	        bug #70910 - unable to create report based on table owned
**		by "$ingres".
**	02-mar-1996 (morayf)
**		Use of IIUIdcd_dist as a boolean variable is incorrect.
**		It is declared as a pointer to a boolean-returning function,
**		and hence needs to be _called_ not just referenced, as indeed
**		it is elsewhere.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */

/* extern's */

GLOBALREF	RFCOLDESC	*IIRFcollist;
GLOBALREF	i4		IIRFcccount;
GLOBALREF	i4		IIRFcdcount;

/* static's */

/*{
** Name:    rfattdflt - fill array of global ATT structures for base table.
**
** Description:
**
** Inputs:
**    NumRels - The number of relations passed in.
**    RelList - A list of relation names.
**
** Outputs:
**    none.
**
**    Returns:
**        TRUE
**
**    Exceptions:
**        none.
**
** Side Effects:
**
** History:
**    04-sep-1987 (rdesmond)
**        written.
**    07/17/89 (martym) Changed to pass in parameters to handle more than
**        1 relation at a time.
**    26-oct-1992 (rdrane)
**	Re-work building of the ATT array.  For unsupported datatypes,
**	do not alocate db_data buffers - leave them as NULL pointers.
**	This way, IIretdom() will effectively skip over the column.
**	Further, the NULL value will trigger other RW routines to
**	"ignore" the attribute.  Note that while it would seem
**	simpler to just not allocate the ATT in the first place,
**	it would actually create significant problems interfacing
**	with LIBQ for return/processing of the ROW_DESC.
**	Don't use r_gt_att() to find the ATT structure - at this point,
**	it will be all NULLs, assumed to be unsupported, and so
**	not return a valid pointer.
**	Propagate precision to result DB_DATA_VALUE.  Converted
**	r_error() err_num value to hex to facilitate lookup
**	in errw.msg file.  Fix-up IIUGbmaBadMemoryAllocation() ambiguities.
**    10_nov-1992 (rdrane)
**	Fix-up parameter list to r_JDMaintTabList().
**    2-dec-1992 (rdrane)
**	Correct parameterization of IIAFfedatatype().
**    22-sep-1993 (rdrane)
**	Correct parameterization of r_JDTabNameToRv() and r_JDMaintTabList(),
**	which changed to address bug 54949 in particular and 6.5
**	owner.tablename in general.
**    4-jan-1994 (rdrane)
**	Use OpenSQL level to determine 6.5 syntax features support.
**	25-Aug-2009 (kschendel) b121804
**	    Nothing uses the return value.  Make void.
*/

FUNC_EXTERN bool r_JDMaintAttrList();
FUNC_EXTERN bool r_JDMaintTabList();
FUNC_EXTERN char *r_JDTabNameToRv();


void
rf_att_dflt(i4 NumRels, char *RelList[])
{

    FE_ATT_QBLK        qblk;
    FE_ATT_INFO        attinfo;
    FE_REL_INFO        relinfo;
    register ATTRIB      id;
    i4         	       i = 0;
    bool               RepeatedField;
    char 	       ColName [FE_MAXNAME + 1];
    i4		       ins = 0;
    FE_RSLV_NAME	*rfa_ferslv[MQ_MAXRELS];
    RFCOLDESC		*coldesc;
    ATT			*attptr;
    char		*cname;
    ATTRIB		did;
    i4			real_cols;
    char		dsply_name[(FE_UNRML_MAXNAME + 1)];


    /*
    **  If the user went through the choose columns path, we will
    **  use that information to create the structures since we
    **  alredy have all the needed information.
    */
    if (IIRFcccount != 0)
    {
	En_n_attribs = IIRFcccount;
	real_cols = IIRFcccount - IIRFcdcount;

	/* Allocate memory for the ATT array. */
	if ((Ptr_att_top = (ATT *)FEreqmem((u_i4)Rst4_tag,
		(u_i4)(IIRFcccount*(sizeof(ATT))), TRUE, 
		(STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("rf_att_dflt - Ptr_att_top"));
	}

	/*
	**  Step through the set of column names.  Deleted ones are
	**  placed at the end of the array.  We need to keep them
	**  around so that the user can add them back in the layout
	**  frame.
	*/
	for (i = 0, id = 0, did = real_cols, coldesc = IIRFcollist;
		i < IIRFcccount; i++, coldesc++)
	{
		if (coldesc->deleted)
		{
			attptr = &(Ptr_att_top[did++]);
			attptr->pre_deleted = TRUE;
		}
		else
		{
			attptr = &(Ptr_att_top[id++]);
		}
		cname = coldesc->name;
		attptr->att_name = FEtsalloc(Rst4_tag, cname);

		/* check if attribute name is a keyword */

		if (!St_silent && (r_gt_pcode(cname) != PC_ERROR ||
			r_gt_wcode(cname) != A_ERROR ||
			r_gt_vcode(cname) != VC_NONE ||
			r_gt_cagg(cname) ||
			STcompare(cname, NAM_REPORT) == 0 ||
			STcompare(cname, NAM_PAGE) == 0 ||
			STcompare(cname, NAM_DETAIL) == 0))
		{
			IIUGmsg(ERget(S_RF0047_Column_name_keyword), 
				(bool) FALSE, 1, cname);
		}

		/* set up the data type field */
		attptr->att_value.db_datatype = coldesc->datatype;
		attptr->att_prev_val.db_datatype = coldesc->datatype;
		attptr->att_value.db_length = coldesc->length;
		attptr->att_prev_val.db_length = coldesc->length;
		attptr->att_value.db_prec = coldesc->prec;
		attptr->att_prev_val.db_prec = coldesc->prec;
    
		/*
		** Do not allocate value buffers for un-supported datatypes.
		** We already let the user know that they're being ignored
		** when we passed through ChooseColumns, so don't do it again!
		*/
		if  (!IIAFfedatatype(&attptr->att_value))
		{
			attptr->att_value.db_data = (PTR)NULL;
			attptr->att_prev_val.db_data = (PTR)NULL;
		}
		else
		{
			if ((attptr->att_value.db_data =
				(PTR)FEreqmem((u_i4)Rst4_tag,
				(u_i4)(attptr->att_value.db_length),
				TRUE, (STATUS *)NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(ERx("rf_att_dflt - db_data"));
			}
			if ((attptr->att_prev_val.db_data = 
				(PTR)FEreqmem((u_i4)Rst4_tag,
				(u_i4)(attptr->att_prev_val.db_length),
				TRUE, (STATUS *)NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(ERx("rf_att_dflt - prev db_data"));
			}
		}

		/* flag to indicate when done */
		attptr->att_position = 0;
		attptr->att_lac = NULL;
	}

	return;
    }


    /* 
    ** determine number of attributes in base table: 
    */
    for (i = 0; i < NumRels; i ++)
    {
	/*
	** Decompose any owner.tablename, validate the components,
	** and normalize them for later use.
	*/
	if  ((rfa_ferslv[i] = (FE_RSLV_NAME *)MEreqmem((u_i4)0,
		(u_i4)sizeof(FE_RSLV_NAME),TRUE,
		(STATUS *)NULL)) == (FE_RSLV_NAME *)NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("rf_att_dflt - rfa_ferslv"));
	}
	if  ((rfa_ferslv[i]->name_dest = (char *)MEreqmem((u_i4)0,
		(u_i4)(FE_UNRML_MAXNAME+1),TRUE,(STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("rf_att_dflt - name_dest"));
	}

	if  ((rfa_ferslv[i]->owner_dest = (char *)MEreqmem((u_i4)0,
		(u_i4)(FE_UNRML_MAXNAME+1),TRUE,(STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("rf_att_dflt - owner_dest"));
	}
	rfa_ferslv[i]->name = RelList[i];
	rfa_ferslv[i]->is_nrml = FALSE;
	FE_decompose(rfa_ferslv[i]);
	if  ((rfa_ferslv[i]->owner_spec) &&
	     (((STcompare(IIUIcsl_CommonSQLLevel(),UI_LEVEL_65) < 0)
	     && (IIUIdcd_dist() == FALSE)
	     || IIUIdcd_dist() == TRUE && (STcompare(IIUIcsl_CommonSQLLevel(), 
	     UI_LEVEL_61) < 0))))
	{
		continue;
	}
	if  ((IIUGdlm_ChkdlmBEobject(rfa_ferslv[i]->name_dest,
				    rfa_ferslv[i]->name_dest,
				    rfa_ferslv[i]->is_nrml) == UI_BOGUS_ID) ||
	     ((rfa_ferslv[i]->owner_spec) &&
	      ((IIUGdlm_ChkdlmBEobject(rfa_ferslv[i]->owner_dest,
				      rfa_ferslv[i]->owner_dest,
				      rfa_ferslv[i]->is_nrml) == UI_BOGUS_ID) &&
	      STcompare(rfa_ferslv[i]->owner_dest, UI_FE_CAT_ID_65))))
	{
		continue;
	}
	if  (FE_resolve(rfa_ferslv[i],rfa_ferslv[i]->name_dest,
			rfa_ferslv[i]->owner_dest))
	{
		En_n_attribs += FEnumatts(rfa_ferslv[i]->name_dest,
					  rfa_ferslv[i]->owner_dest);
	}
    }

    /* 
    ** Allocate array of ATT structures:
    */
    if ((Ptr_att_top = (ATT *)FEreqmem((u_i4)Rst4_tag,
        (u_i4)(En_n_attribs*(sizeof(ATT))), TRUE, 
        (STATUS *)NULL)) == NULL)
    {
        IIUGbmaBadMemoryAllocation(ERx("rf_att_dflt - Ptr_att_top"));
    }

    /*
    ** For each relation fill out the ATT structures:
    */
    id = 0;

    if (En_SrcTyp == JDRepSrc)
	_VOID_ r_JDMaintTabList(JD_TAB_INIT_INS_CNT, NULL, NULL, NULL, NULL);

    for (i = 0; i < NumRels; i ++)
    {
        /* 
        ** open access to base table 
        */

	/* 
	** Fill out relinfo with table information, including table owner.
	*/
	FErel_ffetch(rfa_ferslv[i]->name_dest,rfa_ferslv[i]->owner_dest,
		     &relinfo);
	MEfree((PTR)rfa_ferslv[i]->name_dest);
	MEfree((PTR)rfa_ferslv[i]->owner_dest);
	MEfree((PTR)rfa_ferslv[i]);
       	FEatt_fopen(&qblk, &relinfo);

    	if (En_SrcTyp == JDRepSrc)
	    _VOID_ r_JDMaintTabList(JD_TAB_TAB_ADV_INS_CNT, NULL, 
			relinfo.name, relinfo.owner, &ins);

        /* 
        ** For each attribute in table, fill ATT structure 
        ** (make sure no DB access will take place in this loop!) 
        */
        while (FEatt_fetch(&qblk, &attinfo) == OK)
        {
            register ATT    *r_att;
            register ATTRIB chk_id;
            register char    *attname;

            attname = attinfo.column_name;

            /*
            ** Check for duplicate column names. If found one, and 
	    ** the source of data is not a JoinDef we have a problem,
	    ** Bug #5952:
            */
            RepeatedField = FALSE;
	    chk_id = id - 1;
	    while ((--chk_id >= 0) && (!RepeatedField))
	    {
                register ATT *chk_p = &(Ptr_att_top[chk_id]);
                if (STcompare(chk_p->att_name, attname) == 0)
                    RepeatedField = TRUE;
            }

            if ((RepeatedField) && (En_SrcTyp == TabRepSrc))
	    {
            	r_error(0x2C8, FATAL, attname, NULL);
	    }

	    /*
	    ** Make sure that if the source of data is a JoinDef 
	    ** the attribute is being used, and if it's being used 
	    ** in more than one table get the constructed name for 
	    ** it.  Be sure and include the owner in the table look-up!
	    */
	    if (En_SrcTyp == JDRepSrc)
	    {
		i4 cnt = 0;

		/*
		** If we can't find the field then it's not being used so 
		** skip it:
		*/
		if (!r_JDMaintAttrList(JD_ATT_GET_FIELD_CNT,
			r_JDTabNameToRv(relinfo.name, relinfo.owner, ins),
			attname, &cnt))
		   continue;

		
		if (cnt > 1)
		{
		  _VOID_ r_JDMaintAttrList(JD_ATT_GET_ATTR_NAME,
			r_JDTabNameToRv(relinfo.name, relinfo.owner, ins),
			attname, &ColName [0]);
		  attname = ColName;
		}
	    }

            r_att = &(Ptr_att_top[id++]);
            r_att->att_name = FEtsalloc(Rst4_tag, attname);
    
            /* check if attribute name is a keyword */
    
            if (!St_silent && (r_gt_pcode(attname) != PC_ERROR ||
                      r_gt_wcode(attname) != A_ERROR ||
                      r_gt_vcode(attname) != VC_NONE ||
                      r_gt_cagg(attname) ||
                      STcompare(attname, NAM_REPORT) == 0 ||
                      STcompare(attname, NAM_PAGE) == 0 ||
                      STcompare(attname, NAM_DETAIL) == 0))
            {
                IIUGmsg(ERget(S_RF0047_Column_name_keyword), 
                    (bool) FALSE, 1, attname);
            }

            /* set up the data type field */
            r_att->att_value.db_datatype = attinfo.adf_type;
            r_att->att_prev_val.db_datatype = attinfo.adf_type;
            r_att->att_value.db_length = attinfo.intern_length;
            r_att->att_prev_val.db_length = attinfo.intern_length;
            r_att->att_value.db_prec = attinfo.intern_prec;
            r_att->att_prev_val.db_prec = attinfo.intern_prec;
    
	    /*
	    ** Do not allocate value buffers for un-supported datatypes.
	    ** Let the user know that they're being ignored.
	    */
	    if  (!IIAFfedatatype(&r_att->att_value))
	    {
		_VOID_ IIUGxri_id(&attinfo.column_name[0],
				  &dsply_name[0]);
		IIUGerr(E_RW1414_ignored_attrib,
			UG_ERR_ERROR,1,&dsply_name[0]);
		r_att->att_value.db_data = (PTR)NULL;
		r_att->att_prev_val.db_data = (PTR)NULL;
	    }
	    else
	    {
		if ((r_att->att_value.db_data =
				(PTR)FEreqmem((u_i4)Rst4_tag,
					(u_i4)(r_att->att_value.db_length),
					 TRUE, (STATUS *)NULL)) == NULL)
		{
		    IIUGbmaBadMemoryAllocation(ERx("rf_att_dflt - db_data"));
		}
		if ((r_att->att_prev_val.db_data = 
				(PTR)FEreqmem((u_i4)Rst4_tag,
					(u_i4)(r_att->att_prev_val.db_length),
					TRUE, (STATUS *)NULL)) == NULL)
		{
		    IIUGbmaBadMemoryAllocation(ERx("rf_att_dflt - prev db_data"));
		}
	    }

            r_att->att_position = 0;    /* flag to indicate when done */
            r_att->att_lac = NULL;
        }
        FEatt_close(&qblk);

    } /* for */

    /*
    ** Just in case if we have skipped any attributes, 
    ** we're leaking memory though:  Note that unsupported
    ** datatypes will not be reflected as skipped as far as the
    ** id count is concerned!
    */
    En_n_attribs = id;

    return;

}
