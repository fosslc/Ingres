/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<er.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	 <ex.h>
# include	 <afe.h>
# include	<errw.h>

/*
**   R_X_TCMD - follow the TCMD structure path and process the TCMD
**	commands (prints, tabs, etc.). until the end of the
**	path is found.
**
**	Parameters:
**		tcmd - start of linked list of TCMD structures (may
**			be NULL to start).
**
**	Returns:
**		none.
**
**	Side Effects:
**		May set St_ulchar, St_underline, St_ff_on, St_cline,
**		St_c_cblk.  Will set Cact_tcmd and Cact_written.
**
**	Called by:
**		r_do_header,r_do_footer,r_rep_do,r_do_page.
**
**	Error messages:
**		Syserr:bad tcmd type.
**
**	Trace Flags:
**		140, 142.
**
**	History:
**		3/6/81 (ps)	written.
**		1/5/81 (ps)	modified for .WITHIN/.END.
**		5/11/83(nl)	Put in changes made since code was ported
**				to UNIX. Add reference to P_ATT to fix
**				bug 937 and add reference to Cact_written.
**		12/12/83 (gac)	added IF statement.
**		7/30/84 (gac)	fixed bug 3821 -- top line number for block
**				is only saved for outermost block.
**		8/16/84 (gac)	added sequence number to TCMD structure.
**		mar-10-1989 (danielt & cmr)
**			fix for bug 4818
**		7/30/89 (elein) garnet
**			Add evaluation of expressions.  Add new conversion
**			rountine to convert to undetermined datatypes to ints
**		8/1/89 (elein) B7333
**			Add parameter to reitem calls.
**		8/31/89 (elein)
**			Add trimwhite to chconv function.
**		9/6/89 (elein)
**			Remove no-item check for nullstr/ulc. Now
**			done earlier.
**		9/27/89 (elein)
**			Change method of resetting value pointer in rxcolconv.
**			old way caused arith excp when value was 0
**		9/27/89 (elein) garnet
**			Allow ULC, NULLSTR, LM, RM, NPAGE to have item types of
**			I_NONE.  If the item is a constant of the correct type
**			the r_p_* commands will have already set the tcmd value
**			correctly and set item type to I_NONE.
**              3/19/90 (elein)
**                      Add exception handler to bracket adc_cvinto
**                      for assignments (.LETs)
**		20-mar-90 (sylviap)
**			Exits out of RW based on Err_count, not on errcount.
**			Took out errcount since r_error automatically increments
**			Err_count.  (US #724)
**		3/21/90 (elein) performance
**			Check St_no_write before calling r_set_dflt
**		26-mar-90 (sylviap)
**			Backed out change with Err_count.  Will let r_x_tab take
**			care of error.
**		4/19/90 (elein)
**			Forgot to call r_ges_next before r_e_item
**			in .TAB case.
**		8/9/90 (elein)
**			Add context for .LET afe errors
**		10/1/90 (elein) 33913
**			Simplified conversion routines.
**			Allow II_NULLSTRING to be right side of .LET
**			assignment.
**		10/28/90 (elein) 34057
**			Fix seriously damaging typo in conversion routines.  
**		09-dec-1991 (pearl)  40109
**			In r_x_iconv(), if DB_INT_TYPE, check length of data
**			before assignment.
**		19-feb-92 (leighb) DeskTop Porting Change:
**			Added '&' to 'value' being passed to adc_getempty().
**		23-oct-1992 (rdrane)
**			Converted r_error() err_num values to hex
**			to facilitate lookup in errw.msg file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-feb-2009 (gupsh01)
**	    Fix .TAB command on utf8 installs to adjust
**	    for utf8 charcaters on the line.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_x_tcmd(intcmd)
TCMD	*intcmd;
{
	register TCMD	*tcmd = intcmd;
	DB_DATA_VALUE	value;
	STATUS		status;
	i4		isnull;
	LET		*let;

	DB_DATA_VALUE	chrval;
	i4		result;
	ATTRIB		ordinal;
	i4		ei_exhandler();
	EX_CONTEXT      context;
	i4      	errcount;

	/* start of routine */


	Cact_written = FALSE;		/* No write in this block yet */

	St_err_on = TRUE;
	errcount = 0;

	while (tcmd != NULL && errcount == 0)
	{

		Sequence_num = tcmd->tcmd_seqno;

		switch(tcmd->tcmd_code)
		{

			case(P_ERROR):
			case(P_NOOP):
				break;

			case(P_PRINT):
				r_x_prelement(tcmd->tcmd_val.t_v_pel);
				break;

			case(P_TAB):
				if( St_no_write)
				{
					r_set_dflt();
				}

				if( tcmd->tcmd_item->item_type != I_ATT && 
				    tcmd->tcmd_item->item_type != I_NONE )
				{
					r_ges_reset();
					r_e_item(tcmd->tcmd_item, &value, TRUE);
					status = r_x_colconv(&value,
							 tcmd->tcmd_val.t_v_vp);
					if( status != OK)
					{
						r_error(0x3F4,NONFATAL,
							ERx("TAB"), NULL);
						errcount++;
						
					}
				}

				if ((En_need_utf8_adj) && (En_utf8_adj_cnt))
				{
				    r_x_tab((tcmd->tcmd_val.t_v_vp)->vp_value + En_utf8_adj_cnt,
					(tcmd->tcmd_val.t_v_vp)->vp_type);
				}
				else
				  r_x_tab((tcmd->tcmd_val.t_v_vp)->vp_value,
					(tcmd->tcmd_val.t_v_vp)->vp_type);
				St_cline->ln_started = LN_WRAP;
				break;

			case(P_NEWLINE):
				if((tcmd->tcmd_item)->item_type != I_NONE) 
				{
					r_ges_reset();
					r_e_item(tcmd->tcmd_item, &value, TRUE);
					/* t_v_long really declared as a i4  */
					status = r_x_iconv(&value, &result);
					tcmd->tcmd_val.t_v_long = result;
					if( status != OK )
					{
						r_error(0x3F2,NONFATAL,
							ERx("NEWLINE"), NULL);
						errcount++;
					}
				}
				r_x_newline(tcmd->tcmd_val.t_v_long);
				break;

			case(P_AOP):
				/*
				** fix for bug #4818.
				** don't do any processing of aggregates
				** if there is no data (-h flag)
				*/
				if (St_in_retrieve)
					r_a_ops(tcmd->tcmd_val.t_v_acc);
				break;

			case(P_ACLEAR):
				r_a_caccs(tcmd->tcmd_val.t_v_acc);
				break;

			case(P_CENTER):
			case(P_LEFT):
			case(P_RIGHT):
				if( tcmd->tcmd_item->item_type != I_ATT &&
				    tcmd->tcmd_item->item_type != I_NONE )
				{
					r_e_item(tcmd->tcmd_item, &value, TRUE);
					status = r_x_colconv(&value,
							tcmd->tcmd_val.t_v_vp );
					if( status != OK)
					{
						switch(tcmd->tcmd_code)
						{
						case (P_CENTER):
							r_error(0x3F4,
								NONFATAL,
								ERx("CENTER"),
								NULL);
								errcount++;
							break;
						case (P_LEFT):
							r_error(0x3F4,
								NONFATAL,
								ERx("LEFT"),
								NULL);
								errcount++;
							break;
						case (P_RIGHT):
							r_error(0x3F4,
								NONFATAL,
								ERx("RIGHT"),
								NULL);
								errcount++;
							break;
						default:
							break; /* NEVERMIND */
						}
					}
				}
				r_x_adjust(tcmd);
				break;

			case(P_NEED):
				if((tcmd->tcmd_item)->item_type != I_NONE) 
				{
					r_ges_reset();
					r_e_item(tcmd->tcmd_item, &value, TRUE);
					/* t_v_long really declared as a i4  */
					status = r_x_iconv(&value,&result);
					tcmd->tcmd_val.t_v_long = result;
					if( status != OK)
					{
						r_error(0x3F2,NONFATAL,
							ERx("NEED"), NULL);
						errcount++;
					}
				}
				r_x_need(tcmd->tcmd_val.t_v_long);
				break;

			case(P_FORMAT):
				r_x_sformat((tcmd->tcmd_val.t_v_ap)->ap_ordinal,
					(tcmd->tcmd_val.t_v_ap)->ap_format);
				break;

			case(P_TFORMAT):
				r_x_tformat((tcmd->tcmd_val.t_v_ap)->ap_ordinal,
					(tcmd->tcmd_val.t_v_ap)->ap_format);
				break;

			case(P_POSITION):
				r_x_spos((tcmd->tcmd_val.t_v_ps)->ps_ordinal,
					(tcmd->tcmd_val.t_v_ps)->ps_position);
				break;

			case(P_WIDTH):
				r_x_swidth((tcmd->tcmd_val.t_v_ws)->ws_ordinal,
					 (i2)(tcmd->tcmd_val.t_v_ws)->ws_width);
				break;

			case(P_RM):
				r_ges_reset();
				if( tcmd->tcmd_item->item_type != I_NONE)
				{
					r_e_item(tcmd->tcmd_item, &value, TRUE);
					status = r_x_iconv(&value, &result);
					result *=
					      (tcmd->tcmd_val.t_v_vp)->vp_value;
					if( status != OK)
					{
						r_error(0x3F2,NONFATAL,
							ERx("RM"), NULL);
						errcount++;
					}
					r_x_rm(result,
					      (tcmd->tcmd_val.t_v_vp)->vp_type);
				}
				else
				{
					r_x_rm((tcmd->tcmd_val.t_v_vp)->vp_value,
					      (tcmd->tcmd_val.t_v_vp)->vp_type);
				}
				break;

			case(P_LM):
				r_ges_reset();
				if( tcmd->tcmd_item->item_type != I_NONE)
				{
					r_e_item(tcmd->tcmd_item, &value, TRUE);
					status = r_x_iconv(&value, &result);
					result *=
					      (tcmd->tcmd_val.t_v_vp)->vp_value;
					if( status != OK)
					{
						r_error(0x3F2,NONFATAL,
							ERx("LM"), NULL);
						errcount++;
					}
					r_x_lm(result,
					      (tcmd->tcmd_val.t_v_vp)->vp_type);
				}
				else
				{
					r_x_lm((tcmd->tcmd_val.t_v_vp)->vp_value,
					      (tcmd->tcmd_val.t_v_vp)->vp_type);
				}
				break;

			case(P_PL):
				if((tcmd->tcmd_item)->item_type != I_NONE) 
				{
					r_ges_reset();
					r_e_item(tcmd->tcmd_item, &value, TRUE);
					/* t_v_long really declared as a i4  */
					status = r_x_iconv(&value, &result);
					tcmd->tcmd_val.t_v_long = result;
					if( status != OK)
					{
						r_error(0x3F2,NONFATAL,
							ERx("PL"), NULL);
						errcount++;
					}
				}
				r_x_pl(tcmd->tcmd_val.t_v_long);
				break;

			case(P_NPAGE):
				r_ges_reset();
				if((tcmd->tcmd_item)->item_type != I_NONE) 
				{
					r_e_item(tcmd->tcmd_item, &value, TRUE);
					status = r_x_iconv( &value, &result);
					result *=
					      (tcmd->tcmd_val.t_v_vp)->vp_value;
					if( status != OK)
					{
						r_error(0x3F2,NONFATAL,
							ERx("NEWPAGE"), NULL);
						errcount++;
					}
					r_x_npage(result,
					      (tcmd->tcmd_val.t_v_vp)->vp_type);
				}
				else
				{
					r_x_npage((tcmd->tcmd_val.t_v_vp)->vp_value,
					      (tcmd->tcmd_val.t_v_vp)->vp_type);
				}
				break;

			case(P_UL):
				St_underline = TRUE;
				break;

			case(P_FF):
				if (!St_ff_set)
				{
					St_ff_on = TRUE;
				}
				break;

			case(P_NO1STFF):
				if (!St_no1stff_set)
				{
					St_no1stff_on = TRUE;
				}
				break;

			case(P_ULC):
				if((tcmd->tcmd_item)->item_type != I_NONE) 
				{
					r_e_item(tcmd->tcmd_item, &value, TRUE);
					if ((status =
						r_x_chconv(&value,&chrval))
									 != OK)
					{
						r_error(0x3F5,NONFATAL,
							ERx("ULC"), NULL);
						errcount++;
					}
					tcmd->tcmd_val.t_v_char =
							*(char *)chrval.db_data;
				}
				St_ulchar = tcmd->tcmd_val.t_v_char;
				break;

			case(P_BEGIN):
				r_psh_be(tcmd);
				break;

			case(P_WITHIN):
				if( St_no_write)
				{
					r_set_dflt();
				}

				r_x_within(tcmd);	/* Sets Cact_tcmd */
				tcmd = Cact_tcmd;
				continue;

			case(P_BLOCK):
				if( St_no_write)
				{
					r_set_dflt();
				}

				if (St_c_cblk++ == 0)
				{
					St_top_l_number = St_l_number;
				}
				break;

			case(P_TOP):
			case(P_BOTTOM):
			case(P_LINEEND):
			case(P_LINESTART):
				if( St_no_write)
				{
					r_set_dflt();
				}

				r_x_bpos(tcmd->tcmd_code);
				break;

			case(P_END):
				Cact_tcmd = tcmd->tcmd_below;
						/* May set Cact_tcmd */
				r_x_end(tcmd->tcmd_val.t_v_long);
				tcmd = Cact_tcmd;
				continue;

			case(P_IF):
				r_ges_reset();
				r_e_blexpr(tcmd->tcmd_val.t_v_ifs->ifs_exp,
					   &value);
				status = adc_isnull(Adf_scb, &value, &isnull);
				if (status != OK)
				{
					FEafeerr(Adf_scb);
					continue;
				}

				if (!isnull && *((bool *)value.db_data))
				{
					tcmd = tcmd->tcmd_val.t_v_ifs->ifs_then;
				}
				else
				{
					tcmd = tcmd->tcmd_val.t_v_ifs->ifs_else;
				}
				continue;

			case(P_RBFAGGS):
			case(P_RBFFIELD):
			case(P_RBFHEAD):
			case(P_RBFPBOT):
			case(P_RBFPTOP):
			case(P_RBFSETUP):
			case(P_RBFTITLE):
			case(P_RBFTRIM):
				/* used by RBF.	 Ignore here */
				break;

			case(P_NULLSTR):
				if((tcmd->tcmd_item)->item_type != I_NONE) 
				{
					r_e_item(tcmd->tcmd_item, &value, TRUE);
					if ((status =
						r_x_chconv(&value,&chrval))
									 != OK)
					{
						r_error(0x3F5,NONFATAL,
							ERx("NULLSTRING"),
							NULL);
						errcount++;
					}
					tcmd->tcmd_val.t_v_str =
							(char *)chrval.db_data;
				}
				afe_nullstring(Adf_scb, tcmd->tcmd_val.t_v_str);
				break;

			case(P_LET):
				let = tcmd->tcmd_val.t_v_let;

				/* evaluate expression on right side of
				** assignment statement */

				r_ges_reset();
				r_e_item(&(let->let_right), &value, TRUE);

                                if( EXdeclare(ei_exhandler, &context) )
                                {
                                        EXdelete();
                                        adc_getempty(Adf_scb, &value);		 
                                        return;
                                }

				/*
				** Assign value to left variable dbv
				*/
				status = r_x_conv(&value,let->let_left);
                                EXdelete();
                                if (status != OK)
				{
					r_runerr(1039, NONFATAL);
				}
				break;

			default:
				r_syserr(E_RW004E_r_x_tcmd_Bad_tcmd_cod);
		}

	tcmd = tcmd->tcmd_below;
	}
	if( errcount > 0 )
	{
		r_syserr(E_RW13F6_RW_Runtime_errors);
	}

	return;
}

/*
**	Specific conversion routines
**
**	R_X_ICONV   -- convert unknown datatype to an integer or return FAIL.
**	R_X_COLCONV -- convert unknown datatype to an integer or a column name
**			or return FAIL
**	R_X_CHCONV  -- convert unknown datatype to a character string or FAIL
**	These routines are used by each other and rxtcmd to convert the result
**	of expression evaluation to an appropriate type.
**
**	None of these routines call error messages, except for FEafeerr().
**	It is up to the calling routine to take appropriate action.
**
**	History	
**		8/9/89 (elein) garnet
**		10/13/90 (elein) 
**			removed chrval parameter to r_x_iconv.  Changed to use
**			new rxconv routine.
**		11/8/90 (elein) 34125
**			Pass pointer to tdbv  to r_x_conv, not tdbv. For
**			some reason it was ok on sun, but bombed on vms.
**		23-oct-1992 (rdrane)
**			Ensure set/propagate precision in DB_DATA_VALUE
**			structures.
*/
i4  r_x_iconv( value, result)
DB_DATA_VALUE	*value;
i4	*result;
{
	i4	natval;
	i4	status; 
	DB_DATA_VALUE 	tdbv;


	/*
	** initialize result to non destructive value
	*/
	*result = 0;

	/*
	** this only checks for non-nullable INT_TYPEs of i4  length; other
	** cases of INT_TYPES (integer1, smallints, nullable integer)
	** will fall through to r_x_conv(). 
	*/
	if (value->db_datatype == DB_INT_TYPE 
			&& value->db_length == sizeof (i4))
	{
		*result = *(i4 *)(value->db_data);
		return( OK );
	}
	tdbv.db_datatype = DB_INT_TYPE;
	tdbv.db_length = sizeof(i4);
	tdbv.db_prec = 0;
	tdbv.db_data = (PTR) &natval;

	status = r_x_conv( value, &tdbv );
	if (status == OK)
	{
		*result = *(i4 *)(tdbv.db_data);
		return(OK);
	}
	return(FAIL);
}

i4  r_x_colconv( value, vpptr )
DB_DATA_VALUE	*value;
VP	*vpptr;
{
	STATUS		status;
	DB_DATA_VALUE	chrval;
	i4		natval = 0;
	ATTRIB	 	ordinal;
	
	/* If the type is column, 
	 * 	it is already set, so use it
	 * If the type is relative,
	 *    the value must be coercible to an int
	 *    or else it is an error
	 * If the type is not relative, it is absolute.
	 * We could have either a column name or an int here
	 * Try coercing into an int, if that works, weve got it.
	 * If not, find ordinal of column name
	 */

	 /* Initialization:
	 *  reset the value of vp_value to 1 or -1
	 *  the result will be *= into the value pointer value
	 */
	if( vpptr->vp_value==0)
		vpptr->vp_value = 1;
	else
		vpptr->vp_value = abs(vpptr->vp_value)/vpptr->vp_value;
	chrval.db_datatype = DB_NODT;

	/* Try to convert it to an integer,
	 * if that works, great!
	 * If this was a relative command, it had to have
	 * an integer, so stop trying and send back the result.
	 * So...  Try converting to char:
	 * if this works, the value of chrval will contain
	 * the orginal value converted to a character string.
	 * We hope it is a valid column name and try
	 * fetch the ordinal
	 */
	if( (status = r_x_iconv(value, &natval )) == OK)
	{
		vpptr->vp_value *= natval;
		return( status );
	}
	if( vpptr->vp_type == B_RELATIVE  )
	{
		return( status );
	}
	if( (status = r_x_chconv(value, &chrval)) != OK)
		return(status);
	
	/* 
 	* Fetch ordinal of columnname here
 	*/
	if( (ordinal = r_mtch_att(chrval.db_data)) == A_ERROR )
	{
		return(FAIL);
	}
	vpptr->vp_type = B_COLUMN;
	vpptr->vp_value = ordinal;
	return(OK);
	
}


/*
** r_g_chconv
**
** History:
**	23-oct-1992 (rdrane)
**		Ensure set/propagate precision in DB_DATA_VALUE
**		structures.
*/

i4  r_x_chconv( value, chrval)
DB_DATA_VALUE	*value;
DB_DATA_VALUE	*chrval;
{
	i4	status; 

	chrval->db_datatype = DB_CHR_TYPE;
	chrval->db_length = FE_MAXNAME+1;
	chrval->db_prec = 0;
	chrval->db_data = (PTR)get_space (1, FE_MAXNAME+1);

	status = r_x_conv( value, chrval);
	if (status == OK)
	{
		chrval->db_length = STtrmwhite(chrval->db_data);
		return( OK );
	}
	return(FAIL);
}
/*
**
**   R_X_CONV - Convert from any dbv to any dbv, if possible
**		This routine should be used when there is not
**		any knowledge of either of the data types involved.
**		If there is some knowledge about datatypes
**		involved, other methods may be preferred.
**
**		This routine recognizes II_NULLSTRING as a possible
**		value, checking for that first.
**
**	Function:

**		Convert the first into longtext and check out II_NULLSTRING
**		values.  If it is not II_NULL_STRING, then adc_cvinto
**		with the *original* dbvs.
**
**      Parameters:
**              fromvalue - source db data value
**              tovalue   - target db data value
**
**      Returns:
**              OK/FAIL.
**
**      Side Effects:
**
**      Called by:
**              r_x_tcmd
**
**      Error messages:
**              May set Adf_scb.  Caller should check for
**		failure.
**
**      History:
**		10/3/90 (elein)
**			created.
**		1/31/91 (elein) 35690
**			Assignment of null fromvalues to nullable
**			tovalues was not working because the
**			conversion to longtext did not check for
**			nullability and then failed.  If the from
**			value is nullable and null and the tovalue
**			is nullable, just set it to null and get
**			out asap.
**		8/8/91 (elein) 38189
**			Non-null empty string cannot represent NULL even if 
**			II_NULLSTRING value is empty string.
**		23-oct-1992 (rdrane)
**			Ensure set/propagate precision in DB_DATA_VALUE
**			structures.
**		31-aug-1993 (rdrane)
**			Cast u_char db_t_text references to (char *) so
**			prototyping in STbcompare() doesn't complain.
*/

i4  r_x_conv( fromvalue, tovalue)
DB_DATA_VALUE	*fromvalue;
DB_DATA_VALUE	*tovalue;
{
	i4		status; 
	DB_DATA_VALUE	ltextval;
	DB_TEXT_STRING  *text;
	i4		nullstrlen;
	char		ltdata[DB_MAXSTRING+DB_CNTSIZE];

 	/*
	** Input is Nullable and null and tovalue is nullable
	** set tovalue to null and return 
	*/
	if ( AFE_NULLABLE(fromvalue->db_datatype) &&
             AFE_ISNULL(fromvalue) &&
	     AFE_NULLABLE_MACRO(tovalue->db_datatype) )
	{
		status = adc_getempty(Adf_scb, tovalue);
		return status;
	}

	ltextval.db_datatype = DB_LTXT_TYPE;
	ltextval.db_length   = DB_MAXSTRING+DB_CNTSIZE;
	ltextval.db_prec     = 0;
	ltextval.db_data     = ltdata;
	/*
	 * Coerce fromvalue to long text--should always be ok!!
 	 */
	if( afe_cvinto(Adf_scb, fromvalue, &ltextval) != OK)
	{
		FEafeerr(Adf_scb);
		return (FAIL);
	}

	/*
	** Check for II_NULLSTRING value of ltextval
	** If it matches, set tovalue to null and return
	*/
	text = (DB_TEXT_STRING *) (ltextval.db_data);
	nullstrlen = Adf_scb->adf_nullstr.nlst_length;
	if (AFE_NULLABLE_MACRO(tovalue->db_datatype) &&
       text->db_t_count > 0 && nullstrlen > 0 && 
        text->db_t_count == nullstrlen &&
         STbcompare((char *)text->db_t_text, text->db_t_count,
         Adf_scb->adf_nullstr.nlst_string, nullstrlen, TRUE) == 0)
	{   /* from is null string so return null value */
		status = adc_getempty(Adf_scb, tovalue);
		return status;
	}

	/* assign to declared variable */
	/* if it works, that's it! */
        status = adc_cvinto(Adf_scb, fromvalue, tovalue);
	if( status == OK)
		return(status);

	return(FAIL);
}
