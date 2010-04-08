/*
** Copyright (c) 1995, 2000 Ingres Corporation, Inc.
** All Rights Reserved
*/

#include <compat.h>
#include <lo.h>
#include <st.h>
#include <tr.h>
#include <ut.h>
#include <descrip.h>
#include <stsdef.h>


/*
**
**  Name: DLADTREG.C - 	- Dynamic loading of registry symbols from II_USERADT.
**
**  Description:
**
**	This file contains the routines to dynamically load symbols from
**	II_USERADT.  This makes it no longer necessary to hard link against
**	the shared image.
**
**	    IIudadt_register()
**	    IIclsadt_register()
**
**
**  History:
**	31-jul-95 (albany)
**	    Created.
**	30-nov-95 (dougb) bug 727??
**	    Correct TRdisplay() calls.  Don't want to display random amounts
**	    of memory!
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
[@history_template@]...
*/

/*
**  Forward and/or External function references.
*/
FUNC_EXTERN STATUS
 LOnoconceal( char *input, char *expand, char *filename, i4 *vms_stat );

FUNC_EXTERN i4 UTfind_file_symbol( struct dsc$descriptor_s fn_dsc,
					  struct dsc$descriptor_s sn_dsc,
					  i4 (**symbol)(),
					  struct dsc$descriptor_s in_dsc);

static STATUS DLget_adt_path( VOID );


/*
** Definition of all global variables used by this file.
*/
#define INIT_STATIC_DESCRIPTOR( dsc, buffer ) \
				dsc.dsc$w_length = STlength( buffer ); \
				dsc.dsc$b_dtype = DSC$K_DTYPE_T;       \
				dsc.dsc$b_class = DSC$K_CLASS_S;       \
				dsc.dsc$a_pointer = buffer;

/* buffers for UTfind_image_symbol() */
static char			fn_buffer[ MAX_LOC + 1 ];
static char			in_buffer[ MAX_LOC + 1 ];


/*{
** Name: IIudadt_register	- Dynamic loading of registry symbol
**
** Description:
**	This routine dynamically finds the IIudadt_register entry point in
**	II_USERADT, making a hard link against II_USERADT no longer necessary.
**	It does this by first getting the physical location of the installed
**	shared image, then uses LIB$FIND_IMAGE_SYMBOL (via UTfind_image_symbol)
**	to get the entry point.  If everything is successful up to this point,
**	the function returned is called with the provided parameters.
**
**	Failures are logged (if TRset_file has been set) and returned.
**
** Inputs:
**	add_block			The definition structure to be filled
**					out by the called function.
**	callbacks			A list of callback functions supplied
**					to the called function.
** Outputs:
**	*add_block			The definition structure filled out
**					by the called function.
**	Returns:
** 	    STATUS
**	Exceptions:
** 	    none
**
** Side Effects:
**	Provides dynamic loading of the registry functions within II_USERADT.
**	It is no longer necessary to hard link against II_USERADT.
**
** History:
**	31-jul-95 (albany)
**	    Created.
**	30-nov-95 (dougb) bug 727??
**	    Correct TRdisplay() calls.  Don't want to display random amounts
**	    of memory!
[@history_template@]...
*/

STATUS IIudadt_register( void **add_block, void *callbacks )
{
    /* status variables */
    i4				vms_stat;
    STATUS			status = OK;

    /* shared image function to call & associated boolean */
    static i4			(*ud_func)();
    static bool			seen_ud = FALSE;

    /* descriptors & buffer for UTfind_image_symbol() */
    struct dsc$descriptor_s	fn_dsc;
    struct dsc$descriptor_s	sym_dsc;
    struct dsc$descriptor_s	in_dsc;
    char			sym_buffer[ MAX_LOC + 1 ];


    if ( !seen_ud ) {
	/* make sure we have the path to II_USERADT */
	status = DLget_adt_path();

	if ( OK == status ) {
	    /* Set up the symbol name & descriptors */
	    STcopy( "IIudadt_register", sym_buffer );
	    INIT_STATIC_DESCRIPTOR( fn_dsc, fn_buffer );
	    INIT_STATIC_DESCRIPTOR( sym_dsc, sym_buffer );
	    INIT_STATIC_DESCRIPTOR( in_dsc, in_buffer );

	    /* and try to find the symbol in the shared image */
	    vms_stat = UTfind_image_symbol( &fn_dsc, &sym_dsc,
					   &ud_func, &in_dsc );

	    if ( !$VMS_STATUS_SUCCESS( vms_stat ) ) {
		status = FAIL;
		TRdisplay(
		    "UTfind_image_symbol failed to find %s.  vms_stat = %d.\n",
			  sym_buffer, vms_stat );
	    }
	    else
		seen_ud = TRUE;
	}
    }

    if ( OK == status )
	status = ud_func( add_block, callbacks );

    return( status );
}



/*{
** Name: IIclsadt_register	- Dynamic loading of registry symbol
**
** Description:
**	This routine dynamically finds the IIclsadt_register entry point in
**	II_USERADT, making a hard link against II_USERADT no longer necessary.
**	It does this by first getting the physical location of the installed
**	shared image, then uses LIB$FIND_IMAGE_SYMBOL (via UTfind_image_symbol)
**	to get the entry point.  If everything is successful up to this point,
**	the function returned is called with the provided parameters.
**
**	Failures are logged (if TRset_file has been set) and returned.
**
** Inputs:
**	add_block			The definition structure to be filled
**					out by the called function.
**	callbacks			A list of callback functions supplied
**					to the called function.
** Outputs:
**	*add_block			The definition structure filled out
**					by the called function.
**	Returns:
** 	    STATUS
**	Exceptions:
** 	    none
**
** Side Effects:
**	Provides dynamic loading of the registry functions within II_USERADT.
**	It is no longer necessary to hard link against II_USERADT.
**
** History:
**	31-jul-95 (albany)
**	    Created.
**	30-nov-95 (dougb) bug 727??
**	    Correct TRdisplay() calls.  Don't want to display random amounts
**	    of memory!
[@history_template@]...
*/

STATUS IIclsadt_register( void **add_block, void *callbacks )
{
    /* status variables */
    i4				vms_stat;
    STATUS			status = OK;

    /* shared image function to call & associated boolean */
    static i4			(*cls_func)();
    static bool			seen_cls = FALSE;

    /* descriptors & buffer for UTfind_image_symbol() */
    struct dsc$descriptor_s	fn_dsc;
    struct dsc$descriptor_s	sym_dsc;
    struct dsc$descriptor_s	in_dsc;
    char			sym_buffer[ MAX_LOC + 1 ];


    if ( !seen_cls ) {
	/* make sure we have the path to II_USERADT */
	status = DLget_adt_path();

	if ( OK == status ) {
	    /* Set up the symbol name & descriptors */
	    STcopy( "IIclsadt_register", sym_buffer );
	    INIT_STATIC_DESCRIPTOR( fn_dsc, fn_buffer );
	    INIT_STATIC_DESCRIPTOR( sym_dsc, sym_buffer );
	    INIT_STATIC_DESCRIPTOR( in_dsc, in_buffer );

	    /* and try to find the symbol in the shared image */
	    vms_stat = UTfind_image_symbol( &fn_dsc, &sym_dsc,
					    &cls_func, &in_dsc );

	    if ( !$VMS_STATUS_SUCCESS( vms_stat ) ) {
		status = FAIL;
		TRdisplay(
		    "UTfind_image_symbol failed to find %s.  vms_stat = %d.\n",
			  sym_buffer, vms_stat );
	    }
	    else
		seen_cls = TRUE;
	}
    }

    if ( OK == status )
	status = cls_func( add_block, callbacks );

    return( status );
}

static STATUS DLget_adt_path( VOID )
{
    static bool 	havePath = FALSE;
    i4			vms_stat;
    STATUS		status = OK;

    if ( !havePath ) {
	/* Find the full path and filename of II_USERDADT */
	status = LOnoconceal( "II_USERADT", in_buffer, fn_buffer, &vms_stat );
	if ( OK == status )
	    havePath = TRUE;
	else {
	    if ( $VMS_STATUS_SUCCESS( vms_stat ) )
		TRdisplay( "LOnoconceal failed.  status = %d.\n", status );
	    else
		TRdisplay(
		    "LOnoconceal failed.  status = %d.  vms_stat = %d.\n",
			  status, vms_stat );
	}
    }

   return status;
}
