/*
**	Copyright (c) 1987, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ui.h>
exec sql include <ooclass.sh>;
exec sql include <acatrec.sh>;
exec sql include <abfdbcat.sh>;
#include	<ilerror.h>

/**
** Name:	iamcupd.sc -	ABF Catalog Record Update Module.
**
** Description:
**	Contains routines that update the ABF catalogs for an application
**	component object.  Defines:
**
**	iiamUpdateCat()		update abf catalog record.
**	iiamUpdateConst()	update abf catalog record for constant only.
**	iiamUOFupdateOldOsl	change an OLDOSL frame to an OSL frame
**	IIAMugcUpdateGlobCons	Update global constants due to language changes
**
** History:
**	Revision 6.0  87/05  bobm
**	5/87 (bobm)	written
**
**	Revision 6.1  88/08/18  kenl
**	Changed QUEL to SQL.
**	12-sep-88 (kenl)
**		Changed EXEC SQL COMMITs to appropriate calls to the
**		IIUI...Xaction routnies.
**	09-nov-88 (marian)
**		Modified column names for extended catalogs to allow them
**		to work with gateways.
**
**	Revision 6.2  89/01  wong
**	Rewrote as 'iiamUpdateCat()' to only update the ABF catalogs.
**
**	Revision 6.5
**	21-oct-92 (davel)
**		Added IIAMugcUpdateGlobCons().
*/

/*{
** Name:	iiamUpdateCat() -	Update ABF Catalog Record.
**
** Description:
**	Updates a record in the ABF application catalogs.  This should
**	be used in conjunction with the OO module, which will update the
**	corresponding object record.
**
** Input:
**	arec	{ABF_DBCAT *}  The catalog record to be updated:
**			applid	{OOID}  The application ID.
**			ooid	{OOID}  The object ID.
**			source	{char *}  The source file/directory.
**			symbol	{char *}  The object-code symbol.
**			retadf_type  {DB_DT_ID}  The return type.
**			retlength  {longnat}  The return type length.
**			retscale  {i2}  The return type scale (DB_DEC_TYPE.)
**			rettype {char *}  The return type description.
**			arg0-5	{char *}  Object specific values.
**
** Returns:
**	{STATUS}  DBMS query status for UPDATE.
**
** History:
**	5/87 (bobm)	written (as 'IIAMcuCatUpd()'.)
**	18-aug-88 (kenl)  Changed QUEL to SQL.
**	09-nov-88 (marian)  Modified column names for extended catalogs to
**		allow them to work with gateways.
**	01/89 (jhw)  Modified to use ABF_DBCAT, and to only update the ABF
**		catalog, itself.
*/
STATUS
iiamUpdateCat ( arec )
EXEC SQL BEGIN DECLARE SECTION;
register ABF_DBCAT	*arec;
EXEC SQL END DECLARE SECTION;
{
	EXEC SQL REPEATED UPDATE ii_abfobjects
		SET	abf_version = :ACAT_VERSION,
			abf_source = :arec->source,
			abf_symbol = :arec->symbol,
			retadf_type = :arec->retadf_type,
			retlength = :arec->retlength,
			retscale = :arec->retscale,
			rettype = :arec->rettype,
			abf_arg1 = :arec->arg0,
			abf_arg2 = :arec->arg1,
			abf_arg3 = :arec->arg2,
			abf_arg4 = :arec->arg3,
			abf_arg5 = :arec->arg4,
			abf_arg6 = :arec->arg5,
			abf_flags = :arec->flags
		WHERE object_id = :arec->ooid AND applid = :arec->applid;

	return FEinqerr();
}

/* iiamucUpdateConst is for the most part a clone of iiamUpdateCat.  It differs
 * only in that an additional restriction has been added to the predicate so
 * that the update occurs for the correct language version of a constant.
 *
 * Cloned by JCF.
 */

STATUS
iiamucUpdateConst ( arec )
EXEC SQL BEGIN DECLARE SECTION;
register ABF_DBCAT	*arec;
EXEC SQL END DECLARE SECTION;
{
	EXEC SQL REPEATED UPDATE ii_abfobjects
		SET	abf_version = :ACAT_VERSION,
			abf_source = :arec->source,
			abf_symbol = :arec->symbol,
			retadf_type = :arec->retadf_type,
			retlength = :arec->retlength,
			retscale = :arec->retscale,
			rettype = :arec->rettype,
			abf_arg1 = :arec->arg0,
			abf_arg2 = :arec->arg1,
			abf_arg3 = :arec->arg2,
			abf_arg4 = :arec->arg3,
			abf_arg5 = :arec->arg4,
			abf_arg6 = :arec->arg5,
			abf_flags = :arec->flags
		WHERE object_id = :arec->ooid AND applid = :arec->applid AND
                      abf_arg1 = :arec->arg0 ;

	return FEinqerr();
}


/*{
** Name:	iiamUOFupdateOldOsl     change an OLDOSL frame to an OSL frame
**
** Description:
**	Change the objectclass of an OLDOSL Frame from OC_OLDOSL to OC_OSLFRAME
**
** Inputs:
**	ooid		Object id
**
** Outputs:
**
**	Returns:
**		Ddtabase status
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	11/90 (Mike S) Initial version
*/
STATUS
iiamUOFupdateOldOsl(ooid)
EXEC SQL BEGIN DECLARE SECTION;
OOID ooid;
EXEC SQL END DECLARE SECTION;
{
	EXEC SQL UPDATE ii_objects
		SET object_class = :OC_OSLFRAME
		WHERE object_id = :ooid;

	return FEinqerr();
}

/*{
** Name:	IIAMugcUpdateGlobCons - Update Global Constant components.
**
** Description:
**	Change the language of a set of global constant values.  This serves to
**	upgrade the global constant components in the catalogs from the pre-6.5
**	natural langauge scheme to the current scheme.
**
** Inputs:
**	applid		Application OOID.
**	new_lang	The new language (always " default")
**	old_lang	The langauge corresponding to the set of constants
**			that will be changed to the new language.
**
** Outputs:
**
** Returns:
**	Database status.
**
** Exceptions:
**
** Side Effects:
**
** History:
**	20-oct-92 (davel)
**		Initial version.
*/
STATUS
IIAMugcUpdateGlobCons(applid, new_lang, old_lang)
EXEC SQL BEGIN DECLARE SECTION;
OOID applid;
char *new_lang;
char *old_lang;
EXEC SQL END DECLARE SECTION;
{
	EXEC SQL REPEATED UPDATE ii_abfobjects a FROM ii_objects o
		SET abf_arg1 = :new_lang
		WHERE a.applid = :applid AND a.object_id = o.object_id
		AND o.object_class = :OC_CONST
		AND a.abf_arg1 = :old_lang;

	return FEinqerr();
}

