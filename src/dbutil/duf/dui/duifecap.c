/*
**Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include        <lo.h>
#include        <nm.h>
#include	<ci.h>
#include	<me.h>
#include	<ex.h>
#include	<er.h>
#include	<st.h>
#include	<si.h>
#include	<gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<duerr.h>

/**
**
**  Name: DUIFECAP.C -  routines that build a list of client names based on
**			the authorization string and the file "prods.lst".
**			Client names are used by CREATEDB & upgradedb
**			to call upgradefe & create front-end catalogs.
**
**  This file defines:
**	dui_bldDefClientList()	Build list of client names based on Auth strng
**
**  History:
**	17-may-1990	(pete) 	Initial Version.
**	8-aug-93 (ed)
**	    unnest dbms.h
**	31-aug-98 (mcgem01)
**	    Don't do the front-end utility auth string checking during
**	    upgradefe in Ingres II.  Now that license checking is 
**	    externalized its become annoying.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
*/

static STATUS bldLoc();
static VOID   bldCString();
static i4     updClientArray();

/* 4 client names as of 7/90: INGRES, INGRES/DBD, WINDOWS_4GL, VISIBUILD */
#define	MAXCLIENTS 12		/* max # clients can pass to upgradefe */
#define RECSIZE    256
#define NUM_WORDS  2		/* number of words per line in prods.lst */

typedef struct {
	char	name[DB_MAXNAME+1];
	bool	needed;		/* set but not currently used */
} CLIENT;


/*{
** Name:	dui_bldDefClientList() - Build default list of clients.
**
** Description:
**	build default client list, based on authorization string & contents
**	of file "/ingres/files/dictfiles/prods.lst" or
**	[ingres.files.dictfiles]prods.lst
**
**	The file "prods.lst" is in the form:
**		-- comment line
**		<whitespace> c_nat <whitespace> client_name <other>
**		<whitespace> c_nat <whitespace> client_name <other>
**
**	for example:
**		--	used by CREATEDB
**		3	ingres		<remark>
**		5	core
**		39	visibuild	<remark>
**		32	INGRES/DBD
**
**	The file "prods.lst" should only have rows that map front-end
**	authorization string bits to front-end client names. Also, entries 
**	in the file can be in any order, and entries are not case sensitive.
**
** Inputs:
**	char	*clist		Buffer to write client list into.
**	DU_ERROR *duc_errcb	Error control block.
**
** Outputs:
**	char	*clist  Writes list of client names here.
** Returns:
**	{STATUS}  OK	if everthing fine.
**		  FAIL	if anything wrong.
**
** History:
**	may-17-1990	(pete)	Initial Version.
**	28-sep-1990	(pete)	Moved from duicrdb.qsc to separate file.
*/
STATUS
dui_bldDefClientList(clist, duc_errcb)
char		*clist;
DU_ERROR	*duc_errcb;
{
	LOCATION dictloc;
	char	 dictpath[MAX_LOC];
	FILE	 *dictfile;
	char     buf[RECSIZE+1];
	i4	 cnt;
	char	 *(wordarray[NUM_WORDS]);

	CLIENT   clients[MAXCLIENTS];
	i4	 num_clients = 0;

	*clist = EOS;

	/* Build location for file: "prods.lst" in /ingres/files/dictfiles */
	if (bldLoc (&dictloc, dictpath, ERx("prods.lst"), duc_errcb) == OK)
	{
	    if (SIfopen (&dictloc, ERx("r"), SI_TXT, RECSIZE, &dictfile) ==FAIL)
		return FAIL;

	    /* read file & build client list. */
	    while (SIgetrec( buf, RECSIZE, dictfile) != ENDFILE)
	    {
		if (buf[0] == '-' && buf[1] == '-')   /* comment line. skip */
		    continue;

		cnt = NUM_WORDS;
		STgetwords (buf, &cnt, wordarray);

		/* skip over lines with less than 2 words */
		if (cnt < 2)
		    continue;

		/* update list of known names */
		num_clients = updClientArray (num_clients,
		    clients, wordarray[1]);
	    }

	    /* loop thru array of CLIENTs and build up a string */
	    bldCString (num_clients, clients, clist);
	}
	else
	    return FAIL;

	return OK;
}

/*{
** Name:	bldCString() - build string from Client array entries.
**
** Description:
**	Build up a string of client names from the entries in the
**	Client array.
**
** Inputs:
**	i4	num_clients;	number of items in *p_client
**	CLIENT	*p_client;	pointer to first item in array of CLIENTs.
**	char	*clist;		write string to here.
**
** Outputs:
**		Writes string to "clist". If array is empty, then
**		clist will be an empty string.
** Returns:
**		VOID
**
** History:
**	may-17-1990	(pete)	Initial Version.
*/
static VOID
bldCString(num_clients, p_client, clist)
i4	num_clients;
CLIENT	*p_client;
char	*clist;
{
	i4 i;

	*clist = EOS;
	for (i=0; i < num_clients; i++, p_client++)
	{
	    if (i != 0)
	    	STcat (clist, ERx(" "));
	    STcat (clist, p_client->name);
	}
}

/*{
** Name:	updClientArray() - update Client Array with new name.
**
** Description:
**	Search the client array for matching entry. If no match, then
**	insert new row in client array.
**
** Inputs:
**	i4	num_clients;	number of items in *p_client
**	CLIENT	*p_client;	pointer to first item in array of CLIENTs.
**	char	*name;		search for this name in array.
**
** Outputs:
** Returns:
**	i4	new count of number items in array of CLIENTs.
**
** History:
**	may-17-1990	(pete)	Initial Version.
*/
static i4
updClientArray (num_clients, p_client, name)
i4	num_clients;
CLIENT	*p_client;
char	*name;
{
	i4 i;
	bool hit = FALSE;

	CVlower (name);

	for (i=0; i < num_clients; i++, p_client++)
	{
	    if STequal (p_client->name, name)
	    {
		hit = TRUE;
		break;
	    }
	}

	if (!hit)
	{
	    /* not found. add to end of list, unless too long. */

	    if ( (STlength(name) <= DB_MAXNAME)
		&& (num_clients < MAXCLIENTS) )
	    {
		/* ok to add this one to list */

	        num_clients++;
	        STcopy (name, p_client->name);
	        p_client->needed = TRUE;
	    }
	}

	return num_clients;
}

/*{
** Name:	bldLoc() - Build location structure for a file.
**
** Description:
**	Build a location structure for the file whose name is the
**	third argument; the file must exist in the FILES/dictfiles directory.
**
** Inputs:
**	LOCATION *dictloc;	Build this location structure.
**	char	 *dictpath;	Character buffer for above location.
**	char	 *fname;	Filename.
**	DU_ERROR *duc_errcb;	Error control block for duerror.
**
** Outputs:
** Returns:
**	{STATUS}  OK	if everthing fine & file exists.
**		  FAIL	if anything wrong, or if file doesn't exist.
**
** History:
**	may-17-1990	(pete)	Initial Version.
**	jun-14-1990	(pete)	Pick up prods.lst from [ingres.files.dictfiles]
*				rather than from [ingres.files]
*/
static STATUS
bldLoc (dictloc, dictpath, fname, duc_errcb)
LOCATION *dictloc;
char	 *dictpath;
char	 *fname;
DU_ERROR *duc_errcb;
{
	STATUS   clerror = OK;
	LOCATION tmploc;

	LOINFORMATION loinf;
	i4 flagword;

	/* Build FILES/dictfiles path */

	clerror = NMloc(FILES, PATH, NULL, &tmploc);
	if (clerror == OK)
	{
	        LOcopy (&tmploc, dictpath, dictloc);	/* make safe copy */
		/* add "/dictfiles" subdirectory */
		LOfaddpath(dictloc, ERx("dictfiles"), dictloc);
		/* add filename (e.g. prods.lst) */
		LOfstfile (fname, dictloc);
	}
	else
		goto err;

	/* full path all set; see if location (i.e. file) exists. */
	flagword = LO_I_TYPE;
	if (LOinfo(dictloc, &flagword, &loinf) != OK)
	    goto err;		/* location doesn't exist or other error */

	/* Assertion: location exists */

	return OK;

err:

	return FAIL;
}
