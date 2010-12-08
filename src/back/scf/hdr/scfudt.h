/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	scfudt.h - externs for SCF's udt module.
**
** Description:
**	
**	2-Jul-1993 (daveb)
**	    created
**	05-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Add IIclsadt_register() prototype.
**/

FUNC_EXTERN STATUS IIudadt_register( ADD_DEFINITION **add_block,
				 ADD_CALLBACKS *callbacks );
FUNC_EXTERN STATUS IIclsadt_register( ADD_DEFINITION **add_block,
				 ADD_CALLBACKS *callbacks );
