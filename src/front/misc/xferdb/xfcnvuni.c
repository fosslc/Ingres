/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<lo.h>
# include	<st.h>
# include	<me.h>
# include	<er.h>
# include	<si.h>
# include	<adf.h>
# include	<fe.h>
# include	<add.h>
# include	<afe.h>
# include	<ui.h>
# include	<ug.h>
# include	<xf.h>
# include       <uigdata.h>
# include	"erxf.h"

/* GLOBALDEF's */
GLOBALDEF       PTR     Col_list = NULL; 	/* The list of columns specified 
						** by the user to CONVTOUNI. */

GLOBALDEF       i4      Colcount ;		/* count of cols in the list */

GLOBALDEF       PTR     Filter_list = NULL;	/* The list of tables that must
						** be filtered for alter tale 
						** alter column processing.  */

GLOBALDEF       i4      Filtercount ;		/* count of filtered tables */


/* GLOBALREF's */
GLOBALREF	PTR	Obj_list;


/**
** Name:	xfcnvuni.c - Routines to support convtouni utility by copydb.
**
** Description:
**	This file defines routines necessary to carry out the conversion of 
**	a database columns from non unicode to unicode types. The hash routines in
**	this file are derived from xffilobj. The key used is 
**	owner.tablename.columnname.
**
** History:
**	23-Apr-2004 (gupsh01) 
**	    Created.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
*/

/*Name:	writealtcol - routine to write the alter table alter column statements.
**
** History:
**	23-Apr-2004 (gupsh01) 
**	    Created.
**      13-Jan-2008 (hanal04) Bug 119826
**          Resolve problems in convtouni scripts:
**          1) If the row size grows beyond the current page size and we
**          are not spanning pages already (pg version 5), then bump the
**          page size of the table.
**          2) If we are going changing key columns temporarily modify the 
**          table to heap as alter table alter column is not allowed on a key.
**          The new XF_MOD_TO_HEAP is passed to xfmodify().
**      24-Feb-2009 (coomi01) b121608
**          Test result code from IIUGdlm_ChkdlmBEobject()
*/
void
writealtcol( 
XF_TABINFO	*tab,
i4	output_flags2)
{

    XF_COLINFO  *ap;
    auto DB_DATA_VALUE  dbdv;
    auto DB_USER_TYPE   att_spec;
    char        tbuf[256];
    i4		srcdt = 0;
    i4		resdt = 0;
    i4		reslen = 0;
    i4          tupwidth = 0;
    i4          modoutput_flags2 = output_flags2;
    i4          modreqd = FALSE;

    if ( !(output_flags2) || 
	 !(output_flags2 & XF_CONVTOUNI) )
	return;

    /* Before we write the alter table statements lets make sure we won't
    ** need a bigger page size
    */
    for (ap = tab->column_list; ap != NULL; ap = ap->col_next)
    {
        switch (ap->adf_type)
        {
          case DB_CHA_TYPE:
            tupwidth += ap->intern_length * sizeof(UCS2);
            break;
          case -DB_CHA_TYPE:
            tupwidth += (ap->intern_length - 1) * sizeof(UCS2) + 1;
            break;
          case DB_VCH_TYPE:
            tupwidth += (ap->intern_length - DB_CNTSIZE) * sizeof(UCS2)
                        + DB_CNTSIZE;
            break;
          case -DB_VCH_TYPE:
            tupwidth += (ap->intern_length - DB_CNTSIZE - 1) * sizeof(UCS2)
                        + DB_CNTSIZE + 1;
            break;
          default:
            tupwidth += ap->intern_length;
            continue;
        }

        /* Check for key columns. If present we'll need to modify to
        ** heap before the alter table alter column.
        */
        if(ap->key_seq) 
        {
            modoutput_flags2 |= XF_MOD_TO_HEAP;
            modreqd = TRUE;
        }
    } /* get to next column */

    if(tupwidth > tab->pagesize)
    {
        /* generate a modify statement that will have big enough page size
        ** if the table is not already spanning. If current row width exceeds
        ** current page width we have tuples spanning pages.
        */
        if(tab->row_width <= tab->pagesize)
        {
            if(tab->pagesize < P64K)
            {
                /* Modify the table to reconstruct with new page size */
                tab->pagesize *= 2;
                modreqd = TRUE;
            }
            else
            {
                /* Not spanning and need to move to spanning. However,
                ** spanning will not support duplicate checking so need
                ** to be careful. The change required are beyond the scope
                ** of this change. For now we will error.
                */
                IIUGerr(E_XF005A_Alt_Col_Warn, UG_ERR_FATAL, 2,
                             (PTR)&tab->owner, (PTR)&tab->name);
            }
        }
    }
	
    if(modreqd)
        xfmodify(tab, 0, modoutput_flags2);

    /* Write the alter table alter column statements. 
    ** for all the columns of this table
    */ 
    for (ap = tab->column_list; ap != NULL; ap = ap->col_next)
    {
	/* modify the datatype right here */
	switch (ap->adf_type) 
	{
	  case DB_CHA_TYPE:
            resdt = DB_NCHR_TYPE;
	    reslen = ap->intern_length * sizeof(UCS2);
	    break; 
	  case -DB_CHA_TYPE:
            resdt = -DB_NCHR_TYPE;
	    reslen = (ap->intern_length - 1) * sizeof(UCS2) + 1;
	    break; 
 	  case DB_VCH_TYPE:	
            resdt = DB_NVCHR_TYPE;
	    reslen = (ap->intern_length - DB_CNTSIZE) * sizeof(UCS2) 
			+ DB_CNTSIZE;
	    break; 

 	  case -DB_VCH_TYPE:	
            resdt = -DB_NVCHR_TYPE;
	    reslen = (ap->intern_length - DB_CNTSIZE - 1) * sizeof(UCS2) 
			+ DB_CNTSIZE + 1;
	    break; 

	  default:
	    continue;
	}


        /*
        ** fill in DB_DATA_VALUE for call to get 
	** attribute specification.  
	*/
        dbdv.db_datatype = (DB_DT_ID)resdt;
        dbdv.db_length = (i4)reslen;
        dbdv.db_prec = (i2)ap->precision;

	if (afe_tyoutput(FEadfcb(), &dbdv, &att_spec) != OK)
          FEafeerr(FEadfcb());
 
        if ((Col_list) && (xfobjselected (tab->name, XF_TABFILTER_LIST)))
	{ 
	  char key[DB_MAXNAME * 2 + 2 + 1];
	    
          MEfill(DB_MAXNAME *2 + 2 +1, (unsigned char) '\0', key);

	  /* concat table name and column name to make the key */ 
          STpolycat(3, tab->name, ".", ap->column_name, key);

	  /* search the key in the Col_list and the Obj_list */
          if (!(xfobjselected (key, XF_COLUMN_LIST)))
	    continue;
	}

	xfwrite(Xf_in, ERx("alter table "));
              xfwrite_id(Xf_in, tab->owner);
              xfwrite(Xf_in, ERx("."));
              xfwrite_id(Xf_in, tab->name);
              xfwrite(Xf_in, ERx(" alter column "));
              xfwrite_id(Xf_in, ap->column_name);
              xfwrite(Xf_in, ERx(" "));

         /* write attribute spec to input file */
         xfwrite(Xf_in, att_spec.dbut_name);

        /* check nullability in spec */
        if (ap->nulls[0] == 'N')
            xfwrite(Xf_in, ERx(" not null"));

        /* check for defaults and user-defined defaults. */
        writedefault(ap);

	xfwrite(Xf_in, GO_STMT);
    } /* get to next column */
}

/* Name: xfobjselected - find out if the object is in
**			 the convtouni object list.
*/
bool
xfobjselected (char *name, i4 listtype)
{
    auto PTR    dat;
    PTR		CnvObjList = NULL;

    if (listtype == XF_COLUMN_LIST )
	CnvObjList = Col_list;
    else if (listtype = XF_TABFILTER_LIST)
	CnvObjList = Filter_list;
    else
        return (FALSE);

    if ( CnvObjList == NULL)
      return (FALSE);

    if (IIUGhfHtabFind(CnvObjList, name, &dat) == OK)
      return (TRUE);
    else
      return (FALSE);
}

/* Name: xfaddobject - add the object in object list.
**
*/
void
xfaddobject(char *name, i4 listtype)
{
    char        normname[DB_MAXNAME + 1];
    char        *name_ptr, namebuf[(DB_MAXNAME + 3)];

    if (name == NULL)
        return;

    /*
    ** The user gave us this name, so we have to check for a delimited
    ** identifier.  Internally we handle all names in "normalized" form,
    ** since that's how they're stored in the DBMS.
    */
   
    name_ptr = name;
    if (*name_ptr != '"' && STindex(name_ptr, " ", 0) != NULL)
    {
        STpolycat(3, "\"", name_ptr, "\"", namebuf);
        name_ptr = namebuf;
    }

    if ( UI_BOGUS_ID != IIUGdlm_ChkdlmBEobject(name_ptr, normname, FALSE) )
    {
        /*
        ** Must save the normalized name, since the hashing stuff
        ** just points to our data.
        */
        name = STalloc(normname);
    }
    else
    {
        if (IIUIdbdata()->name_Case == UI_UPPERCASE)
            CVupper(name);
        else
            CVlower(name);
    }

    /*
    ** Initialize hash table.  Since we don't pass a pointer to an
    ** 'allocfail' routine (a routine to be called on memory allocation
    ** failure) IIUGhiHtabInit will never return unless it's successful.
    ** Likewise, IIUGheHtabEnter will succeed or abort.
    **
    ** 1st arg is table size.  Buckets chain, so hash table doesn't need to
    ** be very big.
    */
    if (listtype == XF_COLUMN_LIST )
    {
      if (Col_list == NULL)
        (void) IIUGhiHtabInit(64, (i4 (*)())NULL, (i4 (*)())NULL,
                                (i4 (*)())NULL, &Col_list);
    }
    else if (listtype == XF_TABFILTER_LIST )
    {
      if (Filter_list == NULL)
        (void) IIUGhiHtabInit(64, (i4 (*)())NULL, (i4 (*)())NULL,
                                (i4 (*)())NULL, &Filter_list);
    }

    /* make sure we enter the column only once */
    if (!(xfobjselected(name, listtype)))
    {
      if (listtype == XF_COLUMN_LIST )
      {
        (void) IIUGheHtabEnter(Col_list, name, (PTR) name);
	Colcount++;
      }
      else if (listtype == XF_TABFILTER_LIST )
      {
        (void) IIUGheHtabEnter(Filter_list, name, (PTR) name);
        Filtercount++; 
      }
      else return;
    }
}

/* Name: xfremovetab - verifies if the table needs to be removed
**		       from table list.
** 
** Input 
**	tp		table being examined
** Output
**	TRUE		if the table must be removed from list
**	FALSE		if the table should be kept in the list.
** History:
**	
**	28-Apr-2004 (gupsh01)
**	    Created.
*/
bool
xfremovetab(tab)
XF_TABINFO	*tab;
{
    XF_COLINFO	*ap;
    bool	delete_table = TRUE;

    for (ap = tab->column_list; ap != NULL; ap = ap->col_next)
    {
	switch (ap->adf_type)
	{
	    case DB_CHA_TYPE:
	    case -DB_CHA_TYPE:
	    case DB_VCH_TYPE:
	    case -DB_VCH_TYPE:
	      if (Col_list) 
	      {
		if (xfobjselected (tab->name, XF_TABFILTER_LIST))
		{
	    	  char key[DB_MAXNAME * 2 + 2 + 1];

	    	  MEfill(DB_MAXNAME *2 + 2 +1, 
			(unsigned char) '\0', key);

		  /* concat table name and column name to make the key */
          	  STpolycat(3, tab->name, ".", ap->column_name, key);

		  /* if we can find any key in hash table 
		  ** do not delete
		  */
	          if (xfobjselected (key, XF_COLUMN_LIST))
	          {
    		     delete_table = FALSE;
	          }
	        }
		else if (Obj_list && xfselected(tab->name))
		  delete_table = FALSE;
	      }
	      else 
	      {	
		delete_table = FALSE;
	      }	
    	      break;

	     default:
    		continue;
   	} 

	if (delete_table == FALSE)
	  break;	
    }

    return (delete_table);
}

/* Name: xfcnvtabname - gets the table name from convtouni
**	column specification.
** Input
** 	cnvarg - Input argument string 
**	         should be null terminated.
** 	tabname- Result table name, should be 
**		  allocated by the caller. 
**	maxlen - Is maximum length of buffer supplied.
**
** Output.
*/
i4
xfcnvtabname (cnvarg , tabname, maxlen)
char    *cnvarg;
char    *tabname;
i4      maxlen;
{
    char        dot = '.';
    char        *iptr = 0; 
    i4          reslen;

    iptr = STindex (cnvarg, &dot, 0);  

    if (iptr)
    { 
        reslen = iptr - cnvarg;

        if ((reslen > maxlen) || (reslen == 0))
          return (FAIL);

        MEcopy (cnvarg, (u_i2)(reslen), tabname);
        tabname[reslen] = EOS;
    }

    return (OK);
}
