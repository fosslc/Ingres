/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<me.h>
#include	<lo.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>
#include	<ilrffrm.h>
#include	<ilerror.h>
#include	"il.h"
#include	"if.h"
#include	"ilgvars.h"

/**
** Name:	if.c -	4GL Interpreter Stack Frame Manager Module.
**
** Description:
**	The 4GL interpreter frame stack and control stack manager module.
**	All access to the frame and control stack must go through these
**	IIOF routines.
**
** History:
**	Revision 6.2  90/08/23  peterk
**	Modify 'IIOFpuPush()' and 'IIOFdaDataArea()' to guarantee that any
**	DB_DATA_VALUE has an aligned data area.  Otherwise the data area could
**	be unaligned if the number of DBDVs was odd.  This will cause problems
**	whenever data values require alignment.  Bug #32684.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Deleted several functions and added IIOFufPushFrame,
**		IIOFpfPopFrame, IIOFucPushControl, and IIOFpcPopControl.
**		Modified functions to reflect the fact that the new "control"
**		stack is sometimes appropriate instead of the frame stack.
**		Also modified functions to remove check for NULL stack pointer
**		(IIOframeptr == NULL or IIOcontrolptr == NULL);
**		these functions are never used when the relevant stack is empty.
**	08/21/91 (emerson)
**		Fix for bug 39361 in IIOFfpFreePrm.
**
**	Revision 6.5
**	26-jun-92 (davel)
**		minor change in IIOFucPushControl - set db_data to NULL for
**		place-hoder temps.
**	23-dec-93 (donc) Bug 56911
**		Place checks in IIOFufPushControl that allow it to be called
**		prior to a IIOFpfPushFrame (IIOframeptr is NULL).  This sit-    **		can occur if the application starts as a 3GL procedure 
**		calling either a 4GL procedure (EXEC 4GL CALLPROC) or a 4GL
**		frame (EXEC 4GL CALLFRAME). 
**      29-dec-93 (donc)
**		Altered IIOFufPushFrame so that it will allocate an initial
**		IIOframeptr if it has not be done previously. Also added 
**		another argument to indicate if the call is via an EXEC
**		4GL call. In these cases we do not want to do an
**		IIORfgFrmGet on the 3GL procedure. 
**	 6-nov-96 (kch)
**		In the function IIOFucPushControl() change to casts on calls
**		to MEregmem(). Previous cast on size parameter (u_i2) caused
**		truncation of values greater than 65535. This change fixes
**		bugs 54055 and 78218.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* round up a quantity or address to the most restrictive alignment */
#define	ALIGN_UP(x)	((x)+(sizeof(ALIGN_RESTRICT)-1)-((x)+(sizeof(ALIGN_RESTRICT)-1))%sizeof(ALIGN_RESTRICT))

VOID	IIARccdCopyCallersDbdvs();

/*{
** Name:	IIOFufPushFrame() -	Push a new entry onto frame stack
**
** Description:
**	Retrieves a new frame (or non-local procedure),
**	and pushes a new entry onto the frame stack.
**	This new frame becomes the current frame.
**
**	This routine does not switch DBMS sessions to retrieve the frame;
**	that's up to the caller.
**
** Inputs:
**	fid		{FID *}		The FID of the new frame
**					(contains frame name and fid id).
**	formname	{char *}	The name of the new frame's form
**					(NULL if this is a procedure).
**	is_static	{bool}		Is the new frame static?
**
** Returns:
**	OK if successful, else an error status.
**
** Error statuses:
**	ILE_NOMEM	Could not allocate memory for frame stack entry.
**
** Side effects:
**	Resets global IIOframeptr.
**
** History:
**	04/07/91 (emerson)
**		Created for local procedures.
*/
STATUS
IIOFufPushFrame( fid, formname, is_static, is_3glcall )
FID	*fid;
char	*formname;
bool	is_static;
bool    is_3glcall;
{
	IFFRAME	*new_frm;
	STATUS	rval;

	/*
	** Allocate the control block first.
	*/
        if ( !IIOframeptr ) { 
	    IIOframeptr = (IFFRAME *)MEreqmem( 0, (u_i2)sizeof( IFFRAME ), 
				TRUE, (STATUS *)NULL );
	    new_frm = IIOframeptr;
        }
        else
	    new_frm = (IFFRAME *)MEreqmem( 0, (u_i2)sizeof( IFFRAME ), 
				TRUE, (STATUS *)NULL );

	if ( new_frm == NULL || IIOframeptr == NULL )
	{
		return ILE_NOMEM;
	}

	/*
	** Retrieve the frame.
	*/
        if (!is_3glcall ) {
	    rval = IIORfgFrmGet( &il_irblk,fid,&new_frm->frminfo,(bool)TRUE );

	    if ( rval != OK )
	    {
		MEfree( (PTR)new_frm );
		return rval;
	    }
        }
	STRUCT_ASSIGN_MACRO( *fid, new_frm->fid );
	new_frm->formname  = formname;
	new_frm->is_static = is_static;

	/*
	** Add the control block to the frame stack.
	*/
        if ( new_frm != IIOframeptr ) {
	    new_frm->nextfrm = IIOframeptr;
	    IIOframeptr = new_frm;
        }
	return OK;
}

/*{
** Name:	IIOFpfPopFrame() -	Pop an entry from frame stack
**
** Description:
**	Pops an entry from the frame stack.
**	If the frame stack is still non-empty,
**	retrievs the frame (or non-local procedure)
**	that's now on the top of the frame stack.
**	This frame becomes the current frame.
**
**	This routine does not switch DBMS sessions to retrieve the frame;
**	that's up to the caller.
**
** Returns:
**	OK if successful, else an error status.
**
** Side effects:
**	Resets global IIOframeptr.
**
** History:
**	04/07/91 (emerson)
**		Created for local procedures.
*/
STATUS
IIOFpfPopFrame( )
{
	IFFRAME	*old_frm;
	IFFRAME	*new_frm;

	new_frm = IIOframeptr;
	old_frm = IIOframeptr->nextfrm;
	IIOframeptr = old_frm;
	MEfree( (PTR)new_frm );

	if ( old_frm == NULL )
	{
		return OK;
	}

	/*
	** Retrieve the old frame (in case it got kicked out of memory).
	*/
	return IIORfgFrmGet( &il_irblk, &old_frm->fid, &old_frm->frminfo,
			     (bool)FALSE );
}

/*{
** Name:	IIOFucPushControl() -	Push an entry onto control stack
**
** Description:
**	Pushes an entry onto the control stack.
**	Also acquires the data area for the entry being pushed and
**	initializes the portion of the DBDV array associated with the data area,
**	unless the entry is for a static frame.
**
** Inputs:
**	name		{char *}	Name of routine being pushed.
**	tot_num_dbdvs	{nat}		If routine is a "main" routine:
**					total size of dbdv array for frame;
**					if routine is a local procedure: -1.
**	static_data	{PTR}		NULL, or static data for a second or
**					subsequent execution of a static frame.
**	stacksize	{i4}		Size of data area required for routine,
**					*not* including any DBDV array.
**	fdesc_size	{nat}		Number of entries for routine
**					in FDESCV2 structure.
**	vdesc_size	{nat}		Number of entries for routine
**					in IL_DB_VDESC array.
**	fdesc_off	{nat}		Offest into FDESCV2 structure of
**					routine's entries.
**	vdesc_off	{nat}		Offest into IL_DB_VDESC array of
**					routine's entries.
**	dbdv_off	{nat}		Offest into DBDV array of
**					routine's entries.
**
** Returns:
**	OK if successful, else an error status.
**
** Error statuses:
**	ILE_NOMEM	Could not allocate memory for control stack entry.
**
** Side effects:
**	Resets global IIOcontrolptr.
**
** History:
**	04/07/91 (emerson)
**		Created for local procedures.
**	26-jun-92 (davel)
**		set db_data to NULL for place-holder temps.
**	23-dec-93 (donc) Bug 56911
**		Steer clear of NULL IIOcontrolptr and IIOframeptr.
**	 6-nov-96 (kch)
**		Change to casts on calls to MEregmem(). Previous cast on
**		size parameter (u_i2) caused truncation of values greater
**		than 65535. This change fixes bugs 54055 and 78218.
*/
STATUS
IIOFucPushControl( name, tot_num_dbdvs, static_data, stacksize,
		   fdesc_size, vdesc_size, fdesc_off, vdesc_off, dbdv_off )
char	*name;
i4	tot_num_dbdvs;
PTR	static_data;
i4	stacksize;
i4	fdesc_size;
i4	vdesc_size;
i4	fdesc_off;
i4	vdesc_off;
i4	dbdv_off;
{
	IFCONTROL	*tmp;
	FDESC		*fdesc = IIOframeptr->frminfo.symtab;
        FDESCV2         *fdescv2 = NULL;

        if ( fdesc )
    	    fdescv2 = (FDESCV2 *)fdesc->fdsc_ofdesc;

	/*
	** Allocate the control block first.
	*/
	tmp = (IFCONTROL *)MEreqmem( 0, (u_i4)sizeof( IFCONTROL ), 
				     TRUE, (STATUS *)NULL );
	if ( tmp == NULL )
	{
		return ILE_NOMEM;
	}

	tmp->if_name       = name;
	tmp->if_fdesc_size = fdesc_size;
	tmp->if_vdesc_size = vdesc_size;
	tmp->if_fdesc_off  = fdesc_off;
	tmp->if_vdesc_off  = vdesc_off;
	tmp->if_dbdv_off   = dbdv_off;

        if ( fdesc && fdescv2 ) {
	    tmp->if_fdesc.fdsc_zero    = fdesc->fdsc_zero;
	    tmp->if_fdesc.fdsc_version = fdesc->fdsc_version;
	    tmp->if_fdesc.fdsc_cnt     = fdesc_size;
	    tmp->if_fdesc.fdsc_ofdesc  = (OFDESC *)&fdescv2[ fdesc_off ];

	    /*
	    ** Hack: Logic in many places in abfrt expects the routine's FDESCV2
	    ** entries to be terminated by an entry with a NULL fdv2_name.
	    ** I tried having IGgenRefs in igalloc write out an FDESCV2 entry
	    ** with a NULL fdv2_name, but in the process of writing the IL
	    ** to the database and then reading it back in at execution time,
	    ** the NULL pointer got changed to a pointer to an empty string.
	    ** Rather than tracking it down, I've patched things up here.
	    */
	    fdescv2[ fdesc_off + fdesc_size - 1 ].fdv2_name = (char *)NULL;
        }
	tmp->if_data = (DB_DATA_VALUE *)static_data;

	if (static_data == NULL)
	{
		i4		num_dbdvs, data_off, dbdv_array_size, totsize;
		DB_DATA_VALUE	*dbdvp;
		IL_DB_VDESC	*vdescp, *end_vdescp;
		PTR		areap;

		/* 
		** No old data area passed in, so the routine is non-static
		** and we must allocate a new data area for it.
		**
		** The data area will contain a dbdv array followed by
		** the data area proper (into which the routine's dbdbv's
		** will point).  If this is a main routine, the dbdv array
		** will be for the entire frame.  If this is a local procedure,
		** the dbdv array will be a save area big enough for
		** for the local procedure's dbdv's.
		**
		** We allocate at least 1 byte so that logic accessing
		** the stack area pointer will be happy without special-casing.
		*/
		if ( tot_num_dbdvs >= 0 )
		{
			num_dbdvs = tot_num_dbdvs;
		}
		else
		{
			num_dbdvs = vdesc_size;
		}
		dbdv_array_size = sizeof(DB_DATA_VALUE) * num_dbdvs;
		data_off = ALIGN_UP( dbdv_array_size );
		totsize = data_off + stacksize + 1;

		dbdvp = (DB_DATA_VALUE *)MEreqmem( 0, (u_i4)totsize, 
						   (bool)TRUE, (STATUS *)NULL );
		if ( dbdvp == NULL )
		{
			MEfree( (PTR)tmp );
			return ILE_NOMEM;
		}
		tmp->if_data = dbdvp;

		/*
		** We got the data area.
		**
		** Now we need to create DBDV entries for the data items
		** in the routine we're initializing.  (We create them
		** by "pointerizing" the corresponding VDESC entries).
		**
		** If this is a local procedure we're initializing, we must
		** first save off the DBDV entries that we're about to overlay,
		** and then adjust DBDV pointers to parameters and return values
		*/
		areap = (PTR)dbdvp + data_off;

		if ( tot_num_dbdvs < 0 && IIOcontrolptr )
		{
			dbdvp = IIOframeptr->dbdvs + dbdv_off;

			IIARccdCopyCallersDbdvs( IIOcontrolptr->if_param,
						 dbdvp, vdesc_size,
						 tmp->if_data );
		}
		vdescp = IIOframeptr->frminfo.dbd + vdesc_off;
		end_vdescp = vdescp + vdesc_size;

		while ( vdescp < end_vdescp )
		{
			dbdvp->db_datatype = vdescp->db_datatype;
			dbdvp->db_prec     = vdescp->db_prec;
			dbdvp->db_length   = vdescp->db_length;
			dbdvp->db_data     = vdescp->db_offset == -1 ?
						NULL : 
						vdescp->db_offset + areap;
			dbdvp++;
			vdescp++;
		}
	}

	/*
	** If this is a main (non-local) routine,
	** info about the data area we just got (or was passed in)
	** needs to be recorded in the top frame stack entry.
	*/
	if ( tot_num_dbdvs >= 0 )
	{
		IIOframeptr->num_dbdvs = tot_num_dbdvs;
		IIOframeptr->dbdvs = tmp->if_data;
	}

	/*
	** Add the control block to the control stack.
	*/
	tmp->if_nextcntrl = IIOcontrolptr;
	IIOcontrolptr = tmp;

	return OK;
}

/*{
** Name:	IIOFpcPopControl() -	Pop an entry from control stack
**
** Description:
**	Pops an entry from the control stack.
**	Also frees the data area for the entry being popped,
**	unless the entry is for a static frame.
**
** Returns:
**	OK if successful, else an error status.
**
** Side effects:
**	Resets global IIOcontrolptr.
**
** History:
**	04/07/91 (emerson)
**		Created for local procedures.
*/
STATUS
IIOFpcPopControl( )
{
	IFCONTROL	*tmp = IIOcontrolptr;

	/*
	** If this is a local procedure we're terminating,
	** we must restore the DBDV entries that we overlaid
	** and then adjust DBDV pointers to parameters and return values;
	** then we can free the data area.
	*/
	if ( tmp->if_data != IIOframeptr->dbdvs )
	{
		IIARccdCopyCallersDbdvs( tmp->if_nextcntrl->if_param,
					 tmp->if_data,
					 tmp->if_vdesc_size,
					 IIOframeptr->dbdvs
					 + tmp->if_dbdv_off );

		MEfree( (PTR)tmp->if_data );
	}
	/* 
	** If this is a main routine we're terminating,
	** we mustn't free the data area if the routine is a static frame.
	** (ILRF is keeping track of the data area.) 
	*/
	else if ( !( IIOframeptr->is_static ) )
	{
		MEfree( (PTR)tmp->if_data );
		IIOframeptr->dbdvs = (DB_DATA_VALUE *)NULL;
	}

	/*
	** Pop the top control stack entry and free it.
	*/
	IIOcontrolptr = tmp->if_nextcntrl;
	MEfree( (PTR)tmp );
}

/*{
** Name:	IIOFujPushJump	-	Create a new jump table
**
** Description:
**	Creates a new jump table for a menu or submenu within
**	the current routine.
**	The jump table will contain pointers to the IL statement beginning
**	each operation within a menu.  The value returned by IIretval()
**	serves as the index into this table.
**
** Inputs:
**	numops		Number of operations in the menu or submenu.
**
** Outputs:
**	None.
**
** Side effects:
**	Allocates memory for the jump table.
**	Resets the pointer within the stack frame to the current jump table.
*/
IIOFujPushJump(numops)
IL	numops;
{
	i4	size;
	ILJUMP	*jtab;

	/*
	** Allocate the correct sized jump table.  The jump table has
	** a dynamic array.  It is declared as a C structure with an
	** i4  array of 1 element.  Thus, sizeof(ILJUMP) only gives
	** room for 1 element in the array.  We need numops-1 more nats.
	*/
	size = sizeof(ILJUMP) + ((numops - 1) * sizeof(i4));
	jtab = (ILJUMP *) MEreqmem(0, (u_i2) size, TRUE, (STATUS *) NULL);
	if (jtab == NULL)
	{
		IIOerError(ILE_FRAME, ILE_NOMEM, (char *) NULL);
		return;
	}
	jtab->ilj_numele = numops;
	jtab->ilj_next = IIOcontrolptr->if_jump;
	IIOcontrolptr->if_jump = jtab;
}

/*{
** Name:	IIOFpjPopJump		-	Pop off a jump table
**
** Description:
**	Pops off a jump table for a submenu within the current frame.
**
** Inputs:
**	None.
**
** Side effects:
**	Resets the pointer within the stack frame to the current jump table.
*/
IIOFpjPopJump()
{
	ILJUMP	*cur;

	/*
	** Error if there is currently no jump table for a submenu.
	*/
	cur = IIOcontrolptr->if_jump;
	if (cur == NULL)
	{
		IIOpeProErr(ERx("IIOFpjPopJump"), ILE_STMT, ILE_ARGBAD,
				(char *) NULL);
		return;
	}

	IIOcontrolptr->if_jump = cur->ilj_next;
	MEfree((PTR) cur);
}

/*{
** Name:	IIOFsjSetJump	-	Set element in a jump table
**
** Description:
**	Sets an element in a jump table for a menu or submenu within
**	the current routine.
**	The jump table contains pointers to the IL statement beginning
**	each operation within a menu.  The value returned by IIretval()
**	serves as the index into this table.
**
**	The callers must assume jump table indices start at 1.
**
** Inputs:
**	index		Index into the jump table.
**	stmt		The current IL statement.
**	offset		The first IL statement for the operation, as
**			an offset from the current statement.
**
** Outputs:
**	None.
**
** History:
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails supporting long SIDs.
*/
IIOFsjSetJump(index, stmt, offset)
i4	index;
IL	*stmt;
IL	offset;
{
	ILJUMP	*table;
	i4	stmt_offset;

	IL	*temp;

	table = IIOcontrolptr->if_jump;
	stmt_offset = (i4) offset;
	if (*stmt&ILM_LONGSID)
	{
		stmt_offset = IIITvtnValToNat(offset, 0, ERx("IIOFsjSetJump"));
	}
	temp = stmt + stmt_offset;
	stmt_offset = (temp - IIOframeptr->frminfo.il);
	table->ilj_offsets[index-1] = stmt_offset;
}

/*{
** Name:	IIOFsmSetPrm	-	Set the param structure for a call
**
** Description:
**	Stores a pointer to the passed-in ABRTSPRM structure in the
**	top-most routine on the control stack.
**	This parameter structure can then be used by the frame (or
**	procedure) which the current routine is about to call.
**
** Inputs:
**	prm		The param structure.
**
**	tag		The tag for the param structure.
**
** Outputs:
**	None.
**
** History:
**	23-nov-1988 (Joe)
**		Updated from PC.  Added tag argument.
*/
IIOFsmSetPrm(prm, tag)
ABRTSPRM	*prm;
i2		tag;
{
	IIOcontrolptr->if_param = prm;
	IIOcontrolptr->if_ptag = tag;
}

/*{
** Name:        IIOFfpFreePrm   - Free a param structure.
**
** Description:
**      Frees the top param structure by doing a FEfree on its tag.
**
** History:
**      25-aug-1987 (Joe)
**              First Written for ABF maintenance release.
**	08/21/91 (emerson)
**		Fix bug 39361: Memory tag was not being freed:
**		Use IIUGtagFree instead of FEfree.
**		Also check for non-zero if_ptag rather than non-null
**		if_param, because IIIT4gCallEx in ilcall.sc may now
**		set if_ptag and use it to get memory without setting
**		if_param.
*/
IIOFfpFreePrm()
{
	if (IIOcontrolptr->if_ptag != 0)
	{
	       IIUGtagFree(IIOcontrolptr->if_ptag);
	       IIOcontrolptr->if_ptag = 0;
	       IIOcontrolptr->if_param = NULL;
	}
}

/*{
** Name:	IIOFdgDbdvGet	-	Return ptr to DB_DATA_VALUE
**
** Description:
**	Returns a pointer to a specified DB_DATA_VALUE in the base array in the
**	stack frame.
**	Uses the global frame pointer IIOframeptr.
**
** Inputs:
**	base	Index of this DB_DATA_VALUE in the base array
**		If base == 1, then the address of the DBV array is returned.
**
** History:
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		changed logic of bounds check.
**		Also removed special logic for base == 0;
**		I changed all calls of IIOFdgDbdvGet(0) to IIOFdgDbdvGet(1).
*/
DB_DATA_VALUE *
IIOFdgDbdvGet(base)
IL	base;
{

	/*
	** Check that the argument is within the legal range of
	** DB_DATA_VALUEs for this frame.
	*/
	--base;	/* convert to zero-based offset */
	if (base < 0 || base > IIOframeptr->num_dbdvs)
	{
		IIOpeProErr( ERx("IIOFdgDbdvGet"), ILE_FRAME, ILE_ARGBAD,
				(char *) NULL
		);
		return (DB_DATA_VALUE *) NULL;
	}

	return (DB_DATA_VALUE *)IIOframeptr->dbdvs + base;
}

/*{
** Name:	IIOFsesSetExecutedStatic - Set the current frame to be static.
**
** Description:
**	Marks a frame as being static *and* executed at least once.
**	Also tells ILRF there's data to be saved.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Changed the name of the function from IIOFssSetStatic
**		to IIOFsesSetExecutedStatic and expanded the description
**		to better reflect what this function has been doing all along.
**		Changed the way the frame is marked (set a pointer to non-null
**		instead of setting a bit).
*/
VOID
IIOFsesSetExecutedStatic()
{
	IIOframeptr->frminfo.static_data = (PTR)IIOframeptr->dbdvs;
	IIORscsSetCurStatic(&il_irblk, (PTR)IIOframeptr->dbdvs);
}
