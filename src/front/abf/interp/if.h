/*
**Copyright (c) 1986, 2004 Ingres Corporation
*/

/**
** Name:	IF.h		-	Header file for stack frame manager
**
** Description:
**	Header file for stack frame manager.
**
** Defines:
**	Macros for:
**		IIOFgcGetFdesc()
**		IIOFgbGetCodeblock()
**		IIOFgjGetJump()
**		IIOFgpGetPrm()
**		IIOFcpTopPrm()
**		IIOFgfaGetFrmAlign()
**		IIOFfnFormname()
**		IIOFfrFramename()
**		IIOFrnRoutinename()
**		IIOFgffGetFrameFid()
**		IIOFgtGetPrmTag()
**		IIOFgtSetPrmTag()
**		IIOFilpInLocalProc()
**		IIOFisIsStatic()
**		IIOFssSetStatic()
**		IIOFgsGetStaticData()
**		IIOFiesIsExecutedStatic()
**		IIOFgaGetAddr()
**		IIOFsaSetAddr()
**		IIOFscSetFdesc()
**		IIOFgffsGetFrameFdescSize()
**		IIOFgfvsGetFrameVdescSize()
**		IIOFgfssGetFrameStackSize()
**
**	IFCONTROL structure: information about an active frame, main procedure,
**			     or local procedure
**	IFFRAME   structure: information about an active frame or main procedure
**
** History:
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Split IFCONTROL structure into
**		IFCONTROL (one for each frame, main proc, *or* local proc), and
**		IFFRAME   (one for each frame or main proc);
**		added several fields to both structures.
**		Modified macros to reflect the fact that the new "control"
**		stack is sometimes appropriate instead of the frame stack.
**		Also modified macros to remove check for NULL stack pointer
**		(IIOframeptr == NULL or IIOcontrolptr == NULL);
**		these macros are never used when the relevant stack is empty.
**	08/21/91 (emerson)
**		Added IIOFgtSetPrmTag (for internal fix in ilcall.sc).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*::
** Name:	IIOFgcGetFdesc()	- Return FDESC structure for a routine
*/
# define IIOFgcGetFdesc() (&(IIOcontrolptr->if_fdesc))

/*::
** Name:	IIOFgbGetCodeblock()	- Return the beginning of the IL array
*/
# define IIOFgbGetCodeblock() (IIOframeptr->frminfo.il)

/*::
** Name:	IIOFgjGetJump()		- Return the jump table for the current
**					  menu or submenu
*/
# define IIOFgjGetJump() (IIOcontrolptr->if_jump)

/*::
** Name:	IIOFgpGetPrm()		- Return a routine's ABRTSPRM structure
**
**	Gets the ABRTSPRM structure for the current routine (the top-most
**	routine on the control stack).  This structure is actually pointed
**	to by the 'if_param' element of the calling routine (the next-to-top
**	routine on the stack).
*/
# define IIOFgpGetPrm() ((IIOcontrolptr->if_nextcntrl == NULL) ? \
			 (ABRTSPRM *) NULL : \
			 IIOcontrolptr->if_nextcntrl->if_param)

/*::
** Name:	IIOFtpTopPrm()		- Return top frame's ABRTSPRM
**
**	Gets the ABRTSPRM structure for the top routine on the control stack.
*/
# define IIOFtpTopPrm() (IIOcontrolptr->if_param)

/*::
** Name:	IIOFgfaGetFrmAlign()	- Return frame's alignment info
*/
# define IIOFgfaGetFrmAlign() (IIOframeptr->frminfo.align)

/*::
** Name:	IIOFfnFormname()	- Return the name of the current form
*/
# define IIOFfnFormname() (IIOframeptr->formname)

/*::
** Name:	IIOFfrFramename()	- Return the name of the current frame
*/
# define IIOFfrFramename() (IIOframeptr->fid.name)

/*::
** Name:	IIOFrnRoutinename()	- Return the name of the current routine
**					  (frame, proc, or local proc)
*/
# define IIOFrnRoutinename() (IIOcontrolptr->if_name)

/*::
** Name:	IIOFgffGetFrameFid()	- Return the fid of the current frame
*/
# define IIOFgffGetFrameFid() (IIOframeptr->fid.id)

/*::
** Name:	IIOFgtGetPrmTag()	- Return the tag used for parm structure
*/
# define IIOFgtGetPrmTag() (IIOcontrolptr->if_ptag)

/*::
** Name:	IIOFgtSetPrmTag()	- Set the tag used for parm structure
*/
# define IIOFgtSetPrmTag(tag) (IIOcontrolptr->if_ptag = tag)

/*::
** Name:	IIOFilpInLocalProc()	- Are we in a local procedure?
*/
# define IIOFilpInLocalProc() (IIOcontrolptr->if_data != IIOframeptr->dbdvs)

/*::
** Name:	IIOFisIsStatic()	- Is the current frame static?
*/
# define IIOFisIsStatic() (IIOframeptr->is_static)

/*::
** Name:	IIOFssSetStatic()	- Set: is the current frame static?
*/
# define IIOFssSetStatic(val) (IIOframeptr->is_static = val)

/*::
** Name:	IIOFgsGetStaticData()   - Get pointer to static data
*/
# define IIOFgsGetStaticData() (IIOframeptr->frminfo.static_data)

/*::
** Name:	IIOFiesIsExecutedStatic()     - Is the current frame static,
**						and has it already been
**						excuted at least once?
*/
# define IIOFiesIsExecutedStatic() (IIOframeptr->frminfo.static_data != NULL)

/*::
** Name:	IIOFgaGetAddr()		- Return offset of current instruction
*/
# define IIOFgaGetAddr() (IIOcontrolptr->if_current)

/*::
** Name:	IIOFsaSetAddr()		- Set offset of current instruction
*/
# define IIOFsaSetAddr(off) (IIOcontrolptr->if_current = off)

/*::
** Name:	IIOFscSetFdesc()	- Set FDESC after calling a routine
**
** Description:
**	Recomputes the fdsc_ofdesc field of an FDESC.
**	Should be called after receiving control back from a called routine
**	(because the FDESCV2 array may have moved).
*/
# define IIOFscSetFdesc() \
	( IIOcontrolptr->if_fdesc.fdsc_ofdesc = (OFDESC *) \
	( (FDESCV2 *)IIOframeptr->frminfo.symtab->fdsc_ofdesc + \
	  IIOcontrolptr->if_fdesc_off ) )

/*::
** Name:	IIOFgffsGetFrameFdescSize()	- Get frame FDESC size
**		IIOFgfvsGetFrameVdescSize()	- Get frame VDESC size
**		IIOFgfssGetFrameStackSize()	- Get frame stack size
** Description:
**	These 3 macros are used by IL_START.
**	IL_START can only appear in programs compiled before release 6.4.
**	When interpreting IL_START, we emulate an IL_MAINPROC instruction,
**	by looking at info in FRMINFO.
*/
# define IIOFgffsGetFrameFdescSize() (IIOframeptr->frminfo.symtab->fdsc_cnt)
# define IIOFgfvsGetFrameVdescSize() (IIOframeptr->frminfo.num_dbd)
# define IIOFgfssGetFrameStackSize() (IIOframeptr->frminfo.stacksize)

/*}
** Name:	IFCONTROL - Information about an active frame, main procedure,
**			    or local procedure
** History:
**    25-aug-1987 (Joe)
**            Added if_ptag field so that param structure could
**            be freed easily.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		extracted frame-specific stuff into new IFFRAME structure;
**		got rid of unused if_context; replaced if_retaddr by if_current;
**		got rid of if_static (we look at new static_data in FRMINFO
**		instead); added several new fields.
*/
typedef struct ifcntrl {
	i4		if_current;	/* offset of current instruction
					** (relative to IL code block);
					** filled in only while this routine
					** is calling another */
	DB_DATA_VALUE	*if_data;	/* ptr to base of data area.
					** for a frame or main procedure,
					** this is equal to the dbvds field in
					** the coresponding IFFRAME structure */
	ILJUMP		*if_jump;	/* top of stack of routn's jump tables*/
	char		*if_name;	/* name of routine */
	FDESC		if_fdesc;	/* pseudo-symbol table for routine */
	ABRTSPRM	*if_param;	/* parameters passed to routine */
	i2		if_ptag;	/* fealloc tag for param structure */
	i2		if_fdesc_size;	/* number of entries for routine in
					** FDESCV2 structure */
	i2		if_vdesc_size;	/* number of entries for routine in
					** IL_DB_VDESC array */
	i2		if_fdesc_off;	/* offset into FDESCV2 structure of
					** routine's entries */
	i2		if_vdesc_off;	/* offset into IL_DB_VDESC array of
					** routine's entries */
	i2		if_dbdv_off;	/* offset into dbdv array of
					** routine's entries */
	struct ifcntrl	*if_nextcntrl;	/* next element in the control stack */
} IFCONTROL;

/*}
** Name:	IFFRAME - Information about an active frame or main procedure
**
** History:
**	04/07/91 (emerson)
**		Created for local procedures:
**		extracted from old IFCONTROL structure
**		(but the names are different); added several new fields.
*/
typedef struct ifframe {
	FRMINFO		frminfo;	/* frame info supplied by ILRF */
	FID		fid;		/* the fid for this object */
	DB_DATA_VALUE	*dbdvs;		/* ptr to dbdv array for frame */
	i4		num_dbdvs;	/* size of dbdv array for frame */
	char		*formname;	/* name of the form for this frame */
	bool		is_static;	/* was frame defined to be static? */
	struct ifframe	*nextfrm;	/* element of calling frame */
} IFFRAME;
