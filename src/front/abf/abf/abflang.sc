/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<ui.h>
exec sql include	<abclass.sh>;
exec sql include	<oocat.sh>;
#include	<oocatlog.h>
#include	<oodefine.h>
#include	<abfcnsts.h>
exec sql include	'abclinfo.sh';
exec sql include	'abcatrow.sh';
#include	"erab.h"

/*
** Name:	abflang.sc -	Constant Languages Module.
**
** Description:
**	Provides routines needed by ABF to obtain information about languages
**	and language dependent constants, and to manipulate the latter.  This
**	was written with Application Constants in mind.  Defines:
**
**	iiabLanguageDefault()	get the II_LANGUAGE language string.
**	iiabCopyConstants()	copy language constants from one to another.
**
** History:
**	Revision 6.3/03/00  89/10  Jfried
**	Initial revision.
**
**	Revision 6.4/02     92/01  davel
**
**	20-jan-92 (davel) bug 41920
**		Part of fix for bug 41920 - change iiabCopyConstants() to 
**		search for any language except for '', which is used for
**		numeric constants.
**
**	Revision 6.5
**
**	21-jan-92 (davel)
**		Eliminated the obsolete iiabCkLanguage() and iiabQrLangCons().
**	16-oct-92 (davel)
**		Change iiabCopyConstants() to update the set of constant values
**		associated with the current language (based on II_LANGUAGE) to
**		have the value "default".  The "default" set of values is the
**		only set that is used by 6.5 (and beyond) ABF.
**	21-oct-92 (davel)
**		Use the new IAOM routine IIAMugcUpdateGlobCons() in 
**		iiabCopyConstants instead of issuing the query from here.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:	iiabLanguageDefault() -	Get the Default Language.
**
** Side Effects:
**	Returns a ptr to a static that must not be modified by the caller.
**
** History:
**	Written 9/4/89 (jfried)
*/

static char	env_language[ER_MAX_LANGSTR + 1] = {EOS};

char *
iiabLanguageDefault()
{
    i4	   lang_code ;

    return ( *env_language != EOS 
		|| ( ERlangcode( (char *) NULL, &lang_code ) == OK
    			&& ERlangstr( lang_code, env_language ) == OK ) )
	  ? env_language : (char *)NULL;
}

/*{
** Name:	iiabCopyConstants() -	Copy Language Constants
**						from One to Another.
** Description:
**	Copies the language dependent constant values of an application
**	for one language to those of another.  This constant values will
**	still be in the first language, but can then be modified without
**	changing the values for that language.
**
** Input:
**	applid    {OOID}  The application's identifier.
**
** Returns:
**	{nat}  >= 0  The number of rows copied.
**		< 0  On error.
**
** Side Effects:
**	Adds constant entries to the ABF application catalogs for the new
**	language.
**
** History:
**	Written 9/4/89 (jfried)
**	11/89 (jhw) -- Rewrote as single INSERT.
**	20-jan-92 (davel) bug 41920
**		In searching for a valid language contained in column abf_arg1,
**		do not accept the value '', as this the value stored for 
**		numeric constants.
**	22-jan-92 (davel)
**		Change to a UNIONed SELECT, so that the value of II_LANGUAGE
**		is used preferentially if constants exist for that language
**		string.
**	16-oct-92 (davel)
**		No longer insert a new set of string constants; instead, update
**		the current language's constants to the language "default", 
**		using the new IAOM routine IIAMugcUpdateGlobCons().
*/

i4
iiabCopyConstants ( applid, new_language )
EXEC SQL BEGIN DECLARE SECTION;
OOID	applid;
char	*new_language;
EXEC SQL END DECLARE SECTION;
{
    STATUS IIAMugcUpdateGlobCons();
    STATUS stat;
    i4	nrows;

    EXEC SQL BEGIN DECLARE SECTION ;
    char	language[ER_MAX_LANGSTR + 1] ;
    char	*_ii_language;
    EXEC SQL END DECLARE SECTION ;
	
    if ( (_ii_language = iiabLanguageDefault()) == NULL)
	_ii_language = language;

    /* get a language that has string constants defined for it - preferably
    ** use the current value of II_LANGAUGE if constants exist for that;
    ** otherwise just get *any* language, in this case the first one
    ** alphabetically...
    */
    language[0] = EOS;
    EXEC SQL REPEATED SELECT DISTINCT abf_arg1
    			INTO :language FROM ii_abfobjects a, ii_objects o
    		WHERE applid = :applid AND a.object_id = o.object_id
    			AND object_class = :OC_CONST 
    			AND a.abf_arg1 = :_ii_language
	UNION ALL
		SELECT DISTINCT abf_arg1
			FROM ii_abfobjects a, ii_objects o
		WHERE applid = :applid AND a.object_id = o.object_id
			AND object_class = :OC_CONST AND a.abf_arg1 <> '';
    EXEC SQL BEGIN;
    {
    	EXEC SQL ENDSELECT;
    }
    EXEC SQL END;

    if ( *language == EOS )
    	return 0;	/* no languages */

    /* Update the language associated with the default set of values to
    ** the specified (default) language.  Note this will always be "default".
    */

    iiuicw1_CatWriteOn();
    stat = IIAMugcUpdateGlobCons(applid, new_language, language);
    nrows = ( stat != OK ) ? -1 : FEinqrows();

    iiuicw0_CatWriteOff();
    return nrows;
}
