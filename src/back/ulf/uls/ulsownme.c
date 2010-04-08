/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <psfparse.h>
#include    <uls.h>

/**
**
**  Name: ULSOWNME.C
**
**  Description:
**
**  This file contains a function used to determine if the table name may be
**  prefixed with an owner name, and if so, whether or not it may be delimited,
**  and if so, the type of delimiter.  Determination will be made based on
**  type of statement (query mode), language, and server version.
**
**	uls_ownname() -	determine if the table name may be prefixed with an
**			owner name, and if so, whether or not it may be
**			delimited, and if so, the type of delimiter.
**
**  History:
**	13-Feb-89 (andre)
**	    Written.
**	01-Sep-1992 (rog)
**	    Prototype ULF.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**      15-nov-94 (newca01)
**          Added PSQ_DEFCURS and PSQ_REPCURS to datatypes which have 
**           owner name appended to table name in SQL statements.
**            6.4 bug 57691 (and 42237?)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:uls_ownname	- determine when/how to use ownername for tables
**			    in distributed Ingres.
**
** Description:
**      Determine when/how to use the 'owner.tablename' construct in
**	distributed Ingres.  
**
**	This routine looks at the DD_CAPS structure provided (i.e. the
**	ldb capabilities) to determine whether table names should be
**	prefaced with ownername., and whether the owner name may be surrounded
**	with single quotes, double quotes, or neither.
**
**	    The following info in DD_CAPS structure is being used:
**
**	.dd_c8_owner_name:
**
**		DD_0OWN_NO	--> return(FALSE)
**		DD_1OWN_YES	--> return(TRUE) for certain query types;
**				    indicate to the caller that the quotes may
**				    not be used
**		DD_2OWN_QUOTED	--> return(TRUE) for certain query types;
**				    use other info to determine type of quotes
**				    to use (if any)
**
**	.dd_c2_ldb_caps:
**
**		DD_0CAP_PRE_602_ING_SQL (what an ugly name)
**				--> can quote owner name in SQL only in some of
**				    the cases, also use single, rather than
**				    double quotes 
**	
**
**	DD_CAPS reflects the following values in iidbcapabilities:
**
**	"OWNER_NAME"	May be set to "NO", "YES", or "QUOTED".  The default is
**			QUOTED if the row is not present.
**		o "NO" means that the table names may not be qualified by
**		   owner name (see below).
**		o "YES" means that the table names may be qualified by
**		   owner name (see below), but the owner name may
**		   NOT be quoted.
**		o "QUOTED" means that the table names may be qualified by
**		   owner name (see below), and the owner name MAY
**		   be quoted.  QUOTED is needed to allow STAR to use the
**		   "$ingres" username or other usernames that do not
**		   follow the SQL/QUEL naming conventions.
**
**  Note that if SQL_LEVEL has a value before 00602 (i.e., 00600 or 00601) then 
**  STAR (PSF, RDF) must assume that the LDB supports the "OWNER_NAME" construct
**  as described above EXCEPT that owner names are not allowed in cases (1) and
**  (6-8) AND single quotes are used instead of double quotes for
**  cases (2-5).  
**
**    In SQL:
**
**	(1) COPY [["]owner["].]table ...
**	(2) DELETE FROM [["]owner["].]table ...
**	(3) INSERT INTO [["]owner["].]table ...
**	(4) SELECT ... FROM [["]owner["]].table ...
**	(5) UPDATE [["]owner["].]table ...
**
**
**    In QUEL:
**
**	(6) APPEND [TO] [[']owner['].]table ...
**	(7) COPY [[']owner['].]table ...
**	(8) RANGE OF range_var IS [[']owner['].]table ...
**
**
** Inputs:
**      cap_ptr			Ptr to a structure describing db capabilities.
**	qmode			The query mode (as in pst_qmode)
**	qlang			The query language
**	
**
** Outputs:
**      *quotechar              NULL if owner name may not be quoted or if
**				owner.table is not allowed; otherwise will
**				point to the character to be used as a
**				delimiter around the owner name.
** Returns:
**	    TRUE of the table may be qualified by the owner name;
**	    FALSE otherwise
** Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-jan-89 (robin)
**          Created
**	13-Feb-89 (andre)
**	    written.
**	05-may-89 (andre)
**	    SET LOCKMODE ON <table> WHERE ... have been added to the list
**	    of statements for which owner.table is acceptable.
**	08-may-89 (andre)
**	    For the time being, we will not try to use reverse quoting.
**	20-oct-92 (barbara)
**	    Removed comments from PSQ_SETLOCKMODE so that owner name may
**	    now be generated.
*/
bool	
uls_ownname( DD_CAPS *cap_ptr, i4  qmode, DB_LANG qlang, char **quotechar )
{
    if (cap_ptr->dd_c8_owner_name == DD_0OWN_NO)
    {
	*quotechar = (char *) NULL;
	return(FALSE);
    }
    else if (qlang == DB_QUEL)
    {
	if (~cap_ptr->dd_c2_ldb_caps & DD_0CAP_PRE_602_ING_SQL &&
	    (qmode == PSQ_APPEND || qmode == PSQ_COPY || qmode == PSQ_RANGE))
	/*
	** 6.2 or later -- single-quoted owner name may be used only with some
	** query types.
	** For the time being, reverse delimiters will not be supported.
	*/
	{
	    *quotechar = (cap_ptr->dd_c8_owner_name == DD_2OWN_QUOTED)
								? "\""
								: (char *) NULL;
	    return(TRUE);
	}
	else
	{
	    *quotechar = (char *) NULL;
	    return(FALSE);
	}
    }
    else
    {
	switch (qmode)
	{
	    /* PSQ_DEFCURS and PSQ_REPCURS added 111594 newca01  */
	    case PSQ_RETRIEVE:
	    case PSQ_APPEND:
	    case PSQ_DELETE:
	    case PSQ_REPLACE:
	    case PSQ_DEFCURS:
	    case PSQ_REPCURS:
	    {
		*quotechar = (cap_ptr->dd_c8_owner_name == DD_2OWN_QUOTED)
								? "'"
								: (char *) NULL;

		return(TRUE);
	    }
	    case PSQ_COPY:

	    case PSQ_SLOCKMODE:
  
	    {
		if (~cap_ptr->dd_c2_ldb_caps & DD_0CAP_PRE_602_ING_SQL)
		{
		    *quotechar = (cap_ptr->dd_c8_owner_name == DD_2OWN_QUOTED)
								? "'"
								: (char *) NULL;
		    return(TRUE);
		}
		/*
		** In pre 6.2, owner.table was not allowed with COPY or
		** SET LOCKMODE.
		*/
	    }
	    default:
	    {
		*quotechar = (char *) NULL;
		return(FALSE);
	    }
	}
    }
}
