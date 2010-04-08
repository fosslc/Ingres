/*
**	Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<me.h>		 
#include	<er.h>
#include	<cv.h>
#include	<nm.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	"irstd.h"
#include	<ilrffrm.h>
#include	"irmagic.h"
#include	"eror.h"
# include <ooclass.h>

/**
** Name:	irfget.c -	ILRF Get Frame Module.
**
** Description:
**	Retrieves code for a frame.  Defines:
**
**	IIORfgFrmGet()		get code for frame.
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Consolidated IIORfnFrmNoptrz, recover_frame,
**		and the static function get_frame into IIORfgFrmGet.
**		Eliminated the static function push_stack.
**		Note: ILRF now longer handles the run-time stack:
**		it's up to the client to push and pop stack frames,
**		pointerize dbdv arrays, etc.
**	08/17/91 (emerson)
**		Removed superfluous function declaration of list_new.
**	01-may-92 (davel)
**		Introduce II_4GL_FRAMES_CACHED logical to override number
**		of frames kept in memory by the interpreter.
**	01-jun-92 (davel)
**		Remove call to obsolete IIORfcFixCurrent().
**	08-jun-92 (leighb) DeskTop Porting Change: added nm.h
**
**	Revision 6.0  87/02  bobm
**	Added 'IIORfnFrmNoptrz()'.
**
**	Revision 5.1  86/08  bobm
**	Initial revision.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
*/

#ifdef ILRFDTRACE
GLOBALREF FILE	*Fp_debug;
#endif

GLOBALREF QUEUE	M_list;

VOID pointerize();

static STATUS	fetch_ILframe();
static VOID	swap_frame();
static IR_F_CTL	*list_new();

/*{
** Name:	IIORfgFrmGet() -	get code for a frame
**
** Description:
**	Makes the code associated with a specified frame the current code.
**
** Inputs:
**	irblk	ilrf client
**	fid	desired frame
**	db_message produce message if reading from DB
**
** Outputs:
**	info	filled in FRMINFO structure.
**
**	return:
**		OK		info valid
**		ILE_CLIENT	not a legal client
**		ILE_FIDBAD	not a legal fid
**		ILE_NOMEM	cannot allocate frame, current frame recovered
**		ILE_FRDWR	can't read frame - i/o or db failure
**		ILE_CFRAME	can't allocate frame, or recover current
**
** Side Effects:
**	changes current frame, may deallocate memory / write temp. file
**
** History:
**	8/86 (rlm)	written
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		No longer pointerize or push a data area onto the stack;
**		that's now purely the client's responsibility.
**		Added new parm db_message: IIORfgFrmGet now serves for
**		the old IIORfnFrmNoptrz and recover_frame.
**		Fill in new fields in FRMINFO.
*/

STATUS
IIORfgFrmGet( irblk, fid, info, db_message )
IRBLK *irblk;
FID *fid;
FRMINFO *info;
bool db_message;
{
	IR_F_CTL	*fm;		/* pointer to fetched frame     */
	STATUS		rc;		/* return code from fetch_ILframe  */
	IR_F_CTL	*fid_fetch();
	i4		tfpos;		/* temp file position */

	/* see that we have a legal client */
	CHECKMAGIC (irblk,ERx("fg"));

#ifdef ILRFTIMING
	IIAMtprintf(ERx("Client %d, request frame %s"),irblk->irbid,fid->name);
#endif

	/*
	** Fetch the frame whose id is fid->id into fm.
	** For IORI_CORE frames, we'll always get our frame from this call.
	*/
	fm = fid_fetch(fid->id, irblk);

	/*
	** fm may be a current frame only if it is OUR current frame.
	** otherwise we need a new copy.  If it is our current frame
	** this check keeps us from winding up with 2 copies on the
	** chain for a recursive frame (we'd only get 2 anyway, no
	** matter how many calls it made - 1 current, 1 non-current)
	*/
	if (fm != NULL && fm->is_current &&
				fm != (IR_F_CTL *)(irblk->curframe))
		fm = NULL;

	if (fm != NULL)
	{
		/* if in memory already, point to new frame, return */
		if (fm->in_mem)
		{
			swap_frame(irblk,fm);
			rc = OK;
			goto ret;
		}

		/*
		** Frame not in memory - recover from temp file
		** (i.e. pretend Temporary file is an image file).
		** Since we don't want to re-cache when we're done, we
		** get a new list node, and set "don't file cache".
		*/
		tfpos = fm->d.f.pos;
		list_unlink(fm);

		if ((fm = list_new(irblk, fid)) == NULL)
		{
			rc = ILE_NOMEM;
			goto ret;
		}
			
		fm->tmp_cache = FALSE;

		rc = fetch_ILframe(irblk, fm, IORI_IMAGE, (OLIMHDR *)NULL,
			tfpos, FALSE);
		if (rc == OK)
			goto ret;

		list_unlink(fm);
		
		/* if out of memory then just return.  on other error,
		   try to read from .img or database */
		if (rc == ILE_NOMEM)
			goto ret;
	}

	/*
	** We have no record of frame, so it is not cached in memory
	** or in Temp file - get it from image file or DB.  We need a
	** new list node.  Image file frames not cachable, DB frames are.
	*/
	if ((fm = list_new(irblk, fid)) == NULL)
	{
		rc = ILE_NOMEM;
		goto ret;
	}
	if (irblk->irtype == IORI_IMAGE)
		fm->tmp_cache = FALSE;
	else
		fm->tmp_cache = TRUE;

	/* now get the frame via method indicated in irblk */
	rc = fetch_ILframe(irblk, fm, irblk->irtype,
				&(irblk->image), fid->id, db_message);

	if (rc != OK)
	{
		list_unlink(fm);
	}
	
ret:
	if (rc == OK)
	{
		info->il        =   (fm->d.m.frame)->il;
		info->symtab    = &((fm->d.m.frame)->sym);
		info->align     = &((fm->d.m.frame)->fa);
		info->dbd       =   (fm->d.m.frame)->dbd;
		info->num_dbd   =   (fm->d.m.frame)->num_dbd;
		info->stacksize =   (fm->d.m.frame)->stacksize;
		info->static_data =  fm->frmem;
	}

# ifdef ILRFTIMING
	IIAMtprintf(ERx("frame %s finish, status %d"),fid->name,rc);
# endif

# ifdef ILRFDTRACE
	dump_lists();
	fflush(Fp_debug);
# endif

	/* at this point the frame has been either located
	** and loaded or a problem (such as memory allocation
	** failure) has occured. return the status.           
	*/
	return (rc);
}

/*
** local utility for fetching object into memory.
** Herein is the logic for freeing and retrying while we fail do
** memory allocation.
*/
static STATUS
fetch_ILframe(irblk, fm, type, image, pos, db_message)
IRBLK		*irblk;
IR_F_CTL	*fm;
i4		type;		/* DB / file read indicator */
OLIMHDR		*image;		/* image file to read, if applicable */
i4		pos;		/* file position / id of frame */
bool		db_message;	/* produce message if reading from DB */
{
	i2	tag;
	STATUS	rc;

# ifdef ILRFDTRACE
	fprintf(Fp_debug,ERx("fetch loop: %d, = %d(pos) / %d(id)\n"),
						type,pos,fm->fid.id);
# endif

	/* get a new frame object and storage tag */
	tag = fm->d.m.tag = FEgettag();

# ifdef ILRFDTRACE
	fprintf(Fp_debug,ERx("new frame tag: %d\n"),tag);
# endif

	/* any memory problems encountered under file_read or IIAMffFrmFetch
	** will be taken care of by the MEerrfunc recovery routine.  If
	** there still is not enough memory, then we will get error status
	** of ILE_NOMEM back here. 
	*/

	/* we should NEVER see IORI_CORE here. */
	if (type == IORI_IMAGE)
		rc = file_read(image,pos,tag,&(fm->d.m.frame));
	else if (type == IORI_DB)
	{
		if (db_message)
			IIUGmsg(ERget(F_OR0000_fetching),FALSE,1,fm->fid.name);
		rc = IIAMffFrmFetch(&(fm->d.m.frame),pos,tag);
	}

#ifdef ILRFDTRACE
	fprintf(Fp_debug,ERx("fetch loop iteration, rc = %d\n"),rc);
#endif
	if (rc == OK)
	{
		swap_frame(irblk,fm);
		return (OK);
	}

	/*
	** free the partial storage we may have built up, including
	** the frame object itself.
	*/
	FEfree(tag);
	fm->d.m.frame = NULL;	/* safety */

	return (rc);
}

/*
** local utility for swapping current frame of client.	Moves frame to
** tail of memory list, to keep list in order of least to most recently
** accessed.
*/
static VOID
swap_frame(irblk,frm)
IRBLK *irblk;
IR_F_CTL *frm;
{
	IR_F_CTL *curf;

	/*
	** unflag old current frame
	*/
	if ((curf = (IR_F_CTL *) irblk->curframe) != NULL)
		curf->is_current = FALSE;

	/* tail of M_list */
	list_tail(&M_list, frm);

	/*
	** set IR_F_CTL flags.	Make NULLIRBID if a DB frame, so
	** we can use it commonly among clients.  fids are not unique
	** for IMAGE frames, so we have to maintain the irbid's on them.
	*/
	frm->is_current = TRUE;

	if (irblk->irtype != IORI_IMAGE)
		frm->irbid = NULLIRBID;
	else
		frm->irbid = irblk->irbid;

	frm->in_mem = TRUE;

	/* set current frame pointer */
	irblk->curframe = (i4 *) frm;

# ifdef ILRFDTRACE
	fprintf (Fp_debug,ERx("NEW CURRENT FRAME\n"));
	IIAMdump(Fp_debug,ERx("IRBLK"),irblk,sizeof(IRBLK));
	IIAMdump(Fp_debug,frm->fid.name,irblk->curframe,sizeof(IR_F_CTL));
# endif
}

/*
** local utility for getting a new list node, which always goes in the
** M_list.  If we can't get a new node, we have to free something.
** return NULL for allocation failure. 
** The IR_F_CTL item returned is marked as the CURRENT so that memory
** freeing attempts will not free it.
*/
static i4  _max_in_mem = 0;	/* maximum number of frames kept in memory */

static IR_F_CTL *
list_new(irblk,fid)
IRBLK *irblk;
FID *fid;
{
	IR_F_CTL	*fm;
	IR_F_CTL	*list_add();
	i4		i = 0;
	char 		*cp;

	/* figure out how many items are in M_list list */
	for (fm = (IR_F_CTL *)M_list.q_next; fm != (IR_F_CTL *)&M_list;
			fm = (IR_F_CTL *)fm->fi_queue.q_next)
		i++;

	/* set max number of frames that will be kept in memory - this is
	** usually just the constant MAX_IN_MEM, but can be overridden by
	** an environment variable.  Keep this value in a static and set the
	** first time through.
	*/
	if (_max_in_mem == 0)
	{
		/* check unsupported "max frames in memory" env. variable */
		_max_in_mem = MAX_IN_MEM;	/* set default first */
		NMgtAt(ERx("II_4GL_FRAMES_CACHED"), &cp);
		if ( cp != NULL && *cp != EOS )
		{
			i4 val;
			if (CVan(cp, &val) == OK && val > 0)
			{
				_max_in_mem = val;
			}
		}
	}
	while (i >= _max_in_mem)
	{
		if (IIORfaFreeAllframes(irblk) != OK)
			break;
		--i;
	}

	while ((fm = list_add(&M_list)) == NULL)
	{
		if (IIORfaFreeAllframes(irblk) == OK)
			continue;
		return (NULL);
	}

	MEcopy((char *)fid, sizeof(FID), (char *)&(fm->fid));

	fm->in_mem = TRUE;

	/* make the frame as CURRENT  */
	swap_frame(irblk, fm);

	return(fm);
}

/*{
** Name:    IIORscsSetCurStatic  -- set the current frame to be static.
**
** Description:
**	takes the pointer and saves it, associating it with the current
**	frame.
**
** Inputs:
**	irblk	ilrf client
**	data	static data area from client, to be saved with the frame.  This
**		 is saved, and will be passed back on all subsequent
**		 invocations of the frame.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	4/89 (billc)	written
*/

IIORscsSetCurStatic(irblk, data)
IRBLK *irblk;
PTR data;
{
	IR_F_CTL *frmctl;

	frmctl = (IR_F_CTL *)irblk->curframe;
	frmctl->frmem = data;
}
