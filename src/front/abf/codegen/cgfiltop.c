/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<nm.h>
#include	<lo.h>
#include	<si.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
/* Interpreted Frame Object definition */
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	<ilrffrm.h>
/* ILRF Block definition */
#include	<abfrts.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>

#include	"cggvars.h"
#include	"cgil.h"
#include	"cgilrf.h"
#include	"cglabel.h"
#include	"cgout.h"

/**
** Name:    cgfiltop.c -	Code Generator Routine Prologue/End Module.
**
** Description:
**	Contains routines to generate code for the routine prologue and end
**	in the output file.  Defines:
**
**	IICGfltFileTop()	Output the required code for the top of a file.
**	IICGpgProcGen()		Start a frame or procedure.
**	IICGpeProcEnd()		Output the required code for the end of a
**				frame or procedure.
**	IICGdgDbdvGet()		Find the VDESC corresponding to a DBDV.
**
** History:
**	Revision 6.0  87/07  arthur
**	Initial revision.
**
**	Revision 6.3/03/00  89/11  wong
**	Moved hidden field initialization here from "cgdisp.c".
**	89/11  billc  Added complex-object support.
**
**	Revision 6.3/03/01
**	01/13/91 (emerson)
**		Fix for bug 34840 in ProcInit.
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Added functions IICGpgProcGen and IICGdgDbdvGet.
**		Renamed IICGfleFileEnd to IICGpeProcEnd.
**		Modified several functions.
**	05/07/91 (emerson)
**		"Uniqueify" local procedure names within the executable
**		(in ProcHead).  Also added logic (in IICGpgProcGen)
**		to generate STATIC declarations for local procedures
**		at the top of the source file.
**	07/03/91 (emerson)
**		Fix a syntactic error noted by more than one porter:
**		code (PTR *)&x instead of &((PTR)x).
**	08/19/91 (emerson)
**		"Uniqueify" names by appending the frame id (generally 5
**		decimal digits) instead of the symbol name.  This will reduce
**		the probability of getting warnings (identifier longer than
**		n characters) from the user's C compiler.  (Bug 39317).
**
**	Revision 6.5
**	29-jun-92 (davel)
**		Added support for decimal constants. Added new argument to 
**		IIORcqConstQuery() calls.
**	21-aug-92 (davel)
**		when initializing FDESCV2 arrays, put the name out using
**		IICGpscPrintSconst(), as now field names may have embedded
**		quotes or other delim ID characters that may require
**		C-style escaping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      25-sep-2000 (hanch04)
**          missed more nats, code generated should use type i4 and not nat.
**	26-Mar-2000 (hanch04)
**	    Replace MEfill call with C stdlib call memset.
**	7-Jan-2004 (schka24)
**	    We'd better init the new collID member of DB_DATA_VALUE.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
*/

static	char	*Stat_str;

/*
** Variables containing info about the routine
** (frame, main procedure, or local procedure) being generated.
*/
static	IL	*Rtn_proc_stmt;	/* IL_MAINPROC or IL_LOCALPROC  for routine */
static	i4	Rtn_stacksize;	/* size of data area (no dbdvs) for routine */
static	i4	Rtn_fdesc_size;	/* number of FDESCV2    entries for routine */
static	i4	Rtn_vdesc_size;	/* number of VDESC      entries for routine */
static	i4	Rtn_fdesc_off;	/* offset of FDESCV2    entries for routine */
static	i4	Rtn_vdesc_off;	/* offset of VDESC      entries for routine */
static	i4	Rtn_dbdv_off;	/* offset of dbdv array entries for routine */
static	i4	Rtn_tot_dbdvs;	/* for a frame or main procedure:
				** total size of dbdv array for frame or proc;
				** for a local procedure: -1 */
VOID	IICGpeProcEnd();

/*{
** Name:	IICGfltFileTop() -	Output the top of a file
**
** Description:
**	Outputs the top of a file.
**	Each file must include the following:
**		1) includes of header files;
**		2) declarations of external variables.
** History:
**	04/07/91 (emerson)
**		Modified for local procedures:
**		all stuff that applies to each procedure has been moved.
*/
static VOID	HdrInclude();
static VOID	GenDbdvs();
static VOID	GenV2Fdescs();
static VOID	GenFdesc();
static VOID	ProcHead();
static VOID	ProcVars();
static VOID	ConstArrs();
static VOID	ProcInit();
static VOID	_genReturn();

VOID
IICGfltFileTop()
{
    Stat_str = (CG_static ? ERx("static ") : ERx(""));
    HdrInclude();
    GenDbdvs();
    GenV2Fdescs();
    IICGoleLineEnd();		/* leave a blank line */
    ConstArrs();

    return;
}

/*{
** Name:	IICGpgProcGen() -	Start a frame or procedure 
**
** Description:
**	Generates code for the OSL 'start', 'main procedure',
**	and 'local procedure' statements.
**
**	Each frame or proc must include the following:
**		1) procedure name and argument list;
**		2) declarations of local variables.
**
** IL statements:
**	START
**
**	MAINPROC VALUE1 VALUE2 VALUE3 VALUE4
**
**	The values have the following meanings:
**
**	VALUE1	The ILREF of an integer constant containing the stack size
**		required by the frame or procedure (not including dbdv's).
**	VALUE2	The number of FDESC entries for the frame or procedure.
**	VALUE3	The number of VDESC entries for the frame or procedure.
**	VALUE4	The total number of entries in the dbdv array
**		for the frame or procedure and any local procedures it may call.
**
**	LOCALPROC VALUE1 VALUE2 VALUE3 VALUE4 VALUE5 VALUE6 VALUE7 VALUE8
**
**	The values have the following meanings:
**
**	VALUE1	The ILREF of an integer constant containing the stack size
**		required by the local procedure (not including dbdv's).
**	VALUE2	The number of FDESC entries for the local procedure.
**	VALUE3	The number of VDESC entries for the local procedure.
**	VALUE4	The offset of the FDESC entries for the local procedure
**		(within the composite FDESC for the entire frame or procedure).
**	VALUE5	The offset of the VDESC entries for the local procedure
**		(within the composite VDESC array for the entire frame or proc).
**	VALUE6	The offset of the dbdv's for the local procedure
**		(within the composite dbdv array for the entire frame or proc).
**	VALUE7	A reference to a char constant containing the name of
**		the local procedure.
**	VALUE8	The SID of the IL_MAINPROC or IL_LOCALPROC statement
**		of the "parent" main or local procedure (i.e, the procedure
**		in which this procedure was declared).
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	04/07/91 (emerson)
**		Created for local procedures.
**	05/07/91 (emerson)
**		Added logic to generate STATIC declarations for local procedures
**		at the top of the source file.
**	08/02/91 (emerson)
**		Added logic for LeighB to generate, for PC's, pragmas
**		to force the first 9 local procedures into their own
**		code segments.
**	29-jun-92 (davel)
**		added new argument to call to IIORcqConstQuery().
*/
VOID
IICGpgProcGen( stmt )
IL		*stmt;
{
	IL	op = *stmt & ILM_MASK;

	/*
	** Save pointer to IL_MAINPROC or IL_LOCALPROC statement
	** of current routine (for IICGdgDbdvGet).
	*/
	Rtn_proc_stmt = stmt;

	/*
	** If we're starting a local procedure, finish the previous routine,
	** and indicate that we're no longer in a frame.
	**
	** Then, for all kinds of routines,
	** extract information about the new routine into static variables.
	*/
	if ( op == IL_LOCALPROC )
	{
		ILREF	name_ref;

		IICGpeProcEnd();	/* Finish previous procedure */
		CG_form_name = NULL;
		CG_static = FALSE;
		Stat_str = ERx("");

		name_ref       = ILgetOperand( stmt, 7 );

		(VOID)IIORcqConstQuery( &IICGirbIrblk, - name_ref, DB_CHR_TYPE,
					(PTR *)&CG_routine_name, (i4 *)NULL );

		CG_frame_name  = CG_routine_name;

		Rtn_fdesc_off  = ILgetOperand( stmt, 4 );
		Rtn_vdesc_off  = ILgetOperand( stmt, 5 );
		Rtn_dbdv_off   = ILgetOperand( stmt, 6 );
	}
	else /* IL_START or IL_MAINPROC */
	{
		CG_routine_name = CG_symbol_name;
		Rtn_fdesc_off  = 0;
		Rtn_vdesc_off  = 0;
		Rtn_dbdv_off   = 0;
	}

	if ( op == IL_START )
	{
		Rtn_stacksize  = CG_frm_stacksize;
		Rtn_fdesc_size = CG_fdesc->fdsc_cnt;
		Rtn_vdesc_size = CG_frm_num_dbd;
		Rtn_tot_dbdvs  = Rtn_vdesc_size;
	}
	else /* IL_MAINPROC or IL_LOCALPROC */
	{
		PTR	constval;
		ILREF	size_ref;

		size_ref       = ILgetOperand( stmt, 1 );

		(VOID)IIORcqConstQuery( &IICGirbIrblk, - size_ref, DB_INT_TYPE,
					&constval, (i4 *)NULL );
		Rtn_stacksize  = *( (i4 *)constval );
		Rtn_fdesc_size = ILgetOperand( stmt, 2 );
		Rtn_vdesc_size = ILgetOperand( stmt, 3 );

		if ( op == IL_LOCALPROC )
		{
			Rtn_tot_dbdvs  = -1;
		}
		else /* IL_MAINPROC */
		{
			Rtn_tot_dbdvs  = ILgetOperand( stmt, 4 );
		}
	}

	/*
	** Now that we've extracted information about the new routine
	** into static variables, generate code for the beginning
	** of the new routine.
	**
	** For IL_MAINPROC, generate a declaration of the form
	**
	**	static i4  * <CG_routine_name>_<CG_fid>();
	**
	** for each local routine.
	*/
	if ( op == IL_MAINPROC )
	{
		i4	pragma_count = 0;

		IICGoleLineEnd( );		/* leave a blank line */

		for ( ;; )
		{
			ILREF	sid, name_ref;
			char	*proc_name, full_proc_name[ CGBUFSIZE ];

			stmt = IICGnilNextIl( stmt ); /* get NEXTPROC */

			sid = ILgetOperand( stmt, 1 );
			if ( sid == 0 )
			{
				break;
			}
			stmt = IICGesEvalSid( stmt, sid ); /* get LOCALPROC */

			name_ref = ILgetOperand( stmt, 7 );

			(VOID)IIORcqConstQuery( &IICGirbIrblk,
						- name_ref, DB_CHR_TYPE,
						(PTR *)&proc_name, (i4 *)NULL);

			(VOID)STprintf( full_proc_name, ERx("%s_%d"),
					proc_name, CG_fid );
			IICGosbStmtBeg( );
			IICGoprPrint( ERx("static i4  *") );
			IICGocbCallBeg( full_proc_name );
			IICGoceCallEnd( );
			IICGoseStmtEnd( );
# ifdef PMFE
			/*
			** For the PC, we'd like to emit a pragma to force
			** each local procedure into its own code segment.
			** Unfortunately, the Microsoft C compiler
			** allows only 9 pragmas, so only the first 9
			** local procedures get their own code segments.
			*/
			pragma_count++;
			if ( pragma_count <= 9 )
			{
				IICGoprPrint( ERx("#pragma alloc_text(") );
				IICGoprPrint( full_proc_name );
				IICGoprPrint( ERx("_TEXT, ") );
				IICGoprPrint( full_proc_name );
				IICGoprPrint( ERx(")") );
				IICGoleLineEnd( );
			}
# endif
		}
	}
	ProcHead();
	GenFdesc();
	IICGoleLineEnd();		/* leave a blank line */
	ProcVars();
	IICGoleLineEnd();		/* leave a blank line */
	ProcInit();
	IICGoleLineEnd();		/* leave a blank line */

	return;
}

/*
** Name:    HdrInclude() -	Generate include stmt for header file
**
** Description:
**	Generates the include statement for the header file which must
**	be included into every .c file generated by the code generator.
**
** Code generated:
**
**	# include "ii_config!oslhdr.h"
**
** Inputs:
**	None.
**
** History:
**	30-oct-89 (marian)
**		Changed ERx("") to (char *)NULL because NMloc() cannot handle
**		ERx("").  NMloc() returns fail with "" but understands NULL.
*/
static VOID
HdrInclude()
{
	LOCATION	pathloc;
	LOCATION	loc;
	char		pathbuf[MAX_LOC+1];

	IICGoleLineEnd();	/* return to left margin */
	if ( NMloc(FILES, PATH, (char *)NULL, &loc) == OK
		&& ( LOcopy(&loc, pathbuf, &pathloc),
			LOfstfile(ERx("oslhdr.h"), &pathloc) == OK ) )
	{
		char	*cp;
		char	buf[CGBUFSIZE+1];

		LOtos(&pathloc, &cp);
		IICGoprPrint(STprintf(buf, ERx("# include \"%s\""), cp));
		IICGoleLineEnd();
	}

	IICGoleLineEnd();	/* skip line */
}

/*
** Name:    GenDbdvs() -	Generate external defs of DB_DATA_VALUEs
**
** Description:
**	Generates external definitions of DB_DATA_VALUEs
**	(actually IL_DB_VDESCs).
**
** Code generated:
**
**	static IL_DB_VDESC IIDB_symbol-name[num-dbdvs] = {
**		offset-into-data-area, len, type, 0,
**		...
**	};
**
** History:
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Get number of VDESCs from CG_frm_num_dbd instead of
**		passed-in parm; get VDESC directly from CG_frm_dbd
**		instead of calling the now-obsolete IIORdgDbdvGet.
*/
static VOID	DbdvEle();

static VOID
GenDbdvs ()
{
    if (CG_frm_num_dbd > 0)
    {
	register i4	i;
	char		buf[CGBUFSIZE];

	IICGostStructBeg(ERx("static IL_DB_VDESC"), 
				STprintf(buf, ERx("IIDB_%d"), CG_fid),
				CG_frm_num_dbd, CG_INIT
	);

	for (i = 1 ; i <= CG_frm_num_dbd ; i++)
	{
	    IL_DB_VDESC	*dbdv;

	    dbdv = &CG_frm_dbd[i - 1];
	    DbdvEle(dbdv);
	    IICGoeeEndEle((i == CG_frm_num_dbd) ? CG_LAST : CG_NOTLAST);
	}

	IICGosdStructEnd();
    }
    return;
}

/*
** Name:    DbdvEle() - Generate a DB_DATA_VALUE element
**
** Description:
**	Generates an element of an array of DB_DATA_VALUEs.
**
** Inputs:
**	dbv		The DB_DATA_VALUE, or NULL if a place_holder
**			DB_DATA_VALUE should be generated.
*/
static VOID
DbdvEle (dbv)
register IL_DB_VDESC	*dbv;
{
    char	buf[CGBUFSIZE];

    IICGosbStmtBeg();
    IICGoprPrint(STprintf(buf, ERx("%d, %d, %d, %d"),
	dbv->db_offset, dbv->db_length, dbv->db_datatype, dbv->db_prec
    ));
}

/*
** Name:    GenV2Fdescs() -	Generate external defs of FDESCV2s
**
** Description:
**	Generates external definitions of FDESCV2s.
**
** Code generated:
**
**	static FDESCV2 IINFD_symbol-name[num-fields-plus-one] = {
**		fld-name, tblfld-name, 'v', 0, index-into-dbdv-array, 
**						flags, datatype-name
**		...
**		NULL, NULL, '\0', 0, 0, 0, NULL
**	};
**
** History:
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Get number of FDESCs from CG_fdesc->fdsc_cnt instead of
**		passed-in parm (which was incorrect: it was the number of
**		VDESCs, not FDESCs - luckily, it was never too small).
**		Change check for end of FDESCV2 array to reflect the fact
**		that it now may embedded NULL entries.
*/
static VOID	Fdescv2Ele();

static VOID
GenV2Fdescs ()
{
    FDESCV2	*fdp, *efdp;
    char	buf[CGBUFSIZE];

    IICGostStructBeg( ERx("static FDESCV2"), 
			STprintf(buf, ERx("IINFD_%d"), CG_fid),
			CG_fdesc->fdsc_cnt, CG_INIT
    );

    fdp = (FDESCV2 *)CG_fdesc->fdsc_ofdesc;
    efdp = fdp + CG_fdesc->fdsc_cnt - 1;
    while (fdp < efdp)
    {
	Fdescv2Ele(fdp);
	IICGoeeEndEle(CG_NOTLAST);
	fdp++;
    }
    Fdescv2Ele(fdp);
    IICGoeeEndEle(CG_LAST);
    IICGosdStructEnd();
    return;
}

/*
** Name:    Fdescv2Ele() -	Generate an FDESCV2 element
**
** Description:
**	Generates an element of an array of FDESCV2s.
**
** Inputs:
**	fdesc		The FDESCV2 structure to be output.
**
** History:
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Change check for NULL entries to reflect what ILG
**		generated at the end of each local procedure.
**	21-aug-92 (davel)
**		put filed name out with IICGpscPrintSconst(), as now field
**		names may have embedded quotes or other special characters.
*/
static VOID
Fdescv2Ele (fdesc)
register FDESCV2 *fdesc;
{
    IICGosbStmtBeg();
    if (fdesc->fdv2_name == NULL)
    {
	IICGoprPrint(ERx("NULL, NULL, '\\0', 0, 0, 0, NULL"));
    }
    else
    {
	char	buf[CGBUFSIZE];

	/* assume that nulls cannot exist in the fdv2_name, so that
	** using STlength is okay.
	*/
	IICGpscPrintSconst(fdesc->fdv2_name, STlength(fdesc->fdv2_name));
	_VOID_ STprintf( buf, ERx(", \"%s\", '%c', %d, %d, 0x%x, \"%s\""),
		fdesc->fdv2_tblname,
		fdesc->fdv2_visible, fdesc->fdv2_order,
		fdesc->fdv2_dbdvoff, fdesc->fdv2_flags,
		(fdesc->fdv2_typename == NULL ? ERx("") : fdesc->fdv2_typename)
	);
	IICGoprPrint(buf);
    }
    return;
}

/*
** Name:    GenFdesc() -	Generate external definition of an FDESC
**
** Description:
**	Generates the external definition of an FDESC.
**
** Code generated:
**
**	static FDESC IIfdesc = {
**		0, version-num, num-fdescv2s, IINFD_symbol-name
**	};
**
** Inputs:
**	None.
**
** History:
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		We now build an FDESC for each routine;
**		each points into a different part of the FDESCV2.
*/
static VOID
GenFdesc()
{
	char	buf[CGBUFSIZE];

	IICGostStructBeg( ERx("static FDESC"), ERx("IIfdesc"), 0, CG_INIT );

	IICGosbStmtBeg();
	IICGoprPrint(STprintf(buf, ERx("%d, %d, %d, &IINFD_%d[%d]"),
				CG_fdesc->fdsc_zero, CG_fdesc->fdsc_version,
				Rtn_fdesc_size, CG_fid,
				Rtn_fdesc_off
			)
	);
	IICGoleLineEnd();
	IICGosdStructEnd();
	return;
}

/*
** Name:	ProcHead() -	Generate the head for a procedure
**
** Description:
**	Generates the head for each procedure of the file.
**	The procedure name of each frame, main procedure, or local procedure
**	is retrieved from the global variable CG_routine_name.
**
** Code generated:
**
**#ifdef local_proc
**	static
**#endif
**	i4 *
**#ifdef local_proc
**	symbol-name(rtsfparam, IIdbdvs)
**	ABRTSPRM * rtsfparam;
**	DB_DATA_VALUE * IIdbdvs;
**#else
**	symbol-name(rtsfparam)
**	ABRTSPRM * rtsfparam;
**#endif
**	{
**
** History:
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		they have IIdbdvs as a second parm.
**
**		Also declare procedure as returning a pointer to nat.
**		It used to be declared as returning an int (by default),
**		but we actually return NULL, and callers expect a pointer.
**		This caused trouble on Motorola 68xxx platforms,
**		which have separate data and address registers.
**		I chose (i4 *) rather than (char *) or PTR,
**		because we generally declare these user procedures and frames
**		as returning (i4 *), e.g. in ABRTSNUSER and ABRTSPRO structures
**		(These structures appear in extract files).
**	05/07/91 (emerson)
**		"Uniqueify" local procedure names within the executable.
**		and make local procedures static.
*/
static VOID
ProcHead()
{
	char	local_proc_name[CGBUFSIZE];

	/*
	** Output return type of procedure.
	*/
	IICGoleLineEnd();		/* leave a blank line */
	if (Rtn_tot_dbdvs < 0)
	{
		IICGosbStmtBeg();
		IICGoprPrint(ERx("static"));
		IICGoleLineEnd();
	}
	IICGosbStmtBeg();
	IICGoprPrint(ERx("i4 *"));
	IICGoleLineEnd();

	/*
	** Output name and argument list of procedure.
	*/
	IICGosbStmtBeg();

	if (Rtn_tot_dbdvs < 0)
	{
		(VOID)STprintf(local_proc_name, ERx("%s_%d"), CG_routine_name,
				CG_fid
		);
		IICGocbCallBeg(local_proc_name);
	}
	else
	{
		IICGocbCallBeg(CG_routine_name);
	}

	IICGocaCallArg(ERx("rtsfparam"));

	if (Rtn_tot_dbdvs < 0)
	{
		IICGocaCallArg(ERx("IIdbdvs"));
	}
	IICGoceCallEnd();
	IICGoleLineEnd();
	IICGostStructBeg(ERx("ABRTSPRM *"), ERx("rtsfparam"), 0, CG_NOINIT);

	if (Rtn_tot_dbdvs < 0)
	{
		IICGostStructBeg(ERx("DB_DATA_VALUE *"), ERx("IIdbdvs"),
				 0, CG_NOINIT);
	}

	IICGobbBlockBeg();
	return;
}

/*
** Name:	ProcVars() -	Generate the vars for the main procedure
**
** Description:
**	Generates the local variables for the main procedure of the file.
**
** Code generated:
**
**	ALIGN_RESTRICT	IIdata_area[(Rtn_stacksize/sizeof(ALIGN_RESTRICT)) + 1];
**#ifdef local_proc
**	DB_DATA_VALUE IIdbdv_save[Rtn_vdesc_size];
**#else
**	DB_DATA_VALUE IIdbdvs[Rtn_tot_dbdvs];
**#endif
**	DB_DATA_VALUE * IIrvalue;
**	static int first = 1;	(only for static frames)
**	int	IIbrktype;
**	int	IIi;
**	i4	IIrownum;
**	char	IImode[2];
**	DB_DATA_VALUE	IIelt;
**	i4	IIARivlIntVal();
**	int	IIARrpgRtsPredGen();
**	int	IIQG_send();
**
** History:
**	11/89 (billc)  Added local data definitions for complex objects support.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Moved generation of optional form symbol and FRMALIGN structure
**		from ProcVars to ConstArrs (because we want just one copy
**		of each per source file).
**		Generate IIdbdv_save instead of IIdbdvs for a local proc.
**		Get number of DBDVs from Rtn_tot_dbdvs or Rtn_vdesc_size
**		instead of passed-in parm; get stack size from Rtn_stacksize
**		instead of calling the now-obsolete IIORdsDataSize.
*/
static VOID
ProcVars ()
{
	char	tbuf[CGBUFSIZE];
	char	buf[CGBUFSIZE];
	char	*db_dv_str = ERx("DB_DATA_VALUE");

	if (Rtn_stacksize > 0)
	{ /* Data area. */
		IICGosbStmtBeg();
		IICGoprPrint( STprintf(buf,
		ERx("%sALIGN_RESTRICT\tIIdata_area[(%d/sizeof(ALIGN_RESTRICT)) + 1]"),
		Stat_str, (i4)Rtn_stacksize
		)
		);
		IICGoseStmtEnd();
	}

	if (Rtn_tot_dbdvs >= 0) /* frame or main procedure */
	{
		if (Rtn_tot_dbdvs == 0)
		{ /* No fields, hidden or visible, and no temporaries. */
			IICGoprPrint(ERx("#define IIdbdvs (DB_DATA_VALUE *)NULL"));
			IICGoleLineEnd();
		}
		else
		{ /* Fields or temporaries in main *or* local procedure. */
			/* setup '[static ]DB_DATA_VALUE'. */
			STprintf(tbuf, ERx("%s%s"), Stat_str, db_dv_str);
			IICGostStructBeg( tbuf, ERx("IIdbdvs"),
					  Rtn_tot_dbdvs, CG_NOINIT );
		}
	}
	else /* local procedure */
	{
		if (Rtn_vdesc_size > 0)
		{ /* Fields or temporaries in local procedure. */
			/* setup 'DB_DATA_VALUE'. */
			IICGostStructBeg( db_dv_str, ERx("IIdbdv_save"),
					  Rtn_vdesc_size, CG_NOINIT );
		}
	}

	/* set up '[static] DB_DATA_VALUE *' */
	STprintf(tbuf, ERx("%s%s *"), Stat_str, db_dv_str);
	IICGostStructBeg(tbuf, ERx("IIrvalue"), 0, CG_NOINIT);
	
	if (CG_static)
	{ /* 'first-time' flag */
		IICGosbStmtBeg();
		IICGoprPrint(ERx("static int first = 1"));
		IICGoseStmtEnd();
	}

	IICGocvCVar(ERx("IIbrktype"), DB_INT_TYPE, 0, CG_NOINIT);
	if (Rtn_vdesc_size > 0)
		IICGocvCVar(ERx("IIi"), DB_INT_TYPE, 0, CG_NOINIT);

	IICGosbStmtBeg();
	IICGoprPrint(ERx("i4\tIIrownum"));
	IICGoseStmtEnd();

	IICGocvCVar(ERx("IImode"), DB_CHR_TYPE, 2, CG_NOINIT);

	IICGosbStmtBeg();
	IICGoprPrint(ERx("DB_DATA_VALUE\tIIelt"));
	IICGoseStmtEnd();

	IICGosbStmtBeg();
	IICGoprPrint(ERx("i4\tIIARivlIntVal()"));
	IICGoseStmtEnd();

	IICGocvCVar(ERx("IIARrpgRtsPredGen()"), DB_INT_TYPE, 0, CG_NOINIT);
	IICGocvCVar(ERx("IIQG_send()"), DB_INT_TYPE, 0, CG_NOINIT);
	return;
}

/*
** Name:	ConstArrs() -	Generate the definitions for constants
**
** Description:
**	Generates the definitions for constants for the frame.
**	There are 5 types of constants:	 integer, float, char, hex and decimal.
**
**	Also generates the constant FRMALIGN structure, and,
**	if this is a frame, the constant IICform.
**
** Inputs:
**	None.
**
** History:
**	18-oct-88 (kenl)
**		Change constval in routine ConstArrs() from a PTR to (i4 *).
**	21-apr-89 (kenl)
**		Changed constval back to a PTR since I had to change routine
**		IIORcqConstQuery to work with a PTR.
**	01/09/91 (emerson)
**		Cast constval to (i4 *), not (i4 *), to avoid problems
**		on platforms (if any) where i4 and i4  are of different size.
**		(For DB_INT_TYPE, IIORcqConstQuery yields a pointer to an i4).
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Moved generation of optional form symbol and FRMALIGN structure
**		from ProcVars to ConstArrs (because we want just one copy
**		of each per source file).
**
**		Also "uniqueified" the names of constants.
**		Historically, codegen seems to generate "unique" names for
**		static vars at the outermost level (which constants now are).
**		"Unique" here means unique across the image load module.
**		I'm not sure why this was done.  Perhaps it's to facilitate
**		debugging: adb knows about statics declared at the outer level.
**	29-jun-92 (davel)
**		added new argument to call to IIORcqConstQuery().
**	30-jun-92 (davel)
**		add support for decimal constants.
*/
static VOID
ConstArrs()
{
    i4			num_ints = 0;
    i4			num_flts = 0;
    i4			num_strs = 0;
    i4			num_hexs = 0;
    i4			num_decs = 0;
    register i4	i;
    char		buf[CGBUFSIZE];

    PTR			constval = NULL;

    /*
    ** If this is a frame, generate a declaration for IICform_<symbol_name>
    ** and initialize the name of the variable.
    */
    if ( CG_form_name != NULL )
    {
	STprintf( _FormVar, ERx("IICform_%d"), CG_fid );
	IICGostStructBeg( ERx("static char"), _FormVar, 0, CG_ZEROINIT );
	IICGosbStmtBeg();
	IICGoprPrint( STprintf(buf, ERx("\"%s\""), CG_form_name ) );
	IICGoeeEndEle(CG_LAST);
	IICGosdStructEnd();
    }

    /*
    ** Generate FRMALIGN structure.
    */
    STprintf( buf, ERx("IIfrmalign_%d"), CG_fid );
    IICGostStructBeg(ERx("static FRMALIGN"), buf,  0, CG_INIT);
    IICGiaeIntArrEle((i4)CG_frmalign->fe_version, CG_NOTLAST);
    IICGiaeIntArrEle((i4)CG_frmalign->fe_i1_align, CG_NOTLAST);
    IICGiaeIntArrEle((i4)CG_frmalign->fe_i2_align, CG_NOTLAST);
    IICGiaeIntArrEle((i4)CG_frmalign->fe_i4_align, CG_NOTLAST);
    IICGiaeIntArrEle((i4)CG_frmalign->fe_f4_align, CG_NOTLAST);
    IICGiaeIntArrEle((i4)CG_frmalign->fe_f8_align, CG_NOTLAST);
    IICGiaeIntArrEle((i4)CG_frmalign->fe_char_align, CG_NOTLAST);
    IICGiaeIntArrEle((i4)CG_frmalign->fe_max_align, CG_NOTLAST);
    IICGiaeIntArrEle((i4)CG_frmalign->afc_veraln.afc_hi_version, CG_NOTLAST);
    IICGiaeIntArrEle((i4)CG_frmalign->afc_veraln.afc_lo_version, CG_NOTLAST);
    IICGiaeIntArrEle((i4)CG_frmalign->afc_veraln.afc_max_align, CG_LAST);
    IICGosdStructEnd();

    _VOID_ IIORcnConstNums(&IICGirbIrblk, &num_ints, &num_flts, &num_strs,
		&num_hexs, &num_decs);
    /*
    ** Generate array of integer constants.
    */
    if (num_ints > 0)
    {
	STprintf( buf, ERx("IICints_%d"), CG_fid );
	IICGostStructBeg(ERx("static i4"), buf, num_ints, CG_INIT);
	for (i = 1; i <= num_ints; i++)
	{
	    _VOID_ IIORcqConstQuery(&IICGirbIrblk, i, DB_INT_TYPE,
				&constval, (i4 *)NULL
	    );
	    IICGiaeIntArrEle((i4) *((i4 *) constval),
			(i == num_ints) ? CG_LAST : CG_NOTLAST
	    );
	}
	IICGosdStructEnd();
    }

    /*
    ** Generate array of float constants, which are returned from the
    ** ILRF as strings.
    */
    if (num_flts > 0)
    {
	STprintf( buf, ERx("IICflts_%d"), CG_fid );
	IICGostStructBeg(ERx("static f8"), buf, num_flts, CG_INIT);
	for (i = 1; i <= num_flts; i++)
	{
	    _VOID_ IIORfcsFltConstStr(&IICGirbIrblk, i, &constval);
	    IICGfaeFltArrEle(constval, (i == num_flts) ? CG_LAST : CG_NOTLAST);
	}
	IICGosdStructEnd();
    }

    /*
    ** Generate static declarations for string constants.
    */
    for (i = 1; i <= num_strs; i++)
    {
	STprintf( buf, ERx("IICst%d_%d"), (i - 1), CG_fid );
	IICGostStructBeg(ERx("static char"), buf, 0, CG_ZEROINIT);
	_VOID_ IIORcqConstQuery(&IICGirbIrblk, i, DB_CHR_TYPE,
					&constval, (i4 *)NULL);
	IICGosbStmtBeg();
	IICGpscPrintSconst(constval, STlength(constval));
	IICGoeeEndEle(CG_LAST);
	IICGosdStructEnd();
    }

    /*
    ** Generate static declarations for hex constants.
    */
    for (i = 1; i <= num_hexs; i++)
    {
	i4	hexlen;

	_VOID_ IIORhxcHexConst(&IICGirbIrblk, i, &hexlen, &constval);
	IICGhxcHexConst(hexlen, constval, (i - 1));
    }

    /*
    ** Generate static declarations for decimal constants.
    */
    for (i = 1; i <= num_decs; i++)
    {
	i2	ps;
	char	*strval;

	_VOID_ IIORdccDecConst(&IICGirbIrblk, i, &ps, &strval, &constval);
	IICGdccDecConst(ps, strval, constval, (i - 1));
    }
    return;
}

/*
** Name:	ProcInit() -	Initialize the vars for the main proc
**
** Description:
**	Initializes the local variables for the main procedure of the file.
**
** Code generated:
**
**	IIrvalue = NULL;
**	IIbrktype = 0;
**	MEfill(sizeof(IIdata_area), '\0', _PTR_ IIdata_area);
**#ifdef local_proc
**	IIARccdCopyCallersDbdvs( rtsfparam,
**		&IIdbdvs[Rtn_dbdv_off],
**		Rtn_vdesc_size,
**		IIdbdv_save );
**#endif
**	for (IIi = 0; IIi < Rtn_vdesc_size; IIi++)
**	{
**		register IL_DB_VDESC	*iidbv =
**					&IIDB_<symbol-name>[IIi+Rtn_vdesc_off];
**		register DB_DATA_VALUE	*dbv = &IIdbdvs[IIi+Rtn_dbdv_off];
**		dbv->db_datatype = iidbv->db_datatype;
**		dbv->db_length = iidbv->db_length;
**		dbv->db_prec = iidbv->db_prec;
**		dbv->db_data = _PTR_((char *)IIdata_area + iidbv->db_offset);
**	}
**	if (IIARhf2HidFldInitV2(&IIfdesc, IIdbdvs, CG_static) != 0)
**	{
**		IIARrvlReturnVal(IIrvalue, rtsfparam, form-name, ERx("frame"));
**		return(NULL);
**	}
**
** History:
**	??/87 (agh) -- Written.
**	11/89 (jhw) -- Moved hidden field initialization here.
**	01/13/91 (emerson)
**		Replace call to IIARhfiHidFldInit by call to 
**		IIARhf2HidFldInitV2 for bug 34840.  (The difference is that
**		IIARhf2HidFldInitV2 takes a third parameter indicating 
**		whether the frame is static).
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Initialize only the portion of IIdbdvs that's relevant
**		to the procedure being initialized.
**		If it's a local procedure, save off that portion first.
**		Get number of DBDVs from Rtn_vdesc_size
**		instead of passed-in parm; get stack size from Rtn_stacksize
**		instead of calling the now-obsolete IIORdsDataSize.
*/
static VOID
ProcInit ()
{
    char nbuf[CGBUFSIZE];

    /*
    ** Initialize (DB_DATA_VALUE *) for return value, and variable
    ** indicating type of break (via 'return' or 'exit') from this frame.
    */
    IICGovaVarAssign(ERx("IIrvalue"), ERx("NULL"));
    IICGovaVarAssign(ERx("IIbrktype"), _Zero);
    IICGovaVarAssign(ERx("IIelt.db_length"), _Zero);
    IICGovaVarAssign(ERx("IIelt.db_prec"), _Zero);
    CVna((i4) DB_DMY_TYPE, nbuf);
    IICGovaVarAssign(ERx("IIelt.db_datatype"), nbuf);

    IICGovaVarAssign(ERx("IImode[0]"), ERx("'\\0'"));

    if (Rtn_vdesc_size > 0)
    { /* Fields or temporaries. */
	char	buf[CGBUFSIZE];

	if (CG_static)
	{
		IICGoibIfBeg();
		IICGoicIfCond( ERx("first"), CGRO_EQ, _One );
		IICGoikIfBlock();
	}

	if (Rtn_stacksize > 0)
	{ /* Clear data area */
	    IICGosbStmtBeg();
	    IICGocbCallBeg(ERx("memset"));
	    IICGocaCallArg(ERx("_PTR_ IIdata_area"));
	    IICGocaCallArg(ERx("'\\0'"));
	    IICGocaCallArg(ERx("sizeof(IIdata_area)"));
	    IICGoceCallEnd();
	    IICGoseStmtEnd();
	}

	/*
	** If this is a local procedure, save off the portion of
	** the dbdv array that we're about to overlay
	** and adjust pointers in ABRTSPRM.
	*/
	if (Rtn_tot_dbdvs < 0)
	{
		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IIARccdCopyCallersDbdvs"));
		IICGocaCallArg(ERx("rtsfparam"));
		IICGocaCallArg( STprintf(buf, ERx("&IIdbdvs[%d]"),
					 Rtn_dbdv_off) );
		IICGocaCallArg( STprintf(buf, ERx("%d"), Rtn_vdesc_size) );
		IICGocaCallArg(ERx("IIdbdv_save"));
		IICGoceCallEnd();
		IICGoseStmtEnd();
	}

	/*
	** Set local DB_DATA_VALUE array values by setting the type
	** description elements and calculate the 'db_data' elements
	** of the local DB_DATA_VALUE array.
	*/
	IICGofbForBeg();
	IICGofxForExpr(ERx("IIi = 0"));
	IICGofxForExpr(STprintf(buf, ERx("IIi < %d"), Rtn_vdesc_size));
	IICGofxForExpr(ERx("++IIi"));
	IICGofkForBlock();
	    IICGovaVarAssign(ERx("register IL_DB_VDESC\t*iidbv"),
			STprintf(buf, ERx("&IIDB_%d[IIi+%d]"), CG_fid,
				Rtn_vdesc_off)
	    );
	    IICGovaVarAssign(ERx("register DB_DATA_VALUE\t*dbv"),
			STprintf(buf, ERx("&IIdbdvs[IIi+%d]"), Rtn_dbdv_off)
	    );

	    IICGovaVarAssign(ERx("dbv->db_datatype"),ERx("iidbv->db_datatype"));
	    IICGovaVarAssign(ERx("dbv->db_length"), ERx("iidbv->db_length"));
	    IICGovaVarAssign(ERx("dbv->db_prec"), ERx("iidbv->db_prec"));
	    IICGovaVarAssign(ERx("dbv->db_collID"), ERx("0"));
	    if (Rtn_stacksize > 0)
		IICGovaVarAssign(ERx("dbv->db_data"),
			ERx("_PTR_((char *)IIdata_area + iidbv->db_offset)")
		);
	IICGofxForEnd();

	/*
	** Generate call to initialize all hidden fields' DB_DATA_VALUEs
	** to the "empty" value for their datatype.
	*/

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIARhf2HidFldInitV2"));
	IICGocaCallArg(ERx("&IIfdesc"));
	IICGocaCallArg(ERx("IIdbdvs"));
	IICGocaCallArg(CG_static ? ERx("(i4) 1") : ERx("(i4) 0"));
	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_NE, _Zero );
	IICGoikIfBlock();
		_genReturn( CG_form_name == NULL
				? ERx("\"procedure\"") : ERx("\"frame\"")
		);
	IICGobeBlockEnd();

	if (CG_static)
	{
		IICGobeBlockEnd();
	}

    }
    return;
}

/*{
** Name:	IICGpeProcEnd() -	Output the end of a frame or procedure
**
** Description:
**	Outputs the end of a frame or procedure.
**
** Code generated:
**	1) For an OSL frame:
**
**	IIendfrm();
**#ifdef CG_static
**	first = 0;
**#endif
**	if (FEinqferr() != 0)
**	{
**		IIARrvlReturnVal(IIrvalue, rtsfparam, form-name, "frame");
**		return(NULL);
**	}
**	IIARfclFrmClose(rtsfparam, IIdbdvs);
**	if (IIbrktype == 1)
**	{
**		IIARrvlReturnVal(IIrvalue, rtsfparam, frame-name, "frame");
**		return(NULL);
**	}
**	else if (IIbrktype == 2)
**	{
**		abrtsexit();
**	}
**	IIARrvlReturnVal(IIrvalue, rtsfparam, frame-name, ERx("frame"));
**	return(NULL);
**    }
**
**	2) For an OSL procedure:
**
**	label-for-close-of-file: ;
**	IIARpclProcClose(rtsfparam, IIdbdvs);
**	if (IIbrktype == 1)
**	{
**		IIARrvlReturnVal(IIrvalue, rtsfparam, "<calledp>", "procedure");
**		return(NULL);
**	}
**	else if (IIbrktype == 2)
**	{
**		abrtsexit();
**	}
**	IIARrvlReturnVal(IIrvalue, rtsfparam, "<calledp>", "procedure");
**	return(NULL);
**    }
**
** History:
**	04/89 (jhw) -- Removed test for "IIbrktype == 0"; code now generated
**		in 'IICGmEndMenu()' ('IIchkfrm()') should cover this case.
**	07/90 (jhw) -- Moved static "first" clear here.  This guarantees that
**		it is cleared before the frame is left for the first time, but
**		not sooner than it need be.  Bug #30608.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Renamed from IICGfleFileEnd to IICGpeProcEnd,
**		because it's now called at the end of each frame or procedure
**		in the source file, not just at the end of the source file.
*/
VOID
IICGpeProcEnd ()
{
    register char	*object;

    object = CG_form_name == NULL ? ERx("\"procedure\"") : ERx("\"frame\"");

    /*
    ** If current stack frame has a formname, then we are within
    ** an OSL frame rather than an OSL procedure.
    */
    if (CG_form_name == NULL)
    {
	IICGpxlPrefixLabel(CGL_FDEND, CGLM_FORM);

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIARpclProcClose"));
    }
    else
    {
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIendfrm"));
	IICGoceCallEnd();
	IICGoseStmtEnd();

	if (CG_static)
	{
		IICGovaVarAssign(ERx("first"), _Zero);
	}

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("FEinqferr"));
	IICGoceCallEnd();
	IICGoicIfCond((char *) NULL, CGRO_NE, ERx("0"));
	IICGoikIfBlock();
	    _genReturn(object);
	IICGobeBlockEnd();

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIARfclFrmClose"));
    }

    IICGocaCallArg(ERx("rtsfparam"));
    IICGocaCallArg(ERx("IIdbdvs"));
    IICGoceCallEnd();
    IICGoseStmtEnd();

    IICGoibIfBeg();
    IICGoicIfCond(ERx("IIbrktype"), CGRO_EQ, ERx("1"));
    IICGoikIfBlock();
	_genReturn(object);
    IICGobeBlockEnd();
    IICGoeiElseIf();
    IICGoicIfCond(ERx("IIbrktype"), CGRO_EQ, ERx("2"));
    IICGoikIfBlock();
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("abrtsexit"));
	IICGoceCallEnd();
	IICGoseStmtEnd();
    IICGobeBlockEnd();
	_genReturn(object);
    IICGobeBlockEnd();

    if (CG_form_name != NULL)
	IICGlexLblExitBlk();
    return;
}

/*
** Name:	_genReturn() -	Generate Code to Return From a 4GL Object.
**
** Description:
**	Generate C code for return from a 4GL object.  This includes setting
**	the default return value and then returning.
**
** Input:
**	object	{char *}  The 4GL object type type.
**
** Code generated:
**	IIARrvlReturnVal(IIrvalue, rtsfparam, "<CG_frame_name>"), object);
**	IIARhffHidFldFree(&IIfdesc, IIdbdvs);
**#ifdef local_proc
**	IIARccdCopyCallersDbdvs( rtsfparam,
**		&IIdbdvs[Rtn_dbdv_off],
**		Rtn_vdesc_size,
**		IIdbdv_save );
**#endif
**	return(NULL);
**
** History:
**	11/89 (jhw)  Abstracted.
**	11/89 (billc)  Added support for freeing complex objects.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		If we're returning from a local procedure,
**		restore the portion of the dbdv array we overlaid.
*/
static VOID
_genReturn ( object )
char	*object;
{
	char	buf[CGSHORTBUF];

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIARrvlReturnVal"));
	IICGocaCallArg(ERx("IIrvalue"));
	IICGocaCallArg(ERx("rtsfparam"));
	IICGocaCallArg(STprintf(buf, ERx("\"%s\""), CG_frame_name));
	IICGocaCallArg(object);
	IICGoceCallEnd();
	IICGoseStmtEnd();

	if ( !CG_static )
	{
		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IIARhffHidFldFree"));
		IICGocaCallArg(ERx("&IIfdesc"));
		IICGocaCallArg(ERx("IIdbdvs"));
		IICGoceCallEnd();
		IICGoseStmtEnd();
	}

	/*
	** If we're in a local procedure and we had dbdv's, restore the portion
	** of the dbdv array that we overlaid and adjust pointers in ABRTSPRM.
	*/
	if (Rtn_tot_dbdvs < 0 && Rtn_vdesc_size > 0)
	{
		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IIARccdCopyCallersDbdvs"));
		IICGocaCallArg(ERx("rtsfparam"));
		IICGocaCallArg(ERx("IIdbdv_save"));
		IICGocaCallArg( STprintf(buf, ERx("%d"), Rtn_vdesc_size) );
		IICGocaCallArg( STprintf(buf, ERx("&IIdbdvs[%d]"),
					 Rtn_dbdv_off) );
		IICGoceCallEnd();
		IICGoseStmtEnd();
	}

	IICGortReturnStmt();
}

/*{
** Name:	IICGdgDbdvGet() -	Find the VDESC corresponding to a DBDV
**
** Description:
**	Given a DBDV number from an IL reference in the current frame, proc,
**	or local proc: return the address of the coresponding VDESC.
**	This involves searching the IL statements for the current procedure
**	and its ancestors until the procedure containing the DBDV is found.
**	See the description of IL_MAINPROC and IL_LOCALPROC in IICGpgProcGen.
**
** Inputs:
**	val	{nat}  The 1-relative index of the DBDV.
**
** Returns:
**	The address of the coresponding VDESC.
**
** History:
**	04/07/91 (emerson)
**		Created for local procedures.
*/
IL_DB_VDESC *
IICGdgDbdvGet( val )
i4		val;
{
	IL	*proc_stmt, parent_sid;
	i4	dbdv_off, vdesc_off;

	val -= 1;
	for ( proc_stmt = Rtn_proc_stmt; ; )
	{
		if ( ( *proc_stmt & ILM_MASK ) != IL_LOCALPROC )
		{
			dbdv_off = 0;
			vdesc_off = 0;
			break;
		}
		dbdv_off = ILgetOperand( proc_stmt, 6 );
		if ( val >= dbdv_off )
		{
			vdesc_off = ILgetOperand( proc_stmt, 5 );
			break;
		}
		parent_sid = ILgetOperand( proc_stmt, 8 );
		proc_stmt = IICGesEvalSid( proc_stmt, parent_sid );
	}
	return &CG_frm_dbd[ val - dbdv_off + vdesc_off ];
}
