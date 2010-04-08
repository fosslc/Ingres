/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<lo.h>
#include	<er.h>
#include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<abclass.h>
#include	<abfcnsts.h>
#include	<abfglobs.h>
#include	"gncode.h"
#include	"erab.h"

/**
** Name:	abfnames.c -	ABF Names Generation Utilities.
**
** Description:
**	Contains some general names utilties used by ABF to interface to the
**	GN module.  Defines:
**
**	iiabOldFileName()	add old file name to object name pool.
**	iiabCkObjectName()	check file name for object conflict.
**	iiabCkFileName()	check file name for conflict.
**	IIABcpnCheckPathName()	check file name for pathname.
**	iiabNewFileName()	delete old/add new file name to name pool.
**	iiabFoRefNames()	generate form reference names.
**	iiabFrRemoveNames()	remove form reference names.
**	iiabGenName()		generate a unique source-code file name.
**
** History:
**	Revision 6.3  89/11  wong
**	Use PATH&FILENAME for locations to allow users to specify relative paths
**	for file names.
**
**	Revision 6.2  89/09  wong
**	Modified to add file names to a separate pool in addition to the object-
**	code name pool.  Note that the file name pool is not cleared when names
**	are deleted since identical source-code file names are allowed.  #8106.
**
**	Revision 6.2  88/02  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Aug-2009 (kschendel) 121804
**	    Need abfglobs.h to satisfy gcc 4.3.
**/

char	*iiabGenName();
FUNC_EXTERN bool IIGNisInSpace();

/*{
** Name:	iiabOldFileName() -	Add Old File Name to Name Pools.
**
** Description:
**	Adds a file name from the catalogs into both the file name and the
**	object-code file name pools.  The latter pool consist of all the
**	base names (sans extension) for all source-code file names for the
**	application.  The routine removes the extension before adding the name
**	to the pool.
**
** Inputs:
**	file	{char *}  The name of the source-code file including extension.
**
** History:
**	02/89 (jhw)  Written.
**	09/89 (jhw)  Modified to add file name to separate pool.
*/
VOID
iiabOldFileName ( file )
char	*file;
{
	LOCATION	tloc;
	char		junk[MAX_LOC+1];
	char		name[MAX_LOC+1];
	char		lbuf[MAX_LOC+1];

	STcopy(file, lbuf);
	_VOID_ LOfroms(PATH&FILENAME, lbuf, &tloc);
	_VOID_ LOdetail(&tloc, junk, junk, name, junk, junk);
	IIGNonOldName(AGN_SFILE, file);
	IIGNonOldName(AGN_OFILE, name);
}

/*{
** Name:	iiabCkObjectName() -	Check File Name for Object Conflict.
**
** Description:
**	Check that a modified file name does not conflict with any other file
**	name.  File names are checked using their base name without their
**	extensions.  The old file name, which may be empty, is used as check
**	for when only the extension changed, which means the name is not in
**	conflict (since the old file name was not in conflict.)
**
** Inputs:
**	oldfile	{char *}  The old file name, which may be empty, but otherwise
**				will include an extension.
**	newfile	{char *}  The new file name including extension.
**
** Returns:
**	{bool}  TRUE if it does NOT conflict.
**
** History:
**	02/89 (jhw)  Written.
**	2/90 (Mike S) Fix logic if old filename is bad
*/
bool
iiabCkObjectName ( char *oldfile, char *newfile )
{
	char            newname[MAX_LOC+1];
        LOCATION        tloc;
        char            locbuf[MAX_LOC+1];
        char            junk[MAX_LOC+1];
 
        /* Get new file name (minus extension) */
        STcopy(newfile, locbuf);
        if ((LOfroms(PATH&FILENAME, locbuf, &tloc) != OK)  ||
            (LOdetail(&tloc, junk, junk, newname, junk, junk) != OK))
        {
                /*
                ** We shouldn't have been called with a bad filename.
                ** We'll call it no conflict, since it presumably won't compile.                */
                return TRUE;
        }
 
        /* See if we can optimize the test */
        if ((oldfile != NULL) || (*oldfile != EOS))
        {
                char            oldname[MAX_LOC+1];
 
                STcopy(oldfile, locbuf);
                if ( (LOfroms(PATH&FILENAME, locbuf, &tloc) == OK)  &&
                     (LOdetail(&tloc, junk, junk, oldname, junk, junk) == OK) &&                     (STequal(oldname, newname)) )
                {
                        /*
                        ** The previous name was OK, and only the file
                        ** extension has changed. We're still OK.
                        */
                        return TRUE;
                }
        }
 
        /* We have to check the whole namespace for conflicts */
        if (IIGNisInSpace(AGN_OFILE, newname))
        {
                IIUGerr(E_AB026B_ObjConflict, UG_ERR_ERROR, 1, newfile);
                return FALSE;
        }
        return TRUE;
}

/*{
** Name:	iiabCkFileName() -	Check File Name for Conflict.
**
** Description:
**	Check that a modified file name is not the same as any other file
**	name.  File names include both the base name and the extension.
**
** Inputs:
**	file	{char *}  The file name including extension.
**
** Returns:
**	{bool}  TRUE if it is NOT the same.
**
** History:
**	09/89 (jhw)  Written.
*/
bool
iiabCkFileName ( char *file )
{
	LOCATION	tloc;
	char		lbuf[MAX_LOC+1];

	STcopy(file, lbuf);
	return (bool)( LOfroms(PATH&FILENAME, lbuf, &tloc) == OK  &&
			!IIGNisInSpace(AGN_SFILE, file) );
}

/*{
** Name:	IIABcpnCheckPathName() -	Check a pathname.
**
** Description:
**	Check that a modified file name is not the a pathname
**
** Inputs:
**	file	{char *}  The file name including extension.
**
** Returns:
**	{bool}  TRUE if it is a pathname.
**
** History:
**	25-feb-93 (connie)  Written.
**	27-apr-93 (connie) Bug #51520
**		Pass address of tloc to LOisfull (broken in VMS if not) 
*/
bool
IIABcpnCheckPathName ( char *src_path, char *file )
{
	LOCATION	tloc;
	char		lbuf[MAX_LOC+1];

	STcopy(file, lbuf);
	if ( LOfroms(PATH&FILENAME, lbuf, &tloc) == OK && LOisfull(&tloc) )
        {
                IIUGerr(E_AB0281_PathName, UG_ERR_ERROR, 1, src_path);
                return TRUE;
        }
	return FALSE;
}

/*{
** Name:	iiabNewFileName() -	Delete Old/Add a New File Name to the
**						Name Pools.
** Description:
**	Add a new file name into both the file name and the object-code name
**	pools after removing the old object-code name.  Either may be empty, so
**	this routine covers both deletion and addition.
**
** Inputs:
**	oldfile	{char *}  The old file name, which may be empty.
**	newfile	{char *}  The new file name, which may also be empty.
**
** History:
**	02/89 (jhw)  Written.
**	09/89 (jhw)  Modified to add file name to separate pool.
*/
VOID
iiabNewFileName ( oldfile, newfile )
char	*oldfile;
char	*newfile;
{
	LOCATION	tloc;
	char		junk[MAX_LOC+1];
	char		name[MAX_LOC+1];
	char		lbuf[MAX_LOC+1];

	if ( oldfile != NULL && *oldfile != EOS )
	{ /* Remove old */
		STcopy(oldfile, lbuf);
		_VOID_ LOfroms(PATH&FILENAME, lbuf, &tloc);
		_VOID_ LOdetail(&tloc, junk, junk, name, junk, junk);
		IIGNdnDeleteName(AGN_OFILE, name);
	}

	if ( newfile != NULL && *newfile != EOS )
	{ /* Add new */
		STcopy(newfile, lbuf);
		_VOID_ LOfroms(PATH&FILENAME, lbuf, &tloc);
		_VOID_ LOdetail(&tloc, junk, junk, name, junk, junk);
		IIGNonOldName(AGN_SFILE, newfile);
		IIGNonOldName(AGN_OFILE, name);
	}
}

/*{
** Name:	iiabFoRefNames() -	Generate Names for a Form Reference.
**
** Description:
**	Generate a symbol and source-code file name for a form reference.
**
** Inputs:
**	form	{FORM_REF *}  The form reference.
**
** Outputs:
**	form	{FORM_REF *}  The form reference.
**			.symbol	{char *}  The object-code symbol for the form.
**			.source	{char *}  The object-code file name.
** History:
**	02/89 (jhw)  Written.
**      09-Feb-90 (mrelac) (hp9_mpe)
**              MPE/XL cannot handle filenames that begin with numeric chars!
**              For our port, the form source filename must start with an
**              alpha character; thus, we'll set the first byte to 'f'.
**	9/90 (Mike S)
**		The previous fix risks non-unique file names, if the 
**		object_id's differ by 10000.  I've improved it.	
*/
VOID
iiabFoRefNames ( form )
register FORM_REF	*form;
{
	char	*np;
	char	bufr[16];

	IIGNgnGenName(AGN_FORM, form->name, &form->symbol);

	/* generate file name */

# ifdef IBM
	_VOID_ STprintf(bufr, ERx("f%ld"), form->ooid);
# else
#  ifdef hp9_mpe
	bufr[0] = 'f';
	CVla( form->ooid, bufr + 1);
#  else
	CVla( form->ooid, bufr );
#  endif
# endif

	form->source = iiabGenName(form->data.tag, bufr, (char *)NULL);

	form->data.dirty = TRUE;
}

/*{
** Name:	iiabFrRemoveNames() -	Remove Names for a Form Reference.
**
** Description:
**	Removes the names for the form symbol and source-code file if it is
**	the last reference.
**
** Inputs:
**	form	{FORM_REF *}  The form reference.
**			.symbol	{char *}  The object-code symbol for the form.
**			.source	{char *}  The object-code file name.
**
** History:
**	02/89 (jhw)  Written.
*/
VOID
iiabFrRemoveNames ( form )
register FORM_REF	*form;
{
	GLOBALREF char	_iiOOempty[];

	if ( form->refs == 1 )
	{ /* last reference */
		if ( *form->symbol != EOS )
		{
			IIGNdnDeleteName(AGN_FORM, form->symbol);
			form->symbol = _iiOOempty;

			if ( *form->source != EOS )
			{
				/* delete file name */
				iiabNewFileName(form->source, (char*)NULL);
				form->source = _iiOOempty;
			}

			form->data.dirty = TRUE;
		}
	}
}

/*{
** Name:	iiabGenFileName() - 	Generate Unique Source-Code File Name.
**
** Description:
**	Generates an object-code unique source-code file name from a name and
**	an extension.  The object-code name for this file will be unique for
**	the application.
**
** Inputs:
**	tag	{u_i4}  The memory tag into which to allocate the file name.
**	name	{char *}  The file base name.
**	extension {char *}  The file extension (usually language dependent.)
**
** Returns:
**	{char *}  An address of the generated name; NULL on error.
**
** History:
**	02/89 (jhw)  Written.
**	09/89 (jhw)  Modified to add file name to separate pool.
**	10/89 (jhw)  Don't add NULL or empty extension (with '.' too.)
*/
char *
iiabGenName ( tag, name, extension )
u_i4	tag;
char	*name;
char	*extension;
{
	char	*newp;
	char	buf[MAX_LOC+1];

	IIGNgnGenName(AGN_OFILE, name, &newp);
	newp = FEtsalloc( tag,
		( extension != NULL && *extension != EOS )
			? STprintf(buf, ERx("%s.%s"), newp, extension)
			: newp
	);
	IIGNonOldName(AGN_SFILE, newp);

	return newp;
}
