/*
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>
# include	<erui.h>
# include	<dictutil.h>
# include	<ui.h>
# include	<ug.h>

/**
** Name:	cleanup.sc - remove garbage from fe catalogs
**
** Description:
**	Cleanup for FE catalogs.  Defines:
**
**	IIUIdgDeleteGarbage
**	IIUInslNoSessionLocks
*	IIUIadaAddDepAppl
**
** History:
**
**	2/90 (bobm)	Written
**	7/18/91 (bobm)	Fix for 38709 - check ii_encodings against ii_entities
**			(Windows4GL table) as well as ii_objects.
**	7/19/91 (pete)	Followup to what Bob did above: change table of
**			cleanup info so ii_encodings is only cleaned up for
**			CORE. Was being cleaned for both CORE and for APPDEV1.
**			The ii_encodings cleanup is considerably slower now.
**	21-jul-1993 (rdrane)
**		Extensive modification to ensure that FErelexists() is always
**		passed the catalog name in the proper case, so that FIPS
**		databases are supported.
**	05-Aug-1993 (fredv)
**		The arguments in the STcopy call were reversed;thus, Cats
**		got corrupted, fixed it.
**  28-mar-95 (angusm)
**          Since default in server is not to flatten queries,
**          some of the subselects here perform badly.
**          Force flattening with SET TRACE POINT OP133 at start
**          of this portion of processing
**	04-mar-95 (lawst01)
**	   Windows 3.1 port changes - added include <ui.h.> and <ug.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Buffer in which to effect casing of table names
** prior to passing to FErelexists().  Since we deal
** exclusively with non-compound, regular identifiers, we don't
** need to size the buffer as FE_UNRML_MAXNAME or FE_MAXTABNAME.
*/
static	char	table_buf[(FE_MAXNAME + 1)];
STATUS IIUIffuFidFixUp();
STATUS IIUIadaAddDepAppl();

static struct driver
{
	char *table;
	char *column;
	u_i4 mask;	/* module(s) table applies to */
	bool doit;	/* set at runtime */
	bool chkent;	/* check ii_entities table */
}
Cats[] =
{
	{ ERx("ii_abfobjects"), ERx("object_id"), DMB_APPDEV1, TRUE, FALSE },
	{ ERx("ii_abfobjects"), ERx("applid"), DMB_APPDEV1, TRUE, FALSE },
	{ ERx("ii_encodings"), ERx("encode_object"), DMB_CORE, TRUE, TRUE },
	{ ERx("ii_framevars"), ERx("object_id"), DMB_APPDEV3, TRUE, FALSE },
	{ ERx("ii_longremarks"), ERx("object_id"), DMB_CORE, TRUE, FALSE },
	{ ERx("ii_menuargs"), ERx("object_id"), DMB_APPDEV3, TRUE, FALSE },
	{ ERx("ii_abfclasses"), ERx("catt_id"), DMB_APPDEV1, TRUE, FALSE },
	{ ERx("ii_vqjoins"), ERx("object_id"), DMB_APPDEV3, TRUE, FALSE },
	{ ERx("ii_vqtabcols"), ERx("object_id"), DMB_APPDEV3, TRUE, FALSE },
	{ ERx("ii_vqtables"), ERx("object_id"), DMB_APPDEV3, TRUE, FALSE }
};

static char *Fstr =
ERx("DELETE FROM %s WHERE %s NOT IN ( SELECT object_id FROM ii_objects )");
static char *Fstr2 =
ERx("DELETE FROM %s WHERE %s NOT IN ( SELECT object_id FROM ii_abfobjects )");
static char *FstrWEnt =
ERx("DELETE FROM %s WHERE %s NOT IN ( SELECT object_id FROM ii_objects ) AND %s NOT IN ( SELECT entity_id FROM ii_entities )");

/*{
** Name:	IIUIdgDeleteGarbage
**
** Description:
**	Removes unused records from the FE catalogs.  These records may
**	be left around when FE objects are deleted - one only has to
**	delete the ii_objects record to make an FE object unreachable.
**	Rather than delete records from the other catalogs joined to
**	ii_objects, making for slow deletes, we leave them for eventual
**	garbage collection by this routine.
**
** Inputs:
**	which	bit mask of which modules to cleanup, 0 for all.
**	verbose	TRUE to produce informational messages during cleanup.
**	
**
** Outputs:
**
**	count - count of rows deleted.
**
**	returns STATUS of first failed query.  Will go ahead and continue
**	with all queries in any case, in order to clean up all the junk we
**	can, at the cost of getting more failed queries.
**
** History:
**
**	2/90 (bobm)	Written
**	7/91 (bobm)	Fix for 38709 - check ii_encodings against ii_entities
**			(Windows4GL table) as well as ii_objects.
**	27-jul-1992 (rdrane)
**		Change parameterization of call to FErelexists() to include
**		owner (actually, we can get away with (char *)NULL -
**		users shouldn't have ii* tables).
**	22-jul-1993 (rdrane)
**		Effect case translation of catalog names prior to calling
**		FErelexists() for FIPS support.
*/

STATUS
IIUIdgDeleteGarbage(which,verbose,count)
u_i4 which;
bool verbose;
i4  *count;
{
	i4	i;
	i4	tot;
	STATUS	rval;
	STATUS	op;
	i4	depcount;
	struct driver	*tn;
	bool	have_ent;

	EXEC SQL BEGIN DECLARE SECTION;
	i4	ooclass;
	char	tbuf[240];
	i4	rc;
	EXEC SQL END DECLARE SECTION;

	if (which == 0)
		which = 0xffffL;

	iiuicw1_CatWriteOn();

	rval = OK;
	*count = 0;
/*
**  bug 65653:
**  force flattening for the "delete from where not in (select * from y)"
**  type of query which we use here.
**
*/
    EXEC SQL SET TRACE POINT OP133;


	/*
	** get rid of unused abfdependencies before finding unreferenced
	** IL records.  Use the first condition to gate the case translation
	** of the ii_abfdependencies catalog name.
	*/
	if  ((which & DMB_APPDEV1) != 0)
	{
		STcopy(ERx("ii_abfdependencies"),&table_buf[0]);
		IIUGdbo_dlmBEobject(&table_buf[0],FALSE);
	}
	if ((which & DMB_APPDEV1) != 0 &&
			FErelexists(&table_buf[0],NULL) == OK)
	{
		if (verbose)
			IIUGmsg(ERget(S_UI0020_Depend),FALSE,0);
		STprintf(tbuf,Fstr,ERx("ii_abfdependencies"),ERx("object_id"));
		EXEC SQL EXECUTE IMMEDIATE :tbuf;
		op = FEinqerr();
		if (op == OK)
			*count += FEinqrows();
		else
		{
			if (rval == OK)
				rval = op;
		}
		STprintf(tbuf,Fstr2,ERx("ii_abfdependencies"),ERx("object_id"));
		EXEC SQL EXECUTE IMMEDIATE :tbuf;
		op = FEinqerr();
		if (op == OK)
			*count += FEinqrows();
		else
		{
			if (rval == OK)
				rval = op;
		}

		/* Check for ii_abfdependencies records which 6.3 has mangled */
		_VOID_ IIUIadaAddDepAppl(-1, verbose, TRUE, &depcount);
		count += depcount;


		/*
		** Delete unused IL object records from ii_objects.  This will
		** allow unused IL to get deleted from ii_encodings.
		**
		** Background:
		**
		** We are worried about two ways IL dependencies have been
		** entered into ii_abfdependencies and ii_objects.  Some
		** names have "fid" prepended to the number, and later simply
		** the id without the "fid" prefix was getting used for the
		** dependency.  BUT, the name was STILL being entered with
		** "fid" into ii_objects.  Currently, both are
		** entered as pure numeric.
		**
		** Because of difficulties with open SQL (no conversion
		** of numeric to character), we check for old style
		** names, and convert them before doing the delete.
		**
		** This will involve cursors, so we avoid it if possible.  If
		** conversion fails, we make sure we do not do the delete from
		** ii_objects.
		*/
		ooclass = OC_ILCODE;

		EXEC SQL SELECT COUNT(*) INTO :rc
			FROM ii_objects
			WHERE object_name LIKE 'fid%'
					AND object_class = :ooclass;

		op = FEinqerr();

		if (op == OK && rc == 0)
		{
			EXEC SQL SELECT COUNT(*) INTO :rc
				FROM ii_abfdependencies
				WHERE abfdef_name LIKE 'fid%'
					AND object_class = :ooclass;

			op = FEinqerr();
		}

		if (op == OK && rc > 0)
		{
			if (verbose)
				IIUGmsg(ERget(S_UI0021_FidFix),FALSE,0);
			op = IIUIffuFidFixUp();
			iiuicw1_CatWriteOn();
		}

		if (op == OK)
		{
			EXEC SQL DELETE FROM ii_objects
				WHERE object_class = :ooclass AND
				object_name NOT IN
				( SELECT abfdef_name FROM ii_abfdependencies
					WHERE object_class = :ooclass );

			op = FEinqerr();
		}
		if (op != OK)
		{
			if (rval == OK)
				rval = op;
		}
		else
		{
			*count += FEinqrows();
		}
	}

	/*
	** if ii_entities table exists, we have to check some tables
	** against it as well as ii_objects.
	*/
	STcopy(ERx("ii_entities"),&table_buf[0]);
	IIUGdbo_dlmBEobject(&table_buf[0],FALSE);
	if (FErelexists(&table_buf[0],NULL) == OK)
		have_ent = TRUE;
	else
		have_ent = FALSE;

	/*
	** we scan through the table setting doit bits if we are going
	** to do the table.  The only real reason we scan twice is to have
	** a count of the total number of tables we are processing for
	** the "verbose" message, and we don't want to invoke relexists
	** twice.  Yeah, it's a little silly ....
	*/
	tn = Cats;
	tot = 0;
	for (i=0; i < sizeof(Cats)/sizeof(Cats[1]); ++i,++tn)
	{
		tn->doit = FALSE;
		if ((which & tn->mask) == 0)
			continue;
		STcopy(tn->table, &table_buf[0]);
		IIUGdbo_dlmBEobject(&table_buf[0],FALSE);
		if (FErelexists(&table_buf[0],NULL) != OK)
			continue;
		++tot;
		tn->doit = TRUE;
	}

	tn = Cats;
	for (i=0; i < sizeof(Cats)/sizeof(Cats[1]); ++i,++tn)
	{
		if (! tn->doit)
			continue;
		if (verbose)
			IIUGmsg(ERget(S_UI0022_Table),
					FALSE, 3, &i, &tot, tn->table);
		if (tn->chkent && have_ent)
			STprintf(tbuf,FstrWEnt,tn->table,tn->column,tn->column);
		else
			STprintf(tbuf,Fstr,tn->table,tn->column);
		EXEC SQL EXECUTE IMMEDIATE :tbuf;
		op = FEinqerr();
		if (op != OK)
		{
			if (rval == OK)
				rval = op;
		}
		else
		{
			*count += FEinqrows();
		}
	}

	iiuicw0_CatWriteOff();

	return rval;
}

/*{
** Name:	IIUInslNoSessionLocks
**
** Description:
**	Removes all records from ii_locks having a non-zero session id
**	This may be done at times when we know that no real user sessions
**	can exist, which is the reason it is not lumped together with the
**	cleanup routine, which is called from ABF.
**
** History:
**
**	2/90 (bobm)	Written
**	27-jul-1992 (rdrane)
**		Change parameterization of call to FErelexists() to include
**		owner (actually, we can get away with (char *)NULL -
**		users shouldn't have ii* tables).
**	22-jul-1993 (rdrane)
**		Effect case translation of catalog names prior to calling
**		FErelexists() for FIPS support.
*/

STATUS
IIUInslNoSessionLocks()
{
	STATUS rval;

	STcopy(ERx("ii_locks"),&table_buf[0]);
	IIUGdbo_dlmBEobject(&table_buf[0],FALSE);
	if  (FErelexists(&table_buf[0],NULL) != OK)
	{
		return OK;
	}

	iiuicw1_CatWriteOn();

	EXEC SQL DELETE FROM ii_locks WHERE session_id <> 0;

	rval = FEinqerr();

	iiuicw0_CatWriteOff();

	return rval;
}

/*{
** Name:	IIUIffuFidFixUp
**
** Description:
**	This takes care of old style fid names, so that we can easily
**	delete unused IL records.
**
** History:
**
**	3/90 (bobm)	Written
*/

STATUS
IIUIffuFidFixUp()
{
	STATUS rval;
	EXEC SQL BEGIN DECLARE SECTION;
	i4 cls;
	char nm[FE_MAXNAME+1];
	char *ptr;
	i4 done;
	i4 eno;
	EXEC SQL END DECLARE SECTION;

	cls = OC_ILCODE;
	ptr = nm+3;

	iiuicw1_CatWriteOn();

	EXEC SQL DECLARE cobj CURSOR FOR
		SELECT object_name
			FROM ii_objects
			WHERE object_name like 'fid%' and object_class = :cls
			FOR UPDATE OF object_name;

	EXEC SQL OPEN cobj;

	EXEC SQL INQUIRE_INGRES ( :eno = ERRORNO );

	if (eno != 0)
		goto errobj;

	for (;;)
	{
		EXEC SQL FETCH cobj INTO :nm;

		EXEC SQL INQUIRE_INGRES ( :done = ENDQUERY, :eno = ERRORNO );

		if (done)
			goto endobj;

		if (eno != 0)
			goto errobj;

		EXEC SQL UPDATE ii_objects SET object_name = :ptr
			WHERE CURRENT OF cobj;

		EXEC SQL INQUIRE_INGRES ( :eno = ERRORNO );

		if (eno != 0)
			goto errobj;
	}

errobj:
	rval = FEinqerr();
	EXEC SQL CLOSE cobj;
	goto errout;

endobj:
	EXEC SQL CLOSE cobj;

	EXEC SQL DECLARE cdep CURSOR FOR
		SELECT abfdef_name
			FROM ii_abfdependencies
			WHERE abfdef_name like 'fid%' and object_class = :cls
			FOR UPDATE OF abfdef_name;

	EXEC SQL OPEN cdep;

	EXEC SQL INQUIRE_INGRES ( :eno = ERRORNO );

	if (eno != 0)
		goto errdep;

	for (;;)
	{
		EXEC SQL FETCH cdep INTO :nm;

		EXEC SQL INQUIRE_INGRES ( :done = ENDQUERY, :eno = ERRORNO );

		if (done)
			goto enddep;

		if (eno != 0)
			goto errdep;

		EXEC SQL UPDATE ii_abfdependencies SET abfdef_name = :ptr
			WHERE CURRENT OF cdep;

		EXEC SQL INQUIRE_INGRES ( :eno = ERRORNO );

		if (eno != 0)
			goto errdep;
	}

enddep:

	EXEC SQL CLOSE cdep;
	iiuicw0_CatWriteOff();
	return OK;

errdep:
	rval = FEinqerr();
	EXEC SQL CLOSE cdep;

errout:
	iiuicw0_CatWriteOff();
	if (rval == OK)
		rval = FAIL;
	return rval;
}


/*{
** Name:	IIUIadaAddDepAppl
**
** Description:
**	If 6.3 or earlier ABF has been running against 6.4 or later catalogs, it
**	may have added dependencies with NULL abfdef_applid values.  We will
**	fix them up here by joining agains ii_abfobjects to get the application
**	id.  This isn't quick, but we don't expect to do it often (if at all).
**
** Inputs:
**	applid		i4		Application to do -- -1 for all
**	verbose		bool		Whether to yakety-yak 
**	catwrite	bool		Is CatWrite privilege enabled?
**
** Outputs:
**	rows		i4 *		How many rows we touched
**
**	Returns:
**		STATUS		Database status
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	8/90 (Mike S)	Initial version
*/
STATUS
IIUIadaAddDepAppl(applid, verbose, catwrite, count)
EXEC SQL BEGIN DECLARE SECTION;
i4	applid;
EXEC SQL END DECLARE SECTION;
bool	verbose;
bool	catwrite;
i4	*count;
{
EXEC SQL BEGIN DECLARE SECTION;
	i4	local_count = 0;
	i4	status;
	i4	done;
	i4	objid;
	i4	nullapplid;
	i4	rows;
	i2	ind;
EXEC SQL END DECLARE SECTION;
	bool	wasinxact;

	*count = 0;

	/* 
	** Look for NULL application ID's.  Usually, there won't be any.
	** Even if we're only concerned with one application, we'll do this
	** quick check.
	*/
	EXEC SQL SELECT count(*) 
		INTO :local_count
		FROM ii_abfdependencies
		WHERE abfdef_applid IS NULL;
	
	if ((status = FEinqerr()) != OK)
		return status;

	if (local_count == 0)
		return OK;
	
	/* 
	** There were some.  Let's fix them.  If we're fixing all of them, we
	** need a (gasp!) cursor.
	*/
	if (verbose)
	{
		if (applid > 0)
			IIUGerr(S_UI002E_DepFix, 0, 0);
		IIUGmsg(ERget(S_UI0021_FidFix), FALSE, 0);
	}

	if (!catwrite)
		iiuicw1_CatWriteOn();

	if (applid > 0)
	{
		/* One application only */
		EXEC SQL UPDATE ii_abfdependencies
			SET abfdef_applid = :applid
			WHERE object_id IN
				(SELECT d.object_id 
				 FROM ii_abfobjects o, ii_abfdependencies d
				 WHERE o.object_id = d.object_id 
				 AND o.applid = :applid
				 AND abfdef_applid IS NULL);
		status = FEinqerr();
		local_count = FEinqrows();
	}
	else
	{
		/* Do the whole catalog */
		wasinxact = IIUIinXaction();
		if (!wasinxact)
			IIUIbeginXaction();

		EXEC SQL DECLARE cabfdepupd CURSOR FOR
			SELECT object_id, abfdef_applid
				FROM ii_abfdependencies
				WHERE abfdef_applid IS NULL
			FOR UPDATE OF abfdef_applid;
		EXEC SQL OPEN cabfdepupd;
		EXEC SQL INQUIRE_INGRES ( :status = ERRORNO );
		if (status != OK)
			goto curerr;

		/* Update each record we find */
		for ( local_count = 0; ;)
		{
			EXEC SQL FETCH cabfdepupd 
				INTO :objid, :nullapplid:ind;
			EXEC SQL INQUIRE_INGRES 
				( :status = ERRORNO, :done = ENDQUERY);
			if (done || status != OK)
				break;

			EXEC SQL SELECT applid, object_id
				INTO :applid, :objid
				FROM ii_abfobjects
				WHERE ii_abfobjects.object_id = :objid;
			EXEC SQL INQUIRE_INGRES 
				( :status = ERRORNO, :rows = rowcount);
			if (status != OK)
				break;
			if (rows == 0)
			{
				/* we'll let cleanup delete it */
				continue;
			}

			EXEC SQL UPDATE ii_abfdependencies
				SET abfdef_applid = :applid
				WHERE CURRENT OF cabfdepupd;
			EXEC SQL INQUIRE_INGRES ( :status = ERRORNO);
			if (status != OK)
				break;
			local_count++;		/* We updated one */
		}

		EXEC SQL CLOSE cabfdepupd;
curerr:
		if (!wasinxact)
			if (status == OK)
				IIUIendXaction();
			else
				IIUIabortXaction();
	}

	if (!catwrite)
		iiuicw0_CatWriteOff();
	if (status == OK)
		*count = local_count;
	return status;
}
