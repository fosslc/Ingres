/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<er.h>
#include	<duerr.h>

#include	<di.h>
#include	<dub.h>

#include	<cs.h>
#include	<dm.h>
#include	<lo.h>
#include	<st.h>

/**
**
**  Name: DUBCRDB.C -	CREATEDB support routines that are owned
**			by the DBMS group.
**
**  Description:
**        The routines in the module are owned by the DBMS group.
**	They are used exclusively by the database utility, createdb.
**      These routines are used primarily to copy database system
**	catalog templates.
**
**          dub_copy_file() -	copy a DBMS file from one location to another.
**
**
**  History:    $Log-for RCS$
**      17-Oct-86 (ericj)
**          Initial creation.
[@history_template@]...
**	4-aug-93 (ed)
**	    unnest dbms.h
**	30-Jan-2001 (hanje04)
**	    Added #include st.h to resolve STcopy
**/

/*
[@forward_type_references@]
[@forward_function_references@]
*/


/*
**  Defines of other constants.
*/

/*
**      Define I/O constants.
*/

#define                 MAX_PAGE_IO     16

/*
[@group_of_defined_constants@]...
*/

/*
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/


/*{
** Name: dub_copy_file() -	copy a DBMS file from one location to another.
**
** Description:
**        This routine takes as input the descriptions of source and
**	destination files.  It copies the source file to the destination
**	file.  If the destination file already exists, a new one will
**	be created.
**
** Inputs:
**      from_path			DUB_PDESC which describes the path
**					to copy the file from.
**	    .dub_pname			Buffer containing pathname to copy file
**					from.  This string is null-terminated.
**	    .dub_plength		Length of the path description.
**	from_file			DUB_FDESC which describes the file
**					to be copied.
**	    .dub_fname			Buffer containing filename to be
**					copied.  This string is null-terminated.
**	    .dub_flength		Length of the filename.
**	to_path				Same as from_path, but this describes
**					the destination path.
**	to_file				Same as from_file, but this describes
**					the destination file.
**	du_errcb			Error-handling control block.
**
** Outputs:
**      *du_errcb                       If an error occurs, this block is set
**					by a call to du_error().
**	Returns:
**	    E_DU_OK			Completed successfully.
**	    E_DU3150_BAD_FILE_CREATE	Couldn't create the destination file.
**	    E_DU3151_BAD_FILE_OPEN	Couldn't open either the source or
**					the destination file.
**	    E_DU3152_BAD_SENSE		Couldn't sense the source file.
**	    E_DU3153_BAD_READ		Bad read of the source file.
**	    E_DU3154_BAD_ALLOC		Couldn't allocate page(s) for the
**					destination file.
**	    E_DU3155_BAD_WRITE		Bad page write to destination file.
**	    E_DU3156_BAD_FLUSH		Bad flush of destination file's header.
**	    E_DU3157_BAD_CLOSE		Couldn't close either the source
**					or the destination file.
**	Exceptions:
**	    none
**
** Side Effects:
**	      This routine creates a new file and copies the source file
**	    to this newly created destination file.  Two files are opened
**	    and closed in the process.  Some disk space will be consumed.
**
** History:
**      17-Oct-86 (ericj)
**          Initial creation.
**	09-Jun-95 (emmag)
**	    NT porting changes.  
[@history_template@]...
*/

DU_STATUS
dub_copy_file(from_path, from_file, to_path, to_file, du_errcb)
DUB_PDESC	*from_path;
DUB_FDESC	*from_file;
DUB_PDESC	*to_path;
DUB_FDESC	*to_file;
DU_ERROR	*du_errcb;
{

    DI_IO	ffrom;
    DI_IO	fto;
    char	pbuf[DM_PG_SIZE * MAX_PAGE_IO];
    char	from_fspec[DI_PATH_MAX + DI_FILENAME_MAX + 1];
    char	to_fspec[DI_PATH_MAX + DI_FILENAME_MAX + 1];
    i4	page;
    i4	last_pg;
    i4	io_pcnt;
    i4	alloc_page;
    VOID	dub_mk_fspec();

    /* Make string descriptions of the from file_spec and the to file_spec
    ** in case an error is encountered by one of the DI calls.
    */
    dub_mk_fspec(from_path->dub_pname, from_file->dub_fname, from_fspec);
    dub_mk_fspec(to_path->dub_pname, to_file->dub_fname, to_fspec);

    for(;;)
    {
	if (du_errcb->du_clerr =
		DIcreate(&fto, to_path->dub_pname, (u_i4) to_path->dub_plength,
			 to_file->dub_fname, (u_i4) to_file->dub_flength,
			 (i4) DM_PG_SIZE, &du_errcb->du_clsyserr))
	    return(du_error(du_errcb, E_DU3150_BAD_FILE_CREATE, 2,
			    0, to_fspec));

	if (du_errcb->du_clerr =
	    DIopen(&fto, to_path->dub_pname, (u_i4) to_path->dub_plength,
		   to_file->dub_fname, (u_i4) to_file->dub_flength,
# ifdef NT_GENERIC
		   (i4) DM_PG_SIZE, DI_IO_WRITE, DI_SYNC_MASK, 
# else
		   (i4) DM_PG_SIZE, DI_IO_WRITE, 0, 
# endif
		   &du_errcb->du_clsyserr))
	    return(du_error(du_errcb, E_DU3151_BAD_FILE_OPEN, 2,
			    0, to_fspec));

	if (du_errcb->du_clerr =
		DIopen(&ffrom, from_path->dub_pname,
		       (u_i4) from_path->dub_plength,
		       from_file->dub_fname,
		       (u_i4) from_file->dub_flength,
		       (i4) DM_PG_SIZE, DI_IO_READ, 
		       0, &du_errcb->du_clsyserr))
	    return(du_error(du_errcb, E_DU3151_BAD_FILE_OPEN, 2,
			    0, from_fspec));

	if (du_errcb->du_clerr = DIsense(&ffrom, &last_pg, &du_errcb->du_clsyserr))
	    return(du_error(du_errcb, E_DU3152_BAD_SENSE, 2,
		   0, from_fspec));
	
	io_pcnt	= 1;
	page	= 0;
	while (page <= last_pg &&
		(du_errcb->du_clerr = DIread(&ffrom, &io_pcnt, page,
					     pbuf, &du_errcb->du_clsyserr)
		) == OK
	      )
	{

	if (du_errcb->du_clerr =
		DIalloc(&fto, (i4) 1, &alloc_page, &du_errcb->du_clsyserr)
	   )
		return(du_error(du_errcb, E_DU3154_BAD_ALLOC, 2,
				0, to_fspec));

	    if (du_errcb->du_clerr = DIwrite(&fto, (i4 *) &io_pcnt,
					     alloc_page, pbuf, &du_errcb->du_clsyserr)
	       )
		return(du_error(du_errcb, E_DU3155_BAD_WRITE, 2,
				0, to_fspec));

	    if (du_errcb->du_clerr = DIflush(&fto, &du_errcb->du_clsyserr))
		return(du_error(du_errcb, E_DU3156_BAD_FLUSH, 2,
				0, to_fspec));

	    page    +=	io_pcnt;
	}

	if (du_errcb->du_clerr)
	    return(du_error(du_errcb, E_DU3153_BAD_READ, 2,
			    0, from_fspec));

	if (du_errcb->du_clerr = DIclose(&fto, &du_errcb->du_clsyserr))
	    return(du_error(du_errcb, E_DU3157_BAD_CLOSE, 2,
			    0, to_fspec));

	if (du_errcb->du_clerr = DIclose(&ffrom, &du_errcb->du_clsyserr))
	    return(du_error(du_errcb, E_DU3157_BAD_CLOSE, 2,
			    0, from_fspec));

	break;

    }	    /* end of for(;;) stmt */
    
    return(E_DU_OK);
}

/*
[@function_definition@]...
*/

VOID
dub_mk_fspec(pathname, filename, filespec)
char		*pathname;
char		*filename;
char		*filespec;
{
    LOCATION	    fspec_loc;

    /* Copy the pathname to the filespec buffer so the pathname is not
    ** altered as a side-effect.
    */
    STcopy(pathname, filespec);

    LOfroms(PATH, filespec, &fspec_loc);
    LOfstfile(filename, &fspec_loc);
    return;
}

