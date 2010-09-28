/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: SAXIngres.cpp - function implementation for supporting impxml utility.
**
** Description:
**		Various functions used for supporting the impxml utility
**	are implemented here. A brief description is as follows. 
**
**	
** History:
**      07-jun-2001 (gupsh01)
**          written
**	19-Jul-2001 (hanje04)
**	    Removed ',' from after second argument STcopy calls
**	17-Sep-2001 (hanch04)
**	    Corrected spelling for is_foriegn_key to is_foreign_key
**	19-Oct-2001 (hanch04)
**	    added check for precision and scale to correctly print out the 
**	    decimal type.A
**      21-Dec-2001 (hanch04)
**	    Read in values for unique_keys and persistent indexes 
**	11-Apr-2003 (gupsh01)
**	    Removed the code to open the datafiles from here it is done in
**	    SAXImportHandlers::process_tag() function.
**	29-sep-2003 (somsa01)
**	    For .NET 2003, iostream has changed.
**	21-Sep-2009 (hanje04)
**	   Use new include method for stream headers on OSX (DARWIN) to
**	   quiet compiler warnings.
**	   Also add version specific definitions of parser funtions to
**	   allow building against Xerces 2.x and 3.x
*/

#if defined(NT_GENERIC) || defined(LINUX) || defined(DARWIN)
#include <iostream>
#include <fstream>
#else
#include <iostream.h>
#include <fstream.h>
#endif
#include "SAXIngres.hpp"


/* Globals */
static 	i2 tabcolcount = 0;
static 	i2 indexcolcount = 0;

/* char 	*owner[FE_MAXNAME + 1]; */

/*{
** Name: set_meta_ingres_attributes : 
**
** Description:
**
** This function will read the general Ingres attributes and their values 
** 
**
** Inputs:
**			
**		att_name : An array of the names of the attributes.
**		value	 : An array of the values read for the attributes.
**		size	 : The size of the array ie # of attributes.
**
**      Returns:
**              none.
**      8-june-2001 (gupsh01)
**        Added.
*/

void 
set_meta_ingres_attributes(char **att_name, char **value, i2 size, char *ownername)
{
    i4		index_nr;
    char	locale[MAX_DB_NAME];
    i4		schema_nr;
    i4 		table_nr;
    char 	version[MAX_DB_NAME];
    i4 		view_nr;
    char 	db_name[MAX_DB_NAME];

    for(i2 i=0; i <size; i++)
    {
	if (STequal(att_name[i],"index_nr")) 
	     CVal (value[i], &index_nr); 
	else if (STequal(att_name[i],"locale"))
	    STcopy (value[i], locale);
	else if (STequal(att_name[i],"owner"))
	    STcopy (value[i], ownername );
	else if (STequal(att_name[i],"schema_nr"))
	     CVal (value[i], &schema_nr); 
	else if (STequal(att_name[i],"table_nr"))
	     CVal (value[i], &table_nr); 
	else if (STequal(att_name[i],"version"))
	     STcopy (value[i], version);
	else if (STequal(att_name[i],"view_nr"))
	      CVal (value[i], &view_nr); 
	else if (STequal(att_name[i],"db_name"))
	     STcopy (value[i], db_name);
	/* else give out error that the attribute is not recognized */
    }
}

/*{
** Name: set_meta_table_attributes : 
**
** Description:
**
** This function will read the table attributes and their values 
** and set these values into the XF_TAB_INFO structure passed in.
**
** Inputs:
**			
**		table	 : The XF_TAB_INFO structure which will be set.
**		att_name : An array of the names of the attributes.
**		value	 : An array of the values read for the attributes.
**		size	 : The size of the array ie # of attributes.
**
**      Returns:
**              none.
**      8-june-2001 (gupsh01)
**        Added.
**	14-jun-2001 (gupsh01)
**	  Corrected the setting of row_nr attribute into tablist. 
**	06-Dec-2002 (gupsh01)
**	    Moved the code to handle opening the data tables to here 
**	    We will open them early on when the metadata information 
**	    is being processed. xmlimport expects all xml files to 
**	    generate data files, even if they contain only the
**	    metadata information.
**	02-Jan-2002 (gupsh01)
**	    Fixed the for loop redefining the variable i.
*/

void 
set_meta_table_attributes(XF_TABINFO *table, char **att_name, 
				char **value, i2 size)
{
   /*
   ** Here we start reading the data so open a
   ** file for the table data based on table and
   ** its owner.
   */
   
   i2 i=0;
	
    for(i=0; i <size; i++)
    {
        if (STequal(att_name[i],"table_name"))
            STcopy(value[i], table->name);
        else if (STequal(att_name[i],"table_owner"))
            STcopy(value[i], table->owner);
        else if (STequal(att_name[i],"id"))
            CVal (value[i], &table->table_reltid);
	else if (STequal(att_name[i],"column_nr"))
	    tabcolcount = 0;
	else if (STequal(att_name[i],"page_size"))
	    CVal (value[i], &table->pagesize);
        else if (STequal(att_name[i],"row_nr"))
             CVal (value[i], &table->num_rows);  
        else if (STequal(att_name[i],"table_type"))
            STcopy (value[i], table->table_type);
        else if (STequal(att_name[i],"allow_duplicates"))
            STcopy (value[i], table->duplicates );
        else if (STequal(att_name[i],"row_width"))
             CVal (value[i], &table->row_width); 
        else if (STequal(att_name[i],"data_compression"))
            STcopy (value[i], table->is_data_comp);
        else if (STequal(att_name[i],"key_compression"))
            STcopy( value[i], table->is_key_comp);
        else if (STequal(att_name[i],"journaling"))
            STcopy(value[i], table->journaled);
        else if (STequal(att_name[i],"table_structure"))
            STcopy(value[i], table->storage);
        else if (STequal(att_name[i],"unique_keys"))
            STcopy(value[i], table->is_unique);
        else if (STequal(att_name[i],"persistent"))
            STcopy(value[i], table->persistent);
       /* else give out error that the attribute is not recognized */
      }
}

/*{
** Name: set_tabcol_attributes : 
**
** Description:
**
** This function will read the table column attributes and their values 
** and set these values into the XF_COLINFO structure passed in.
**
** Inputs:
**			
**		cp		 : The XF_COLINFO structure which will be set.
**		att_name : An array of the names of the attributes.
**		value	 : An array of the values read for the attributes.
**		size	 : The size of the array ie # of attributes.
**
**      Returns:
**              none.
**      8-june-2001 (gupsh01)
**          Added.
**	4-june-2002 (gupsh01)
**	    Added has_default attribute parsing.
**	17-Oct-2005 (thaju02)
**	    For object_key/table_key (fixed-length), don't set 
**	    cp->extern_length, otherwise adc_lenchk fails with E_AD2007. 
**	19-Aug-2010 (troal01)
**	    cp->srid is defaulted to -1 and set properly if defined
*/

void
set_tabcol_attributes(XF_COLINFO  *cp, char **att_name,
                                char **value, i2 size)
{
    char        *datatype;
    char 	is_foreign_key[MAXSTRING]; /* Extra information */
    char 	column_label[MAXSTRING];   /* Extra for now */
    i4		precision = 0;
    i4		scale = 0;

    /*
     * Set SRID to the default -1
     */
    cp->srid = -1;

    for(i2 i=0; i <size; i++)
    {
        if (STequal (att_name[i], "column_name"))
	{
        STcopy (value[i], cp->column_name);
	    tabcolcount++;
	    cp->col_seq = tabcolcount;
	}
        else if (STequal (att_name[i], "column_type"))
	{
            STcopy (value[i], cp->col_int_datatype);
	    if( (STcasecmp(cp->col_int_datatype,ERx("dec")) == 0) ||
		     (STcasecmp(cp->col_int_datatype,ERx("decimal")) == 0) ||
		     (STcasecmp(cp->col_int_datatype,ERx("numeric")) == 0))
	    {
		cp->adf_type= DB_DEC_TYPE;
	    }

	}
        else if (STequal (att_name[i], "column_width"))
	{
	    CVal (value[i], &cp->intern_length); 
	    if ( STcasecmp(cp->col_int_datatype, ERx("object_key")) &&
		 STcasecmp(cp->col_int_datatype, ERx("table_key")) )
		cp->extern_length =  cp->intern_length; 
	}
        else if (STequal (att_name[i], "allow_nulls"))
	{
            STcopy(value[i], cp->nulls);
	}
        else if (STequal (att_name[i], "has_default"))
	{
            STcopy (value[i], cp->defaults);
	}
	else if (STequal (att_name[i], "default_value"))
	{
	    cp->default_value = STalloc(value[i]);
	}
        else if (STequal (att_name[i], "is_key"))
	{
	    if (STequal (att_name[i], "Y"))
		cp->log_key = TRUE; 
	    else
		cp->log_key = FALSE; 
	}
        else if (STequal (att_name[i], "key_position"))
	{
             CVal (value[i], &cp->key_seq); 
	}
        else if (STequal (att_name[i], "is_foreign_key"))
	{
            STcopy (value[i], is_foreign_key);
	}
        else if (STequal (att_name[i], "column_label"))
	{
            STcopy (value[i], column_label);
	}
        else if (STequal (att_name[i], "precision"))
	{
            CVal(value[i], &precision);
	}
        else if (STequal (att_name[i], "scale"))
	{
            CVal(value[i], &scale);
	}
        else if (STequal (att_name[i], "srid"))
	{
            CVal (value[i], &cp->srid);
	}
       /* else give out error that the attribute is not recognized */
    }
    if( abs(cp->adf_type) == DB_DEC_TYPE )
    {
	cp->precision = DB_PS_ENCODE_MACRO(precision, scale);
	cp->extern_length = precision;
    }
}

/*{
** Name: set_meta_table_attributes : 
**
** Description:
**
** This function will read the table attributes and their values 
** and set these values into the XF_TAB_INFO structure passed in.
**
** Inputs:
**			
**		table	 : The XF_TAB_INFO structure which will be set.
**		att_name : An array of the names of the attributes.
**		value	 : An array of the values read for the attributes.
**		size	 : The size of the array ie # of attributes.
**
**      Returns:
**              none.
**      8-june-2001 (gupsh01)
**        Added.
**	23-Feb-2009 (thaju02)
**	  Set pagesize value in XF_TAB_INFO struct passed in.
*/

void
set_meta_index_attributes(XF_TABINFO *index, char **att_name,
                                char **value, i2 size)
{
    i4 column_number;
    for(i2 i=0; i <size; i++)
    {
	if (STequal(att_name[i],"index_name"))
        { 
	  indexcolcount = 0;	 	
	  STcopy(value[i], index->name);
	}
	else if (STequal(att_name[i],"column_number"))
	 CVal (value[i], &column_number); 
	else if (STequal(att_name[i],"index_structure"))
	STcopy (value[i], index->storage);
	else if (STequal(att_name[i],"unique_keys"))
	STcopy (value[i], index->is_unique);
	else if (STequal(att_name[i],"persistent"))
	STcopy (value[i], index->persistent);
	else if (STequal(att_name[i], "page_size"))
	 CVal(value[i], &index->pagesize);
	/* 
	** else give out error 
    ** that the attribute is not recognized 
    */
    }
}

/*{
** Name: set_tabcol_attributes : 
**
** Description:
**
** This function will read the table column attributes and their values 
** and set these values into the XF_COLINFO structure passed in.
**
** Inputs:
**			
**		cp		 : The XF_COLINFO structure which will be set.
**		att_name : An array of the names of the attributes.
**		value	 : An array of the values read for the attributes.
**		size	 : The size of the array ie # of attributes.
**
**      Returns:
**              none.
**      8-june-2001 (gupsh01)
**        Added.
*/

void
set_indcol_attributes(XF_COLINFO  *icp, char **att_name,
                                char **value, i2 size)
{
    for(i2 i=0; i <size; i++)
    {
	  if (STequal(att_name[i],"column_name"))
	  {
	    indexcolcount++;
	    STcopy( value[i], icp->column_name);
	    icp->col_seq = indexcolcount;
	  }
	  else if (STequal(att_name[i],"index_position"))
	     CVal (value[i], &icp->key_seq); 
      /* else give out error that the attribute is not recognized */
    }
}

/*{
** Name: put_tab_col : Add a column to the column_list of the table.
**
** Description:
**
**	This will complete the XF_COLINFO structure, passed to it and 
** add it to the column_list of the XF_TABINFO structure.
**
** Inputs:
**		
**		tab		 : The XF_TABINFO structure which will be set.	
**		col		 : The XF_COLINFO structure which will be added to
**				   the tab.
**
**      Returns:
**              none.
**      8-june-2001 (gupsh01)
**        Added.
*/

void put_tab_col(XF_TABINFO *tab, XF_COLINFO  *col)
{
/* fill in the various values for the column */

   if( xffillcolinfo(tab->name, 
	Owner, 
	col->col_int_datatype,
	col->col_int_datatype,
	col->adf_type,
	col->extern_length, 
	col->intern_length, 
	DB_S_DECODE_MACRO(col->precision),
	col,
	col->default_value
	))
    {
        /* put the column into the column_list */
        col->col_next = tab->column_list;
        tab->column_list = col;
    }
	/* else add error message break out of code */
}

/*{
** Name: put_ind_col : Add a column to the column_list of the index
**
** Description:
**
**	This will add the XF_COLINFO structure, passed to it 
**  to the column_list of the XF_TABINFO structure.
**
** Inputs:
**		
**		indlist	 : The XF_TABINFO structure which will be set.	
**		col		 : The XF_COLINFO structure which will be added to
**				   the indlist.
**
**      Returns:
**              none.
**      8-june-2001 (gupsh01)
**        Added.
*/

void put_ind_col(XF_TABINFO *indlist, XF_COLINFO  *col)
{
    col->col_next = indlist->column_list;
    indlist->column_list = col;
}

/*{
** Name: grt_cols : Obtain the column information.
**
** Description:
**
**	Obtains the information about the datatype, etc of this column.
**
** Inputs:
**		
**		indlist	 : The XF_TABINFO structure which will be set.	
**		col		 : The XF_COLINFO structure which will be added to
**				   the indlist.
**
**      Returns:
**              none.
**      8-june-2001 (gupsh01)
**        Added.
**	11-May-2005 (gupsh01)
**	  remove call to norm function to remove leading and
**	  trailing blanks in the data field. These blanks may be
**	  present in the date retrieved from database, and should
**	  not be removed.
*/
bool 
get_cols(
    char        *table_name,
    char        *owner_name,
    char        *datatype,
    char        *int_datatype,
    i4          extern_len,
    i4          intern_len,
    XF_COLINFO  *cp
)
{
    auto DB_DATA_VALUE  dbdv;
    auto DB_USER_TYPE   utype;
    char                len_buf[12];
    char                *scale_buf = NULL;

    auto XF_TABINFO     *tp;

    ADF_CB   *Adf_cb = NULL;

    if (Adf_cb == NULL)
        Adf_cb = FEadfcb();

    xfread_id(table_name);
    xfread_id(owner_name);
    xfread_id(cp->column_name);

    /* determine internal length and datatype */
    utype.dbut_kind = (cp->nulls[0] == 'Y') ? DB_UT_NULL : DB_UT_NOTNULL;

    /*
    ** bug 9148
    ** if the internal datatype is of type 'logical_key', or a UDT you
    ** must pass in the internal datatype, and internal length
    ** otherwise, pass in datatype and extern_len.
    */
    (void) STtrmwhite(datatype);
    (void) STtrmwhite(int_datatype);

    cp->log_key = FALSE;

    /* Is this a logical_key type? 
    ** not supported now may be later.
    */
    if (STbcompare(int_datatype, 0, ERx("table_key"), 0, TRUE) == 0
         || STbcompare(int_datatype, 0, ERx("object_key"), 0, TRUE) == 0)
    {
        cp->log_key = TRUE;
        if (cp->sys_maint[0] != 'Y')
        {
            /*
            ** user-maintained logical keys, the feature from hell.
            ** Warn the user that disaster looms.
            
            IIUGerr(E_XF014A_Log_key_warn, UG_ERR_ERROR,
                        3, cp->column_name, owner_name, table_name);
	    */
        }
    }

    if (STcompare(int_datatype, datatype) == 0)
    /* There may be a case later when we may add internal datatypes 
    ** put this if in anticipation, but we will get rid of this. 
    */	
    {
        CVlower(datatype);
        STlcopy(datatype, utype.dbut_name, (i4) sizeof(utype.dbut_name));
        CVla((i4) extern_len, len_buf);
    }
    /*
    ** Decimal types deal with later, need to extract the scale from the 
    ** length. 
    */
    if (afe_ctychk(Adf_cb, &utype, len_buf, scale_buf, &dbdv) != OK) 
    {
        /* Handle UDT's later. */
        {
            FEafeerr(Adf_cb); 
            return (FALSE); 
        }
    }

    cp->adf_type = dbdv.db_datatype;
    cp->intern_length = dbdv.db_length;
    return(TRUE);
}

void
norm(char *value)
{
    char *result = NULL;
    i4 length = 0;
  
    length = STlength(value);
    if( value != NULL )
    {
	result = STskipblank(value, length);
	if( result != NULL )
	{
	    length = STtrmwhite(result);
	    if( result != NULL )
	    {
                STcopy(result, value);
	    }
	}
    }
}

/*{
** Name: mkcollist : Rearranges the columns in the column list. 
**
** Description:
**
**	Obtains the information about the datatype, etc of this column.
**
** Inputs:
**		cp	 : A list of XF_COLINFO structures.
**
**      Returns:
**              none.
**      8-june-2001 (gupsh01)
**	    Added.
**	14-jun-2001 (gupsh01)
**	    Use get_empty_column to allocate a new column. 
**	04-june-2001 (gupsh01)
**	    Modified copying default_value.  
*/

XF_COLINFO *mkcollist(XF_COLINFO  *cp) 
{
  XF_COLINFO  *new_list = NULL;

  for(XF_COLINFO  *temp = cp;temp;temp = temp->col_next)
  {
    XF_COLINFO  *element = get_empty_column();
   
    if (temp->column_name) 
      STcopy (temp->column_name, element->column_name);  
    element->extern_length = temp->extern_length;
    element->intern_length = temp->intern_length;
    element->precision = temp->precision;
    element->adf_type = temp->adf_type;
    if (temp->nulls) 
      STcopy (temp->nulls, element->nulls);
    if (temp->defaults) 
      STcopy (temp->defaults, element->defaults);
    if (temp->sys_maint) 
      STcopy (temp->sys_maint, element->sys_maint);
    element->log_key = temp->log_key;
    if (temp->col_int_datatype) 
      STcopy (temp->col_int_datatype, element->col_int_datatype);
    element->col_seq = temp->col_seq;
    element->key_seq = temp->key_seq;
    if (temp->default_value) 
      element->default_value = STalloc (temp->default_value);
    if (temp->has_default) 
      STcopy (temp->has_default, element->has_default);
    if (temp->audit_key) 
      STcopy (temp->audit_key, element->audit_key);
    element->srid = temp->srid;
 
    element->col_next = new_list; 
    new_list = element; 
  }
  return new_list;
}


/*{
** Name: addOtherLocs - Adds the location to the buffers.
**
** Description:
**
**	 Concats the buffer with the location information.
**
** Inputs:
**
**	buf		:	the buffer containg the list of locations.
**	add_loc	:	other locations we are adding to the location list.
**
**      Returns:
**              none.
**      8-june-2001 (gupsh01)
**  
*/
void addOtherLocs(char buf[XF_INTEGLINE + 1], char add_loc[FE_MAXNAME + 1])
{
   /* add the locations to the list */
   (void) STtrmwhite(add_loc);
   if (buf)
   {
     STcat(buf, ", ");
     STcat(buf, add_loc );
   }
}

/* 
** code for memory management This has been put here because it is 
** not being managed from inside xferdb. 
*/

XF_COLINFO *ColBuffList = NULL;
# define COL_BLOCK       32



/*{
** Name: get_empty_column - Obtain an empty column from a preallocated 
**							buffer.
**
** Description:
**
**	 Used for more effecient memory management. 
**
** Inputs:
**
**      Returns:
**              none.
**      8-june-2001 (gupsh01)
**	   Added.
**	14-june-2001 (gupsh01)
**	   initialize the memory with MEfill before 
**	   returning the pointer.
**  
*/
XF_COLINFO *get_empty_column()
{
    XF_COLINFO *tmp;

    if (ColBuffList == NULL)
    {
        i4  i;

        /*
        ** Nothing on the freelist, so allocate a bunch of new structs.
        */

        ColBuffList = (XF_COLINFO *) XF_REQMEM(sizeof(*tmp) * COL_BLOCK, FALSE);
        if (ColBuffList == NULL)
        {
    /*        IIUGerr(E_XF0060_Out_of_memory, UG_ERR_FATAL, 0); */
            /* NOTREACHED */
        }

        /* set up the 'next' pointers, linking the empties together. */
        for (i = 0; i < (COL_BLOCK - 1); i++)
             ColBuffList[i].col_next = &ColBuffList[i+1];
        ColBuffList[COL_BLOCK - 1].col_next = NULL;
    }
    tmp = ColBuffList;
    ColBuffList = ColBuffList->col_next;
    tmp->col_next = NULL;

	MEfill((u_i2) sizeof(*tmp), (unsigned char) 0, (PTR) tmp);
    return (tmp);
}

/*{
** Name: col_free - Free preallocated memory buffer.
**
** Description:
**
**	 Used for more effecient memory management. 
**
** Inputs:
**
**      Returns:
**              none.
**      8-june-2001 (gupsh01)
**  
*/
void col_free(XF_COLINFO      *list)
{
    XF_COLINFO  *cp;

    if (list == NULL)
        return;

    for (cp = list; cp->col_next != NULL; cp = cp->col_next)
    {
        if (cp->default_value != NULL)
            MEfree((PTR) cp->default_value);
    }

    cp->col_next = ColBuffList;
    ColBuffList = list;
}

/* The freelist */
XF_TABINFO *TabFreeList = NULL;
# define TAB_BLOCK      20

/*{
** Name: get_empty_table - Obtain an empty table from a preallocated 
**							buffer.
**
** Description:
**
**	 Used for more effecient memory management. 
**
** Inputs:
**
**      Returns:
**              none.
**      8-june-2001 (gupsh01)
**  
*/

XF_TABINFO *
get_empty_table()
{
    XF_TABINFO *tmp;

    if (TabFreeList == NULL)
    {
        i4 i;

        TabFreeList = (XF_TABINFO *) XF_REQMEM(sizeof(*tmp) * TAB_BLOCK, FALSE);
        if (TabFreeList == NULL)
        {
        /*    IIUGerr(E_XF0060_Out_of_memory, UG_ERR_FATAL, 0); */
            /* NOTREACHED */
        }
        /* set up the 'next' pointers. */
        for (i = 0; i < (TAB_BLOCK - 1); i++)
        {
             TabFreeList[i].name[0] = EOS;
             TabFreeList[i].tab_next = &TabFreeList[i+1];
        }
        TabFreeList[TAB_BLOCK - 1].tab_next = NULL;
    }
    tmp = TabFreeList;
    TabFreeList = TabFreeList->tab_next;

    MEfill((u_i2) sizeof(*tmp), (unsigned char) 0, (PTR) tmp);

    return (tmp);
}

/*{
** Name: tab_free - Free preallocated memory buffer.
**
** Description:
**
** Inputs:
**      Returns:
**              none.
**      8-june-2001 (gupsh01)
*/

void
tab_free(XF_TABINFO *list)
{
    XF_TABINFO         *rp;

    if (list == NULL)
        return;

    for (rp = list; rp->tab_next != NULL; rp = rp->tab_next)
        col_free(rp->column_list);

    rp->tab_next = TabFreeList;
    TabFreeList = list;
}
 
