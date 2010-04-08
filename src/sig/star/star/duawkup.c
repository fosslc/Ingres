/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** duawkup.c - create a scheduler wakeup file
**
** History:
**	23-aug-1993 (dianeh)
**		Created (from ingres63p!generic!starutil!duf!dua!duawkup.c);
**		unchanged, except for removing OWNER = INGUSER (not used).
**	04-apr-06 (toumi01)
**	    Check that the wakeup def file is a regular file and, if it is,
**	    that we can clear it.
**	05-apr-06 (drivi01) on behalf of toumi01.
**	    Port change to Windows by simplifying call to LOinfo to return
**	    only the basic file type.
*/
 
/*
NEEDLIBS = COMPATLIB MALLOCLIB
 
MODE	= SETUID
 
PROGRAM = wakeup
*/
 
# include	<compat.h>
# include	<dbms.h>
# include	<st.h>
# include	<pc.h>
# include	<lo.h>
# include	<nm.h>
# include	<me.h>
# include       <si.h>
 
main()
{
    LOCATION	wakeup_loc;
    LOCATION	save_loc;
    char	wakeup_file[MAX_LOC];
    char	save_file[MAX_LOC];
#ifdef hp9_mpe
    FILE        *file;
#endif /* hp9_mpe */
 
    MEadvise ( ME_INGRES_ALLOC );
    /* first delete wakeup file if it exists - this avoids getting multiple
       wakekup files created in VMS */
    wakeupexists();
 
    (void) STpolycat(2, "alarmwkp.", /*T_alarm_checker_id*/ "def", wakeup_file);
    NMloc(FILES, FILENAME, wakeup_file, &wakeup_loc);

    LOcopy(&wakeup_loc, save_file, &save_loc);
#ifdef hp9_mpe
    if (SIfopen(&save_loc, "w", SI_TXT, 256, &file) != OK)
	PCexit(FAIL);
#else
    if (LOcreate(&save_loc) != OK)
	PCexit(FAIL);
#endif /* hp9_mpe */
}
 
 
wakeupexists()
{
    LOCATION	wakeup_loc;
    LOCATION	save_loc;
    LOINFORMATION	loinf;
    char	wakeup_file[MAX_LOC];
    char	save_file[MAX_LOC];
    i4		flaginf;
 
    (void) STpolycat(2, "alarmwkp.", /*T_alarm_checker_id*/ "def", wakeup_file);
    NMloc(FILES, FILENAME, wakeup_file, &wakeup_loc);
    LOcopy(&wakeup_loc, save_file, &save_loc);
    flaginf = LO_I_TYPE ;
    if (LOinfo(&save_loc, &flaginf, &loinf) == OK)
    {
	if (!(flaginf & LO_I_TYPE))
	{
	    SIprintf("Error detecting type of wakeup file\n");
	    PCexit(FAIL);
	}
	if (loinf.li_type != LO_IS_FILE)
	{
	    SIprintf("Wakeup file is not a regular file\n");
	    PCexit(FAIL);
	}
	if (LOdelete(&save_loc) != OK)
	{
	    SIprintf("Error clearing the wakeup file\n");
	    PCexit(FAIL);
	}
	return (TRUE);
    }
    return (FALSE);
}
