/*
**  Copyright (c) 1985, 2005 Ingres Corporation
**
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<me.h>
# include	<ds.h>
# include	<si.h>
# include	<lo.h>
# include	<nm.h>
# include	<ex.h>
# include	<feds.h>
# include	"dsmapout.h"

/**
** Name:	dsstore.c -	stores relocatable data structure
**
**	DSstore stores and constructs a relocatable version of data structure
**	in the shared data region.  Address dependent data will be stored as
**	offsets from the beginning of the shared data region by utilizing the
**	information in the template table.
**	When DSstore is called, sh_desc->sh_type indicates the type of the
**	shared data region; If the shared data region is a file and the file
**	needs to be opened (as is the case on VMS), the caller must initialize
**	sh_desc->sh_reg.file.fp to NULL to indicate that the shared data file
**	needs to be opened.  It is possible to share files across process
**	boundaries on some systems.  In those cases, if the shared data region
**	is a file and the file has been opened, the caller should provide
**	sh_desc->sh_reg.file.fp with the file pointer of the opened file.
**	At the end of DSstore, ds_id will be written to the shared data region
**	in order for DSload to know what kind of data structure is going to be
**	read.  Upon termination, sh_desc will contain proper descriptions(e.g.,
**	size, file descriptor, file name, etc) of the shared data region.
**	DSstore returns OK or FAIL. An error message will be printed on the
**	error output.
**
**	As of 6.4/04, the data structure can be written out along with a
**	"segment map", which indicates where the data region can be
**	be divided into separate memory segments. This is only valuable on
**	platforms which have a hard time allocating a single large chunk
**	of memory, e.g. on 16-bit machines that can only allocate 64K
**	in a single MEreqmem() call.  This is also only supported for RACC
**	type shared data regions.  The format of the data structure in
**	this segmented case is:
**
**	 i4    i4     data size bytes      i4          i4   i4      numseg*i4
**	+---------------------------------------------------------------------+
**	|   | data |                     | offset    | DS | # of  | array of  |
**	| 0 | size |  entire data region | to top    | id | seg-  | segment   |
**	|   |      |                     | of struct |    | ments | sizes     |
**	+---------------------------------------------------------------------+
**
**	The leading zero in the above structure allows it to be compatible with
**	the non-segmented format which is:
**
**	   i4     data size bytes      i4          i4
**	+---------------------------------------------+
**	| data |                     | offset    | DS |
**	| size |  entire data region | to top    | id |
**	| +3   |                     | of struct |    |
**	+---------------------------------------------+
**
** History:
**	Revision 3.0  85/02/13  09:39:46  ron
**	Initial version of new DS routines from the CFE project.
**
**	Revision 5.1  87/04/14  13:57:37  arthur
**	Changes made to DS module in PC project.  These changes include:
**
**	allocate blocks of nodes to avoid ME overhead
**	fix up hash table size.
**	IS_RACC changes, destructive store capability, avoid alloc by placing
**	hash table on the stack
**	changed to tagged memory allocations.
** 
**	Revision 6.0 87/08/05   08:30:01  hgc
**	Removal of Mealloc() for MEreqmem().
**	20-aug-87 (boba) -- Changed memory allocation to use MEreqmem.
**
**	Revision 6.1  11-aug-88  mikem
**	Added missing EXdelete() calls for all returns from this function.
**
**	Revision 6.4
**	03/24/91 (dkh) - Integrated changes from PC group.
**	04/06/91 (dkh) - Fixed bug 36806 which came from w4gl version
**			 of code.  Check routine map_out() for explanation.
**	07/01/91 (emerson)
**		Fix for bug 38333 in write_out.
**
**	08/19/91 (dkh) - Integrated porting changes from PC group.
**	10/13/92 (dkh) - Dropped the use of ArrayOf in favor of DSTARRAY.
**			 The former confuses the alpha C compiler.
**	21-apr-93 (davel)
**		Add support for writing the data region out with header
**		information that lists segment sizes that DSload can use
**		to allocate separate chunks of memory.  This is needed to fix
**		bug 51236, where DSload would fail to load data regions larger
**		than 64K (the largest amount of memory that can be allocated
**		in a single MEreqmem on the PC).
**	15-jan-1996 (toumi01; from 1.1 axp_osf port)
**		(schte01) merged 640502 change 05-jul-93 (kchin)
**		Added a couple of changes to fix formindex problem due to
**		pointer being truncated when porting to 64-bit platform,
**		axp_osf.
**		Also, changed dkh's cast of pointer from i4 to long.
**	21-feb-1997 (walro03)
**		Reapply OpIng changes: linke01's of 31-oct-96, bonro01's of
**		09-dec-96:
**		Cast values in a proper type for ris_us5 and rs4_us5 to avoid
**		compile error.
**	21-jan-1998 (fucch01)
**	    Added sgi_us5 to if def'd constructs added by walro03 above.
**      03-jun-1999 (hanch04)
**          Change SIfseek to use OFFSET_TYPE and LARGEFILE64 for files > 2gig
**	30-jul-1999 (hanch04)
**	    Use an i4 when writing out the relocatable version of data
**	    structures.  Writing out an OFFSET_TYPE may be 8 bytes.  These
**	    files should not grow > 2 gig anyway.
**      02-Aug-1999 (hweho01)
**          Cast values in a proper type for ris_u64 to avoid compile error.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      02-Feb-2001 (fanra01, hanch04)
**          Bug 103864
**          Adjust the SIftell comparison introduced by the move to OS
**          functions.
**	31-jul-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
**	29-oct-2001 (somsa01)
**	    Modified 64-bit changes, as first pass caused 'formindex' to
**	    hang when it got to ingcntrl on UNIX platforms.
**	03-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

GLOBALREF DSTEMPLATE	**IIFStemptab;	/* template table */
GLOBALREF i4		IIFStabsize;
GLOBALREF FILE		*IIFSfp;
GLOBALREF DSHASH	**IIFShashtab;

FUNC_EXTERN	STATUS		IIFSexhandl(EX_ARGS *);
FUNC_EXTERN	DSTEMPLATE	*IIFStplget();
FUNC_EXTERN	PTR		IIFSfindoff(PTR);
FUNC_EXTERN	VOID		IIFSinsert();
FUNC_EXTERN	VOID		IIFSndinit();

static bool	Destructive;

# if defined(i64_win) || defined(a64_win)
static OFFSET_TYPE	cur_off;	/* cur pos in the shared data region */
# else
static i4	cur_off;	/* cur pos in the shared data region */
# endif /* i64_win a64_win */

# define	INITIAL_MAX_SEGMENTS	8
static bool	segmented_regions_allowed;
static bool	segment_overflow = FALSE;
static i4	max_segment_size;
static i4	max_segments = INITIAL_MAX_SEGMENTS;
static i4	num_segments;
static i4	*segments;

static i4	map_out();
static i4	write_ary(PTR, i4);
static i4	write_out();

# define	SUFX	"ds"

STATUS
DSstore(sh_desc, ds_id, ds, DStab)
SH_DESC	*sh_desc;
i4	ds_id;
PTR	ds;
DSTARRAY	*DStab;
{
	i4		root;
	OFFSET_TYPE	osize, fsize;
	i4		osize_i4;
	i4		dummycnt;	/* Used in SIwrite */
	EX_CONTEXT	context;
	DSHASH		*hashtab[HASHTAB_SZ];

	if (EXdeclare(IIFSexhandl, &context))
	{
		MEtfree(DSMEMTAG);
		EXdelete();
		return(FAIL);
	}
	IIFStemptab = DStab->array;
	IIFStabsize = DStab->size;

	/* "allocate" the hash table on the stack */
	MEfill ((u_i2)(HASHTAB_SZ*sizeof(DSHASH *)), (u_char) 0, hashtab);
	IIFShashtab = (DSHASH **) hashtab;

	IIFSndinit();

	/*
	** backwards compatability: destructive store only applies to IS_RACC
	*/
	Destructive = (sh_desc->sh_type == IS_RACC)
			? sh_desc->sh_dstore : FALSE;

	/* segmented regions are only needed on some platforms, and only
	** for RACC files.
	*/
#ifdef PMFE
	segmented_regions_allowed = (sh_desc->sh_type == IS_RACC) 
						? TRUE : FALSE;
	max_segment_size = 0xFFE8;
# else
	segmented_regions_allowed = FALSE;

# endif /* PMFE */

	if (sh_desc->sh_type == IS_FILE || sh_desc->sh_type == IS_RACC)
	{
		if (sh_desc->sh_reg.file.fp == NULL)
		{ /* first time DSstore called */
			char		*fname;
			LOCATION	tloc;
			LOCATION	loc;
			char		locbuf[MAX_LOC + 1];

			/*  Open the file read/write. */
			if ( NMloc(TEMP, PATH, (char *)NULL, &tloc) != OK  ||
				(LOcopy(&tloc, locbuf, &loc),
					LOuniq("ds", SUFX, &loc)) != OK  ||
				SIfopen(&loc, "w", SI_VAR, 0, &IIFSfp) != OK )
			{
				EXsignal(OPENERR, 0, 0);
			}
			LOtos(&loc, &fname);
			sh_desc->sh_reg.file.name = STalloc(fname);

			/*
			**	Show that the file is open
			** 	by placing the file pointer in the shared
			**	data structure descriptor.
			**
			*/
			sh_desc->sh_reg.file.fp = IIFSfp;
		}
		IIFSfp = sh_desc->sh_reg.file.fp;

		/*
		** for IS_RACC files, save the current position, and write
		** out placeholder info.  This info will 
		** be backpatched after the write operation completes.
		*/
		if (sh_desc->sh_type == IS_RACC)
		{
		    if ((osize=SIftell(IIFSfp)) == -1)
		    {
			EXdelete();
			return(FAIL);
		    }
		    if (segmented_regions_allowed)
		    {
			/* allocate an array of i4's to hold the sizes of each
			** segment of the data region (actually during 
			** processing, they hold the offsets, but later we 
			** transform the offsets into sizes).  Write out
			** a leading zero (indicating that this region is
			** segmented - see DSload()) and another i4 for the
			** total size of the data region as
			** a place holder in the file.
			** Note also that it's important to allocate this
			** with zero-fill set to TRUE.
			*/
	    		segments = (i4 *) MEreqmem(DSMEMTAG, 
					max_segments*sizeof(i4), 
					TRUE, (STATUS *)NULL);
			root = 0;
			SIwrite(sizeof(i4), (char *)&root, &dummycnt, IIFSfp);
			SIwrite(sizeof(i4), (char *)&root, &dummycnt, IIFSfp);
			num_segments = 0;
			segments[0] = 1;
		    }
		    else
		    {
			/* for non-segmented regions, we only need a single
			** i4 to hold the region size.
			*/
			osize_i4 = (i4)osize;
			SIwrite(sizeof(i4), (char *)&osize_i4, &dummycnt,
				IIFSfp);
		    }
		}
		cur_off = 1;
		if (IIFSfindoff(ds))
		{
			MEtfree(DSMEMTAG);
			EXdelete();
			return(OK);
		}
		root = map_out(ds, ds_id);

		if (segment_overflow)
		{
			/* this thing musta been huge; fail it here. */
			MEtfree(DSMEMTAG);
			EXdelete();
			return(FAIL);
		}
		SIwrite(sizeof(i4), (char *)&root, &dummycnt, IIFSfp);
		SIwrite(sizeof(i4), (char *)&ds_id, &dummycnt, IIFSfp);
	}

	/*
	**	Close the file if not IS_RACC (and indicate so in sh_desc).
	**	Check using sh_desc->sh_type. For IS_RACC
	**	files we get the file size, backpatch the file with its size,
	**	and seek back to the end of the write again.
	*/
	if (sh_desc->sh_type == IS_RACC)
	{
		if ((fsize=SIftell(IIFSfp)) == -1)
		{
			EXdelete();
			return (FAIL);
		}
		if (SIfseek(IIFSfp, osize, SI_P_START) != OK)
		{
			EXdelete();
			return (FAIL);
		}
		if (segmented_regions_allowed)
		{
		    /* write the header info back out, which is a leading
		    ** zero followed by the data region size.
		    */
		    root = 0;
		    osize = cur_off - 1;	/* this is data region size */
		    SIwrite(sizeof(i4), (char *)&root, &dummycnt, IIFSfp);
		    osize_i4 = (i4)osize;
		    SIwrite(sizeof(i4), (char *)&osize_i4, &dummycnt, IIFSfp);
		}
		else
		{
		    /* calculate the region size - this size also contains
		    ** the 3 i4's (the trailing root and ds_id as well as 
		    ** this i4 itself).
		    */
		    osize = fsize - osize;
		    osize_i4 = (i4)osize;
		    SIwrite(sizeof(i4), (char *)&osize_i4, &dummycnt, IIFSfp);
		}
		if (SIfseek(IIFSfp, fsize, SI_P_START) != OK)
		{
		    EXdelete();
		    return (FAIL);
		}
		if (segmented_regions_allowed)
		{
		    i4  i;
		    /* first, transform the current "segment" offsets to
		    ** sizes.  The last segment is calculated from the final
		    ** value of "cur_off".
		    */
		    for (i = 0; i < num_segments; i++)
		    {
			segments[i] = segments[i+1] - segments[i];
		    }
		    segments[num_segments] = (i4)(cur_off -
						  segments[num_segments]);

		    num_segments++;
		    /* write the segment map out */
		    SIwrite(sizeof(i4), (char *)&num_segments, &dummycnt,
			    IIFSfp);
		    SIwrite(num_segments*sizeof(i4), (PTR)segments, 
						&dummycnt, IIFSfp);
		}
	}
	else
	{
		SIclose(IIFSfp);
		sh_desc->sh_reg.file.fp = NULL;

		/*
		** we don't call IIFSfsize to fill in this item because LOsize
		** is not accurate on VMS, and will result in the wrong number.
		*/
		sh_desc->size = (i4)(cur_off - 1 + 2*sizeof(i4));
	}

	MEtfree(DSMEMTAG);
	EXdelete();
	return(OK);
}

/*
** map_out is the routine actually doing the work of constructing a relocatable
** version of data structure.  It recursively calls itself to write each
** pointer/union structure to the shared data region(file or shared memory).
** map_out returns the position of the written data structure in the
** shared data region.
**
** History:
**	29-apr-1990 (Joe)
**	    Fixed a portability problem on VMS.  The case for DSARRAY had
**	    an i4 ** being used to index each element of the array.  This
**	    only worked is the element size of the array was an integer
**	    multiple of sizeof(i4).  I changed to char * which will
**	    work on any size array element.
**	04/06/91 (dkh) - Fixed Joe's change so that it handles array of
**			 pointers correctly.  In essence, we need to CAST
**			 "a" to be an i4 pointer before dereferencing it
**			 or we only end up with one byte of value.  This
**			 is less than desirable when dealing with pointer
**			 values.
*/
static i4
map_out(ds, ds_id)
PTR	ds;
i4	ds_id;
{
	PTR		*ptr;
	PTR		subptr;
	PTR		realaddr;
	PTR		temp;
	i4		i;
	PTR		offset;
	DSPTRS		*p;
	DSTEMPLATE	*t;
	bool		dontwrite;
	i4		d_size;


	if (ds == NULL)		/* a null data structure */
		return (0);

	dontwrite = FALSE;
	if (ds_id < 0)
	{
		dontwrite = TRUE;
		ds_id = -(ds_id + 1);
	}
	t = IIFStplget(ds_id);

	if((d_size = t->ds_size) <= 0)
	{
		d_size *= -1;
		for (p = t->ds_ptrs, i = t->ds_cnt; i > 0; i--, p++)
		{
			if (p->ds_kind == DSVSIZE || p->ds_kind == DS2VSIZE)
				break;
		}
		if (i > 0)
		{
			ptr = (PTR *) DSset(ds, p);
			if (p->ds_kind  == DSVSIZE)
				d_size += (*((i4 *) ptr)) * p->ds_sizeoff;
			else
				d_size += (*((u_i2 *) ptr)) * p->ds_sizeoff;
		}
	}

	realaddr = ds;
	if (!dontwrite && !Destructive)
	{
		temp = MEreqmem(DSMEMTAG, (i4)d_size, FALSE,
				(STATUS *) NULL);
		MEcopy((char *)ds, (i4)d_size, (char *)temp);
		ds = temp;
	}

	/* loop to process each pointer/union field */

	for (p = t->ds_ptrs, i = t->ds_cnt; i > 0; i--, p++)
	{
		if (p->ds_kind == DSVSIZE || p->ds_kind == DS2VSIZE)
			continue;
		/*
		** get the offset of the pointer from the beginning
		** of the data structure
		*/
		ptr = (PTR *)DSset(ds, p);

		if (*ptr == NULL)	/* a null pointer */
			continue;	/*
					** continue to process next
					** pointer/union field
					*/

		if (p->ds_kind != DSUNION)
		{
			if (p->ds_kind == DSSTRP)
				subptr = (PTR)DSsaddrof(ds, p);
			else
				subptr = (PTR)DSaddrof(ds, p);

			if (offset = IIFSfindoff(subptr))
			{
#if defined(i64_win) || defined(a64_win)
				*((OFFSET_TYPE *)ptr) = (OFFSET_TYPE)offset;
# else
# if defined(any_aix) || defined(sgi_us5)
				*((long *)ptr) = (long)offset;
# else
				*((long *)ptr) = offset;
# endif /* aix sgi_us5 */
# endif /* i64_win a64_win */
				continue;
			}
		}

		switch ((i4)p->ds_kind)
		{
		  case DSNORMAL:
			/*
			** map_out returns the offset of the written
			** data structure from the beginning of the shared
			** data region
			*/
# if defined(i64_win) || defined(a64_win)
			*((OFFSET_TYPE *)ptr) = map_out((PTR)DSaddrof(ds,p),
							p->ds_temp);
# else
			*((long *)ptr) = map_out((PTR)DSaddrof(ds,p),p->ds_temp);
# endif /* i64_win a64_win */
			break;

		  case DSSTRP:
			/*
			** write the string out and return the beginning
			** offset of the string.
			*/

		        /* STlength() + 1 to write the null char out */
# if defined(i64_win) || defined(a64_win)
			*((OFFSET_TYPE *)ptr) = write_out(DSsaddrof(ds, p),
# else
			*((long *)ptr) = write_out(DSsaddrof(ds, p),
# endif /* i64_win a64_win */
					 (i4) STlength(DSsaddrof(ds, p)) + 1, 
					 DSsaddrof(ds, p));
			break;

		  case DSPTRARY:
		  {
			PTR	*a;
			i4	size;
			i4	j;

			/* get the number of array elements */
			size = p->ds_sizeoff;

			for (j = 0, a = ptr ; j < size; j++, a++)
			{
			    if (j != 0)
			    {
				if (offset = IIFSfindoff(*a))
				{
# if defined(i64_win) || defined(a64_win)
				    *((OFFSET_TYPE *)a) = (OFFSET_TYPE)offset;
# else
# if defined(any_aix) || defined(sgi_us5)
				    *((long *)a) = (long) offset;
# else
				    *((long *)a) = offset;
# endif /* aix sgi_us5 */
# endif /* i64_win a64_win */
				    continue;
				}
			    }
# if defined(i64_win) || defined(a64_win)
			    *((OFFSET_TYPE *)a) = write_ary(*a, p->ds_temp);
# else
			    *((long *)a) = write_ary(*a, p->ds_temp);
# endif /* i64_win a64_win */
			}
			break;
		  }

		  case DSARRAY:
		  {
			/*
			** 29-apr-1990 (Joe)
			**   Changed type of a and addr to be char * from
			**   i4.  See history for reasons.
			*/
			i4	dsid;
			char	*a;
			i4	elesize;
			i4	size;
			i4	j;
			char	*addr;

			/* get the number of array elements */
			size = DSsizeof(ds, p);

			/* get the size of the array elements */
			dsid = (p->ds_temp < 0) ? -p->ds_temp : p->ds_temp;
			elesize = (p->ds_temp < 0) ? sizeof(PTR) : (IIFStplget(dsid)->ds_size);
			if (Destructive)
			{
				addr = (char *)DSaddrof(ds, p);
			}
			else
			{
				addr = (char *)MEreqmem(DSMEMTAG,
					(i4)(size*elesize), FALSE,
					(STATUS *)NULL);
				MEcopy((char *)DSaddrof(ds, p),
					(i4)(size*elesize), (char *)addr);
			}
			if ((p->ds_temp < 0) || (IIFStplget(dsid)->ds_cnt != 0))
			{
			    for (j=0, a=addr; j<size; j++, a+=elesize)
			    {
				if (p->ds_temp < 0)
				{
				    /* array of pointers */
				    if (offset = IIFSfindoff(
# if defined(i64_win) || defined(a64_win)
						    (PTR)*((OFFSET_TYPE *)a)))
# else
						    (PTR)*((long *)a)))
# endif /* i64_win a64_win */
				    {
# if defined(i64_win) || defined(a64_win)
					*((OFFSET_TYPE *)a) =
						    (OFFSET_TYPE)offset;
# else
# if defined(any_aix) || defined(sgi_us5)
					*((long *)a) = (long)offset;
# else
					*((long *)a) = offset;
# endif /* aix sgi_us5 */
# endif /* i64_win a64_win */
				    }
				    else
				    {
# if defined(i64_win) || defined(a64_win)
					*((OFFSET_TYPE *)a) = write_ary(
						    (PTR)*((OFFSET_TYPE *)a),
						    dsid);
# else
					*((long *)a) = write_ary(
						    (PTR)*((long *)a),
						    dsid);
# endif /* i64_win a64_win */
				    }
				}
				else
				{
				    /* array of structs */
				    write_ary((PTR)a, -dsid-1);
				}
			    }
			}


			/* then write out the array itself */
#if defined(i64_win) || defined(a64_win)
			*((OFFSET_TYPE *) ptr) = write_out((PTR)addr,
# else
			*((long *) ptr) = write_out((PTR)addr,
# endif
							  (i4) size*elesize, 
							  (PTR)DSaddrof(ds, p));
			break;
		  }

		  case DSUNION:
			/*
			** if member of the union has no pointer type,
			** then no need to call map_out
			*/
			if (IIFStplget((i4)DSsizeof(ds, p))->ds_cnt != 0)
				map_out((PTR)ptr, (i4)(-(i4)DSsizeof(ds, p)-1));
			break;

		default:
			break;
		}
	}


	/*
	** finished processing each pointer/union field, so write the parent
	** struct out.
	*/
	/*
	** return the beginning position of the written
 	** data structure in the shared data region
	*/
	if (dontwrite)
		return(0);	/* formerly return; --daveb */
	else
		return(write_out(ds, d_size, realaddr));
}

/*
** write_out
**
** History:
**	07/01/91 (emerson)
**		Align the structure being written out on the most
**		restrictive alignment (ALIGN_RESTRICT) instead of i4.
**		This corrects a problem (bug 38333) that showed up
**		on an HP port (but presumably existed for sun4's as well)
**		where ABF/4GL code that referenced a global float constant
**		would sometimes cause the ABF/4GL interpreter to crash,
**		because the global float constant sometimes got placed
**		into a location that wasn't aligned on an 8-byte boundary.
**		(The imaged application ran fine).
**
**		The fix implemented here may seem a bit of an overkill,
**		in that all structures now get the most restrictive alignment
**		whereas that may in fact be necessary only for floats,
**		so there may be some wasted space for other data types.
**		But I don't see how we can avoid this, because
**		floats can be referenced via pointers of type PTR
**		(as is done in the DB_DATA_VALUEs for global float constants);
**		DSstore can't tell that the data being pointed to by such a PTR
**		is in fact a float.
*/
static i4
write_out(ds, size, realaddr)
PTR	ds;
i4	size;
PTR	realaddr;	/* the addr used in IIFSfindoff */
{
	i4	pad;
	char	dummy_buf[sizeof(ALIGN_RESTRICT)];
	PTR	off_save;
	i4	count;

	if ((ds == NULL) || (size == 0))
	{
		return(0);
	}
	else
	{
		/* offset alignment check */
		if (pad = (i4)(((cur_off - 1) % sizeof(ALIGN_RESTRICT))))
		{
			pad = sizeof(ALIGN_RESTRICT) - pad;
			cur_off = cur_off + pad;
			SIwrite(pad, (PTR)dummy_buf, &count, IIFSfp);
		}
	
		off_save = (PTR)cur_off;
		SIwrite((i4)size, (char *) ds, &count, IIFSfp);
		cur_off += size;
		IIFSinsert(realaddr, off_save);

		/* if segmented regions are enabled, and if this last
		** SIwrite causes the current segment to exceed the segment
		** size limit, then increment current segment number after 
		** storing off_save in the segments array.
		*/
		if (segmented_regions_allowed && 
		    (cur_off - segments[num_segments]) > max_segment_size)
		{
		    if (++num_segments >= max_segments)
		    {
			i4 *oldmap = segments;

			/* we overflowed the segment map - allocate a bigger one
			** and copy the old one over; then free the old one.
			*/
			max_segments *= 4;
	    		segments = (i4 *) MEreqmem(DSMEMTAG, 
					max_segments*sizeof(i4), 
					TRUE, (STATUS *)NULL);
			if (segments == NULL)
			{
			    segment_overflow = TRUE;
			    num_segments = 0;	/* keep it from corrupting */
			    segments = oldmap;
			}
			else
			{
			    MEcopy( (PTR)oldmap, 
				    (u_i2)(num_segments*sizeof(i4)),
				    (PTR)segments);
			}
		    }
		    segments[num_segments] = (i4)off_save;
		}
		return((i4)off_save);
	}
}


static i4
write_ary(a, ds_id)
PTR	a;
i4	ds_id;
{
	i4	retval;

	if ((ds_id == DS_CHAR) || (ds_id == DS_STRING))
	{
		retval = write_out(a, (i4) STlength(a) + 1, a);
	}
	else
	{
		/* call map_out to write out each data structure pointed */
		retval = map_out(a, ds_id);
	}
	return(retval);
}
