/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<lo.h>
#include 	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>

/**
** Name:    fevmstat.c -      Front-End Utility Virtual Memory Statistic Module.
**
** Description:
**	Contains routines used to gather memory usage statistics for analysis
**	by front-end programs.
**
** History:
**	Revision 3.0  84/10/30  ron
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** FEvminit - Initialization routine.
**
** 	FEvminit initializes all global variables and opens any files 
**	required for virtual memory statistic gathering.  Specifically,
**	1)  the user specified file is opened,
**	2)  the static variables FE_vm_open, FE_high_vm, 
**	    FE_vm_fp and FE_tot_delta are initialized,
**	3)  an initial vm statistic is entered in the file.
**
**	FEvminit is used in conjunction with FEgetvm and FEendvm.
**
**	Parameters:
**		filename	- Name of the file to collect vm statistics.
**		statfile	- FE id for which stats are being gathered.
**
** Returns:
**	OK or FAIL	
**
** Assumptions:
**	The file specified by the user is not open.
**
** History:
**	10/30/84 (rlk) - First Written
**
** FEendvm - Finalization routine.
**
** 	FEendvm terminates the virtual memory gathering statistics for
**	the current file.  Specifically, it
**	1)  writes a final statisics record to the file,
**	2)  closes the file.
**	
**	FEendvm is used in conjunction with FEvminit and FEgetvm.
**
**	Parameters:
**		None.
**
**	Returns:
**		OK or FAIL	
**
**	Assumptions:
**		None.
**
**	Side Effects:
**		None.
**
**	History:
**		10/30/84 (rlk) - First Written
**
** FEgetvm - Collect and write virtual memory statistics.
**
** 	FEgetvm collects three virtual memory statistics:
**	1)  Highest virtual address,
**	2)  The change in virtual memory since last call to FEgetvm,
**	3)  The total change in virtual memory (since calling FEvminit).
**
**	FEgetvm then writes those statistics to the file opened in FEvminit.
**
**	Parameters:
**		string	- An identification string to add as a comment to the
**			  statistics file.
**
**	Returns:
**		OK or FAIL	
**
**	Assumptions:
**		None.
**
**	Side Effects:
**		None.
**
**	History:
**		10/30/84 (rlk) - First Written
**
** FEwrstat - Write statistics to the file opened in FEvminit.
**
**	Parameters:
**		h_vm	- Highest virtual memory address,
**		l_delta_vm - Change in vm since last call to FEgetvm,
**		t_delta_vm - Total change in vm since call to FEvminit.
**
**	Returns:
**		OK or FAIL	
**
**	Assumptions:
**		None.
**
**	Side Effects:
**		None.
**
**	History:
**		10/30/84 (rlk) - First Written
*/

static	bool	FE_vm_open = FALSE;	/* Flag showing if the statistics file 
					   is open (= TRUE). */
static  FILE	*FE_vm_fp = NULL;	/* Pointer to statistics file */
static  PTR	FE_tot_vm = (PTR) NULL;/* Change in vm since FE_vminit call */
static	PTR	FE_high_vm = (PTR) NULL;/* Highest vm address encountered */

VOID
FEvminit (filename, statfile)
char	*filename;
char	*statfile;
{
# ifdef FE_VM_STAT
    LOCTYPE	loc_type;
    char	loc_buf[MAX_LOC];
    LOCATION	loc[MAX_LOC];

    PTR	cur_vm;		/* Current vm usage */

    PTR	sbrk();	/* System function to get vm addr */

    /*	Convert the filename to a location */

    loc_type = FILENAME;
    STcopy(filename, loc_buf);
    LOfroms(loc_type, loc_buf, loc);

    /*	Open the file for writing */

    if(SIopen(loc, ERx("a"), &FE_vm_fp) != OK)
    {
	FE_vm_open = FALSE;
	return FAIL;
    }
    else
    {
	FE_vm_open = TRUE;
	SIfprintf(FE_vm_fp,
		ERx("----- Virtual Memory Statistics for %s -----\n"), statfile
	);
	SIfprintf(FE_vm_fp,
	      ERx("Highest Address (hex)     VM Change     Total VM Change\n\n")
	);
    }

    /*	Initialize the total vm change */

    FE_tot_vm = 0;

    /*	Get the initial high vm address */

    FE_high_vm = sbrk(0);

    /*	Write a statistics record */

    FEwrstat(FE_high_vm, (PTR)0, FE_tot_vm, ERx("Initialization"));

    return (OK);
#endif /* FE_VM_STAT */
}

STATUS
FEendvm()
{
# ifdef FE_VM_STAT
    FEgetvm(ERx("End of Virtual Memory Statistics"));

    FE_vm_open = FALSE;

    return SIclose(FE_vm_fp);
# endif /* FE_VM_STAT */
}

VOID
FEgetvm (comment)
char	*comment;
{
# ifdef FE_VM_STAT
    PTR		cur_vm;		/* Current high vm addr */
    PTR		delta_vm;	/* Change since last time */
    PTR		total_vm;	/* Total vm change */

    PTR	sbrk();		/* System routine */

    /*	Get the current high vm address */

    cur_vm = sbrk(0);

    /*	Compute the change in vm since last call */

    delta_vm = (PTR)((i4)cur_vm - (i4)FE_high_vm);
    FE_high_vm = cur_vm;

    /*	Update the total change since call to FEvminit */

    FE_tot_vm = (PTR)((i4)FE_tot_vm + (i4)delta_vm);
    total_vm = FE_tot_vm;

    /*	Write out the results */

    FEwrstat(cur_vm, delta_vm, total_vm, comment);
# endif /* FE_VM_STAT */
}

VOID
FEwrstat (h_vm, l_delta_vm, t_delta_vm, comment)
PTR	h_vm;
PTR	l_delta_vm;
PTR	t_delta_vm;
char	*comment;
{
# ifdef FE_VM_STAT
    if (FE_vm_fp != NULL)
	SIfprintf(FE_vm_fp, ERx("%-21lx     %-9ld     %-15ld     %s\n"), 
			(long)h_vm, (long)l_delta_vm, (long)t_delta_vm, comment
	);
# endif /* FE_VM_STAT */
}
