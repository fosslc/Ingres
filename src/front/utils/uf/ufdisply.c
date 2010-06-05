/*
** Copyright (c) 1989, 2005 Ingres Corporation
**
*/

# include	<compat.h>
# include	<me.h>		 
# include	<nm.h>		 
# include	<st.h>		 
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<uf.h>
# include	<fstm.h>
# include	<ft.h>
# include	<menu.h>
# include	<help.h>
# include	<qr.h>
# include	<fmt.h>
# include	<frame.h>
# include	<frsctblk.h>
# include	"eruf.h"
# include	<ex.h>
# include	<cv.h>

/**
** Name:	ufdisply.c -	Output Browser Control Module.
**
** Description:
**	This file contains routines used to display an output file.
**
**	This file defines:
**		IIUFdsp_Display	- Drives the output display form
**    		IIUFbot_Bottom	- Loads the in-memory buffer with the
**				  'bottommost' records from the browsefile
** 		IIUFdcl_DisplayCleanup
**				- Clears output screen, reset FRSkeys
** 		IIUFint_Init	- Initializes BCB (Buffer Control Block)
**              IIUFallocate_bblist
**                              - Create a Browse Buffer.
**
** History:
**	12-oct-89 (sylviap)
**		Created from fstm files.
**	24-oct-89 (sylviap)
**		PC porting change.  Added headers fmt.h, frame.h and
**		frsctblk.h because FTgetmenu call requires pointer to
**		FRS_EVCB. Frsctblk.h defines FRS_EVCB.  This is needed because
**		otherwise, PC doesn't know if pointer is 2-bytes or 4-bytes.
**	04-jan-90 (sylviap)
**		Added new parameter to IIUFint_Init.
**	05-feb-90 (sylviap)
**		Took out Top and Bottom as menu items, as per FRC decision -
**		US #116.
**	09-feb-90 (sylviap)
**		Added parameter to IIUFdsp_Display to control scrolling.
**	02-mar-90 (Mike S)
**		Use function interface for (IIUF)oof_OnOutputForm
**	04-mar-90 (teresal)
**		Bug fix 8484 and 4064 - increased file buffer size to 8K
**		to avoid access violation when output row is very long.
**	28-mar-90 (bruceb)
**		Added locator support for menuline scrolling.  Not necessary
**		at the moment, but if the menuline ever gets longer....
**	01-may-90 (sylviap)
**		Added support for popups in scrollable output.
**	15-may-90 (sylviap)
**		Added parameter for calls to reopenSpill.
**	27-aug-90 (kathryn/emerson) 
**		Integrate VM porting changes:
**		- Removed many "ifdef FT3270" no longer needed after
**		  ESD's reworking of FT3270(wolf/emerson 5-6-90).
**	8-mar-93 (rogerl) 
**		add parm (nointr) to IIUFdsp_Display to turn off interrupts for
**		submenus/help scrs
**	20-mar-2000 (kitch01)
**		Bug 99136. Changes to support the use of the spill file for 
**		displaying records that are too large for the browse buffer area.
**  17-nov-2000 (kitch01)
**      Amend above fix to be CL compliant. File access is now random and
**      (for VMS) we pass the max column size.
**  20-jun-2001 (bespi01)
**      Changed SIfopen() for the spill file to use a fixed blocksize
**      rather than the maximum line width. 
**      Bug 104175, Problem INGCBT 335
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-sep-2002 (wanfr01)
**            Bug 107573, INGCBT 415
**            Do not attempt to close a spill file that is already closed.
**  11-mar-2002 (horda03) Bug 107271
**      Created new function IIUFallocate_bblist() to create Browse Buffer
**      areas. During initialisation of the BCB, the first browse buffer
**      is created, and a memory buffer zone (that will be released if a
**      spill file is required.
**	30-jan-2004 (drivi01)
**	      Added UFsetMax_num_bb function for utm.
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
**	26-Aug-2009 (kschendel) b121804
**	    Redo prototype for gcc 4.3.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/
#define BFRSIZE 8192

#define DIAG(msg)	if (iiuftfdDiagFunc != NULL) (*iiuftfdDiagFunc)(msg);

STATUS		   MTinit();
VOID		   FDdmpmsg();
bool		   IIUFmro_MoreOutput();
VOID		   FDdmpcur();
VOID 		   IIUFbot_Bottom();
FUNC_EXTERN STATUS iiufBrSave();

FUNC_EXTERN STATUS IIUFallocate_bblist();

static i4	_PutMenu();

static bool	oof_OnOutputForm = FALSE;
static i4       max_num_bb = -1;
/* By default allow a maximum number of Browse Buffers
** to handle 30M Bytes of data. For 65K BB_SIZE, this
** equates to 450 pages.
*/
#define DEFAULT_MAX_BB ((30000000 / BB_SIZE) + 1)


GLOBALREF VOID  (*iiuftfdDiagFunc)();   /* Diagnostic tracing function */

static struct com_tab mistr1[] =
{
	/* name;   func();   enum; flags; begpos; endpos; desc; */
	NULL,	   NULL,	PRTKEY,	0,    0,	    0, NULL,
	NULL,	   NULL,	SVEKEY,	0,    0,	    0, NULL,
	NULL,	   NULL,	HELPKEY,0,    0,	    0, NULL,
	NULL,	   NULL,	ENDKEY,	0,    0,	    0, NULL,
	NULL,	   NULL,	0,	0,    0,	    0, NULL
};

static struct menuType mstr1 =
{
	NULL,				/* mu_prline */
	mistr1,				/* mu_coms */
	NULLRET|MU_RESET|MU_NEW,	/* mu_flags */
	NULL,				/* mu_prompt */
	NULL,				/* mu_window */
	0,				/* mu_dfirst */
	0,				/* mu_dsecond */
	4,				/* mu_last */
	NULL,				/* mu_prev */
	NULL,				/* mu_act */
	-1,				/* mu_back */
	-1,				/* mu_forward */
	-1				/* mu_colon */
};


/*{
**    IIUFdsp_Display  --  Drive the output display form
**
**    Present a menu and display the results from the backend. Each time
**    the user tries to scroll beyond the current 'end of file' we
**    will call IIUFmro_MoreOutput to get another chunk of output.
**
** History:
**	08/25/87 (scl) Put back some #ifdefs required for FT3270
**	11/12/87 (dk) - FTdiag -> FSdiag.
**	11/21/87 (dkh) - Fixed jup bug 1390.
**	19-may-88 (bruceb)
**		Changed ME[c]alloc calls into calls on MEreqmem.
**	26-oct-88 (bruceb)	Fix for bug 2603.
**		Added IIMFdcl_DisplayCleanup to clear output screen and
**		reset FRSkeys.  Called when output screen is interrupted
**		after completion of query request.
**	03-apr-89 (wolf)
**		Put back the FTputmenu call in #ifdef FT3270
**	02-aug-89 (sylviap)
**		Made FSdisp a boolean function to return to a caller if
**		an error occurred
**	03-aug-89 (sylviap)
**		Added the PRINT Menu item.
**	01-sep-89 (sylviap)
**		Changed the number of menu items to 6 to include PRINT and END.
**	12-oct-89 (sylviap)
**		Extracted from FSdisp.c.  Changed routine names.
**	17-aug-1990 (kathryn) Bugs 7410(ISQL) & 32102(RBF)
**		Changed function of HELPKEY from FEfehelp() to IIRTdohelp().
**		We need to call IIRTdohelp directly so we can pass in the menu.
**		This is needed for "help keys" to work correctly, as the
**		output display form is NON-FRS.
**	17-aug-91 (leighb) DeskTop Porting Change:
**		Moved static decl of func _top to outside calling function.
**	8-mar-93 (rogerl) nointr TRUE to ignore ^c on subscreens; assume EX_ON
**		at entry
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

static VOID	_top();				 

bool
IIUFdsp_Display(BCB *bcb, QRB *qrb, char *help_scr, char *help_title,
	bool scroll_fl, char *print_title, char *file_title, bool nointr)
{
	register i4	key;
	register MENU	mptr;
	bool		error_fl;

	i4		xpos;
	i4		ypos;

	if ( mistr1[0].ct_name == NULL )
	{
		mistr1[0].ct_name = ERget(FE_Print);
		mistr1[0].description = ERget(F_UF0003_Print_this_output);
		mistr1[1].ct_name = ERget(FE_File);
		mistr1[1].description = ERget(F_UF0004_Save_this_output);
		mistr1[2].ct_name = ERget(FE_Help);
		mistr1[2].description = ERget(F_UF0005_Help_on_the_form);
		mistr1[3].ct_name = ERget(FE_End);
		mistr1[3].description = ERget(F_UF0006_End_this_display);
		mistr1[4].ct_name = NULL;
		mistr1[4].description = ERx("");
	}

	DIAG(ERx("IIUFdsp_Display entered\n"));

	error_fl = FALSE;
	xpos = 0;
	ypos = 0;

	mptr = &mstr1;

	/* Force the next FTputmenu() call to re-construct the menu. */
	mptr->mu_flags |= MU_NEW;

	FTclear();
	_PutMenu(mptr);

	key = 0;
	while ((key != ENDKEY) && (!error_fl))
	{
#ifdef FT3270
		MTview(bcb, mptr, qrb, scroll_fl, &xpos, &ypos);
		key = FTgetmenu(mptr, NULL);
#else
		if ((key = MTview(bcb, mptr, qrb, scroll_fl, &xpos, &ypos)) == MENUKEY)
		{
			_PutMenu(mptr);

			/*
			**  Fix for BUG 7123. (dkh)
			*/
			key = FTgetmenu(mptr, NULL);
		}
#endif

		switch (key)
		{
		  case HELPKEY:
		  {
			if ( nointr )
			    EXinterrupt(EX_OFF);

			/* Changed fix for bugs 7729 and 7730 (kathryn)*/
			/*
			** Use IIRTdohelp() instead of FEhelp():
			** last arg to IIRTdohelp() is H_IQUEL--telling help
			** that this is isquel.	 this is required since
			** the output display form is a non-FRS form
			** (similar to vifred's layout form) and can't
			** be treated in a standard fashion.
			*/
			/* END FIX FOR BUG 7730 */

			if ( !IIRTdohelp(help_scr,help_title,mptr,TRUE,
				H_IQUEL, (VOID(*)())NULL))
			/*
			** if user actually got help, and thus altered
			** more than just the menu line, clear and
			** refresh the screen.
			*/
			{
				FTclear();
			}
#ifdef FT3270
			/*
			** FEfehelp has reset the "current menu"pointer;
			** set it back where it should be.
			*/
			_PutMenu(mptr);
#endif
			if ( nointr )
			    EXinterrupt(EX_ON);

			break;
		  }

		  case BOTKEY:

			if ( ! bcb->req_complete )
			{
				IIUGmsg(ERget(S_UF0018_RunToCompletion), FALSE, 0);
				if (!IIUFmro_MoreOutput(bcb, 0, qrb))
				{
					error_fl = TRUE;
					break;
				}
			}
			IIUFbot_Bottom(bcb);
			break;

		  case TOPKEY:
			_top(bcb);
			break;

		  case SVEKEY:
			if ( nointr )
			    EXinterrupt(EX_OFF);

			if (iiufBrSave(bcb, TRUE, xpos, ypos, print_title, 
				file_title, qrb) != OK)
				error_fl = TRUE;

			if ( nointr )
			    EXinterrupt(EX_ON);

			break;

		  case PRTKEY:
			if ( nointr )
			    EXinterrupt(EX_OFF);

			if (iiufBrSave(bcb, FALSE, xpos, ypos, print_title,
				file_title, qrb) != OK)
				error_fl = TRUE;

			if ( nointr )
			    EXinterrupt(EX_ON);

			break;

		  case ENDKEY:
			break;
		}
	}

	FTclear();
	IIresfrsk();	  /* Reset FRS keys */

	DIAG(ERx("IIUFdsp_Display exit\n"));

	oof_OnOutputForm = FALSE;	/* Leaving the output form */

	/* did an error occur? */
	return (bool) ! error_fl ;
}

/*{
**    IIUFbot_Bottom	--  Load the in-memory buffer with the 'bottommost'
**		    records from the browsefile
**
**    Close the browsefile for write and open it for read.
**    Read from the browsefile and add the records to the in-memory
**    buffer until all records have been read. Then position the
**    'first visible record' pointer to point to the very last page
**    in memory.
**
**    When done, the browsefile will be reopened for 'append'.
**
**    Returns: nothing
**
** History:
**	  20-mar-2000 (kitch01)
**		Add logic to rewind the spill file index to the appropriate point
**    17-nov-2000 (kitch01)
**      Amend above fix to be CL compliant. File access is now random and
**      (for VMS) we pass the max column size.
*/
static VOID	_reopenSpill();

VOID
IIUFbot_Bottom(bcb)
register BCB	*bcb;
{
	register i4	i;

	DIAG(ERx("IIUFbot_Bottom: entering\n"));

	if (!bcb->bf_in_use)
	{
		/*
		**    If the bottom of the file is not currently in memory then we
		**    will have to read it in
		*/
		if (bcb->lrec < bcb->nrecs)
		{
			STATUS	rc;

			_reopenSpill(bcb);

			for ( i = bcb->nrecs ; --i >= 0 ; )
			{
				char	bfr[BFRSIZE];

				if (iiuftfdDiagFunc != NULL)
					(*iiuftfdDiagFunc)(ERx("IIUFbot_Bottom: getting record number %d\n"), i+1);
				if ( (rc = SIgetrec(bfr, sizeof(bfr), bcb->bffd)) != OK)
				{
					iiufBrExit( bcb, E_UF001A_EOFonFile,
							2, bcb->bfname, (PTR)&rc
					);
				}
				bfr[ STlength(bfr)-1 ] = EOS;
				IIUFadd_AddRec(bcb, bfr, TRUE);
			}

			DIAG(ERx("IIUFbot_Bottom: closing spill file for read\n"));
			SIclose(bcb->bffd);
			bcb->bffd = NULL;

			/*
			**    Re-open the file for append so we will be positioned to
			**    add more records.
			*/
			DIAG(ERx("IIUFbot_Bottom: opening spill file for append\n"));
			if ((rc = SIfopen(&bcb->bfloc, ERx("a"), SI_RACC, RACC_BLKSIZE, &bcb->bffd)) != OK)
			{
				iiufBrExit( bcb, E_UF001B_FileOpenAppend,
						2, bcb->bfname, (PTR)&rc
				);
			}
		}

		/*
		**    Make the last page in memory visible
		*/
		bcb->vrec = bcb->lrec;
		bcb->vrcb = bcb->lrcb;
		for ( i = bcb->mx_rows - 3 ; --i > 0 ; )
		{
			if (bcb->vrcb->prcb == NULL)
				break;
			if (bcb->vrec == bcb->frec)
				break;
			--bcb->vrec;
			bcb->vrcb = bcb->vrcb->prcb;
		}
	}
	else
	{
		/* All of the displayable records are in the spill file on 
		** disk. Make the first visible record the last record and then
		** rewind until we have the record that will be shown at the top
		** of the output window.
		*/
		DIAG(ERx("IIUFbot_Bottom: Reversing spill file position\n"));
		bcb->vbfidx = bcb->lbfidx;
#ifdef HORDA03
		for ( i= bcb->mx_rows - 3; --i > 0; )
		{
			if (bcb->vbfidx->pbfpos == NULL)
				break;
			bcb->vbfidx = bcb->vbfidx->pbfpos;
		}
#endif
		/* horda03 - Spill file records are now grouped into
                **           consequative records of equal length.
                **           Determine the vbfidx records required
                **           and the record number.
                */
                bcb->vbf_rec = bcb->nrecs - (bcb->mx_rows - 3);

                if (bcb->vbf_rec < 1) bcb->vbf_rec = 1;

                while (bcb->vbfidx->recnum > bcb->vbf_rec)
                {
                   bcb->vbfidx = bcb->vbfidx->pbfpos;
                }
	}

	DIAG(ERx("IIUFbot_Bottom: leaving\n"));
	return;
}

static VOID
_reopenSpill ( bcb )
register BCB	*bcb;
{
	STATUS	rc;

	IIUFclb_ClearBcb(bcb);

	/* Close the file for write to flush any buffers still in memory. */
	DIAG(ERx("IIUFbot_Bottom: closing spill file\n"));

	if (bcb->bffd) 
        {
	   if ((rc = SIclose(bcb->bffd)) != OK)
	   { /* shouldn't happen */
		iiufBrExit(bcb, E_UF0019_CloseFile, 2, bcb->bfname, (PTR)&rc);
	   }
	   bcb->bffd = NULL;
	}
	else
	   DIAG(ERx("IIUFbot_Bottom: _reopening closed spill file\n"));

	/*
	**  Open the file for read and read all the way to end-of-file,
	**  so we will have the bottommost records in memory.
	*/

	DIAG(ERx("IIUFbot_Bottom: opening spill file for read\n"));
	if ((rc = SIfopen(&bcb->bfloc, ERx("r"), SI_RACC, RACC_BLKSIZE, &bcb->bffd)) != OK)
	{
		iiufBrExit(bcb, E_UF0017_FileOpenRead, 2, bcb->bfname, (PTR)&rc);
	}
}


/*
** Name:	_top() -	 Load the in-memory buffer with the 'topmost'
**					 records from the browsefile
**
**    Close the browsefile for write and open it for read.
**    Read from the browsefile and add the records to the in-memory
**    buffer until the buffer is full. Then position the 'first
**    visible record' pointer to point to the very first record
**    in memory.
**
**    When done, the browsefile will be LEFT OPEN FOR READ, since
**    the user will probably scroll forward sequentially through
**    the browsefile.
**
** History:
**	  20-mar-2000 (kitch01)
**		Add logic to move the spill file index to the appropriate point
*/
static VOID
_top ( bcb )
register BCB	*bcb;
{
	DIAG(ERx("_top: entering\n"));
	if (!bcb->bf_in_use)
	{
		if (bcb->frec == 1)
		{
			bcb->vrec = 1;
			bcb->vrcb = bcb->frcb;
		}
		else
		{
			register i4	i;
			STATUS		rc;

			IIUFfsh_Flush(bcb);
			_reopenSpill(bcb);

			for ( i = bcb->nrecs ; --i >= 0 ; )
			{
				i4	len;
				char	bfr[BFRSIZE];

				if (iiuftfdDiagFunc != NULL)
					(*iiuftfdDiagFunc)(ERx("_top: getting record number %d\n"), i+1);
				if ( (rc = SIgetrec(bfr, sizeof(bfr), bcb->bffd)) != OK)
				{
					iiufBrExit( bcb, E_UF001A_EOFonFile,
							2, bcb->bfname, (PTR)&rc
					);
				}
				bfr[ len = STlength(bfr) - 1 ] = EOS;
				/*
				**    If the record we just read won't fit in the browse
				**    buffer we must squirrel it away so that IIUFmro_MoreOutput
				**    can pick it up when the user moves forward.
				*/
				if (!IIUFadd_AddRec(bcb, bfr, FALSE))
				{
					if (bcb->rd_ahead)
						MEfree(bcb->rd_ahead);
					bcb->rd_ahead = (char *)MEreqmem((u_i4)0,
						(u_i4)(len + 1), (bool)FALSE,
						(STATUS *)NULL);
					MEcopy(bfr, len+1, bcb->rd_ahead);
					break;
				}
			} /* end for */
		}
	}
	else
	{
		/* set the first visible index record to the first spill
		** file record
		*/
		DIAG(ERx("_top: Moving to top of spill file\n"));
		bcb->vbfidx = bcb->fbfidx;
                bcb->vbf_rec = 1;
	}

	DIAG(ERx("_top: leaving\n"));
	return;
}

static i4
_PutMenu(mptr)
MENU	mptr;
{
	FTclrfrsk();
	FTaddfrs(1, HELPKEY, NULL, 0,0);
	FTaddfrs(3, ENDKEY, NULL, 0,0);
	FTaddfrs(5, TOPKEY, ERget(F_UF0001_Go_to_top_of_output), 0,0);
	FTaddfrs(6, BOTKEY, ERget(F_UF0002_Go_to_bottom_of_outpu), 0,0);
	FTputmenu(mptr);
}

/*{
** Name:	IIUFdcl_DisplayCleanup	- clear output screen, reset FRSkeys
**
** Description:
**	Clear the output screen (which may well be larger/longer than the
**	input screen tablefield) and reset the FRSkey mappings.  This is
**	called from FSrun following an interrupt AFTER a request has been
**	completed (either because all data has been shown or because of a
**	prior interrupt.)
**
**	This is only done if not FT3270.  The reason for that is whatever
**	reason exists for the same choice being made in FSdisp.
**
** History:
**	26-oct-88 (bruceb)
**		Written.
*/
VOID
IIUFdcl_DisplayCleanup()
{
	FTclear();
	IIresfrsk();	  /* Reset FRS keys */
}


/*{
** Name:	IIUFint_Init - Initialize BCB (Buffer Control Block)
**
** Description:
**	This file contains initializing routines for Output Buffer.
**
** History:
**	08/25/87 (scl) Put back some FT2370 #ifdefs
**	11/12/87 (dkh) - Moved parts of routine FSinit() to FT directory.
**	11/21/87 (dkh) - Eliminated any possible back references from mt.
**	19-may-88 (bruceb)
**		Changed ME[c]alloc calls to calls to MEreqmem.
**	28-oct-88 (bruceb)
**		Initialize req_begin and req_complete.
**	05/04/89 (kathryn) Bug #5606
**		Reduce info.lines by 1, to match the size of the output
**		screen. Output Form was previously changed to one less line,
**		to allow for 2 blank lines on screen instead of 1.
**	14-aug-89 (sylviap)
**		Changed FSmore to be a bool function from a i4  function.
**	12-oct-89 (sylviap)
**		Extracted from fsnit.c.
**		Moved FSclrbb and FSdone to the uf directory - ufdisply.c.
**		Made changes to the FS routines to call the IIUF routines.
**	04-jan-90 (sylviap)
**		Passes to MTinit a new parameter, title, to display as the
**		title in the output browser.
**      11-mar-2002 (horda03) Bug 107271.
**              Allocate the first of the browse buffers, and the memory
**              to allow creation of a spill file.
**	7-oct-2004 (thaju02)
**	        Change no_pages to SIZE_TYPE.
*/

static VOID	_dummy();

BCB *
IIUFint_Init(filename, title)
char	*filename;
char	*title;
{
	register BCB	*bcb;
	FT_TERMINFO	info;
	FILE		*fp;
	STATUS		rc;
	FSMTCB		fscb;
	SIZE_TYPE	no_pages;
	CL_SYS_ERR	err;
        char            *no_bb;
        i4              max_bb;

	/*
	**    Allocate Browse Control Block
	*/
	if ((bcb = (BCB *)MEreqmem((u_i4)0, (u_i4)sizeof(struct _bcb),
		(bool)TRUE, (STATUS *)NULL)) == NULL)
	{
		return (NULL);
	}

	FTterminfo(&info);
	bcb->mx_rows = info.lines-2;	/*  leave 2 blank lines - Bug 5606 */
	bcb->mx_cols = info.cols;

	fscb.mfputmu_proc = _PutMenu;
	fscb.mfdmpmsg_proc = FDdmpmsg;
	fscb.mffsmore_proc = IIUFmro_MoreOutput;
	fscb.mfdmpcur_proc = FDdmpcur;
	if (iiuftfdDiagFunc != NULL)
		fscb.mffsdiag_proc = iiuftfdDiagFunc;
	else
		fscb.mffsdiag_proc = _dummy;


	if (MTinit(&fscb, title) != OK)
	{
		return(NULL);
	}

	/*
	**    Allocate Browse Buffer
	*/
	bcb->sz_bb = BB_SIZE;

        /* Has the user asked for more Browse BUffers ? */
        if (max_num_bb == -1)
        {
           NMgtAt( ERx("II_NUM_BROWSE_BUF"), &no_bb);
           if (no_bb && *no_bb)
           {
              CVal(no_bb, &max_bb);

              max_num_bb = (max_bb <= 0) ? DEFAULT_MAX_BB : max_bb;
           }
           else
           {
              max_num_bb = DEFAULT_MAX_BB;
           }
        }

        bcb->n_bb = 0;

	/* Allocate the first Browse Control Block */
	if (IIUFallocate_bblist( bcb, &bcb->firstbb ) == FAIL)
	{
	   return (NULL);
	}

	/* Allocate memory to be released when spill file is required. */
	MEget_pages(ME_NO_MASK, BBMEMBUFPAGES, "", &bcb->membuffer, &no_pages, &err);

	bcb->lastbb = bcb->firstbb;

	bcb->nrecs = 0;
	bcb->mxcol = 0;

	if ((rc = NMt_open(ERx("w"), ERx("fs"), ERx("out"),
						&bcb->bfloc, &fp)) != OK)
	{
		IIUGerr(E_UF0024_TempFile, 0, 1, (PTR)&rc);
		return NULL;
	}
	LOcopy(&bcb->bfloc, bcb->bfname, &bcb->bfloc);
	SIclose(fp);

	IIUFclb_ClearBcb(bcb);

	if (filename != NULL)
	{
		bcb->backend = FALSE;
		STcopy(filename, bcb->ifname);
		LOfroms(PATH&FILENAME, bcb->ifname, &bcb->ifloc);
		bcb->ifrc = SIfopen(&bcb->ifloc, ERx("r"), SI_TXT,
							512, &bcb->iffd);
	}
	else
	{
		bcb->backend = TRUE;
	}

	bcb->req_begin = FALSE;	/* Not yet in a query--never FALSE again */
	bcb->req_complete = TRUE;	/* Not yet in an incomplete query */

	return(bcb);
}

/*
** Name:	_dummy() -	Do not register a diagnostic function
**
** Description:
**	 FSTM is built with diagonstic function calls throughout its source
**	 code.  Since other programs (ReportWriter, teamwork) do not need the
**	 diagonstic tests, IIUFnod_NoDiag is a dummy routine that just returns.
**
** History:
**	16-oct-89 (sylviap)
**		Created.
*/
static VOID
_dummy()
{
	return;
}

/*{
** Name:	IIUFoosOnOutputSet - Set "on output form" flag
** Name:	IIUFoogOnOutputGet - Get "on output form" flag
**
** Description:
**	Set/Get the flag which indicates whether we're on the output form.
**
** Inputs:
**	newoof	{bool}	Whether we're on the output form (IIUFoosOnOutputSet)
**
** Returns:
**	{bool}  Whether we're on the output form (IIUFoogOnOutputGet)
**
** History:
**	3/1/90 (Mike S) created
*/
VOID
IIUFoosOnOutputSet(newoof)
bool newoof;
{
	oof_OnOutputForm = newoof;
}

bool
IIUFoogOnOutputGet(newoof)
bool newoof;
{
	return oof_OnOutputForm;
}

/*
** Name:	IIUFallocate_bblist() -	Allocate Browse Buffer
**
** Description:
**	 Allocates the memory for a Browse Buffer element. It then
**       sets up the pointer to the data area (bb) and the end of the
**       data area (bbend).
**
** Parameter:
**      bcb  -- Pointer to the browse control block.
**
** Returns:
**      bblist -- Address of the new browse buffer.
**      OK     -- If new browse buffer created.
**      FAIL   -- If memory allocation failed.
**
** History:
**	11-mar-2002 (horda03)
**		Created.
*/
STATUS
IIUFallocate_bblist(bcb, bblist)
BCB *bcb;
BBLIST **bblist;
{
   
   if ( (bcb->n_bb >= max_num_bb) ||
        ((*bblist = (BBLIST *)MEreqmem( 0, sizeof(bblist) + bcb->sz_bb, TRUE, NULL)) == NULL) )
   {
      /* Either run out of memory, or used up all the allowed
      ** Browse buffers.
      */
      return FAIL;
   }

   (*bblist)->bb = (char *) ( (char *) *bblist + sizeof(BBLIST));

   (*bblist)->bbend = (char *) ((*bblist)->bb + bcb->sz_bb);

   bcb->n_bb++;

   return OK;
}

VOID
UFsetMax_num_bb()
{
	max_num_bb = DEFAULT_MAX_BB;
	return;
}
