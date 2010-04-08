/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include <compat.h>
# include <cv.h>		/* 6-x_PC_80x86 */
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <fe.h>
# include <er.h>
# include <cm.h>
# include <st.h>
# include <ug.h>
EXEC SQL INCLUDE <ooclass.sh>;
# include <oocat.h>
# include	<uigdata.h>
# include "eram.h"
# include <abfdbcat.h>

/**
** Name:	iamfutil.c -	Forms utilities
**
** Description:
**
**	Forms catalog utilities called from abf, mainly for VISION use.
**
**		IIAMgoiGetObjInfo
**		IIAMfoiFormInfo
**		IIAMforFormRead
**		IIAMufnUniqFormName
**		IIAMggdGetGenDate
**/

static char	*uniq_sub();

/*{
** Name - IIAMgoiGetObjInfo	- get information about an object
**
** Description:
**	Return the date and id associated with a object, and an empty string
**	and OC_UNDEFINED if the object doesn't exist.  
**	OK is returned  for non-existent object.
**	failure is returned only if DB access fails.
**
** Inputs:
**	name - object name
**	class - object class
**	dbuf - char buffer to hold date string
**	ooid_p - pointer to OOID.
**
** Outputs:
**	dbuf - filled in date, or empty string.
**	ooid_p - pointer to OOID, or OC_UNDEFINED.
**
** Returns:
**	STATUS
**
** History:
**	7/89 (bobm)	written
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

STATUS
IIAMgoiGetObjInfo(name, class, dbuf, ooid_p)
EXEC SQL BEGIN DECLARE SECTION;
char	*name;
OOID	class;
EXEC SQL END DECLARE SECTION;
char	*dbuf;
OOID	*ooid_p;
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	ad[OODATESIZE+1];
	char	cd[OODATESIZE+1];
	char	own[FE_MAXNAME+1];
	OOID	id;
	EXEC SQL END DECLARE SECTION;
	
	OOID	frid = OC_UNDEFINED;
	bool	found = FALSE;
	i4	seq = 0;

	*dbuf = EOS;
	*ooid_p = OC_UNDEFINED;

	EXEC SQL REPEATED SELECT 
			alter_date, create_date, object_owner, object_id
		INTO :ad, :cd, :own, :id 
		FROM ii_objects
		WHERE object_class = :class AND object_name = :name;
	EXEC SQL BEGIN;
	{
		if ( STtrmwhite(own) > 0)
		{
			++seq;
			/*
			** if we have an empty alter date, use the create date
			*/
			if (ad[0] == EOS)
				STcopy(cd, ad);

			if (STequal(own, IIUIdbdata()->user))
			{
				STcopy(ad, dbuf);
				frid = id;
				found = TRUE;
				EXEC SQL ENDSELECT;
			}
			else if (STequal(own, IIUIdbdata()->dba))
			{
				frid = id;
				found = TRUE;
				STcopy(ad, dbuf);
			}
			else if ( frid == OC_UNDEFINED )
			{
				frid = id;
			}
		}
	}
	EXEC SQL END;

	if ( FEinqerr() != OK )
		return FAIL;

	if ( !found && seq > 1 )
		frid = OC_UNDEFINED;
	*ooid_p = frid;

	return OK;
}

/*{
** Name:	IIAMfoiFormInfo - get information about a form.
**
** Description:
**	Instance of IIAMgoiGetObjInfo.  See description above.
**
*/
STATUS
IIAMfoiFormInfo(fname, dbuf, ooid_p)
char    *fname;
char    *dbuf;
OOID    *ooid_p;
{
	return IIAMgoiGetObjInfo(fname, OC_FORM, dbuf, ooid_p);
}

/*{
** Name - IIAMforFormRead	- Read forms fields.
**
** Description:
**	Reads the fields of a given form and calls a function for action.
**
** Inputs:
**	fname - the form name.
**	frid - form id.
**	p    - PTR, for use of client.  We just pass it along.
**	func - function to call on each field.
**
** Returns:
**	STATUS
**
** History:
**	1/90 (billc)	written
*/

STATUS
IIAMforFormRead (fname, frid, p, func)
char	*fname;
EXEC SQL BEGIN DECLARE SECTION;
OOID	frid;
EXEC SQL END DECLARE SECTION;
PTR	p;
STATUS	(*func)();
{
EXEC SQL BEGIN DECLARE SECTION;
	char	nbuf[FE_MAXNAME+1];
	i4	length;
	i2	type;
	i2	scale;
	i4	seq;
	i4	kind;
EXEC SQL END DECLARE SECTION;
	STATUS	stat = OK;

	if (frid == OC_UNDEFINED)
		return FAIL;

	/*
	** Step through the form retrieving all the fields
	*/
	EXEC SQL
		SELECT fldname, fldatatype, fllength, flprec,
			fltype, flsubseq
		  INTO :nbuf, :type, :length, :scale, :kind, :seq
		  FROM ii_fields
		WHERE object_id = :frid
		ORDER BY flsubseq;
	EXEC SQL BEGIN;
	{
		DB_DATA_VALUE   dbv;

		STtrmwhite(nbuf);
		dbv.db_datatype = type;
		dbv.db_length = length;
		dbv.db_data = NULL;
		dbv.db_prec = scale;

		if ((stat = (*func)(fname, nbuf, p, kind, &dbv)) != OK)
			EXEC SQL ENDSELECT;
	}
	EXEC SQL END;	/* end select loop */

	return stat != OK ? stat : FEinqerr();
}

/*{
** Name - IIAMufnUniqFormName
**
** Description:
**	Return a unique name for a form to be attached to a metaframe
**	type.  We make sure the name is unique with reference to all
**	OC_FORM's and OC_AFORMREF's in the database.  The latter is
**	important because VISION may not actually generate the form for
**	a long time, but abf will put an AFORMREF record in the DB
**	shortly after generating the name.
**
** Inputs:
**	name - frame name for a starting point
**	tag - storage tag for generated name
**
** Returns:
**	pointer to name, or NULL for failure.
**
** History:
**	7/89 (bobm)	written
*/

char *
IIAMufnUniqFormName(name,tag)
char *name;
TAGID tag;
{
	char *nm;

	nm = uniq_sub(name,tag,1);

	if (nm == NULL)
		IIUGerr(E_AM0027_Form_name);

	return nm;
}

/*
** The real work for name generation.  What is basically going on here:
**
**	If the name all by itself works, simply use it.  Otherwise
**	we make numeric suffixes of length ndig (numbers with fewer
**	than ndig digits may appear, zero padded to ndig).  If all the
**	purely numeric suffixes of this length are taken, we bump
**	ndig by one and recurse.  Yes, this is a tail recursion which
**	we could unwind, but this makes it easy to distinguish not
**	having an empty slot at a given number of digits from failure.
**	Also, since this involves DB access, the price for a recursive
**	call is pretty well swamped.  Will never go very many levels,
**	so I don't care about the stack buffers either.  We cut off
**	at 4 levels so that we'll be OK with 16 bit nat's.  This gives
**	us 11111 unique names for a given frame name, which ought to
**	be enough.
**
**	The scheme:
**
**		<name> <name>0 .... <name>9	ndig = 1
**		<name>00 .... <name>99		ndig = 2
**		<name>000 .... <name>999	ndig = 3
*/
static char *
uniq_sub(name,tag,ndig)
char *name;
TAGID tag;
i4  ndig;
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	nbuf[FE_MAXNAME+1];
	char	pat[FE_MAXNAME+2];
	EXEC SQL END DECLARE SECTION;

	char	ntrunc[FE_MAXNAME+1];
	i4	lastnum;
	i4	hinum;
	i4	num;
	char	*suffix;
	bool	name_ok;

	if (ndig > 4)
		return NULL;

	/*
	** this is for safety.  VISION actually generates short
	** frame names, so ntrunc should = name.
	*/
	STcopy(name,ntrunc);
	ntrunc[FE_MAXNAME-ndig] = EOS;

	/*
	** largest number that can fit in our suffix
	*/
	STcopy(ERx("9999"),nbuf);
	nbuf[ndig] = EOS;
	_VOID_ CVan(nbuf,&hinum);

	STprintf(pat,ERx("%s%%"),ntrunc);
	suffix = nbuf + STlength(ntrunc);
	name_ok = TRUE;
	lastnum = -1;

	/*
	** we use the "order by" to assure that the name by itself
	** comes first, and that the type of suffixes we generate
	** (ndig digit numbers with leading 0's) come out sorted in relation
	** to each other.  This allows us to quit as soon as we find
	** an empty gap.
	**
	** lastnum starts out -1, so that if no ndig-length pure numerics
	** exist, we use 0.  If we finish the select loop with
	** lastnum = hinum, all the possible suffixes are used, and we need
	** to go to more digits.
	*/
	EXEC SQL REPEATED SELECT object_name 
		INTO :nbuf 
		FROM ii_objects
		WHERE object_name LIKE :pat 
		    AND (object_class = :OC_FORM OR object_class = :OC_AFORMREF)
		ORDER BY object_name;
	EXEC SQL BEGIN;
	{
		if (*suffix == EOS)
			name_ok = FALSE;
		else
		{
			if (name_ok)
			{
				EXEC SQL ENDSELECT;
			}
			if (STlength(suffix) == ndig && CVan(suffix,&num) == OK)
			{
				/* quit if we found a gap */
				if ((num - lastnum) > 1)
				{
					EXEC SQL ENDSELECT;
				}
				lastnum = num;
			}
		}
	}
	EXEC SQL END;

	if ( FEinqerr() != OK )
		return NULL;

	if (name_ok)
		return STtalloc(tag,ntrunc);

	if (lastnum >= hinum)
		return uniq_sub(name,tag,ndig+1);

	++lastnum;
	STprintf(pat,ERx("%%s%%0%dd"),ndig);
	STprintf(nbuf,pat,ntrunc,lastnum);

	return STtalloc(tag,nbuf);
}

/*
** Name - IIAMggdGetGenDate
**
** Description:
**	Get the current database value of the generation date of an objects
**	source file from the ii_abfobjects system catalog. 
**
** Inputs:
**	aid - application id of the object.
**	oid - object id.
**	gdate - Pointer to location to hold gendate value.
**
** Returns:
**	STATUS
**
** History:
**	24-nov-93 (rudyw)
**		Written to fix bug 51091. Possibility existed for using routine
**		IIAMcgCatGet (iamcget.c) instead but it had memory implications.
**		Needed to add include of abfdbcat.h to get ABFARG_SIZE.
*/

STATUS
IIAMggdGetGenDate(aid,oid,gdate)
EXEC SQL BEGIN DECLARE SECTION;
i4	aid;
i4	oid;
EXEC SQL END DECLARE SECTION;
char    *gdate;
{
	EXEC SQL BEGIN DECLARE SECTION;
	char  buf[ABFARG_SIZE+1];
	EXEC SQL END DECLARE SECTION;

	STATUS rval;
	char *datepart;

	EXEC SQL SELECT abf_arg6
		INTO :buf
		FROM ii_abfobjects
		WHERE applid = :aid AND object_id = :oid;

	rval=FEinqerr();

	if ( rval != OK )
		return rval;

	/*
	** If a value has been returned and there is a date part
	**  Copy the datepart to input pointer location.
	*/
	datepart = STindex(buf,"@",ABFARG_SIZE);
	if ( datepart == NULL || *(datepart+1) == '\0' )
		return FAIL;
	datepart++;
	STcopy(datepart, gdate);

	return OK;
}
