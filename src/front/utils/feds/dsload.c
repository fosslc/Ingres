/*
** Copyright (c) 2004 Ingres Corporation
** All rights reserved.
*/

# include	<compat.h>
# include	<st.h>		 
# include	<me.h>
# include	<si.h>
# include	<lo.h>
# include	<ds.h>
# include	<ex.h>
# include	<feds.h>
# include	"dsmapout.h"

/**
** Name:	dsload.c -- Relocate address independent data
**
**	DSload relocates address independent data in the shared data region
**	to the real addresses. 
**	When DSload is called, sh_desc->sh_type indicates the type of the
**	shared data region; If the shared data region is a file and the file
**	needs to be opened (as is the case on VMS), the caller has to initialize
**	sh_desc->sh_reg.file.fp to NULL to indicate that the shared data file
**	needs to be opened.  On those systems which allows sharing of files
**	across process boundaries and if the shared data region is a file and
**	the file has been opened, the called should provide
**	sh_desc->sh_reg.file.fp with the file pointer of the opened file.
**	Sh_desc->sh_keep indicates the shared data area needs to be kept after
**	the relocation.
**	Upon termination, *mem_reg will have the pointer to the beginning
**	address of a contiguous memory for shared data references.
**	DSload returns OK, FAIL or ME_GONE (memory allocation failure).
**	An error message will be printed on the stderr.
**
**	As of 6.4/04, the data structure may be in a different format, in which
**	a "segment map" is included, which indicates where the data region 
**	can be divided into separate memory segments. This is only valuable on
**	platforms which have a hard time allocating a single large chunk
**	of memory, e.g. on 16-bit machines that can only allocate 64K
**	in a single MEreqmem() call.  This is also only supported for RACC
**	type shared data regions.  See the module dsstore.c for more information
**	on format differences for the segmented case.
**	
** History:
**	85/02/13  09:38:13  ron
**	Initial version of new DS routines from the CFE project.
**	24-aug-87 (boba) -- Changed memory allocation to use MEreqmem.
**	11-aug-88 (mmm)
**	    Added EXdelete() call in the if (EXdeclare()) case, so
**	    that any longjumps back to this routine will unwind the
**	    stack correctly.
**	07/22/89 (dkh) - Integrated changes from IBM.
**	03/24/91 (dkh) - Integrated changes from PC group.
**	17-aug-91 (leighb) DeskTop Porting Change: remove 3/24 PC changes.
**	10/13/92 (dkh) - Dropped the use of ArrayOf in favor of DSTARRAY.
**			 The former confuses the alpha C compiler.
**	23-apr-93 (davel)
**		Added support for loading segmented data regions from RACC
**		files.  This fixes bug 51236, where DSload would fail to load 
**		data regions larger than 64K (the largest amount of memory 
**		that can be allocated in a single MEreqmem on the PC).
**	17-may-93 (davel)
**		Fix previous change - eliminate extraneous second read of the
**		size for non-segmented regions. (Bug 51792)
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	19-jun-95 (fanra01)
**	    Desktop platforms require that we open binary files with SIfopen
**	    instead of SIopen.
**	15-jan-1996 (toumi01; from 1.1 axp_osf port)
**	    (schte01) added 640502 change 06-apr-93 (kchin)
**	    Added a couple of changes to fix formindex problem due to pointer
**	    being truncated when porting to 64-bit platform, axp_osf.
**	    Changed type of offset (argument of addr_from_off() from i4 to
**	    long), this is required as i4 is not sufficient to hold a long
**	    value on 64-bit platform.
**	21-feb-1997 (walro03)
**		Reapply OpIng changes: linke01's of 31-oct-96, bonro01's of
**		09-dec-96:
**		Cast values in a proper type for ris_us5 and rs4_us5 to avoid 
**		compile error.
**	21-jan-1998 (fucch01)
**	    Added sgi_us5 to if def'd constructs for walro03's cast changes above.
**      21-mar-2001 (zhahu02)
**          Added TRdsiplay("") for rs4_us5 before calling IIFSinsert() to
**          get around from an optimized code problem (b92235).
**          It appears unnecessary, but needed for the opcode to really work. 
**      03-jun-1999 (hanch04)
**          Change SIfseek to use OFFSET_TYPE and LARGEFILE64 for files > 2gig
**      02-Aug-1999 (hweho01)
**          Cast values in a proper type for ris_u64 to avoid compile errors.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	31-jul-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix OPTIMER change for beta xlc_r - FIXME !!!
**	03-nov-2001 (somsa01)
**	    Due to problems with frames on Solaris, #ifdef'd the WIN64
**	    changes.
**	03-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

GLOBALREF i4		IIFStabsize;
GLOBALREF char		*IIFSbase;
GLOBALREF FILE		*IIFSfp;
GLOBALREF DSHASH	**IIFShashtab;
GLOBALREF DSTEMPLATE	**IIFStemptab;	/* template table */
GLOBALREF STATUS	IIFSretstat;

FUNC_EXTERN	DSTEMPLATE	*IIFStplget();
FUNC_EXTERN	STATUS		IIFSexhandl(EX_ARGS *);
FUNC_EXTERN	PTR		IIFSfindoff(PTR);
FUNC_EXTERN	VOID		IIFSinsert();
FUNC_EXTERN	VOID		IIFSndinit();
FUNC_EXTERN	i4		IIFSfsize(LOCATION *);

static bool	segmented_regions_enabled;
static i4	max_segments;
static i4	num_segments;
static i4	*segment_offsets;
static PTR	*segment_ptrs;

static	reloc();
static	reloc_ary();
static	PTR addr_from_off();

STATUS
DSload(sh_desc, ptr, DStab)
SH_DESC	*sh_desc;
PTR	**ptr;
DSTARRAY	*DStab;
{
	i4		off;
	i4		size;
	i4		t;
	i4		count;
	char		locbuf[MAX_LOC + 1];
	LOCATION	loc;
	i2		tag;
	DSHASH		*hashtab[HASHTAB_SZ];

	EX_CONTEXT	context;

	if (EXdeclare(IIFSexhandl, &context))
	{
		MEtfree(DSMEMTAG);
		EXdelete();
		return(IIFSretstat);
	}

	IIFStemptab = DStab->array;
	IIFStabsize = DStab->size;

	/* "allocate" the hash table on the stack */
	MEfill ((u_i2)(HASHTAB_SZ*sizeof(DSHASH *)), (u_char) 0, hashtab);
	IIFShashtab = (DSHASH **) hashtab;

	IIFSndinit();

	segmented_regions_enabled = FALSE;	/* usually the case */
	size = sh_desc->size;
	switch (sh_desc->sh_type)
	{
	  case IS_FILE:
		/*
		** On VMS, the file must have been closed by the caller.
		** Therefore, re-open the file for reading.
		*/
		if (sh_desc->sh_reg.file.fp == NULL)	/* file is closed */
		{
			/*
			** Convert the filename to a location and open it.
			*/

			STcopy(sh_desc->sh_reg.file.name, locbuf);
			LOfroms((LOCTYPE)FILENAME, locbuf, &loc);

# if !defined(MSDOS) && !defined(DESKTOP)
			if (SIopen(&loc, "r", &IIFSfp) != OK)
# else
			if (SIfopen(&loc, "r", SI_BIN, 0, &IIFSfp) != OK)
# endif /* MSDOS */
				EXsignal(OPENERR, 0, 0);

			/*
			** Indicate that the file is opened.
			*/

			sh_desc->sh_reg.file.fp = IIFSfp;
		}
		else
		{
			/* Error condition on VMS - signal it */

			EXsignal(OPENERR, 0, 0);
		}

		/*
		** On some systems, notably VMS, LOsize does not return an
		** accurate size.  It DOES return a value >= the real size.
		** So we use the returned count from SIread to tell us how
		** many bytes were really in the file.
		*/
		size = IIFSfsize(&loc);
		IIFSbase = (char *) MEreqmem(0, size, FALSE, (STATUS *)NULL);
		SIread(IIFSfp, (i4) size, &count, IIFSbase);
		size = sh_desc->size = count - 2*sizeof(i4);
		off = *((i4 *)(IIFSbase + size));
		t = *((i4 *)(IIFSbase + size + sizeof(i4)));

		if (sh_desc->sh_keep != KEEP)
		{
			LOdelete(&loc);
		}
		else
		{
			SIclose(IIFSfp);
		}
		break;

	  case IS_RACC:
		/*
		** RACC files must already be opened and positioned
		*/
		if (sh_desc->sh_reg.file.fp == NULL)
			EXsignal(OPENERR, 0, 0);

		IIFSfp = sh_desc->sh_reg.file.fp;

		if ((tag = sh_desc->sh_tag) <= 0)
			tag = 0;

		/* read the first i4 - if it's zero, then the region is
		** segmented.  Otherwise, the i4 represents the
		** size of the entire region, including the first i4,
		** the region itself, and the trailing 2 i4's.
		*/
		SIread(IIFSfp, sizeof(i4), &count, (char *) &size);
		if (size == 0)
		{
		    i4  i;
		    OFFSET_TYPE data_pos, seek_amt;

		    segmented_regions_enabled = TRUE;
		    SIread(IIFSfp, sizeof(i4), &count, (char *) &size);

		    /* move past the data portion and the 2 trailing i4's
		    ** (the offset and ds ID) to the position where the
		    ** segment map is.  Then we'll seek back to the data
		    ** position.
		    */
		    data_pos = SIftell(IIFSfp);
		    seek_amt = size + 2*sizeof(i4);
		    (VOID) SIfseek(IIFSfp, seek_amt, SI_P_CURRENT);

		    SIread(IIFSfp, sizeof(i4), &count, (char *) &max_segments);

		    segment_offsets = (i4 *) MEreqmem(DSMEMTAG, 
						max_segments*sizeof(i4), 
						FALSE, (STATUS *)NULL);
		    segment_ptrs = (PTR *) MEreqmem(DSMEMTAG, 
						max_segments*sizeof(PTR), 
						FALSE, (STATUS *)NULL);

		    /* what we actually are reading into segment_offsets is the
		    ** size (in bytes) of each segment.  The sizes are 
		    ** transformed into offsets a little further down in the
		    ** code...
		    */
		    SIread(IIFSfp, (max_segments*sizeof(i4)), &count, 
				(PTR)segment_offsets);

		    /* seek back to location of the data */
		    (VOID) SIfseek(IIFSfp, data_pos, SI_P_START);

		    for (num_segments = 0; num_segments < max_segments; 
			 num_segments++)
		    {
			if (segment_offsets[num_segments] <= 0)
			{
			    break;
			}
			segment_ptrs[num_segments] = 
					MEreqmem(tag, 
						segment_offsets[num_segments],
						FALSE, (STATUS *)NULL);
			SIread(IIFSfp, (i4)segment_offsets[num_segments], 
				&count, segment_ptrs[num_segments]);
		    }
		    SIread(IIFSfp, sizeof(i4), &count, (char *)&off);
		    SIread(IIFSfp, sizeof(i4), &count, (char *)&t);

		    /* now that we've read it all, seek back to the end of 
		    ** the region - just need to seek past the segment
		    ** map, which is an i4 followed by "max_segments"
		    ** worth of i4's.
		    */
		    seek_amt = (max_segments+1)*sizeof(i4);
		    (VOID) SIfseek(IIFSfp, seek_amt, SI_P_CURRENT);

		    /* convert the sizes into the maximum offset that should
		    ** be referenced in that segment.
		    */
		    segment_offsets[0] += 1;
		    for (i = 1; i < num_segments; i++)
		    {
			segment_offsets[i] += segment_offsets[i-1];
		    }
		}
		else
		{
		    /* size is leading i4 in file */
		    IIFSbase = (char *) MEreqmem(tag, (size - 3*sizeof(i4)),
						FALSE, (STATUS *)NULL);

		    SIread(IIFSfp, (i4)(size-3*sizeof(i4)), &count, IIFSbase);
		    SIread(IIFSfp, sizeof(i4), &count, (char *)&off);
		    SIread(IIFSfp, sizeof(i4), &count, (char *)&t);
		}
		break;

	  case IS_MEM:
		IIFSbase = (char *) *ptr;
/*
		MEcopy((PTR) ((char *)IIFSbase+size-2*sizeof(i4)),
			(u_i2) sizeof(i4), (PTR) &off);
		MEcopy((PTR) ((char *)IIFSbase+size-sizeof(i4)),
			(u_i2) sizeof(i4), (PTR) &t);
*/
		MEcopy(((char *)IIFSbase+size-2*sizeof(i4)), sizeof(i4), &off);
		MEcopy(((char *)IIFSbase+size-sizeof(i4)), sizeof(i4), &t);
		break;
	}

	/*
	** ptr will point to the beginning of the contiguous memory
	** for shared data reference
	*/
#if defined(i64_win) || defined(a64_win)
	*(OFFSET_TYPE **)ptr = (OFFSET_TYPE *)addr_from_off(off);
#else
	*ptr = (long *)addr_from_off(off);
#endif /* i64_win a64_win */

	reloc(*ptr, t);
	MEtfree(DSMEMTAG);
	EXdelete();
	return(OK);
}


/*
** reloc is the routine actually doing the work of relocating the address
** independent pointers to the real addresses.  It calls itself to add the
** offset in the pointer/union fields to the base address of contiguous
** memory
**	History:
**	17-aug-91 (leighb) DeskTop Porting Change: j should be i4 (not i2)!
**	27-aug-91 (leighb) DeskTop Porting Change:
**		Fixed case DSARRAY to work on systems with structures sizes
**		other than modulo 4.  Introduced 'ap' as a char ptr to point
**		to where to put next element.  Incr it by length of element
**		at end of for-loop and put result in i4 ptr 'a'.  The old
**		"a += elesize/(sizeof(*a));" led to trucating struct sizes
**		to modulo-4 and overwritting the last 1-3 characters.
*/
static
reloc(ss, ti)
PTR	ss;
i4	ti;
{
	PTR		*ptr;
	i4		i;
	DSPTRS		*p;
	DSTEMPLATE	*t;
# if defined(i64_win) || defined(a64_win)
	OFFSET_TYPE	save_off;
# else
	long		save_off;
# endif /* i64_win a64_win */

	/* loop to process each pointer/union field */

	if (ti < 0)
		ti = -ti;

	t = IIFStplget(ti);

	for (p = t->ds_ptrs, i = t->ds_cnt; i > 0; i--, p++)
	{
		/* skip structure size changers */
		if (p->ds_kind == DSVSIZE || p->ds_kind == DS2VSIZE)
			continue;

		ptr = DSget(ss, p);

		/*
		** if a null pointer, continue to process next
		** pointer/union field
		*/
		if (*ptr == NULL)
			continue;

		/*
		** if ds_kind is union then no need to call reloc and
		** no need to add the base to the offset
		*/
		if (p->ds_kind != DSUNION)
		{
# if defined(i64_win) || defined(a64_win)
			if (save_off = (OFFSET_TYPE)IIFSfindoff((PTR)*ptr))
# elif defined(any_aix) || defined(sgi_us5)
			if (save_off = (long)IIFSfindoff((PTR)*ptr))
# else
			if (save_off = IIFSfindoff((PTR)*ptr))
# endif

			if (save_off = (OFFSET_TYPE)IIFSfindoff((PTR)*ptr))
			{
				/*
				** save_off already in proper format,
				** so no casts on return values from
				** IIFSfindoff.
				*/

# if defined(any_aix) || defined(sgi_us5) || \
     defined(i64_win) || defined(a64_win)
				*ptr = (char *)save_off;
# else
				*ptr = save_off;
# endif
				continue;
			}
			/* add the base address to the offset */
# if defined(i64_win) || defined(a64_win)
			save_off = (OFFSET_TYPE)*ptr;
# elif defined(any_aix) || defined(sgi_us5)
			save_off = (long)*ptr;
# else
			save_off = *ptr;
# endif
			if (p->ds_kind == DSSTRP)
				*((char **)ptr) = (char *)addr_from_off(save_off);
			else
# if defined(i64_win) || defined(a64_win)
				*(OFFSET_TYPE **)ptr = (OFFSET_TYPE *)addr_from_off(save_off);
# else
				*(long **)ptr = (long *)addr_from_off(save_off);
# endif /* i64_win a64_win */

# if defined(any_aix)
                        TRdisplay("");
# endif
			IIFSinsert((PTR)save_off, *ptr);
		}

		switch ((i4)p->ds_kind)
		{
		  case DSNORMAL:
			reloc((PTR)DSaddrof(ss, p), p->ds_temp);
			break;

		  case DSSTRP:
			break;

		  case DSPTRARY:
		  {
			PTR		*a;
			i4		size;
			i4		j;
# if defined(i64_win) || defined(a64_win)
			OFFSET_TYPE	offset;
# else
			long		offset;
# endif /* i64_win a64_win */

			size = p->ds_sizeoff;
			for (j = 0, a = ptr; j < size; j++, a++)
			{
				if (*a != NULL)
				{
					if (j != 0)
					{
# if defined(i64_win) || defined(a64_win)
						if (offset = (OFFSET_TYPE)IIFSfindoff((PTR)*a))
# elif defined(any_aix) || defined(sgi_us5)
						if (offset = (long)IIFSfindoff((PTR)*a))
# else
						if (offset = IIFSfindoff((PTR)*a))
# endif
						{
# if defined(any_aix) || defined(sgi_us5) || \
     defined(i64_win) || defined(a64_win)
							*a = (char *)offset;
# else
							*a = offset;
# endif
							continue;
						}
# if defined(i64_win) || defined(a64_win)
						offset = (OFFSET_TYPE)*a;
# elif defined(any_aix) || defined(sgi_us5)
						offset = (long)*a;
# else
						offset = *a;
# endif
						/*
						** no need to check for object
						** type!?
						*/
# if defined(i64_win) || defined(a64_win)
						*(OFFSET_TYPE **)a =
							(OFFSET_TYPE *)addr_from_off(offset);	
# else
						*(long **)a =
							(long *)addr_from_off(offset);	
# endif /* i64_win a64_win */
						IIFSinsert((PTR)offset, *a);
					}
					reloc_ary((PTR)*a, p->ds_temp);
				}
			}
			break;
		  }

		  case DSARRAY:
		  {
			i4		dsid;
			PTR		*a;
			i4		elesize;
			i4		size;
			i4		j;
# if defined(i64_win) || defined(a64_win)
			OFFSET_TYPE	offset;
# else
			long		offset;
# endif /* i64_win a64_win */
			char		*ap;		 

			/* get the number of array elements */
			size = DSsizeof(ss, p);

			/* get the size of the array elements */
			dsid = (p->ds_temp < 0) ? -p->ds_temp : p->ds_temp;
			elesize = (p->ds_temp < 0) ? sizeof(PTR) : (IIFStplget(dsid)->ds_size);
			if ((p->ds_temp < 0) || (IIFStplget(dsid)->ds_cnt != 0))
			{
				a = DSaddrof(ss, p);
				for (j = 0; j < size; j++)			 
				{
					if (p->ds_temp < 0)
					{
						/* array of pointers */
						if (*a != NULL)
						{
# if defined(i64_win) || defined(a64_win)
							if (offset = (OFFSET_TYPE)IIFSfindoff((PTR)*a))
# elif defined(any_aix) || defined(sgi_us5)
							if (offset = (long)IIFSfindoff((PTR)*a))
# else
							if (offset = IIFSfindoff((PTR)*a))
# endif
							{
# if defined(any_aix) || defined(sgi_us5) || \
     defined(i64_win) || defined(a64_win)
							    *a = (char *)offset;
# else
							    *a = offset;
# endif
							    continue;
							}
# if defined(i64_win) || defined(a64_win)
							offset = (OFFSET_TYPE)*a;
							*(OFFSET_TYPE **)a = (OFFSET_TYPE *)addr_from_off(offset);	
# elif defined(any_aix) || defined(sgi_us5)
							offset = (long)*a;
							*(long **)a = (long *)addr_from_off(offset);	
# else
							offset = *a;
							*(long **)a = (long *)addr_from_off(offset);	
# endif
							IIFSinsert((PTR)offset, *a);
# if defined(i64_win) || defined(a64_win)
							reloc_ary((PTR)*(OFFSET_TYPE **)a,
# else
							reloc_ary((PTR)*(long **)a,
# endif
								dsid);
						}
					}
					else
					{
						/* array of structs */
						reloc_ary((PTR)a, -dsid);
					}
					ap = (char *)a;				 
					ap += elesize;				 
# if defined(i64_win) || defined(a64_win)
					a  = (OFFSET_TYPE *)ap;
# else
					a  = (long *)ap;
# endif /* i64_win a64_win */
				}
			}
			break;
		  }

		  case DSUNION:
			/* if the member of the union has no pointer
			** type defined, then no need to call reloc
			*/
			if (IIFStplget((i4)DSsizeof(ss, p))->ds_cnt != 0)
				reloc((PTR)ptr, (i4)DSsizeof(ss, p));
			break;
		}
	}
}

static
reloc_ary(a, dsid)
PTR	a;
i4	dsid;
{
	if (dsid != DS_STRING)
		reloc(a, dsid);
}

/* convert offsets to the actually data address.  Prior to the addition of
** segmented data region support, this used to be simply "IIFSbase + offset - 1"
** and appeared that way in the above code in several instances.
*/
static PTR
addr_from_off(offset)
long offset;
{
    PTR ptr = NULL;

    if (!segmented_regions_enabled)
    {
	/* the offsets are 1-based */
	ptr = (PTR) (IIFSbase + offset - 1);
    }
    else
    {
        i4  segno;

	/* find which ptr to use in the segment_ptrs array */
	for (segno=0; segno < num_segments; segno++)
	{
	    if (offset < segment_offsets[segno])
	    {
		ptr = (PTR) ((char *)segment_ptrs[segno] 
				+ (offset - 
				   (segno == 0 ? 1 : segment_offsets[segno-1])
				  ));
		break;
	    }
	}
	/* The loop should not terminate without setting the ptr; if it does
	** (due to an unexpected "offset out of range" problem), return a NULL 
	** ptr, enough though that likely will make the callers 
	** seg/access violate.
	*/
    }
    return ptr;
}
