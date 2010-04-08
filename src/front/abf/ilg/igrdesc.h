/*
**	Copyright (c) 1991, 2004 Ingres Corporation
*/

/**
** Name:    igrdesc.h -	OSL Interpreted Frame Object Generator
**				Internal Routine Descriptor Definition.
** Description:
**	Contains the structure definition for the internal ILG routine
**	descriptor passed between the allocation and statement generation
**	modules of the ILG ("igalloc.c" and "igstmt.c".)
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Created for local procedures.
*/

/*}
** Name:    IGRDESC -	Intermediate Language Routine Descriptor Structure.
**
** Description:
**	Defines the structure of the internal ILG routine descriptor.
**	A "routine" may be a local procedure or a "top" routine
**	(a frame or a non-local procedure).
**
**	Notes:
**
**	(1) all the offsets in the "_off" fields are zero-relative indices;
**	    they are zero for a top routine.
**	(2) all the offsets in the "_end" fields are zero-relative indices
**	    of the entry *following* the routine's last entry;
**	    put another way, the offsets in the "_end" fields are 1-relative
**	    indices of the routine's last entry.
*/
typedef struct {
	i2	igrd_fdesc_off;		/* offset into FDESC
					** of this routine's FDESC entries */
	i2	igrd_fdesc_end;		/* offset into FDESC of the end
					** of this routine's FDESC entries */
	i2	igrd_vdesc_off;		/* offset into IL_DB_VDESC array
					** of this routine's VDESC entries */
	i2	igrd_vdesc_end;		/* offset into VDESC array of the end
					** of this routine's VDESC entries */
	i2	igrd_dbdv_off;		/* offset into dbdv array
					** of this routine's dbdv entries */
	i2	igrd_dbdv_end;		/* offset into dbdv array of the end
					** of this routine's dbdv entries */
	ILREF	igrd_stacksize_ref;	/* int const: size of stack area
					** required for routine
					** (not including dbdv array) */
	ILREF	igrd_name_ref;		/* char const: name of routine
					** (0 for a top routine) */
	i2      igrd_dbdv_size;		/* for a top routine:
					** required size of dbdv array (maximum
					** igrd_dbdv_end for all routines);
					** for a local procedure: unused */
	IGSID	*igrd_sid;		/* SID of routine's IL_MAINPROC
					** or IL_LOCALPROC statement */
	IGSID	*igrd_parent_sid;	/* for a local procedure:
					** SID of parent routine's IL_MAINPROC
					** or IL_LOCALPROC statement;
					** for a top routine: NULL */
} IGRDESC;
