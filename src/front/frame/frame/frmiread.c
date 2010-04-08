/*
** Copyright (c) 1988, 2005 Ingres Corporation
**
*/

#include	<compat.h>
#include	<st.h>
#include	<me.h>
#include	<lo.h>
#include	<si.h>
#include	<cm.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<encoding.h>

#include	<frmindex.h>
#include	<erm1.h>

/**
** Name:	frmiread.c -	Form Index File Module Read Routines.
**
** Description:
**	Contains the routines of the Form Index file module necessary to fetch
**	forms from the Form Index.  These routines implement some of the methods
**	for the Form Index file class.  Defines:
**
**	IIFDfiOpen()	open form index file.
**	IIFDfiClose()	close form index file.
**	IIFDfiRead()	fetch form from form index file.
**
**	Note:  These routines are in a seperate file so that the FrontEnds
**	need not link in the other methods for this class (in "frmindex.sc.")
**
** History:
**	Revision 6.1  88/04/01  wong
**	Modified to use Form Index file as class by abstracting ad-hoc code
**	from "tools/formfile.qsc", "tools/formindex.c" and "frame/fdfrcrfl.c"
**	into method interface.
**
**	Revision 5.0K  86/12/20  Kobayashi
**	Initial revision for 5.0 Kanji.
**
**	11/11/89 (dkh) - Removed old HACKFOR50 code.
**	05/17/90 (dkh) - Fixed us #956.
**	04/16/91 (dkh) - Added support to adapt to different formindex file
**			 header sizes.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
*/

/* static but shared with "frmindex.sc" */

bool	IIFDfiFind(/* FORM_INDEX fip, char *form, INDEX_ENTRY **index */);

/*{
** Name:	IIFDfiOpen() -	Open Form Index File.
**
** Description:
**	Opens a Form Index file for reading or writing, creating it if it does
**	not exist and it should be created.  The header information is then
**	read into the Form Index structure.  Finally, if the file is to be
**	written, the index table is read into the structure.
**
**	This allocates a Form Index file structure, if one is not passed in.
**
** Input:
**	frmindx	{FORM_INDEX *}  The Form Index file object (NULL if it
**				must be allocated.
**	iloc	{LOCATION *}  Location of the Form Index file.
**	create	{bool}  Create if non-existent.
**	read	{bool}  Open for reading only.
**
** Returns:
**	{FORM_INDEX *}  The opened Form Index file object;
**			otherwise, NULL on error.
**
** Errors:
**	E_M10014_CantOpen	Cannot open file.
**	E_M10013_CantCreate	Cannot create file.
**	E_M10015_BadVersion	Incompatible version.
**	E_M10016_BadIndex	Cannot read index.
**
** History:
**	04/88 (jhw) -- Written (from Kobayashi's "formindex.c".)
*/

FORM_INDEX *
IIFDfiOpen (iloc, create, read )
LOCATION	*iloc;
bool		create;
bool		read;
{
#define MAXSEARCH 113		/* Hard-coded index size prior to 6.01 */
#define	BLKSIZE 512

	register FORM_INDEX	*fip;

	i4	iocnt;
	char	*cp;
	u_i4	hdrsize;

	/* assert:  create ==> !read */

	hdrsize = sizeof(*fip);

	if ((fip = (FORM_INDEX *)MEreqmem(0, hdrsize,
		TRUE, (STATUS *)NULL)) == NULL)
	{
		return NULL;
	}

	LOtos( iloc, &cp );
	STcopy( cp, fip->fname );

	fip->allocated = (bool) TRUE;

	if ( LOexist(iloc) != OK )
	{ /* file does not exist */
		if ( !create )
		{
			IIUGerr( E_M10014_CantOpen, 0, 1, (PTR)fip->fname );
			if ( fip->allocated )
				MEfree( fip );
			return NULL;
		}

		/* Create file */
		if ( SIfopen(iloc, ERx("w"), SI_RACC, BLKSIZE, &fip->fp) != OK )
		{
			IIUGerr(E_M10013_CantCreate, 0, 1, (PTR)fip->fname );
			if ( fip->allocated )
				MEfree( fip );
			return NULL;
		}

		FEmsg( ERget(S_M10011_Creating), FALSE, fip->fname );

		MEfill( sizeof(fip->index), EOS, &fip->index );	/* clear index table */

		/* set version */
		MEcopy( (PTR)INDEX_VERSION, sizeof(fip->index.header.version),
				(PTR)fip->index.header.version
		);

		fip->index.header.indexsize = MAXINDEX;		/* set maximum size */

		/* write empty index table */
		SIwrite( (i4)sizeof(fip->index), (PTR)&fip->index, &iocnt, fip->fp );

		fip->fhdrsize = sizeof(FORM_HDR);
		fip->lastindex = fip->index.itable - 1;
		fip->create = TRUE;
		fip->write = FALSE;
	}
	else
	{ /* file exists */
		register i4	vers;

		if ( SIfopen(iloc, read ? ERx("r") : ERx("rw"), SI_RACC, BLKSIZE,
					&fip->fp) != OK )
		{
			IIUGerr( E_M10014_CantOpen, 0, 1, (PTR)fip->fname );
			if ( fip->allocated )
				MEfree( fip );
			return NULL;
		}

		if ( !read )
		{
			FEmsg( ERget(S_M10012_Opening), FALSE, (PTR)fip->fname );
		}

		/* Read the index header (with the version and size.) */
		SIfseek( fip->fp, (i4)0, SI_P_START );
		SIread( fip->fp, (i4)sizeof(fip->index.header),
					&iocnt, (PTR)&fip->index.header
		);

		/* Check version.  If prior to 6.01, then it will NOT
		** have the size of the index, so we must use MAXSEARCH
		*/
#define	VERSIZE sizeof(fip->index.header.version)

		if ( (vers = STbcompare(fip->index.header.version, VERSIZE,
						ERx("5.0"), 0, FALSE)) >= 0 &&
				(vers = STbcompare(fip->index.header.version, VERSIZE,
						ERx("6.01"), 0, FALSE)) < 0 )
		{ /* use MAXSEARCH */
			fip->index.header.indexsize = MAXSEARCH;
			SIfseek( fip->fp, (i4)VERSIZE, SI_P_START );
			fip->fhdrsize = FE_MAXNAME + sizeof(i4);
		}
		else if ( vers == 0 )
		{ /* 6.01 */
			fip->index.header.indexsize = MAXINDEX;
			fip->fhdrsize = FE_MAXNAME + sizeof(i4);
		}
		else if ( vers > 0 &&
					(vers = STbcompare(fip->index.header.version, VERSIZE,
							INDEX_VERSION, 0, FALSE)) <= 0 )
		{ /* >= 6.1 */
			fip->fhdrsize = sizeof(FORM_HDR);
		}
		else
		{ /* bad version */
			char	version[VERSIZE+1];

			_VOID_ STlcopy( fip->index.header.version, version, VERSIZE );
			IIUGerr( E_M10015_BadVersion, 0, 3, (PTR)version,
						(PTR)fip->fname, (PTR)INDEX_VERSION
			);
			SIclose( fip->fp );
			if ( fip->allocated )
				MEfree( fip );
			return NULL;
			/*NOTREACHED*/
		}

		/* read index table from file */
		{
			i4 size;

			size = sizeof(fip->index.itable[0]) * fip->index.header.indexsize;
			/*
			**  Hmm, we are trying to read a file that has more
			**  entries in itable than we currently understand.
			**  Must have been created by code that is newer
			**  than us.  We should just allocated a new chunk
			**  that is big enough and get rid of the old chunk.
			**
			**  Basic assumption here is that the only thing
			**  that changes over time is the number of elements
			**  in the itable array.  If this is not true,
			**  we are in deep trouble.
			**
			**  We also assume that formindex files with a smaller
			**  itable size should just drop in with no problem.
			*/
			if (size > sizeof(fip->index.itable))
			{
				i4		noninxsize;
				i4		newsize;
				FORM_INDEX	*nfip;

				/*
				**  Need to calculate a new overall
				**  FORM_INDEX struct size.
				**  We don't worry about an filler
				**  space at end of the INDEX struct.
				**
				**  First, get the overall size that
				**  we currently know about.
				*/
				noninxsize = sizeof(*fip);

				/*
				**  Next, throw away our current INDEX
				**  struct size since it is too small.
				**  We need a new one.
				*/
				noninxsize -= sizeof(fip->index);

				/*
				**  Next, put back the INDEX_HDR struct
				**  size since that's not changing.
				*/
				noninxsize += sizeof(INDEX_HDR);

				/*
				**  All that is left, is to add in
				**  the new size of itable that is
				**  stored in the "size" variable.
				**
				**  We're not going to worry about
				**  the filler at the end since it
				**  never gets filled in so we don't
				**  need to allocated space for it.
				**  If this assumption is wrong, adjust
				**  the values accordingly.
				*/
				newsize = noninxsize + size;

				/*
				**  Allocate new header.
				*/
				if ((nfip = (FORM_INDEX *) MEreqmem(0, newsize,
					TRUE, (STATUS *)NULL)) == NULL)
				{
					if (fip->allocated)
					{
						MEfree(fip);
					}
					return(NULL);
				}

				/*
				**  Move info that exists in the old
				**  index header into the new one.
				*/
				nfip->allocated = TRUE;
				nfip->fp = fip->fp;
				nfip->fhdrsize = fip->fhdrsize;
				STcopy(fip->fname, nfip->fname);
				MEcopy((PTR) &(fip->index.header),
					(u_i2) sizeof(INDEX_HDR),
					(PTR) &(nfip->index.header));

				/*
				**  Free the old index header.
				*/
				if (fip->allocated)
				{
					MEfree(fip);
				}

				/*
				**  Point to new index header.
				*/
				fip = nfip;
			}

			if ( SIread(fip->fp, size, &iocnt, (PTR)fip->index.itable) != OK ||
					iocnt != size )
			{
				IIUGerr( E_M10016_BadIndex, 0, 1, (PTR)fip->fname );
				SIclose( fip->fp );
				if ( fip->allocated )
					MEfree( fip );
				return NULL;
			}
		}

		fip->lastindex = &(fip->index.itable[fip->index.header.indexsize]);

		if ( fip->fhdrsize == sizeof(FORM_HDR) ) 
		{ /* >= 6.1 */
			register INDEX_ENTRY	*ip = fip->lastindex;
			register INDEX_ENTRY	*start = fip->index.itable;

			for ( --ip ; ip >= start ; --ip)
				if ( *ip->frmname != EOS )
					break;
			fip->lastindex = ip;
		}

		fip->create = fip->write = FALSE;
	}

	return fip;
}

/*{
** Name:	IIFDfiClose() -	Close Form Index File.
**
** Description:
**	Closes the Form Index file, writing the index table out if it was
**	modified.  Also, delete the file if it was created but never modified.
**
** Input:
**	fip	{FORM_INDEX *}  The Form Index file.
**
** History:
**	04/88 (jhw) -- Written (from Kobayashi's "formindex.c".)
*/
VOID
IIFDfiClose ( fip )
register FORM_INDEX	*fip;
{
	if ( fip->write )
	{ /* index was changed */
		i4	wcnt;

		/*
		** Rewrite new index table.  First, seek to the start of the index
		** table at the beginning of the file (the header won't change.)
		*/

		SIfseek ( fip->fp, (i4)0, SI_P_START );
		SIwrite( (i4)sizeof(fip->index), (PTR)&fip->index, &wcnt, fip->fp );
	}

	_VOID_ SIclose( fip->fp );	/* close index file */

	if ( fip->create && !fip->write )
	{ /* created, but not written ! */
		LOCATION	loc;

		LOfroms( PATH&FILENAME, fip->fname, &loc );
		LOdelete( &loc );
	}

	if ( fip->allocated )
		MEfree( fip );	/* free allocated Form Index structure */
	else
		fip->fp = NULL;
}

/*{
** Name:	IIFDfiRead() -	Fetch a Form From Form Index File.
**
** Description:
**	Searchs the index table of the Form Index for the input formname and
**	then reads the encoded form into memory.  The index table is searched
**	by being read in from the file and searched in buffered chunks.
**
** Inputs:
**	fip		{FORM_INDEX *}  The Form Index file.
**	form	{char *}  The name of the form to be fetched.
**
** Output:
**	frmsize	{i4 *}  Size of encoded form fetched from the Form Index
**				file returned by reference.
**
** Returns:
**	{PTR}  Address of frame for fetched form.
**		Otherwise, NULL on error.
**
** Errors:
**	E_M10016_BadIndex	Cannot read index.
**	E_M10030_BadForm	Cannot read encoded form (/form header.)
**
** Side Effects:
**	Allocates memory for the fetched form.
**
** History:
**	20-Jan-1987 (Kobayashi) Create new for 5.0K.
**	13-jul-87 (bab) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	10-sep-1987 (peter)
**		Change index page to contain size parameter as well for version "6.01".
**	04/88 (jhw) -- Abstracted from old 'IIFDrffReadFormFile()' (now 
**					'IIFDfrcrfle()') in "frame/fdfrcrfl.c."
*/
PTR
IIFDfiRead ( fip, form, frmsize )
register FORM_INDEX	*fip;
char				*form;
i4				*frmsize;
{
	INDEX_ENTRY	*ip;
	PTR			block;		/* block of memory for frame */
	i4		count;		/* read return count */

	FORM_HDR	fhdr;

	if ( form == NULL || *form == EOS )
		return NULL;

	if ( !IIFDfiFind( fip, form, &ip ) )
		return NULL;	/* not found */

	/* Read encoded form. */

	SIfseek( fip->fp, ip->offset, SI_P_START );
	if ( ( fip->fhdrsize == sizeof(fhdr)
				? ( SIread( fip->fp, (i4)sizeof(fhdr), &count, (PTR)&fhdr )
						!= OK || count != sizeof(fhdr) )
				: ( SIread( fip->fp, (i4)sizeof(fhdr.frmname), &count,
							(PTR)fhdr.frmname ) != OK || count != FE_MAXNAME ||
					SIread( fip->fp, (i4)sizeof(fhdr.totsize), &count,
							(PTR)&fhdr.totsize ) != OK ||
						count != sizeof(i4) ) ) ||
			STbcompare( fhdr.frmname, 0, ip->frmname, 0,
								TRUE ) != 0
		)
	{
		IIUGerr( E_M10030_BadForm, 0, 2, (PTR)form, (PTR)fip->fname );
		return NULL;
	}

	/*
	** allocate space for the FRAME structure
	*/
	if ( (block = MEreqmem(0, (u_i4)fhdr.totsize, TRUE, (STATUS *)NULL))
			== NULL )
	{
		IIUGbmaBadMemoryAllocation( ERx("IIFDfiRead") );
	}

	if ( count == sizeof(fhdr) /* <== version >= 6.1 */)
	{ /* encoded form stored as single block */
		if ( SIread( fip->fp, fhdr.totsize, &count, block ) != OK ||
				count != fhdr.totsize )
		{
			IIUGerr( E_M10030_BadForm, 0, 2, (PTR)form, (PTR)fip->fname );
			MEfree( block );
			return NULL;
		}
	}
	else
	{ /* versions 6.01 and earlier ... */
		register char		*addr = block;
		register i4	totsize = fhdr.totsize;

		do
		{
			i4	bsize;		/* binary size of one row */
			if (SIread(fip->fp, (i4)sizeof(bsize), &count, (PTR)&bsize) ||
						count != sizeof(bsize) ||
					SIread(fip->fp, bsize, &count, addr) != OK ||
						count != bsize )
			{
				IIUGerr( E_M10030_BadForm, 0, 2, (PTR)form, fip->fname );
				MEfree( block );
				return NULL;
			}
			addr += bsize;
		} while ( (totsize -= count) > 0 );
	}

	*frmsize = fhdr.totsize;

	return block;
}

/*
** Name:	IIFDfiFind() -	Return Index Entry for Form (or Empty Slot.)
**
** Description:
**	Find the index entry for the input form and return it.  If the form is
**	not found, return the index entry where the form should be inserted if
**	binary, or the next empty index entry otherwise.
**
** Input:
**	fip		{FORM_INDEX *}  The Form Index file.
**	name	{char *) The form for which to look.
**
** Output:
**	index	{INDEX_ENTRY **}  Index entry form form returned by reference.
**
** Returns:
**	{bool}  TRUE, if found, otherwise, FALSE.
**
** History:
**	11-sep-1987 (peter) -- Written.
**	04/88 (jhw) -- Modified to return reference to index entry.
*/

bool
IIFDfiFind ( fip, name, index )
register FORM_INDEX	*fip;
char				*name;
INDEX_ENTRY			**index;
{
	register INDEX_ENTRY	*ip;
	register INDEX_ENTRY	*end;
	register INDEX_ENTRY	*start;

	start = fip->index.itable;
	end = fip->lastindex;

	/* Search for index entry for form name */

	if ( end == &(fip->index.itable[fip->index.header.indexsize]) )
	{ /* not sorted (version < 6.1) */
		for (ip = start ; ip < end ; ++ip)
		{
			if ( *ip->frmname != EOS && CMcmpnocase(name, ip->frmname) == 0 &&
					STbcompare( name, 0, ip->frmname, 0,
								TRUE ) == 0 )
			{ /* matched */
				*index = ip;
				return TRUE;
			}
		}

		/* assert: ( ip >= end )
		**
		** Form not found.  Check for empty slot.
		*/
		for (ip = start ; ip < end ; ++ip)
		{
			if ( *ip->frmname == EOS )
			{
				break; 
			}
		}
		*index = ip;
		return FALSE;
	}
	else
	{ /* Binary search */
		while ( start <= end )
		{
			register i4	dir;

			ip = start + (end - start) / 2;
			if ( (dir = CMcmpnocase(name, ip->frmname)) == 0 &&
					(dir = STbcompare(name, 0, ip->frmname, 0,
										TRUE)) == 0 )
			{
				*index = ip;
				return TRUE;
			}
			else if ( dir < 0 )
				end = ip - 1;
			else
				start = ip + 1;
		}

		/* assert: ( start > end )
		**
		** Form not found.  Return slot for insertion (if available.)
		*/
		*index = ( fip->lastindex + 1 <
							&(fip->index.itable[fip->index.header.indexsize]) )
					? start : (fip->lastindex + 1);
		return FALSE;
	}
	/*NOTREACHED*/
}
