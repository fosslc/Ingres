
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<cm.h>
# include	<rtype.h>
# include	<rglob.h>
# include	<st.h>
# include	<er.h>

/**
** Name:	rsendqry.c	Construct and send a query to the backend.
**
** Description:
**	This file defines:
**
**	r_sendqry	Construct and send a query to the backend.
**
** History:
**	17-apr-87 (grant)	created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	r_sendqry	Construct and send a query to the backend.
**
** Description:
**	This routine constructs a query from the global target list,
**	qualification, and sort list, then sends it to the backend.
**
** Input params:
**	none.
**
** Output params:
**	none.
**
** Returns:
**	none.
**
** Exceptions:
**	<exception codes>
**
** History:
**	17-apr-87 (grant)	implemented.
**	8/10/89 (elein) garnet
**		added evaluating attributes parameters
**	01-sep-89 (sylviap)	
**		Added check for -6 flag (St_compatible60).  Reports will return
**		duplicate rows if -6 is not set, otherwise will run queries
**		with DISTINCT.
**      1/12/90 (elein)
**              IIUGlbo's for IBM gateways
**              bug 9619: runtime casing required because
**              of copyapp
**	10/25/89 (elein) garnet
**		On second thought, add variable capabilities to sort direction
**	2/7/90 (martym)
**		Coding standard cleanup.
**     3/20/90 (elein) performance
**              Change STcompare to STequal
**	4/30/90 (elein)
**		Change to return status for more consistent error checking
**		since we can now have runtime errors here.
**	03-may-90 (sylviap)	
**		Added 'desc' and 'asc' as valid sort directions.
**	5/30/90 (elein) 30298
**		Both remainder and where sections may now be used in a query
**	13-aug-1992 (rdrane)
**		Converted r_error() err_num value to hex to facilitate lookup
**		in errw.msg file.  Replace En_relation reference with global
**		FE_RSLV_NAME structure name reference.  Removed the
**		IIUGlbo_lowerBEobject() of En_sql_from prior to it write to
**		the BE.  This appears to be unnecessary, and saves a full
**		parse/casing for potentially delimited owner.tablename and
**		correlation name constructs.
**	13-oct-92 (rdrane)   35335, 41861, 41871
**		For .SORT, effect ORDER BY by ordinal number rather than
**		result column name.  This addresses gateway problems regarding
**		casing of AS targets, and UNION SELECT and OPEN/SQL's
**		prohibition against ORDER BY name (e.g., RBF JoinDef reports).
**	18-jan-1993 (rdrane)
**		Use normalized form of sort attribute name to effect ordinal
**		matching!
**	2-feb-1993 (rdrane)
**		For default reports, establish the SQLDA and search it for the
**		first supported datatype and use its ordinal in the ORDER BY
**		clause.
**	26-feb-1993 (rdrane)
**		Only establish the SQLDA if a default report or an SQL .QUERY
**		that has .SORT options.  This will save overhead on reports
**		which include their own ORDER BY clause.
**	9-dec-1993 (rdrane)
**		Oops!  The sort attribute name is already stored as 
**		normalized (b54950).  So, don't forget to normalize any
**		parameter specification.
**	1-apr-1994 (rdrane)
**		Somehow we never wrote out the default sort buffer (b61208).
*/

i4  r_sendqry()
{
    RNG	    *rng;
    SORT    *srt;
    i4	    seq;
    i4	    name_lgth;
    char    *attname;
    char    *direction;
    char    *sort_ordinal;
    char    sa_ord_buf[16];
    char    sa_buf[(FE_UNRML_MAXNAME + 1)];



    name_lgth = STlength(En_ferslv.name);
    /*
    ** For default reports, we need to find the first suppported
    ** column for sorting default reports.  For non-default SQL
    ** reports, we only need to do this if there is a .SORT specification.
    ** For QUEL reports, we don't need to do this at all.  So,
    ** Establish an SQLDA so that we can determine the ordinals of
    ** each result column (bugs 35335, 41861, and 41871).  We have to
    ** do this before we start building the main query, or the
    ** resultant DESCRIBE will flush the query-in-progress buffer!
    */
    if  ((name_lgth > 0) ||
	 ((En_sql_from != NULL) && (En_scount > 0))) 
    {
	r_starg_order();
    }

    IItm(TRUE);

    /* construct target list and qualification */

    if (name_lgth > 0)
    {
	/* .DATA table or default report specified instead of .QUERY */

	IIsqInit(NULL);

 	IIwritedb(ERx("select "));
	/* Only select with DISTINCT if the -6 flag is set */
 	if ((En_rtype == RT_DEFAULT) && (St_compatible60))
 	{
 	    IIwritedb(ERx("distinct "));
 	}
 	
 	IIwritedb(ERx("* from "));
	IIwritedb(En_ferslv.name);

	if (En_rtype == RT_DEFAULT)
	{
	    /*
	    ** First column is sorted for default report on table,
	    ** but it must be the first column which is a supported datatype!
	    */
	    STcopy(ERx(" order by "),&sa_buf[0]);
	    if  ((seq = r_gord_targ()) != 0)
	    {
		CVna(seq,&sa_ord_buf[0]);
		STcat(&sa_buf[0],&sa_ord_buf[0]);
	    }
	    else
	    {
		/*
		** This should never happen, as we should have previously
		** failed any report with no eligible columns.
		*/
		r_error(0x416,FATAL,NULL);
	    }
	}
    }
    else if (En_sql_from != NULL)
    {
	/* SQL */

	IIsqInit(NULL);

	IIwritedb(ERx("select "));
	if (En_pre_tar != NULL)
	{
	    /* 'distinct' or 'all' specified */

	    IIwritedb(En_pre_tar);
	    IIwritedb(ERx(" "));
	}
	else if ((En_scount > 0) && (St_compatible60))
	{
	    /* Need to have the -6 flag set to retain duplicates.  */

	    /* to retain compatibility with 5.0, duplicates are removed if
	    ** .SORT is specified */

	    IIwritedb(ERx("distinct "));
	}

	IIwritedb(En_tar_list);
	IIwritedb(ERx(" from "));
	IIwritedb(En_sql_from);
    }
    else
    {
	/* QUEL */

	/* put out RANGE statements */

	for (rng = Ptr_rng_top; rng != NULL; rng = rng->rng_below)
	{
	    IIwritedb(ERx(" range of "));
	    IIwritedb(rng->rng_rvar);
	    IIwritedb(ERx(" is "));
	    IIwritedb(rng->rng_rtable);
	    IIsyncup((char *)0, 0);
	}

	/* put out RETRIEVE */

	IIwritedb(ERx(" retrieve "));
	if (En_pre_tar != NULL)
	{
	    /* 'unique' was specified */

	    IIwritedb(En_pre_tar);
	    IIwritedb(ERx(" "));
	}

	IIwritedb(ERx("("));
	IIwritedb(En_tar_list);
	IIwritedb(ERx(")"));
    }

    if (En_q_qual != NULL)
    {
        /* add WHERE clause (pre-6.0 reports) */

	IIwritedb(ERx(" where "));
	IIwritedb(En_q_qual);
    }

    if (En_remainder != NULL)
    {
	/* add remainder of query (6.0 reports) */

	IIwritedb(En_remainder);
    }
    
    if (En_scount > 0)
    {
        /* add SORT BY or ORDER BY clause if there is a .SORT command */

	if ((En_sql_from != NULL) || (name_lgth > 0))
	{
	    IIwritedb(ERx(" order by "));
	    for (srt = Ptr_sort_top, seq = 1; seq <= En_scount; srt++, seq++)
	    {
	        if (seq > 1)
	        {
		    IIwritedb(ERx(", "));
	        }
		r_g_set(srt->sort_attid);
		if( r_g_skip() == TK_DOLLAR )
		{
			CMnext(Tokchar);
			attname = r_g_name();
			if( (srt->sort_attid = r_par_get(attname)) == NULL )
			{
				r_error(0x3F0,NONFATAL, attname, NULL);
				return(FALSE);
			}
			/*
			** Normalize any parameter name ...
			*/
			_VOID_ IIUGdlm_ChkdlmBEobject(srt->sort_attid,
						      srt->sort_attid,
						      FALSE);
		}
					/* If we can match and get the
					** ordinal, use it! (35335, 41861,
					** 41871)
					*/
		if  ((sort_ordinal = r_gtarg_order(srt->sort_attid)) != NULL)
		{
	        	IIwritedb(sort_ordinal);
		}
		else
		{
			_VOID_ IIUGxri_id(srt->sort_attid,&sa_buf[0]);
	        	IIwritedb(&sa_buf[0]);
		}
		r_g_set(srt->sort_direct);
		if( r_g_skip() == TK_DOLLAR )
		{
			CMnext(Tokchar);
			direction = r_g_name();
		        if( (srt->sort_direct = r_par_get(direction)) == NULL )
			{
		        	r_error(0x3F0,NONFATAL, direction, NULL);
				return(FALSE);
		        }
		}
		CVlower (srt->sort_direct);
	        if(STequal(srt->sort_direct, O_DESCEND)  || 
	           STequal(srt->sort_direct, ERx("desc"))  || 
	           STequal(srt->sort_direct, ERx("descend"))  || 
	           STequal(srt->sort_direct, ERx("descending"))  )
	        {
		    IIwritedb(ERx(" desc"));
	        }
	        else if (STequal(srt->sort_direct, O_ASCEND)  ||
	           STequal(srt->sort_direct, ERx("asc"))  || 
	           STequal(srt->sort_direct, ERx("ascend"))  || 
	           STequal(srt->sort_direct, ERx("ascending"))  )
	        {
		    IIwritedb(ERx(" asc"));
	        }
		else
		{
		    r_error(0x3FB, NONFATAL, srt->sort_direct, srt->sort_attid,
			NULL);
		    return(FALSE);
		}
	    }
	}
	else
	{
	    IIwritedb(ERx(" sort by "));
	    for (srt = Ptr_sort_top, seq = 1; seq <= En_scount; srt++, seq++)
	    {
	        if (seq > 1)
	        {
		    IIwritedb(ERx(", "));
	        }
		r_g_set(srt->sort_attid);
		if( r_g_skip() == TK_DOLLAR )
		{
			CMnext(Tokchar);
			attname = r_g_name();
			if( (srt->sort_attid = r_par_get(attname)) == NULL )
			{
				r_error(0x3F0,NONFATAL, attname, NULL);
				return(FALSE);
			}
		}

	        IIwritedb(srt->sort_attid);
		r_g_set(srt->sort_direct);
		if( r_g_skip() == TK_DOLLAR )
		{
			CMnext(Tokchar);
			direction = r_g_name();
		        if( (srt->sort_direct = r_par_get(direction)) == NULL )
			{
		        	r_error(0x3F0,NONFATAL, direction, NULL);
				return(FALSE);
		        }
		}
		CVlower(srt->sort_direct);
	        if(STequal(srt->sort_direct, O_DESCEND)  || 
	           STequal(srt->sort_direct, ERx("desc")) || 
	           STequal(srt->sort_direct, ERx("descend")) || 
	           STequal(srt->sort_direct, ERx("descending"))  )
	        {
		    IIwritedb(ERx(":d"));
	        }
	        else if (STequal(srt->sort_direct, O_ASCEND) ||
	           STequal(srt->sort_direct, ERx("asc"))  || 
	           STequal(srt->sort_direct, ERx("ascend"))  || 
	           STequal(srt->sort_direct, ERx("ascending")) )
	        {
		    IIwritedb(ERx(":a"));
	        }
		else
		{
		    r_error(0x3FB, NONFATAL, srt->sort_direct, srt->sort_attid,
			NULL);
		    return(FALSE);
		}
	    }
	}
    }
    else if (En_rtype == RT_DEFAULT)
    {
	IIwritedb(&sa_buf[0]);
    }

    _VOID_ r_ctarg_order();	/* Clean-up any SQLDA we may have allocated
				** (35335, 41861, 41871 and 6.5 non-supported
				** datatypes for default reports).
				*/

    return(TRUE);

}
