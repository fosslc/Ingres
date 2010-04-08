/*
 *	Copyright (c) 1995, 1998 Ingres Corporation
 *
 *	Name:
 *		idcl.h
 *
 *	Function:
 *		None
 *
 *	Arguments:
 *		None
 *
 *	Result:
 *		defines global symbols for ID module
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *
 *	22-may-95 (emmag)
 *	    These fucntions don't have any real meaning on NT, but must
 *	    be defined for the product to be built!
 *	27-oct-1998 (canor01)
 *	    Add definitions for UUID functions.
 *	07-nov-1998 (canor01)
 *	    Remove duplicate declaration of IDname().
 *      20-nov-1998 (canor01)
 *          Remove definition of IDuuid_compare for NT. Use function instead.
 *	10-Feb-2005 (raodi01)
 *	    Added definition of IDuuid_create_seq to generate MAC based uuids 
 *
 */
# include <rpc.h>
# include <rpcndr.h>
# include <rpcdce.h>

#define IDuser(x) (1)
#define IDgroup(x) (1)

# define IDuuid_create		UuidCreate
# define IDuuid_create_seq	UuidCreateSequential
