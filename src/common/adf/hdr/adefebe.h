/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ADEFEBE.H - ADE definitions dependent on FRONTEND or BACKEND.
**
** Description:
**      This file contains definitions of macros and constants that will differ
**      depending on whether the compiler is FRONTEND or BACKEND.  This
**	version of the file is for use by the
**
**			<<<<<    B A C K E N D    >>>>>
**
**				      or
**
**		       <<<<<    F R O N T E N D    >>>>>
**
** History: $Log-for RCS$
**      17-jul-86 (thurston)
**          Initial creation.
**	29-jul-86 (thurston)
**	    Converted the BACKEND version of the ADE_GET_OPERAND_MACRO and 
**	    ADE_SET_OPERAND_MACRO macros to be direct assignments of the
**	    structure elements instead of STRUCT_ASSIGN_MACRO's.
**	14-aug-86 (wong & thurston)
**	    Changed the BACKEND macros to be {...} blocks instead of (...)
**	    expressions.  Also, added the extra parameter "bases", to
**	    ADE_GET_OPERAND_MACRO.  This will be ignored by the BACKEND version,
**	    but will be needed by the FRONTEND version to look up the
**	    associated DB_DATA_VALUE in the DB_DATA_VALUE array pointed to by
**	    base[ADE_DVBASE].
**	06-may-87 (thurston & wong)
**	    Integrated FE changes.
**	13-may-87 (thurston)
**	    Changed the FRONTEND version of ADE_GET_OFFSET_MACRO() to return
**	    ADE_END_OFFSET_LIST instead of zero when it detects the end of the
**	    instruction list.
**	18-jul-89 (jrb)
**	    Changed macros for getting and setting operands to set prec fields
**	    in those operands to proper values.
**	20-dec-04 (inkdo01)
**	    Support opr_collID.
**/


/*
**  Defines of other constants and macros.
*/

/*
**      This specifies whether the file is for the BACKEND or FRONTEND:
*/

#ifndef   ADE_FRONTEND
#   define                 ADE_BACKEND
#endif /* ADE_FRONTEND */


/*
** Name: ADE_GET_OPERAND_MACRO() - Get ADE Operand Macro.
**       ADE_SET_OPERAND_MACRO() - Set ADE Operand Macro.
**
**      This defines the two MACRO definitions needed for the two different
**      internal operand structures (one set for BACKEND, one set for
**      FRONTEND).
**
**	Usage:
**	    ADE_GET_OPERAND_MACRO(bases, iop, op, opP)
**	    ADE_SET_OPERAND_MACRO(op, iop)
**
**		where:
**		    PTR			bases[];
**		    ADE_I_OPERAND	*iop;
**		    ADE_OPERAND		*op;
**		    ADE_OPERAND		**opP;
**
**		description:
**		    ADE_GET_OPERAND_MACRO() builds an external ADE_OPERAND
**		    structure at op from the internal ADE_I_OPERAND structure
**		    at iop.  ADE_SET_OPERAND_MACRO() builds an internal
**		    ADE_I_OPERAND structure at iop from the external ADE_OPERAND
**		    structure at op.  For the FRONTEND version of
**		    ADE_GET_OPERAND_MACRO, the array of PTRs (or base addresses)
**		    is needed to do this, since the internal representation of
**		    an operand for that version will merely be an offset into
**		    an array of DB_DATA_VALUEs pointed to by bases[AD_DVBASE].
**	13-Jan-2005 (jenjo02)
**	    Change ADE_GET_OPERAND_MACRO to point (ADE_OPERAND**)opP to 
**	    identical (ADE_I_OPERAND*)iop, on the backend. Quantify showed
**	    the copying of operands to the stack consumes 27% of the
**	    time in ade_execute_cx. This means that the operands are
**	    read-only; if any modifications to them are needed, the user
**	    must make a writable copy (see ade_execute_cx for examples).
**	    On FRONTEND, the operands are always made, though we'll
**	    set opP to point to it.
*/

#ifdef    ADE_BACKEND

# define ADE_GET_OPERAND_MACRO(bases,iop,op,opP) \
    *opP = (ADE_OPERAND*)iop;

# define ADE_SET_OPERAND_MACRO(op,iop) \
{\
    (iop)->opr_prec = (op)->opr_prec;\
    (iop)->opr_len = (op)->opr_len;\
    (iop)->opr_collID = (op)->opr_collID;\
    (iop)->opr_dt = (op)->opr_dt;\
    (iop)->opr_base = (op)->opr_base;\
    (iop)->opr_offset = (op)->opr_offset;\
}

#else  /* ADE_BACKEND */

# define ADE_GET_OPERAND_MACRO(bases,iop,op,opP) \
{\
    register DB_DATA_VALUE *dp = \
	&(((DB_DATA_VALUE *)((bases)[ADE_DVBASE]))[*(iop)]);\
    (op)->opr_prec = dp->db_prec;\
    (op)->opr_len = dp->db_length;\
    (op)->opr_dt = dp->db_datatype;\
    (op)->opr_base = ADE_DVBASE;\
    (op)->opr_offset = (char *)dp->db_data - (char *)(bases)[ADE_DVBASE];\
    *opP = op;\
}

# define ADE_SET_OPERAND_MACRO(op,iop) {*(iop) = (op)->opr_base;}

#endif /* ADE_BACKEND */


/*
** Name: ADE_SET_OFFSET_MACRO() - Set ADE Offset Macro.
**
** Description
**	Set next CX instruction offset.
**
** Input:
**	ip	{ADE_INSTRUCTION *}  Reference to current CX instruction.
**	offs	{longnat}  Next CX instruction offset.
*/

/*
** Name: ADE_GET_OFFSET_MACRO() - Get ADE CX Offset Macro.
**
** Description
**	Returns next CX instruction offset.
**
** Input:
**	cxhead	The CXHEAD for the current CX.
**	ip	{ADE_INSTRUCTION *}  Reference to current CX instruction.
**	offs	{longnat}  Current CX instruction offset.
**
** Returns:
**	{longnat}  Next CX instruction offset.
*/

#ifdef    ADE_BACKEND

# define ADE_SET_OFFSET_MACRO(ip, offs) {(ip)->ins_offset_next = (offs);}
# define ADE_GET_OFFSET_MACRO(ip, offs, loffs) ((ip)->ins_offset_next)

#else  /* ADE_BACKEND */

# define ADE_SET_OFFSET_MACRO(ip, offs) /* null macro! */
# define ADE_GET_OFFSET_MACRO(ip, offs, loffs) \
    ((offs) == (loffs) ? \
	ADE_END_OFFSET_LIST : \
	(offs) + sizeof(*(ip)) + (ip)->ins_nops * sizeof(ADE_I_OPERAND))

#endif /* ADE_BACKEND */
