/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>
# include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<iirowdsc.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<erlq.h>

/**
**  Name: iirowdesc.c - Read in a tuple descriptor block.
**
**  Defines:
**	IIrdDescribe 	- Read a row descriptor.
**
**  History:
**	25-mar-1987	- Written (ncg)
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	26-aug-1987 	- Modified to use GCA. (ncg)
**	20-may-1988	- Modified for INGRES/NET. (ncg)
**	12-dec-1988	- Generic error processing. (ncg)
**	30-apr-1990	- Recognize GCA_READONLY_MASK. (ncg)
**	01-apr-1993  (kathryn)
**	    Set RD_UNBOUND row descriptor flag for large objects types.
**	04-apr-1996	- Modified to support compressed variable-length
**			  datatypes. (loera01)
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer contains DB_DATA_VALUE, so need to
**	    access members individually.
**/

/*{
**  Name: IIrdDescribe - Read a row descriptor.
**
**  Description:
**	This routine is called to read tuple descriptors returning from
**	the DBMS and fill a known row descriptor data structure.  The queries
**	that return tuple descriptors are:
**
**		## RETRIEVE and OPEN CURSOR
**		EXEC SQL SELECT, PREPARE INTO, DESCRIBE and OPEN
**
**	The data structure passed in may be the global retrieval descriptor
**	(ie, ii_lq_retdesc), may be local to the caller (as in the case
**	of a cursor descriptor).  This routine will free and allocate
**	data structures as needed.
**
**	There is an initial tuple size, mask, id and count field, followed by
**	that many result column descriptors, each containing a DB_DATA_VALUE
**	descriptor and a varying-length name.  If the NAMES modifier is
**	set in the current descriptor, then the varying-length names may be
**	non-zero, otherwise they can be skipped as 2 zero bytes.
**	Ignoring the block boundaries the data is layed out in the following
**	format:
**
**	    typedef struct {
**	      i4		tup_size;		Tuple size
**	      i4		desc_mask;		Masks for descriptor
**	      i4		desc_id;		Internal identifier
**	      i4		num_atts;		Number of attributes
**	      struct {
**		DB_DATA_VALUE 	att_dbv;		Type descriptor
**		i4		att_nmlen;		If a name then length
**		char	  	att_name[att_nmlen];
**	      } da_attribute[da_numatts];
**	    } GCA_TD_DATA;
**	    
**	This data structure is retrieved into a more "usable" data structure.
**	The local data structure includes an identical mapping of the above
**	description (see ROW_GCA), but that is hidden from users who should
**	only access the DB data values.
**
**	NOTE: Whenever the GCA_TD_DATA structure is changed you must also
**	change ROW_GCA accordingly.  One is virtual structure that cannot
**	be used via regular C calls, while the other can be.
**	
**	The routine assumes it is positioned in the correct spot to start
**	requesting descriptor info from the communications.
**
**  Inputs:
**	desc_type	- GCA tuple descriptor type.  The default is GCA_TDESCR
**			  but within a COPY statement it may be different.
**	row_desc	- Row descriptor data structure.
**	   .rd_dballoc	- Number of allocated DBV's in row_desc structure.
**			  If zero then none have been allocated.  If too
**			  small for this call (< num_atts) then free DBV's
**			  and allocate a new set.
**	   .rd_nmalloc	- Number of allocated names in row_desc structure.
**			  Same logic as with rd_dballoc.
**  Outputs:
**	row_desc	- Data structure to be filled.
**	   .rd_flags	- May specify that names have been read for this
**			  descriptor and other flags.
**	   .rd_numcols	- Number of columns in row.  If an error occurred then
**			  this will be zero.  This may also be zero if a non-
**			  SELECT query was being described.
**	   .rd_dballoc	- If there were too few DBV's then they will be freed
**			  and the new number allocated will be set.
**	   .rd_gca	- Equivalent structure to GCA_TD_DATA.
**	      .rd_gc_tsize 	- Size of complete tuple (sent from DBMS).
**	      .rd_gc_res_mod 	- 0.
**	      .rd_gc_tid	- Internal descriptor id.
**	      .rd_gc_l_cols	- Set to rd_numcols; only used by GCA.
**	      .rd_gc_col_desc[] - Array of DBV structures and zero filler.
**				  This array has rd_dballoc elements but there
**				  are exactly rd_numcols in use.
**	   .rd_nmalloc	- If there were too few names then they will be freed
**			  and the new number allocated will be set.
**	   .rd_names[]  - Array of varying length (upto RD_MAXNAME) objects.
**			  This array has rd_nmalloc elements but there are
**			  exactly rd_numcols in use and ONLY if rd_flags
**			  includes the NAMES modifier.
**	Returns:
**	    STATUS - OK (success) or FAIL (failure).
**	Errors:
**	  RETRIEVE/SELECT
**	    E_LQ008A_RDNODESC - Null descriptor pointer.
**	    E_LQ008B_RDTUPSIZE - Bad read of tuple size.
**	    E_LQ0091_RDFLAGS - bad read of flags field.
**	    E_LQ008C_RDCOLS - Bad read of number of columns.
**	    E_LQ008D_RDALLOC - Allocations of descriptor failed.
**	    E_LQ008E_RDTYPE - Bad read of DBV.
**	    E_LQ008F_RDCOLEN - Bad read of column length.
**	    E_LQ0090_RDCONAME - Bad read of column name.
**	    E_LQ0092_RDTOOBIG - Too many columns (descriptor is wrong).
**	    E_LQ0093_RDTID - Bad read of tuple descriptor id.
**	  DATABASE PROCEDURES (BYREF parameters)
**	    E_LQ00AD_PROCTYPE - Bad read of DBV.
**	    E_LQ00AE_PROCCOLEN - Bad read of column length.
**	    E_LQ00AF_RDCONAME - Bad read of column name.
**	    E_LQ00B3_PROCNODESC - Null descriptor pointer.
**	    E_LQ00B4_PROCTUPSIZE - Bad read of tuple size.
**	    E_LQ00B5_PROCFLAGS - bad read of flags field.
**	    E_LQ00B6_RDTID - Bad read of tuple descriptor id.
**	    E_LQ00B7_PROCCOLS - Bad read of number of columns.
**	    E_LQ00B8_RDTOOBIG - Too many columns (descriptor is wrong).
**	    E_LQ00B9_PROCALLOC - Allocations of descriptor failed.
**
**  History:
**	24-mar-1987	- Written for new communications. (ncg)
**	20-may-1988	- Modified for INGRES/NET.  New descriptor. (ncg)
**	22-jun-1988	- Added new descriptor id for INGRES/NET. (ncg)
**	30-apr-1990	- Recognize GCA_READONLY_MASK. (ncg)
**	15-dec-1992 (larrym)
**	    Added support for database procedures passed BYREF.  This
**	    required only modifications to error messages.  The code is
**	    basically unchanged.
**	04-apr-1996 	- Modified to support compressed variable-length
**			  datatypes. (loera01)
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer contains DB_DATA_VALUE, so need to
**	    access members individually.
*/

STATUS
IIrdDescribe(desc_type, row_desc)
i4		desc_type;
ROW_DESC	*row_desc;			/* Users row descriptor */
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    register ROW_DESC	*rd;		/* Local row descriptor */
    register ROW_COLNAME *rcn;		/* Pointer for column name data */
    register i4	col;		/* Index into arrays */
    DB_DATA_VALUE	db_tmp;		/* General DBV for descriptor */
    DB_DATA_VALUE	db_nmlen;	/* 2 DBV's tailored for the names */
    DB_DATA_VALUE	db_nm;
    i4		tup_size;	/* Get tup size, id and num cols */
    i4		tup_id;
    i4		num_cols;
    i4			desc_mask;	/* Descriptor mask */
    i4			db_data;	/* Column data (unused) */
    i4			db_length;	/* Column length */
    i2			db_type;	/* Column type */
    i2			db_prec;	/* Column precision */
    i4			name_len;	/* Length of column names */
    i4  		resval;		/* Returns bytes from communication */
    char		ebuf[20];	/* For error messages */

    /* Point to users row descriptor */
    if ((rd = row_desc) == (ROW_DESC *)0)
    {
	if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	{
	    IIlocerr(GE_LOGICAL_ERROR, E_LQ008A_RDNODESC, II_ERR, 0, (char *)0);
	}
	else
	{
	    IIlocerr(GE_LOGICAL_ERROR, E_LQ00B3_PROCNODESC, II_ERR,0,(char *)0);
	}
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return FAIL;
    }
	
    /* Assume an error will occur */
    rd->rd_flags = RD_ERROR;
    rd->rd_numcols = 0;				/* Assigned at the end */

    /* Beginning of retrieve - get tuple size */
    II_DB_SCAL_MACRO(db_tmp, DB_INT_TYPE, sizeof(tup_size), &tup_size);
    resval = IIcgc_get_value(IIlbqcb->ii_lq_gca, desc_type,
			     IICGC_VVAL, &db_tmp);
    if (resval != sizeof(tup_size))
    {
	if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	{
	    IIlocerr(GE_COMM_ERROR, E_LQ008B_RDTUPSIZE, II_ERR, 0, (char *)0);
	}
	else
	{
	    IIlocerr(GE_COMM_ERROR, E_LQ00B4_PROCTUPSIZE, II_ERR, 0, (char *)0);
	}
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return FAIL;
    }

    /* Get name flags */
    II_DB_SCAL_MACRO(db_tmp, DB_INT_TYPE, sizeof(desc_mask), &desc_mask);
    resval = IIcgc_get_value(IIlbqcb->ii_lq_gca, desc_type,
			     IICGC_VVAL, &db_tmp);
    if (resval != sizeof(desc_mask))
    {
	if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	{
	    IIlocerr(GE_COMM_ERROR, E_LQ0091_RDFLAGS, II_ERR, 0, (char *)0);
	}
	else
	{
	    IIlocerr(GE_COMM_ERROR, E_LQ00B5_PROCFLAGS, II_ERR, 0, (char *)0);
	}
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return FAIL;
    }
    if (desc_mask & GCA_NAMES_MASK)
	rd->rd_flags |= RD_NAMES;
    if (desc_mask & GCA_READONLY_MASK)
	rd->rd_flags |= RD_READONLY;
    /*
    ** If back end has set GCA_COMP_MASK, set RD_CVCH so that the rest of
    ** LIBQ knows that tuple to follow contains a compressed datatype.
    */
    if (desc_mask & GCA_COMP_MASK)
    {
        rd->rd_flags |= RD_CVCH;
    }
    /* Get tuple id */
    II_DB_SCAL_MACRO(db_tmp, DB_INT_TYPE, sizeof(tup_id), &tup_id);
    resval = IIcgc_get_value(IIlbqcb->ii_lq_gca, desc_type,
			     IICGC_VVAL, &db_tmp);
    if (resval != sizeof(tup_id))
    {
	if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	{
	    IIlocerr(GE_COMM_ERROR, E_LQ0093_RDTID, II_ERR, 0, (char *)0);
	}
	else
	{
	    IIlocerr(GE_COMM_ERROR, E_LQ00B6_PROCTID, II_ERR, 0, (char *)0);
	}
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return FAIL;
    }

    /* Get number of columns  - but assign it at the end */
    II_DB_SCAL_MACRO(db_tmp, DB_INT_TYPE, sizeof(num_cols), &num_cols);
    resval = IIcgc_get_value(IIlbqcb->ii_lq_gca, desc_type,
			     IICGC_VVAL, &db_tmp);
    if (resval != sizeof(num_cols))
    {
	if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	{
	    IIlocerr(GE_COMM_ERROR, E_LQ008C_RDCOLS, II_ERR, 0, (char *)0);
	}
	else
	{
	    IIlocerr(GE_COMM_ERROR, E_LQ00B7_PROCCOLS, II_ERR, 0, (char *)0);
	}
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return FAIL;
    }

    /* If no columns (ie, DESCRIBE non-SELECT) then we're done */
    if (num_cols == 0)
    {
	rd->rd_flags = RD_DEFAULT;
	rd->rd_numcols = num_cols;
	return OK;
    }
    else if (num_cols > RD_MAX_COLS)	/* Check for limit */
    {
	CVla(num_cols, ebuf);
	if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	{
	    IIlocerr(GE_CARDINALITY, E_LQ0092_RDTOOBIG, II_ERR, 1, ebuf);
	}
	else
	{
	    IIlocerr(GE_CARDINALITY, E_LQ00B8_PROCTOOBIG, II_ERR, 1, ebuf);
	}
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return FAIL;
    }

    /*
    ** Allocation/deallocation of existing DBV descriptors.  Check if new
    ** descriptor will fit into already allocated one.
    */
    if (rd->rd_dballoc == 0)
	rd->rd_gca = (ROW_GCA *)0;
    else if (rd->rd_gca && rd->rd_dballoc < num_cols)
    {
	MEfree((PTR)rd->rd_gca);
	rd->rd_gca = (ROW_GCA *)0;
    }
    if (rd->rd_gca == (ROW_GCA *)0) 	/* Need to allocate a new one */
    {
	if ((rd->rd_gca =
		(ROW_GCA *)MEreqmem((u_i4)0,
				    (u_i4)(ROW_GCA_SIZE_MACRO(num_cols)),
				    TRUE, (STATUS *)0)) == (ROW_GCA *)0)
	{
	    if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	    {
	        IIlocerr(GE_NO_RESOURCE, E_LQ008D_RDALLOC, II_ERR,0,(char *)0);
	    }
	    else
	    {
	        IIlocerr(GE_NO_RESOURCE, E_LQ00B9_PROCALLOC,II_ERR,0,(char *)0);
	    }
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return FAIL;
	}
	rd->rd_dballoc = num_cols;
    }

    /*
    ** Allocation/deallocation of existing name structures.  Check if (1)
    ** we need names, and (2) they will fit into already allocated array.
    */
    if (rd->rd_flags & RD_NAMES)
    {
	if (rd->rd_nmalloc == 0)
	    rd->rd_names = (ROW_COLNAME *)0;
	else if (rd->rd_names && rd->rd_nmalloc < num_cols)
	{
	    MEfree((PTR)rd->rd_names);
	    rd->rd_names = (ROW_COLNAME *)0;
	}
	if (rd->rd_names == (ROW_COLNAME *)0) 	/* Need to allocate a new one */
	{
	    if ((rd->rd_names =
		   (ROW_COLNAME *)MEreqmem((u_i4)0,
				    (u_i4)(num_cols * sizeof(ROW_COLNAME)),
				    TRUE, (STATUS *)0)) == (ROW_COLNAME *)0)
	    {
	    	if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	   	{
	            IIlocerr(GE_NO_RESOURCE, E_LQ008D_RDALLOC, 
			II_ERR, 0, (char *)0);
	    	}
	    	else
	    	{
	            IIlocerr(GE_NO_RESOURCE, E_LQ00B9_PROCALLOC,
			II_ERR, 0, (char *)0);
	    	}
		IIlbqcb->ii_lq_curqry |= II_Q_QERR;
		return FAIL;
	    }
	    rd->rd_nmalloc = num_cols;
	}
    } /* if names too */

    /* Set up DBV's to retrieve the series of column descriptors */
    II_DB_SCAL_MACRO(db_nmlen, DB_INT_TYPE, sizeof(name_len), 0);
    II_DB_SCAL_MACRO(db_nm, DB_CHR_TYPE, 0, 0);

    /* Fill initial part of GCA descriptor */
    rd->rd_gca->rd_gc_tsize   = tup_size;
    rd->rd_gca->rd_gc_res_mod = 0;
    rd->rd_gca->rd_gc_tid     = tup_id;
    rd->rd_gca->rd_gc_l_cols  = 0;	/* Set on successful return */

    /*
    ** Fill descriptor with DBV data and optional names. We always need to
    ** retrieve the DBV, and the varying length name count, but only if
    ** the NAMES mask is on, and the count > 0 then we need to retrieve a 
    ** name string.  
    **
    ** Loop through the arrays filling the pieces.
    */
    for (col = 0; col < num_cols; col++)
    {
	/* Preset the name length for the INGRES/NET descriptor */
	rd->rd_gca->rd_gc_col_desc[col].rd_gc_l_name = 0;

	/* Get db_data (unused) */
	II_DB_SCAL_MACRO(db_tmp, DB_INT_TYPE, sizeof(db_data), &db_data);
	resval = IIcgc_get_value(IIlbqcb->ii_lq_gca, desc_type,
				 IICGC_VVAL, &db_tmp);
	if (resval != sizeof(db_data))
	{
	    CVna( col +1, ebuf );
	    if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	    {
		IIlocerr(GE_COMM_ERROR, E_LQ008E_RDTYPE, II_ERR, 1, ebuf);
	    }
	    else
	    {
		IIlocerr(GE_COMM_ERROR, E_LQ00AD_PROCTYPE, II_ERR, 1, ebuf);
	    }
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return FAIL;
	}

	/* Get db_length */
	II_DB_SCAL_MACRO(db_tmp, DB_INT_TYPE, sizeof(db_length), &db_length);
	resval = IIcgc_get_value(IIlbqcb->ii_lq_gca, desc_type,
				 IICGC_VVAL, &db_tmp);
	if (resval != sizeof(db_length))
	{
	    CVna( col +1, ebuf );
	    if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	    {
		IIlocerr(GE_COMM_ERROR, E_LQ008E_RDTYPE, II_ERR, 1, ebuf);
	    }
	    else
	    {
		IIlocerr(GE_COMM_ERROR, E_LQ00AD_PROCTYPE, II_ERR, 1, ebuf);
	    }
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return FAIL;
	}

	/* Get db_datatype */
	II_DB_SCAL_MACRO(db_tmp, DB_INT_TYPE, sizeof(db_type), &db_type);
	resval = IIcgc_get_value(IIlbqcb->ii_lq_gca, desc_type,
				 IICGC_VVAL, &db_tmp);
	if (resval != sizeof(db_type))
	{
	    CVna( col +1, ebuf );
	    if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	    {
		IIlocerr(GE_COMM_ERROR, E_LQ008E_RDTYPE, II_ERR, 1, ebuf);
	    }
	    else
	    {
		IIlocerr(GE_COMM_ERROR, E_LQ00AD_PROCTYPE, II_ERR, 1, ebuf);
	    }
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return FAIL;
	}

	/* Get db_prec */
	II_DB_SCAL_MACRO(db_tmp, DB_INT_TYPE, sizeof(db_prec), &db_prec);
	resval = IIcgc_get_value(IIlbqcb->ii_lq_gca, desc_type,
				 IICGC_VVAL, &db_tmp);
	if (resval != sizeof(db_prec))
	{
	    CVna( col +1, ebuf );
	    if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	    {
		IIlocerr(GE_COMM_ERROR, E_LQ008E_RDTYPE, II_ERR, 1, ebuf);
	    }
	    else
	    {
		IIlocerr(GE_COMM_ERROR, E_LQ00AD_PROCTYPE, II_ERR, 1, ebuf);
	    }
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return FAIL;
	}

	/*
	** Save column info
	*/
	rd->rd_gca->rd_gc_col_desc[col].rd_dbv.db_datatype = db_type;
	rd->rd_gca->rd_gc_col_desc[col].rd_dbv.db_length = db_length;
	rd->rd_gca->rd_gc_col_desc[col].rd_dbv.db_prec = db_prec;

	/*
	** Mark if there's a BLOB - allows users to figure out pre-allocation:
	*/

	if (IIDB_LONGTYPE_MACRO(
		(DB_DATA_VALUE *)(&rd->rd_gca->rd_gc_col_desc[col].rd_dbv))
	   )
		rd->rd_flags |= RD_UNBOUND;

	/* Retrieve the name count - always there */
	db_nmlen.db_data = (PTR)&name_len;
	resval = IIcgc_get_value(IIlbqcb->ii_lq_gca, desc_type,
				 IICGC_VVAL, &db_nmlen);
	if (resval != sizeof(name_len))
	{
	    CVna( col +1, ebuf );
	    if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	    {
	        IIlocerr(GE_COMM_ERROR, E_LQ008F_RDCOLEN, II_ERR, 1, ebuf);
	    }
	    else
	    {
	        IIlocerr(GE_COMM_ERROR, E_LQ00AE_PROCCOLEN, II_ERR, 1, ebuf);
	    }
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return FAIL;
	}

	if ((rd->rd_flags & RD_NAMES))  	/* Retrieve name info */
	{
	    rcn = &rd->rd_names[col];
	    rcn->rd_nmlen = min(name_len, RD_MAXNAME);

	    /*
	    ** Retrieve the actual name, but make sure it can fit in our
	    ** varying-length/fixed-length buffers.
	    */
	    db_nm.db_length = rcn->rd_nmlen;
	    db_nm.db_data = (PTR)rcn->rd_nmbuf;
	    if (name_len > 0)			/* Get as much as fits */
	    {
		resval = IIcgc_get_value(IIlbqcb->ii_lq_gca, desc_type,
					 IICGC_VVAL, &db_nm);
		if (resval != name_len)
		{
		    CVna( col +1, ebuf );
	    	    if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	   	    {
		        IIlocerr(GE_COMM_ERROR, E_LQ0090_RDCONAME, 
			II_ERR, 1, ebuf);
	    	    }
	    	    else
	    	    {
		        IIlocerr(GE_COMM_ERROR, E_LQ00AF_PROCCONAME, 
			II_ERR, 1, ebuf);
	    	    }
		    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
		    return FAIL;
		}
	    }

	    if (name_len > RD_MAXNAME)		/* Need to skip the rest? */
	    {
		/* Retrieve (skip) the rest of the chars */
		db_nm.db_length = name_len - RD_MAXNAME;
		db_nm.db_data = (PTR)0;
		_VOID_ IIcgc_get_value(IIlbqcb->ii_lq_gca, desc_type,
				       IICGC_VVAL, &db_nm);
	    } /* if name too big */
	}
	else if (name_len > 0)		/* No NAMES flag but name length > 0 */ 
	{
	    /* Skip the rest of the chars */
	    db_nm.db_length = name_len;
	    db_nm.db_data = (PTR)0;
	    _VOID_ IIcgc_get_value(IIlbqcb->ii_lq_gca, desc_type,
				   IICGC_VVAL, &db_nm);
	} /* if names too */
    } /* for  each element */

    /* We're done */
    rd->rd_flags &= ~RD_ERROR;
    rd->rd_gca->rd_gc_l_cols  = num_cols;
    rd->rd_numcols = num_cols;
    return OK;
}
