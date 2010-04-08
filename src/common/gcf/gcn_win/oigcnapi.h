/*
** Copyright (c) 1998 Ingres Corporation
*/

/*
** Name:	oigcnapi.h
**
** Description:
**	Contains prototype for the II_GCNapi_ModifyNode API function for adding
**	deleting or editing a vnode.
**
** History:
**	05-oct-98 (mcgem01)
**	    Created for the Unicenter team.
**
*/

extern int II_GCNapi_ModifyNode (
	int		opcode, 
	int		entryType,
	char *		virtualNode, 
	char *		netAddress, 
	char *		protocol, 
	char *		listenAddress, 
	char *		username, 
	char *		password
);


/*
** Opcode values
*/

#define ADD_NODE		1
#define DELETE_NODE		2
  
/*
** Private or Global Entry
*/

#define GCN_PRIVATE_FLAG    0x0000
#define GCN_GLOBAL_FLAG    0x0001
