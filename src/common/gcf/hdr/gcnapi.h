/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name:	gcnapi.h
**
** Description:
**	Contains prototype for the II_GCNapi_ModifyNode API function for adding
**	deleting or editing a vnode.
##
## History:
##	05-oct-98 (mcgem01)
##	    Created for the Unicenter team.
##	05-mar-99 (devjo01)
##	    Formally incorporated into Ingres.
##	04-may-1999 (mcgem01)
##	    Add a Virtual Node test function.
##	14-sep-1999 (somsa01)
##	    Slightly changed comment form so that the history will be stripped.
##      22-may-2000 (rodjo04)
##         Created II_GCNapi_Node_Exists() for the Unicenter team.
*/

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

extern int II_GCNapi_TestNode (
        char *          virtualNode
);

extern bool II_GCNapi_Node_Exists (
        char *          virtualNode
);

