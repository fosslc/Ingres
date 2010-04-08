/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
#include "irstd.h"
#include "irmagic.h"

/**
** Name:	irmem.c - ilrf memory free
**
** Description:
**	free memory held by ilrf
*/

#ifdef ILRFDTRACE
extern FILE *Fp_debug;
#endif

extern QUEUE M_list, F_list;

static VOID free_frame();

/*{
** Name:	IIORmfMemFree - ilrf memory free
**
** Description:
**	Releases memory held by ILRF module for use by other modules.
**	Caller may request ILRF to free a single, unspecified frame
**	which is not being used, all unused frames, or a specific frame.
**	The latter may be used when the caller knows that a given frame
**	will not be wanted in the near future.	The caller may delete
**	his current frame with IORM_FRAME, in which case he will no
**	longer have a current frame.  With IORM_FORM, DB clients may
**	also supress the copying of the frame to the temporary file.
**	This can be used if the client knows the frame will not be wanted
**	again during his session.
**
** Inputs:
**	irblk	ilrf client
**	level	what of free:
**			IORM_SINGLE	1 unspecified frame
**			IORM_ALL	all freeable frames
**			IORM_FRAME	user specified frame
**	fid	frame (significant for IORM_FRAME only)
**	tc	temp cache frame (significant for IORM_FRAME, DB clients only)
**
** Outputs:
**
**	return:
**		OK		memory has been freed
**		ILE_CLIENT	not a legal client
**		ILE_NOMEM	nothing to free
**		ILE_FIDBAD	requested frame not in memory / not releasable
**		ILE_ARGBAD	bad level specified.
**
** Side Effects:
**	writes temporary file
**
** History:
**	8/86 (rlm)	written
**	2/87 (rlm)	added tc flag
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

STATUS
IIORmfMemFree (irblk,level,fid,tc)
IRBLK *irblk;
i4  level;
FID *fid;
bool tc;
{
	IR_F_CTL	*frm;
	IR_F_CTL	*tmp;
	IR_F_CTL	*fid_fetch();
	i4		fcount;
	STATUS		retval;

# ifdef PCINGRES
	char		*MEerrfunc();
	char		*saveptr;
# endif

	/* see that we have a legal client */
	CHECKMAGIC(irblk,ERx("mf"));

# ifdef ILRFDTRACE
	SIfprintf(Fp_debug, ERx("memory free level %d\n"), level);
	dump_lists();
# endif

# ifdef PCINGRES
	/* cannot have memory free function active here since might
	   try to write out a frame while we are in the process of
	   writing this one out */
	saveptr = MEerrfunc(NULLPTR);
# endif

	switch (level)
	{
	case IORM_FRAME:
		frm = fid_fetch(fid->id, irblk);

		if (frm == NULL || ! frm->in_mem)
		{
			retval = ILE_FIDBAD;
			goto ret;
		}

		/* let client free own current frame, but "un-current" first */
		if (irblk->curframe == (i4 *)frm)
		{
			irblk->curframe = NULL;
			frm->is_current = FALSE;
			if (irblk->irtype != IORI_IMAGE)
				frm->irbid = NULLIRBID;
		}

		/* if temp-cachable, let user possibly override */
		if (frm->tmp_cache)
			frm->tmp_cache = tc ? TRUE : FALSE;	/* bit field */

		free_frame(frm);
		break;

	case IORM_SINGLE:
	case IORM_ALL:
		/*
		** list is in order of access, least to most recent.
		** free_frame ALWAYS unlinks list element.  That is why pred
		** must stay the same when we free a frame
		*/
		fcount = 0;
		for (frm = (IR_F_CTL *)M_list.q_next;
			frm != (IR_F_CTL *)&M_list; frm = tmp)
		{
			tmp = (IR_F_CTL *)frm->fi_queue.q_next;

			/*
			** if not the current frame, and doesn't have static 
			** date, let's free it.
			*/
			if (!frm->is_current && frm->frmem == NULL)
			{
				++fcount;
				free_frame(frm);
				if (level == IORM_SINGLE)
					break;
			}
		}
		if (fcount == 0)
		{
			retval = ILE_NOMEM;
			goto ret;
		}
		break;

	default:
		retval = ILE_ARGBAD;
		goto ret;
	}

	retval = OK;

    ret:

# ifdef PCINGRES
	/* reset the error routine */
	MEerrfunc(saveptr);
# endif

	return (retval);
}

/*
** IIOR internal call which frees memory and unlinks list for a given client.
** used when closing a client session.
**
** this routine is called for id = NULLIRBID only on closing the last DB
** session, making it safe to blow away all "common" frames.
*/
VOID
free_client(id)
i4	id;
{
	IR_F_CTL	*frm;
	IR_F_CTL	*tmp;

	/*
	** free_frame ALWAYS unlinks list element.  That is why pred
	** must stay the same when we free a frame
	*/
	for (frm = (IR_F_CTL *)M_list.q_next; frm != (IR_F_CTL *)&M_list;
		frm = tmp)
	{
		tmp = (IR_F_CTL *)frm->fi_queue.q_next;
		if (frm->irbid == id)
		{
			frm->tmp_cache = FALSE; /* force unlink */
			free_frame(frm);
		}
	}
}

/*
** local utility to free frame.
**
** Handles the caching of frame out to temp file if cachable, and the
** removal of the frame from the memory list, adding it to the file
** list if applicable.
**
** Because we ALWAYS remove the frame from the in-memory chain, it is
** valid to use this routine in a loop without modifying the predecessor
** for the next list element.
*/
static VOID
free_frame(frm)
IR_F_CTL *frm;
{
	i2	tag;
	i4	pos;

	tag = frm->d.m.tag;	/* union - we overwrite */

#ifdef ILRFDTRACE
	SIfprintf(Fp_debug, ERx("freeing %s, tag %d, cache flag %d\n"),
		frm->fid.name, tag, frm->tmp_cache);
# endif

	if (!frm->tmp_cache)
		list_unlink(frm);
	else
	{
		/* frm is currently on M_list.  This will put it on the tail
		   of F_list */
		list_tail(&F_list, frm);

		if (temp_write(frm->d.m.frame, &pos) == OK)
		{
			frm->d.f.pos = pos;
			frm->in_mem = FALSE;
		}
		else
			list_unlink(frm);
	}

	FEfree(tag);

# ifdef ILRFDTRACE
	SIfprintf(Fp_debug,ERx("freed %d\n"),tag);
	dump_lists();
# endif
}
