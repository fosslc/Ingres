/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<er.h>
# include	<st.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<afe.h>
# include	<rtype.h>
# include	<rglob.h>
# include	<iirowdsc.h>
# include	<ug.h>
# include	<errw.h>

/**
** Name:    rattdflt.c -	Report Writer Setup Attributes Structure Module.
**
** Description:
**	Contains the routine that sets up the attribute table for the report
**	query or data table.
**
**	r_att_dflt()	setup attribute structures for report.
**
** History:
**	Revision 5.0  86/04/11	15:30:00  wong
**	Changes for PC:	 Now retrieves query attribute descriptors
**	through 'FEqry_desc()'.	 Also, moved definition of 'rf_mes_func'
**	to "hdr/rglobi.h" for global use by both REPORT and SREPORT.
**
**	Revision 2.0  81/03/10	peter
**	Initial version.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Aug-2009 (kschendel) 121804
**	    Need afe.h to satisfy gcc 4.3.
**/


/*{
** Name:    r_att_dflt() -	Setup Attribute Structures for Report.
**
** Description:
**   R_ATT_DFLT - set up the attribute structures for each attribute in the
**	data relation.	The array of ATT structures is first dynamically
**	allocated, and the attribute relation is read for each attribute.
**	Default values for the fields in the ATT structures are set.
**
** Returns:
**	TRUE	if successful.
**	FALSE	if error found.
**
** Side Effects:
**	Creates an array of ATT structures, and fills in
**	some fields in them.
**	Sets Ptr_att_top.
**
** History:
**	3/10/81 (ps) - written.
**	9/1/83	(gac)	does not mask off length as lengths can be > 255.
**	2/1/84 (gac)	T_DATE added to att_type.
**	9/10/84 (gac)	fixed bug 3904 -- give warning if column name
**			is also a RW keyword.
**	4/26/85 (bab)	ifdef'd SEINGRES out RBF code so that termdr
**			doesn't get pulled in the CS's report writer.
**	9/9/85 (marian) fixed bug 5952 -- abort if duplicate column
**			names found
**	9/18/85 (bab)	added 'ifdef SEINGRES' the definition of
**			CS_DATE, CS_MONEY and their cases.
**	4/11/86 (jhw)	Changes for PC:	 Retrieve attributes through
**			'FEqry_desc()'.
**	18-nov-86 (marian)	bug 10825
**		Change when user's range variables are being defined because
**		FEqry_desc is doing internal range/retrieves and are bumping
**		out user's definitions.	 Pass the address of r_range() to
**		FEqry_desc() and have FEqry_desc make the call to r_range.
**	24-may-1988 (neil)
**		Modified reference to rd_dbv to macro provided by iirowdesc.h.
**	19-dec-88 (sylviap)
**		Changed memory allocation to use FEreqmem.
**      3/20/90 (elein) performance
**              Change STcompare to STequal
**	04-mar-92 (leighb) DeskTop Porting Change:
**		Moved function declaration outside of calling function.
**		This is so function prototypes will work.
**	20-oct-1992 (rdrane)
**		Remove declarations of FEtsalloc(), r_gt_att(), En_n_attribs,
**		and En_tar_list since they're already declared in hdr files
**		already included by this module.  Changed data type of id
**		and chk_id local variables to match the type of En_n_attribs,
**		which is the controling variable anyway.  This should correct
**		the ANSI compiler complaints.  Converted r_error() err_num
**		values to hex to facilitate lookup in errw.msg file.  Customized
**		IIUGbmaBadMemoryAllocation() text to disambiguate which may have
**		occured.  Removed obsolete reference to r_range() and
**		En_relation, and "demoted" the file to a ".c" since there is no
**		ESQL processing occuring here anymore.  Eliminate references to
**		r_syserr() and use IIUGerr() directly.
**		Ensure that db_prec is set in the associated DB_DATA_VALUE
**		structure for DECIMAL datatype support.
**	26-oct-1992 (rdrane)
**		Re-work building of the ATT array.  For unsupported datatypes,
**		do not alocate db_data buffers - leave them as NULL pointers.
**		This way, IIretdom() will effectively skip over the column.
**		Further, the NULL value will trigger other RW routines to
**		"ignore" the attribute.  Note that while it would seem
**		simpler to just not allocate the ATT in the first place,
**		it would actually create significant problems interfacing
**		with LIBQ for return/processing of the ROW_DESC.
**		Don't use r_gt_att() to find the ATT structure - at this point,
**		it will be all NULLs, assumed to be unsupported, and so
**		not return a valid pointer.
**	3-feb-1993 (rdrane)
**		Only put out the unsupported datatype message if it's
**		a default report.
**	7-jul-1993 (rdrane)
**		Put out the unsupported datatype message with IIUGerr(),
**		not FEmsg() (we never used the pause feature anyway ...).
**	12-jul-1993 (rdrane)
**		Correct testing of RD_NAMES flag - it's a bit-mapped mask now,
**		and no longer a discrete value.
**	11-apr-95 (kch)
**              If result columns are used (select col1 AS result)
**              in the .SORT against a gw db which prefers uppercase
**              validation here will fail if the case preference is
**              not applied to the att name in the lookup array here.
**              This change fixes bug 35335.
**	10-apr-1995 (harpa06)
**              Cross Integration change from 6.4/05 by angusm: Use of reserved
**		words (e.g. 'length') as result names can cause unpredictable
**		 output. Force warning if one of these is found. (bug 51071)
*/

# ifdef SEINGRES
/*
** CS_DATE, CS_MONEY had better be the same as in the cs version of
** symbol.h (for DB_DTE_TYPE and DB_MNY_TYPE)
*/
# define	CS_DATE		36
# define	CS_MONEY	37	/* Note: conflicts with DB_TXT_TYPE */
# endif

bool
r_att_dflt()
{
	register ATTRIB		id;	/*
					** Attribute index in row desc
					*/
	register ATTRIB		att_ord;/*
					** Attribute ordinal in ATT array
					*/
	register ATTRIB		chk_ord;
		ROW_DESC	*rd;
	register ATT		*r_att;
		char		attname[(FE_MAXNAME + 1)];
		char		dsply_attname[(FE_UNRML_MAXNAME + 1)];


	/* start of routine */


	/* Get attributes */


	/* read the result descriptor */

	IIretinit((char *)NULL, 0);
	if (IIerrtest() != 0)
	{
	    r_error(0x19, NONFATAL, NULL);
	    return FALSE;
	}

	IItm_retdesc(&rd);
	if ((rd->rd_flags & RD_NAMES) == 0)
	{
	    IIUGerr(E_RW0009_r_att_dflt_No_names,UG_ERR_FATAL,0);
	}

	En_n_attribs = (ATTRIB)rd->rd_numcols;

	if ((Ptr_att_top = (ATT *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(En_n_attribs * (sizeof(ATT))), TRUE,
		(STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_att_dflt - Ptr_att_top"));
	}

	id = 0;
	att_ord = 1;
	while (id < (ATTRIB)rd->rd_numcols)
	{
		STlcopy(rd->rd_names[id].rd_nmbuf, &attname[0],
			rd->rd_names[id].rd_nmlen);

		/*
		** Check to see if duplicate column names -- bug # 5952
		** Pre-decrement chk_ord in the while loop since we're using
		** it as an index (zero-based).
		*/
		chk_ord = att_ord - 1;
		while (--chk_ord >= 0)
		{
			r_att = &(Ptr_att_top[chk_ord]);
			if  (STequal(r_att->att_name, &attname[0]))
			{
				r_error(0x2C8,FATAL,&attname[0],NULL);
			}
		}

		r_att = &(Ptr_att_top[(att_ord - 1)]);
		r_att->att_name = FEtsalloc(Rst4_tag, &attname[0]);

		/*
		** Check if attribute name is a keyword
		*/
/* b51071 */
 /*
 ** r_gt_rword - misc reserved words
 ** r_gt_pcode - rw constants
 ** r_gt_wcode - within...
 ** r_gt_vcode - page constants (LM. RM...)
 ** r_gt_cagg  - cum/cumulative
 **
 */
               if (!St_silent &&
                        (r_gt_rword(attname) == FATAL ||
                        r_gt_pcode(attname) != PC_ERROR ||

			r_gt_wcode(&attname[0]) != A_ERROR ||
			r_gt_vcode(&attname[0]) != VC_NONE ||
			r_gt_cagg(&attname[0]) ||
			STequal(&attname[0], NAM_REPORT) ||
			STequal(&attname[0], NAM_PAGE) ||
			STequal(&attname[0], NAM_DETAIL) ))
		{
			FEmsg(ERget(S_RW000A_Column_name_keyword),
			      FALSE, &attname[0]);
		}

        /*
        ** bug 35335:
        ** ensure case preferences are respected for
        ** result column names for ATT array, else
        ** validation of sort columns fails for
        ** AS result_columns used as sort columns
        ** against GW dbs (e.g. RDB).
        */

                _VOID_ IIUGlbo_lowerBEobject(r_att->att_name);

		/* set up the data type field */
		r_att->att_value.db_datatype = r_att->att_prev_val.db_datatype =
					    rd->RD_DBVS_MACRO(id).db_datatype;
		r_att->att_value.db_length = r_att->att_prev_val.db_length =
					    rd->RD_DBVS_MACRO(id).db_length;
		r_att->att_value.db_prec = r_att->att_prev_val.db_prec =
					    rd->RD_DBVS_MACRO(id).db_prec;

		/*
		** Do not allocate value buffers for un-supported datatypes,
		** and conditionally inform the user.
		*/
		if  (!IIAFfedatatype(&rd->RD_DBVS_MACRO(id)))
		{
			if  ((En_rtype == RT_DEFAULT) && (!St_silent))
			{
				_VOID_ IIUGxri_id(&attname[0],
						  &dsply_attname[0]);
				IIUGerr(E_RW1414_ignored_attrib,
					UG_ERR_ERROR,1,&dsply_attname[0]);
			}
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
				IIUGbmaBadMemoryAllocation(ERx("r_att_dflt - db_data"));
			}

			if ((r_att->att_prev_val.db_data =
				(PTR)FEreqmem((u_i4)Rst4_tag, 
					(u_i4)(r_att->att_prev_val.db_length),
					TRUE, (STATUS *)NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(ERx("r_att_dflt - prev db_data"));
			}
		}

		r_att->att_position = -1;	/* flag to indicate when done */
		r_att->att_lac = NULL;
		id++;
		att_ord++;
	}

	return TRUE;
}

