/*
**    Copyright (c) 1983, 2000 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<rms.h>
# include	<er.h>
# include	<lo.h>
# include	<me.h>
# include	<st.h>
# include	<starlet.h>
# include	"lolocal.h"

/*
**
** LOfroms.c
**
**	Convert a string to a LOCATION.
**	It accepts a ptr to an already allocated location and
**	and allocated buffer where location string is stored.
**	This buffer will always be used by the location to store string.
**	The Null string gives an empty LOCATION.
**	A flag must be sent telling LOfroms whether the string is a:
**
**
**	Parameters:
**		LOCTYPE		flag;   what type of location this is
**		char		string; buffer allocated by user
**		LOCATION	*loc;   data structure for locations
**
**
**	Returns:
**		OK -  if location has legal
**		LO_FR_BAD_SYN - if bad syntax.
**
**	Assumptions:
**		Upon entering function string points to a buffer allocated
**		by the user of the function and the user has copied the
**		vms string representing the location into the buffer.  The
** 		buffer is assumed to big enough.  Logical name translation is
**		done, unless a serach path is present.  Concealed devices are
**		left concealed.
**
**	Side Effects:
**		none
**
**	History:
**		8/20/83 -- (mmm) written
**		9/9/83  -- (dd)  VMS CL
**		4/15/89 -- (Mike S) Do a syntax-only parse
**		10/2/89 -- (Mike S) Clean up.
**		6/90    -- (Mike S) Stay compatible with old frontmain's
**		06/14/93 (dkh) - Updated to check that the original and
**				  constructed location names differ only
**				  in case.  If so, use the original name.
**				  This brings back part of the 6.4 semantics
**				  and corrects bugs 50051, 49363 and 50154.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	11/22/93 (dkh) - Fixed the declaration of check_strings() to be
**			 a static routine since it is local to this file.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/
typedef	struct FAB	_FAB;
typedef struct NAM	_NAM;

typedef struct 
{
	char *string;
	i4 fp;
	char desc;
} oldLocation;

# define DIREXT ERx(".DIR")

static VOID rtnold();
static void check_strings();

STATUS
LOfroms(
LOCTYPE			flag,
register char		*string,
register LOCATION	*loc)

{
	_FAB		fab;
	_NAM		nam;
	i4		fnb;	/* File Name status Bits (from NAM block ) */
	char		esa[MAX_LOC+1];

	bool		oldcall;
	LOCATION	newloc;
	oldLocation	*oldloc;
	char 		*cloc;
	char		orig_string[MAX_LOC + 1];
	i4		orig_length;
	i4		new_length;

	/* check to see if passed arguments are non null */
	if (loc == NULL)
	{
		return(LO_NULL_ARG);

	}

	/*
	** If we were called from an old frontmain.mar, the first argument
	** is actually an oldLocation.  If the LOfroms succeeds, we'll return
	** an oldLocation structure.  All frontmain.mar will do is pass the
	** structure to SIfopen, which will call LOtos to get the path name.
	** This will work because the "string" element hasn't moved.
	**
	** We detect this situation by noticing that the "string" argument
	** appears to overlap the "loc" argument, because LOCATIONs are
	** bigger than oldLocation's were.
	*/
	cloc = (char *)loc;
	if ( cloc < string && string < (cloc + sizeof(LOCATION)) )
	{
		oldcall = TRUE;
		oldloc = (oldLocation *)loc;
		loc = &newloc;
	}
	else
	{
		oldcall = FALSE;
	}

	/* INIT */
	loc->wc = NULL;
	loc->string = string;
	loc->desc = flag;
	nam.nam$l_fnb = 0;

	if (string == NULL)
	{
		/* a null string returns an empty location */
		loc->desc = NULL;
		if (oldcall)
			rtnold(loc, oldloc);
		return(OK);
	}

	if (!oldcall)
	{
		orig_length = STlength(string);
		if (orig_length > MAX_LOC)
		{
			orig_string[0] = EOS;
		}
		else
		{
			STcopy(string, orig_string);
		}
	}

	/* clear storage space */
	MEfill(sizeof(fab), NULL, (PTR)&fab);
	MEfill(sizeof(nam), NULL, (PTR)&nam);

	nam.nam$b_bid = NAM$C_BID;
	nam.nam$b_bln = NAM$C_BLN;
	fab.fab$b_bid = FAB$C_BID;
	fab.fab$b_bln = FAB$C_BLN;
	fab.fab$b_fns = STlength(fab.fab$l_fna = string);
	fab.fab$l_nam = &nam;
	nam.nam$l_esa = esa;
	nam.nam$b_ess = MAX_LOC;
	nam.nam$b_nop = NAM$M_SYNCHK | NAM$M_PWD;
	if ((sys$parse(&fab) &1)!= 1)
	/* syntax error */
	{
		loc->desc = NULL;
		return(LO_FR_BAD_SYN);
	}

	/* If there are any wildcards, reject it */
	if ((nam.nam$l_fnb & NAM$M_WILDCARD) != 0)
	/* syntax error */
	{
		loc->desc = NULL;
		return(LO_FR_BAD_SYN);
	}

	/* Set up the fields in the location */
	fnb = nam.nam$l_fnb;
	if ((fnb & NAM$M_SEARCH_LIST) == 0)
	{
		char *p;
		char *stbrk;		/* starting bracket of relative path */
		char *endbrk;         	/* ending bracket of relative path */
		char relpath[MAX_LOC+1];	/* relative path */
		i4 rellen;      	/* length of relative path */

		/*
		** Without a search path, we expand logical names (except for
		** concealed devices), and use the results of the parse.  We
		** copy the parts of the filename which were found into the
		** location's string.
		** If a relative pathname was specified, we use that instead
		** of the resultant pathname from the parse.  I.e. if our
		** default is [DIR.SUB], and the string includes [--.SUB2],
		** we don't use the directory the parse gave us, which is
		** [SUB2].  This implies some restrictions:
		**	1.	Logical names which translate to relative paths
		**		won't work.
		**	2.	Relative paths have to make sense when combined
		**		with the default.  I.e. [--.HDR] will cause
		**		an error if the default is DEV:[MAIN],
		**		even if our intent is to combine it with
		**		DEV2:[FACIL.RPLUS.SRC.DIR] .  Sorry about this.
		**
		** Relative paths are recognized as:
		**	[.SUB]		Start with "."
		**	[-.SUB]		Start with "-"
		**	[]		Start (and end) with "]" or ">"
		*/

		/* Check for explicit relative path */
		stbrk = STindex(loc->string, OBRK, 0);
		if (stbrk == NULL) stbrk = STindex(loc->string, OABRK, 0);
		if (stbrk != NULL)
		{
			if (stbrk[1] == *DASH || stbrk[1] == *DOT ||
			    stbrk[1] == *CBRK || stbrk[1] == *CABRK)
			{
				if (*stbrk == *OBRK)
					endbrk = STindex(loc->string, CBRK, 0);
				else
					endbrk = STindex(loc->string, CABRK, 0);
				rellen = endbrk - stbrk + 1;
				STlcopy(stbrk, relpath, rellen);
			}
			else
			{
				stbrk = NULL;
			}
		}

		/* Create location string */
		loc->nodeptr = p = loc->string;
		loc->nodelen = 0;
		if ((fnb & NAM$M_NODE) != 0)
		{
			loc->nodelen = nam.nam$b_node;
        		STlcopy(nam.nam$l_node, p, loc->nodelen);
			p += loc->nodelen;
		}
		loc->devptr = p;
		loc->devlen = 0;
		if ((fnb & NAM$M_EXP_DEV) != 0)
		{
			loc->devlen = nam.nam$b_dev;
        		STlcopy(nam.nam$l_dev, p, loc->devlen);
			p += loc->devlen;
		}
		loc->dirptr = p;
		loc->dirlen = 0;
		if ((fnb & NAM$M_EXP_DIR) != 0)
		{
			if (stbrk != NULL)
			{
				loc->dirlen = rellen;
				STcopy(relpath, p);
			}
			else
			{
				loc->dirlen = nam.nam$b_dir;
        			STlcopy(nam.nam$l_dir, p, loc->dirlen);
			}
			p += loc->dirlen;
		}
		loc->nameptr = p;
		loc->namelen = 0;
		if ((fnb & NAM$M_EXP_NAME) != 0)
		{
			loc->namelen = nam.nam$b_name;
        		STlcopy(nam.nam$l_name, p, loc->namelen);
			p += loc->namelen;
		}
		loc->typeptr = p;
		loc->typelen = 0;
		if ((fnb & NAM$M_EXP_TYPE) != 0)
		{
			loc->typelen = nam.nam$b_type;
        		STlcopy(nam.nam$l_type, p, loc->typelen);
			p += loc->typelen;
		}
		loc->verptr = p;
		loc->verlen = 0;
		if ((fnb & NAM$M_EXP_VER) != 0)
		{
			loc->verlen = nam.nam$b_ver;
        		STlcopy(nam.nam$l_ver, p, loc->verlen);
			p += loc->verlen;
		}
		*p = EOS;
	}
	else
	{
		/*
		** If we blindly expanded logical names, we'd lose the search
		** path, so we work a little harder.
		*/
		LOsearchpath(fnb, loc);
	}

	/* is what they said the location is correct */
	if ((flag | NODE) == NODE)
	{
		if ((fnb & NAM$M_NODE) == 0)
			return (LO_FR_BAD_SYN);	/* No node */
	}
	else
	{
		if ((fnb & NAM$M_NODE) != 0)
			return (LO_FR_BAD_SYN);	/* A node */
	}

	/* We switch on what flag contains other then NODE */
	switch (flag | (PATH & FILENAME))
	{
	    case FILENAME:
		if ((fnb & (NAM$M_EXP_NAME|NAM$M_EXP_TYPE)) == 0)
			return LO_FR_BAD_SYN;	/* No name or type */
		if ((fnb & (NAM$M_EXP_DEV|NAM$M_EXP_DIR)) != 0)
			return LO_FR_BAD_SYN;	/* Device or directory */

		if (oldcall)
		{
			rtnold(loc, oldloc);
		}
		else
		{
			check_strings(orig_string, loc);
		}
		return (OK);

	    case PATH:
		/* If the name is mentioned, the path must be ".DIR" */
		if ((fnb & NAM$M_EXP_NAME) != 0)
		{
			if ((fnb & NAM$M_EXP_TYPE == 0) ||
			    (nam.nam$b_type != STlength(DIREXT)) ||
			     (STbcompare(nam.nam$l_type, nam.nam$b_type,
					 DIREXT, 0, TRUE) != 0) )
				return (LO_FR_BAD_SYN);

			/* Convert it to directory format and reparse it */
			LOfile_to_dir(string, string);
			if (LOfroms(flag, string, loc) != OK)
			/* syntax error */
			{
				loc->desc = NULL;
				return(LO_FR_BAD_SYN);
			}
		}
		if (oldcall)
		{
			rtnold(loc, oldloc);
		}
		else
		{
			check_strings(orig_string, loc);
		}
		return (OK);

	    case PATH & FILENAME:
		if (oldcall)
		{
			rtnold(loc, oldloc);
		}
		else
		{
			check_strings(orig_string, loc);
		}
		return (OK);

	    default:
		return (LO_FR_BAD_SYN);
	}
}

static VOID
rtnold(newloc, oldloc)
LOCATION	*newloc;	/* Created by LOfroms */
oldLocation	*oldloc;	/* Old-style location from LOfroms's caller */
{
	oldloc->string = newloc->string;
	oldloc->desc = newloc->desc;
	oldloc->fp = 0;
}

/*{
** Name:	check_strings - compare the original and constructed locations
**
** Description:
**	This routine simply checks the original and constructed location
**	names to see if they only differ in case.  If so, the original
**	name is used for the location.
**
** Inputs:
**	orig_string	The original location name.
**	loc
**	  .string	The constructed location name.
**
** Outputs:
**	loc
**	  .string	The location name to use, possibly changed.
**
** Returns:
**	None.
**
** Exceptions:
**	None.
**
** Side Effects:
**	None.
**
** History:
**	06/14/93 (dkh) - Initial version.
*/
static void
check_strings(orig_string, loc)
char		*orig_string;
LOCATION	*loc;
{
	/*
	**  Check if the string is the same except for
	**  case.  If so, use the original string.
	*/
	if (STbcompare(orig_string, 0, loc->string, 0, TRUE) == 0)
	{
		STcopy(orig_string, loc->string);
	}
}
