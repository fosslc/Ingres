/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"irstd.h"
# include	"irmagic.h"

/**
** Name:	irsess.c - client session administration
**
** Description:
**	open / close routines which handle client sessions.  Also
**	handles general ILRF initialization / cleanup through count
**	of total openings and current open count.
**
*/

/*
** ILRF debug tracing is shortcutted by calling fopen directly.
** Won't work on non UNIX/DOS environments without going through NM/
** LO calls
*/
#ifdef ILRFDTRACE
extern FILE *Fp_debug;
#endif

extern QUEUE M_list;
extern QUEUE F_list;

FUNC_EXTERN VOID free_client();

static VOID first_time();
static VOID first_client();
static VOID no_nims();
static VOID no_clients();

static i4  Sess_count = 0;	/* counter for irbid's */
static i4  Sess_open = 0;	/* total sessions open */
static i4  Nim_open = 0;	/* non-image file sessions open */

/*{
** Name:	IIORsoSessOpen - client session open
**
** Description:
**	opens session on behalf of a new ilrf client.  Client may be
**	running from the database, or from an image file, as indicated
**	by the interaction mode setting.  This routine sets a unique
**	identifier for the client by keeping a counter.	 The other
**	static counters together with this one also control initializations:
**
**		1) first call init
**		2) first open init
**		3) first non-image open init
**
**	2) and 3) may happen several times due to closed sessions leaving
**	ILRF with 0 open sessions
**
** Inputs:
**      stype   type of session: IORI_DB, IORI_IMAGE, or IORI_CORE.
**      obj     a PTR, either a filename of image / runtime-table file or
**              an IR_F_CTL passed in by the client.
**      fid     FID of frame we're interested in reading.  (Only needed for
**              IORI_CORE).
**
** Outputs:
**	irblk	filled in ilrf block for new client
**	rtt	runtime table, if we're fetching from an IMAGE.
**
**	return:
**		OK		success
**		ILE_IMGBAD	bad run time table / image file
**		ILE_FILOPEN	can't open run time table / image file
**
**
** History:
**	8/86 (rlm)	written
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Remove da_get, dbd_get, and push callback routines;
**		ILRF is no longer responsible for stack frame manipulation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


STATUS
IIORsoSessOpen (irblk, stype, obj, fid, rtt)
IRBLK *irblk;
i4  stype;
PTR obj;
FID *fid;
ABRTSOBJ **rtt;
{
        STATUS rc;
	IR_F_CTL *fm;

	/*
	** If we return fail, we make sure that bad magic is set in the IRBLK.
	*/
	if (stype == IORI_IMAGE)
	{
		/* 'obj' is a filename */
		if (obj == NULL
		  || (rc = IIOIifImgFetch((char *) obj,&(irblk->image))) != OK)
		{
			irblk->magic = FAIL_MAGIC;
			return (rc);
		}
		irblk->irtype = irblk->image.iact;
		*rtt = irblk->image.rtt;
	}
	else if (stype == IORI_CORE)
	{
		/* set a couple irblk.image items for safeties sake */
		irblk->image.iact = irblk->irtype = IORI_CORE;

		/* 'obj' is an IR_F_CTL */

		irblk->image.rtt = NULL;
	}
	else
	{
		/* stype is IORI_DB.
		** set a couple irblk.image items for safeties sake
		*/
		irblk->image.iact = irblk->irtype = IORI_DB;
		irblk->image.rtt = NULL;
	}
	
	irblk->magic = CLIENT_MAGIC;
	irblk->curframe = NULL;

	if ((irblk->irbid = ++Sess_count) == 1)
		first_time();

	if ((++Sess_open) == 1)
		first_client();

	if (irblk->irtype == IORI_CORE)
	{
		IR_F_CTL *ifctl;
		IR_F_CTL *dummy = (IR_F_CTL *) NULL;
		IR_F_CTL *list_add();

		/* sneaky.  we'll take our frame and set it up
		** as the internal list as if we'd cached it.
		*/

		ifctl = list_add(&M_list, &dummy);
		ifctl->d.m.tag = ((IFRMOBJ *) obj)->tag;
		ifctl->d.m.frame = ((IFRMOBJ *) obj);
		ifctl->irbid = NULLIRBID;
		ifctl->fid.id = fid->id;
		ifctl->fid.name = fid->name;
		ifctl->in_mem = TRUE;
		ifctl->is_current = FALSE;
	}

# ifdef ILRFDTRACE
	SIfprintf(Fp_debug,ERx("Assigned Client number %d\n"),irblk->irbid);
# endif

# ifdef ILRFTIMING
	IIAMtprintf(ERx("Session open for client %d"),irblk->irbid);
# endif

	return (OK);
}

/*{
** Name:	IIORscSessClose - close client session
**
** Description:
**	Closes ILRF session with client.  Decrements current open and
**	current non-image open counts, and handles cleanup involved
**	with no more clients / no more non-image clients.
**
** Inputs:
**	irblk	ilrf client
**
** History:
**	8/86 (rlm)	written
**      7/87 (steveh)   added call to IIOIifImgFree to free memory
**
*/

STATUS
IIORscSessClose(irblk)
IRBLK	*irblk;
{
	/* see that we have a legal client */
	CHECKMAGIC (irblk,ERx("sc"));

	/* free any frames held explicitly for this client */
	free_client(irblk->irbid);

	if (irblk->irtype != IORI_IMAGE)
	{
		if ((--Nim_open) <= 0)
			no_nims(irblk);
	}

	if ((--Sess_open) <= 0)
		no_clients(irblk);

	/* zap magic AFTER cleanup - cleanup routines may require valid magic */
	irblk->magic = CLOSE_MAGIC;

#	ifdef ILRFDTRACE
	SIclose(Fp_debug);
#	endif

#	ifdef ILRFTIMING
	IIAMtpclose();
#	endif

	/* free misc. allocated memory
	   allocated during the image
	   load process               */
	IIOIifImgFree();

	/* free the memory pool allocated by 
	   IIORsoSessOpen's call to first_time. */                           
	list_free_pool();

# if 0
	/* free memory allocated by abinirts */
	abuinirts();
# endif

	return (OK);
}

/*{
** Name:	IIORssSessSwap - swap ILRF session out (for concurrent DOS)
**
** Description:
**	Closes open files for users ilrf session so a subprocess may be
**	spawned.  IIORsuSessUnswap must be called before continuing.
**	We hack an enforcement for this in by having this routine invalidate
**	the magic # in the IRBLK, and restore it in IIORsuSessUnswap.
**
**	Note - since different ILRF sessions share the temp file, the first
**	IIORssSessSwap call closes it, and the first IIORsuSessUnswap call
**	reopens it.  ALL ILRF clients must make swap calls.
**
** Inputs:
**	irblk	ilrf client
**
** History:
**	6/87 (rlm)	written
**
*/

STATUS
IIORssSessSwap(irblk)
IRBLK	*irblk;
{
#	ifdef CDOS
	/* see that we have a legal client */
	CHECKMAGIC(irblk,"ss");

	irblk->magic = SWAP_MAGIC;

	temp_swap();

	IIOIisImgSwap(&irblk->image);
#	endif

	return (OK);
}

/*{
** Name:	IIORsuSessUnswap - swap ILRF session back in (concurrent DOS)
**
** Description:
**	Reopens files closed by IIORssSessSwap.  IRBLK should have SWAP_MAGIC
**	rather than the normal magic number.  This routine restores the real
**	magic number.
**
** Inputs:
**	irblk	ilrf client
**
** History:
**	6/87 (rlm)	written
**
*/

STATUS
IIORsuSessUnswap(irblk)
IRBLK	*irblk;
{
#	ifdef CDOS
	/* check magic */
	if (irblk->magic != SWAP_MAGIC)
		return (ILE_CLIENT);

	irblk->magic = CLIENT_MAGIC;

	temp_unswap();

	IIOIiuImgUnswap(&irblk->image, "r");
#	endif

	return (OK);
}

/*
** one-time init routine - create empty lists, set NULL temp file
*/
static VOID
first_time()
{
# ifdef ILRFDTRACE
	Fp_debug = fopen(ERx("ilrf.out"),ERx("w"));
	fprintf(Fp_debug,ERx("Sess: first_time\n"));
#endif

	list_create(&M_list);
	list_create(&F_list);
}

/*
** first client initialization
*/
static VOID
first_client()
{
#ifdef ILRFDTRACE
	fprintf(Fp_debug,ERx("Sess: first_client\n"));
#endif
}

/*
** no current non-image clients - close and unlink temp file, release all
** memory and nodes held on behalf of common DB clients.
*/
static VOID
no_nims(irbid)
IRBLK	*irbid;
{
#ifdef ILRFDTRACE
	fprintf(Fp_debug,ERx("Sess: no_nims\n"));
#endif
	free_client(NULLIRBID);

	IIORulUnLink(irbid);
}

/*
** no current clients - release any memory we have, return all list pool nodes
** Actually, having gone through free_client for each client, and no_nims()
** should mean that both lists are empty anyway, but we might as well make
** sure.
*/
static VOID
no_clients(irblk)
IRBLK *irblk;
{
#ifdef ILRFDTRACE
	fprintf(Fp_debug,ERx("Sess: no_clients\n"));
#endif
	IIORmfMemFree(irblk,IORM_ALL,NULL);

	list_zero(&M_list);
	list_zero(&F_list);
}
