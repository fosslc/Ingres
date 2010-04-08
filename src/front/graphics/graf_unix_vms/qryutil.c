/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ergr.h>
# include	<er.h>
# include	<ui.h>
# include	<ug.h>
# include	<adf.h>
# include	<afe.h>
# include	<graf.h>
# include	<dvect.h>
# include	"qry.h"
# include	"grcoerce.h"

VOID	qry_deltab();	/* forward declaration */

/**
** Name:    qryutil.qc	- utility routines for GR library query support
**
** Description:
**	Routines for graf lib which keep track of column information in
**	views by name, and what type they ought to be coerced into.
**	Written to support keeping information on several tables at once,
**	with the ability to delete information individually.
**
** History:
**	circa late 1985 (bobm)	written
**
**	2/87 (bobm)	Changes for ADT and NULL support.
**	20-may-88 (bruceb)
**		Changed FEtalloc calls into calls on FEreqmem.
**	09/07/92 (dkh) - Updated with 6.5 call interface to FEatt_open.
**	10/02/92 (dkh) - Added support for owner.table and delim ids.
**			 Note that the name of the table is in unnormalized
**			 form while column names are normalized.
**	06/19/93 (dkh) - Added additional checks to make sure that user
**			 did not enter a table name that begins or ends
**			 with a period that is not part of a delimited id.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

extern	ADF_CB	*GR_cb;

VOID		qryutil_deltab();	/* forward declaration */

static TABLEATT *Tabdat = NULL;

/*{
** Name:	qry_ftab - get table information.
**
** Description:
**	Returns information about a view.
**
** Inputs:
**	name	name of view desired
**
** Outputs:
**	pred	pointer to predecessor on linked list.	Should not
**		be used except internally to this file, ie. other
**		callers just pass in the address of a pointer to
**		fill in which they may throw away.  filled in with
**		NULL if head of list.
**
**	Returns:
**		Pointer to TABLEATT structure, NULL for failure.
*/

TABLEATT *
qry_ftab(name,pred)
char *name;
TABLEATT **pred;
{
	TABLEATT *ptr;

	*pred = NULL;
	for (ptr = Tabdat; ptr != NULL && STcompare(ptr->name,name) != 0;
								ptr = ptr->link)
		*pred = ptr;
	return (ptr);
}

/*{
** Name:	qry_att - get column information array.
**
** Description:
**	Returns COLDATA array for a table or view
**
** Inputs:
**	tname	name of view desired
**
** Outputs:
**	col	COLDATA array
**
**	Returns:
**		number of items, -1 for failure
*/

qry_att(tname,col)
char *tname;
COLDATA **col;
{
	TABLEATT *tptr,*tpred;

	if ((tptr = qry_ftab(tname,&tpred)) == NULL)
		return (-1);

	*col = tptr->col;
	return (tptr->colnum);
}

/*{
** Name:	qry_col - get column information.
**
** Description:
**	Returns information about a column of a view.
**
** Inputs:
**	tname	name of view desired
**	cname	name of column desired
**
** Outputs:
**
**	Returns:
**		Pointer to COLDATA structure, NULL for failure.
*/

COLDATA *
qry_col(tname,cname)
char *tname;
char *cname;
{
	i4 i,num;
	COLDATA *col;

	num = qry_att(tname,&col);
	if (num < 0)
		return (NULL);

	for (i=0; i < num; ++i)
	{
		if (STcompare(col[i].name,cname) == 0)
			return (col + i);
	}

	return (NULL);
}

/*{
** Name:	qry_addtab - add information about a new table or view
**
** Description:
**	gets the information about a new table or view and adds its to
**	the list of known tables.
**
** Inputs:
**	tabname		name of table or view
**
** Outputs:
**
**	Returns:
**		STATUS, OK / FAIL
**
** Side effects:
**	allocates memory.
**
** History:
**	09-oct-1986 (sandyd)
**		Saturn support.	 Added FEconnect() before accessing
**		"attribute" relation.  Also fixed the standard undiscovered
**		bug: retrieve on "attribute" was not qualified by owner.
**	2/87 (bobm)	changes for ADT usage / removal of unneeded flags.
*/

STATUS
qry_addtab(tabname)
char *tabname;
{
	i2 tag;
	i4 dlen;		/* column counter / actual array length */
	i4 slen;		/* real number of supported columns */
	TABLEATT *addr;

	ADI_FI_DESC	fdesc;
	AFE_OPERS	opers;
	DB_DATA_VALUE	dbflt;
	COLDATA		*cp;
	bool		f_ok;
	ADI_OP_ID	opid;
	DB_DATA_VALUE	*opar[2];

	FE_ATT_QBLK	qblk;
	FE_ATT_INFO	info;

	FE_RSLV_NAME	delim_id;
	char		town[FE_UNRML_MAXNAME + 1];
	char		ttbl[FE_UNRML_MAXNAME + 1];
	char		owner[FE_MAXNAME + 1];
	char		table[FE_MAXNAME + 1];

	tag = FEgettag();
	if ((addr = (TABLEATT *)FEreqmem((u_i4)tag, (u_i4)sizeof(TABLEATT),
		(bool)FALSE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("qry_addtab - table"));
	}
	addr->link = Tabdat;
	Tabdat = addr;

	/*
	**  Passed in "tabnmae" is assumed to be in unnormalized form
	**  with appropriate quotes, etc.  Also, in can be in the
	**  form of owner.table.
	*/
	addr->tag = tag;
	addr->name = FEtsalloc(tag,tabname);

	dlen = 0;
	slen = 0;
	cp = addr->col;

	/*
	**  Break things up before calling FEatt_open.
	*/
	delim_id.is_nrml = FALSE;
	delim_id.owner_dest = town;
	delim_id.name_dest = ttbl;
	delim_id.name = tabname;

	FE_decompose(&delim_id);

	/*
	**  Check to make sure user did not enter a name that
	**  simply begins or ends in a period outside of a delimited
	**  identifier.  If the name
	**  begins with a period, FE_decompose will have set
	**  the owner_spec member of FE_RESLV_NAME but that
	**  owner_dest contains an empty string.  If the name
	**  ends in a period, then the name_dest member will
	**  contain an empty string.
	*/
	if ((delim_id.owner_spec && delim_id.owner_dest[0] == EOS) ||
		delim_id.name_dest[0] == EOS)
	{
		return(FAIL);
	}

	if (town[0] != EOS)
	{
		if (IIUGdlm_ChkdlmBEobject(town, owner, FALSE) == UI_BOGUS_ID)
		{
			return(FAIL);
		}
	}
	else
	{
		owner[0] = EOS;
	}

	if (IIUGdlm_ChkdlmBEobject(ttbl, table, FALSE) == UI_BOGUS_ID)
	{
		return(FAIL);
	}

	if (FEatt_open(&qblk, table, owner) != OK)
		return(FAIL);

	while (FEatt_fetch(&qblk,&info) == OK)
	{
		if (dlen >= DB_GW2_MAX_COLS)
			break;

		/*
		**  Only proceed if we have a datatype that we can handle.
		**  Issue the warning only once.
		*/

		cp->name = FEtsalloc(tag,info.column_name);

		/*
		** fill in type of DB_DATA_VALUE items, alloc data areas.
		** separate alloc's so each one is aligned.
		*/
		FEatt_dbdata(&info,&(cp->db));
		STRUCT_ASSIGN_MACRO(cp->db,cp->dbf);

		/* init flags */
		cp->qflag = cp->xflag = cp->fld = FALSE;
		cp->supported = TRUE;

		/*
		**  If not a supported datatype, set flag and continue;
		*/
		if (!IIAFfedatatype(&(cp->db)))
		{
		/*
			cp->supported = FALSE;
			++dlen;
			++cp;
		*/
			continue;
		}

		if ((cp->db.db_data = FEreqmem((u_i4)tag,
			(u_i4)(cp->db.db_length), (bool)FALSE,
			(STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("qry_addtab - DBDV1"));
		}
		if ((cp->dbf.db_data = FEreqmem((u_i4)tag,
			(u_i4)(cp->db.db_length), (bool)FALSE,
			(STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("qry_addtab - DBDV2"));
		}

		/* 
		** We'll be coercing to i2, but leave space for the null
		** indicator.
		*/
		if ((cp->res.db_data = FEreqmem((u_i4)tag, (u_i4)4,
			(bool)FALSE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("qry_addtab - DBDV3"));
		}

		/*
		** set type of coercion to graphics data type:
		** see if it's something we can get a date part out of.
		** if so, make GRDV_DATE.  Otherwise, if coercible to
		** float, GRDV_NUMB.  If neither, GRDV_STRING.
		*/
		if (afe_opid(GR_cb,ERx("date_part"),&opid) != OK)
			IIUGerr(S_GR0021_afe_opid, UG_ERR_FATAL, 0);
		opers.afe_opcnt = 2;
		opers.afe_ops = opar;
		opar[0] = &dbflt;
		dbflt.db_datatype = DB_CHA_TYPE;
		dbflt.db_length = 5;
		opar[1] = &(cp->db);
		cp->res.db_datatype = DB_INT_TYPE;
		cp->res.db_length = 2;
		if (afe_fdesc(GR_cb,opid,&opers,NULL,&(cp->res),&fdesc) == OK)
		{
			cp->type = GRDV_DATE;
			cp->funcid = fdesc.adi_finstid;
		}
		else
		{
			dbflt.db_datatype = DB_FLT_TYPE;
			if (afe_cancoerce(GR_cb,&(cp->db),&dbflt,&f_ok) != OK)
				IIUGerr(S_GR0022_afe_cancoerce, UG_ERR_FATAL, 0);
			if (f_ok)
				cp->type = GRDV_NUMB;
			else
				cp->type = GRDV_STRING;
		}
		++dlen;
		++slen;
		++cp;
	}

	if (dlen <= 0)
	{
		qry_deltab(tabname);
		return (FAIL);
	}

	addr->colnum = dlen;
	addr->scolnum = slen;

	return (OK);
}

/*{
** Name:	qry_deltab,qry_dalltab - delete information about tables
**
** Description:
**	remove a tables / views from the list and free the memory.
**	qry_deltab removes a specific table from the list
**	qry_dalltab removes the entire list.
**
** Inputs:
**	tabname		name of table or view (qry_deltab only)
**
** Side effects:
**	frees memory.
**
** History:
**	2/87 (bobm)	added qry_dalltab
*/

VOID
qry_deltab(tabname)
char *tabname;
{
	TABLEATT *optr,*ptr;
	i4 i;

	if ((ptr = qry_ftab(tabname,&optr)) != NULL)
	{
		if (optr != NULL)
			optr->link = ptr->link;
		else
			Tabdat = ptr->link;
		FEfree (ptr->tag);
	}
}

VOID
qry_dalltab()
{
	TABLEATT *optr,*ptr;

	for (ptr = Tabdat; ptr != NULL; ptr = optr)
	{
		optr = ptr->link;
		FEfree (ptr->tag);
	}
	Tabdat = NULL;
}

/*{
** Name:	qry_fstr, qry_funmap - allocate strings for fields
**
** Description:
**	Allocate (with GRtxtstore) strings for a field.	 The data
**	is contained in the dbf DB_DATA_VALUE of the COLDATA for
**	the field, if it exists.  qry_funmap generates the string
**	for fields whose columns don't exist.  If the column exists,
**	and its field data is filled in, GRcoerce_type is used to
**	generate the string.
**
** Inputs:
**	tab	name of table or view
**	iname	field internal name (only argument to qry_funmap)
**	colname column name
**
** Outputs:
**
**	Returns:
**		pointer to stored string.
**
** Side effects:
**	allocates memory.
**
** History:
**	2/87 (bobm)	changes for ADT's.
*/

char *
qry_fstr (tab,iname,colname)
char *tab;
char *iname;	/* internal field name */
char *colname;	/* column name */
{
	char		bufr[GRC_SLEN+1];
	DB_TEXT_STRING	*text;
	DB_DATA_VALUE	dbstr;
	COLDATA		*col;
	GRC_STORE	store;

	char	*GRtxtstore();
	char	*qry_funmap();

	if (tab == NULL || *tab == EOS ||
				colname == NULL || *colname == EOS ||
				(col = qry_col(tab,colname)) == NULL ||
				! col->fld )
		return (qry_funmap(iname));
	else
	{
		dbstr.db_data = (PTR) &store;
		GRcoerce_type(col, &(col->dbf), &dbstr, bufr);
		return(GRtxtstore(bufr));
	}
}


char *
qry_funmap (name)
char *name;
{
	char buf[GRC_SLEN+1];	/* better fit in this */
	char *GRtxtstore();

	IIUGfmt(buf, GRC_SLEN+1, ERget(F_GR0001_Field_map), 1, name);
	return (GRtxtstore(buf));
}
