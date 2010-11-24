/*
** Copyright (c) 1987, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<cs.h>
# include	<nm.h>
# include	<lo.h>
# include	<si.h>
# include	<st.h>
# include	"nmlocal.h"
# if defined(su4_cmw)
# include	<sys/label.h>
# endif


/**
** Name: NMSETING.C - set global ingres symbol
**
** Description:
**
**      This file contains the following external routines:
**
**	NMstIngAt()	  -  Set ingres attribute.
**
**      This file contains the following internal routines:
**    
**	NMwritesyms()	  -  Write symbols to the file.
**
** History: Log:	nmseting.c,v
 * Revision 1.1  88/08/05  13:43:15  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.4  87/11/17  10:01:28  mikem
**      Include <st.h> when calling ST routines.
**      
** Revision 1.3  87/11/13  11:34:58  mikem
** use nmlocal.h rather than "nm.h" to disambiguate the global nm.h header from
** the local nm.h header.
**      
**      Revision 1.2  87/11/10  14:44:52  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.3  87/07/27  11:06:18  mikem
**      Updated to meet jupiter coding standards.
**      
**	Revision 1.2  86/06/26  15:28:57  perform
**	NMstIngAt() needed by cichkcap, ciutil and ingnetdef as well as
** 	ing[un]set[env], so put it in COMPATLIB.
** 
**	12-Mar-89 (GordonW)
**	    added call to LOlast to set modification time
**	14-apr-89 (arana)
**	    When LOlast was added to update symbol table mod time,
**	    return value was saved off but not returned.
**	14-oct-94 (nanpr01)
**	    Removed #include "nmerr.h". Bug # 63157. 
**	19-jun-95 (emmag)
**	    Need to include si.h to pick up prototype for SIclose
**	22-aug-95 (canor01)
**	    NMwritesyms() semaphore protected (MCT)
**	 9-aug-95 (allst01)
**	    For su4_cmw stop the MAC label of the symbol.tbl file
**	    from floating upwards. Save and restore the MAC label
**	    of the file (set in NMopensyms in nmfiles.c).
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**      03-jun-1996 (canor01)
**          Clean up semaphore usage for use with operating-system threads.
**          Use local semaphore to protect static structure.
**	23-sep-1996 (canor01)
**	    Move global data definitions to nmdata.c.
**      11-dec-96 (reijo01)
**          Allow generic system parameter overrides.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	13-jan-2004 (somsa01)
**	    Added NMlockSymbolData() and NMunlockSymbolData() to lock
**	    and unlock the symbol.tbl file, respectively, by manipulating
**	    the symbol.lck file. In NMwritesyms(), utilize a properly
**	    named temporary file rather than "symbol.tmp" and backup the
**	    existing symbol.tbl file to symbol.bak before updating. In
**	    NMstIngAt(), accept failure from NMreadsyms() only if symbol.tbl
**	    exists and lock/unlock the symbol.tbl file.
**	31-jan-2004 (somsa01)
**	    In NMwritesyms(), instead of using NMt_open (which utilizes
**	    II_TEMPORARY), "manually" create the temporary file in the same
**	    directory as the symbol table file and use it. This is because,
**	    on UNIX, rename() only works with arguments on the same file
**	    system, and II_TEMPORARY may not be set to the same filesystem as
**	    the location of the symbol table. Also, if all is successful, then
**	    re-write the cached symbols out to the backup symbol table.
**	03-feb-2004 (somsa01)
**	    Backed out prior changes for now.
**	06-apr-2004 (somsa01)
**	    In NMstIngAt(), added logging of update operation. In
**	    NMwritesyms(), added backup of symbol.tbl logic.
**	06-May-2010 (hanje04)
**	    Bug 123694
**	    In NMstIngAt() add check to see if symbol.tbl has been updated
**	    since it was last read. This prevents stale info being written
**	    out in the rare event that prior calls to NMgtIngAt() don't
**	    cause the in memory copy to be flushed.
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */

GLOBALREF II_NM_STATIC	NM_static;

#if defined(su4_cmw)
/*
 * These variables are used to ensure that symbol.tbl's
 * MAC label does not float upwards
 */
extern	bclabel_t	NM_saved_label;
extern	char		*NM_path;
extern	int		NM_got_label;
#endif

GLOBALREF SYM           *s_list;                /* list of symbols */
GLOBALREF SYSTIME       NMtime;                 /* modification time */
GLOBALREF LOCATION      NMSymloc;               /* location of symbol.tbl */
GLOBALREF LOCATION      NMBakSymloc;            /* location of symbol.bak */

/* statics */


/*{
** Name: NMwritesyms - write the symbol table list to file.
**
** Description:
**    Write the symbol table list back out to the file.
**
**    This duplicates the function in ingunset.c, but shouldn't be in 
**    compatlib because no one else has any business writing the file.
**
** Inputs:
**	none.
**
** Output:
**	none.
**
**      Returns:
**          OK                         	    Function completed normally. 
**          FAIL                            Function completed abnormally.
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards.
**	14-apr-89 (arana)
**	    When LOlast was added to update symbol table mod time,
**	    return value was saved off but not returned.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      29-sep-94 (cwaldman)
**          Changed check of NMopensyms return value to compare against
**          (FILE *)NULL (was NULL).
**          Changed write routine to write into a temporary file first,
**          check whether this file has been written OK, and rename
**          temporary file to symbol.tbl if it is alright. This is part
**          of the fix for bug 44445 (symbol.tbl disappears). There is
**          still a slight chance of something going wrong during the
**          rename, but in that case variables are at least preserved
**          in symbol.tmp.
**      19-oct-94 (abowler)
**          Minor correction to change above. Cal to SIopen should be
**          passed address of location structure. (Didn't show up as
**          a bug on su4_u42 !)
**      28-feb-95 (cwaldman)
**          Amendment to change on 29 Sep. Part of the check whether
**          the temporary symbol table had been written OK was to check
**          the file size. The check would return the file size if
**          check was OK and 0 otherwise. If a 0 was returned, the old
**          symbol.tbl would not be replaced. This made it impossible
**          to 'ingunset' the last variable in a symbol.tbl. Changed
**          'OK-check' to use -1 as error indicator.
**	02-jul-1996 (sweeney)
**	    Apply umask fix (bug #71890) from ingres63p
**	07-apr-2004 (somsa01)
**	    Added backup of symbol.tbl logic.
**	15-nov-2010 (stephenb)
**	    Define NMwritesyms() for prototyping.
*/
STATUS
NMwritesyms(void)
{
    register SYM	*sp;
    FILE		*fp, *tfp = NULL;
    register i4		status = OK;
    register i4		closestat;
    char		buf[ MAXLINE + NULL_NL_NULL ];
    char		tbuf[ MAXLINE + NULL_NL_NULL ];
    i4			flagword, size, symcount = 0, bksymcount;
    STATUS		retval;
    LOCATION		t_loc;
    LOINFORMATION	loinfo;
    bool		perform_backup = TRUE;
    OFFSET_TYPE		bk_size;

    /*
    ** ensure sensible umask for symbol.tmp, as it will become symbol.tbl
    */
    PEsave();
    PEumask("rw-r--");

    if ((FILE *)NULL == (fp = NMopensyms( "r" )))
    {
	PEreset();
	return (NM_STOPN);
    }
    SIclose(fp);
 
    LOcopy(&NMSymloc, tbuf, &t_loc);
    if ( OK != LOfstfile("symbol.tmp", &t_loc) ||
         OK != SIopen(&t_loc, "w", &tfp))
    {
	return (NM_STOPN);
    }

    for ( sp = s_list; sp != NULL; sp = sp->s_next )
    {
	(VOID) STpolycat( 3, sp->s_sym, "\t", sp->s_val, buf );
	STmove( buf, ' ', MAXLINE, buf );

	buf[ MAXLINE - 1 ] = '\n';
	buf[ MAXLINE ] = '\0';
 
	if ( ENDFILE == SIputrec(buf, tfp))
	{
	    status=FAIL;
	    break;
	}

	symcount++;
    }
 
    /* Very interested in close status of file being written */

    closestat = SIclose( tfp );
    flagword = (LO_I_SIZE);
    size = (OK == LOinfo(&t_loc,&flagword,&loinfo) ? loinfo.li_size : -1);
 
    retval=(status != OK || closestat != OK || size == -1 ? NM_STAPP : OK);

    /* if file written ok update modification time */
    if(retval == OK)
    {
	LOrename(&t_loc, &NMSymloc);
	LOlast(&NMSymloc, &NMtime);
 
#if defined(su4_cmw)
	/* Now reset the old MAC label if any */
	if (NM_got_label)
	{
	    (void)setcmwlabel(NM_path, &NM_saved_label, SETCL_ALL);
	    NM_got_label=0;
	}
#endif

	/*
	** If we have no backup to check against, just do it. Otherwise,
	** make sure our new symbol table differs from the backup by at most
	** one symbol before performing a backup.
	*/
	if (LOexist(&NMBakSymloc) == OK)
	{
	    LOsize(&NMBakSymloc, &bk_size);
	    bksymcount = (i4)(bk_size / MAXLINE);
	    if (bksymcount != symcount && bksymcount != symcount - 1 &&
		bksymcount != symcount + 1)
	    {
		perform_backup = FALSE;
	    }
	}

	if (perform_backup)
	{
	    if (SIopen(&NMBakSymloc, "w", &tfp) == OK)
	    {
		for (sp = s_list; sp != NULL; sp = sp->s_next)
		{
		    STpolycat(3, sp->s_sym, "\t", sp->s_val, buf);
		    STmove(buf, ' ', MAXLINE, buf);

		    buf[MAXLINE - 1] = '\n';
		    buf[MAXLINE] = '\0';

		    if (ENDFILE == SIputrec(buf, tfp))
			break;
		}

		SIclose(tfp);
	    }
	}
    }

    PEreset();
    return (retval);
}

/*{
** Name: NMstIngAt - set a value in the symbol.tbl.
**
** Description:
**    Used by ingsetenv, ingunset, cichkcap, ciutil and ingnetdef
**
**    Returns status.
**
**    This version rewrites the whole file whenever it changes.
**
** Inputs:
**	name				    Name of attribute to be set.
**	value				    Value to set attribute to.
**
** Output:
**	none.
**
**      Returns:
**          OK                         	    Function completed normally. 
**          FAIL                            Function completed abnormally.
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.
**      11-dec-96 (reijo01)
**          Allow generic system parameter overrides.
**	06-apr-2004 (somsa01)
**	    Added logging of update operation.
**	31-Aug-2004 (jenjo02)
**	    Added calls to NMlocksyms()/NMunlocksyms() to
**	    lock the symbol table while being updated.
*/
STATUS
NMstIngAt(name, value)
register char	*name;
register char	*value;
{
	register STATUS	status	= OK;
	register bool	changed = FALSE;
	register SYM	*sp;
	register SYM	*lastsp;
	register i4	rval = OK;
 
	char	buf[ MAXLINE ];
	char	newname[ MAXLINE ];
	bool	sym_added = FALSE;
	char	*oldval = NULL;
	SYSTIME		thistime;

    	if ( STncmp( name, "II", 2 ) == OK )
            STpolycat( 2, SystemVarPrefix, name+2, newname );
    	else
            STcopy( name, newname );

 
        if ( !NM_static.Init )
        {
            if( ( status = NM_initsym() ) != OK )
            {
		NMlogOperation("", NULL, NULL, NULL, status);
                return (status);
            }
        }
	MUp_semaphore( &NM_static.Sem );

	/* Lock the symbol table for update */
	if ( status = NMlocksyms() )
	{
	    MUv_semaphore( &NM_static.Sem );
	    NMlogOperation("", NULL, NULL, NULL, status);
	    return (status);
	}

	/* Read the symbol table file once */
	if ( s_list == NULL && OK != (status = NMreadsyms()) )
	{
		MUv_semaphore( &NM_static.Sem );
		NMlogOperation("", NULL, NULL, NULL, status);
		NMunlocksyms();
		return (status);
	}
	else
	{
		/* test if symbol table file was modified ? */
		LOlast(&NMSymloc, &thistime);
		if(MEcmp((PTR)&thistime, (PTR)&NMtime, sizeof(NMtime)) != 0)
		{
		    NMflushIng();
		    rval = NMreadsyms();
		    if(rval == OK)
		        LOlast(&NMSymloc, &NMtime);
		    else
		    {
			MUv_semaphore( &NM_static.Sem );
			NMlogOperation("", NULL, NULL, NULL, status);
			NMunlocksyms();
			return (status);
		    }
			
		}
	}
 
	/*
	**  Search symbol table for the desired symbol.
	**  Tedious linear search, but the list isn't very big.
	*/
 
	for ( lastsp = sp = s_list; sp != NULL; sp = sp->s_next )
	{
		lastsp = sp;
 
		/* Check one char, then call slow function */
 
		if ( *sp->s_sym == *newname && !STcompare(sp->s_sym, newname) )
			break;
	}
 
	if ( sp == NULL )
	{
		/* Add new symbol */
		
		changed = (OK == NMaddsym( newname, value, lastsp ));
		sym_added = TRUE;
	}
	else
	{
		/* Replace value if only it has changed */
 
		STcopy( value, buf );
		STtrmwhite( buf );

		if ( STcompare( buf, sp->s_val ) )
		{
			oldval = STalloc(sp->s_val);
			MEfree( sp->s_val );

			if ( NULL == (sp->s_val = STalloc( buf )) )
				status = NM_STREP;
			else
				changed = TRUE;
		}
	}
	if (changed)
		status = NMwritesyms();

	MUv_semaphore( &NM_static.Sem );

	/* Log the update. */
	if (sym_added)
		NMlogOperation("ADD", name, value, NULL, status);
	else if (oldval)
	{
		NMlogOperation("CHANGE", name, oldval, value, status);
		MEfree(oldval);
	}

	/* Release the symbol table lock */
	NMunlocksyms();
 
	return ( status );
}
