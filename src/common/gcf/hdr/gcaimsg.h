/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: gcaimsg.h - Internal message definitions used only by GCA
**
** Description:
**      Definitions in this file are for GCA internal use only.
**
** History: $Log-for RCS$
**      20-Oct-1988 (jbowers)
**          Created.
**      01-Dec-1988 (berrow)
**          Split GCA_O_DESCR into GCA_TO_DESCR and GCA_UO_DESCR since tuple
**	    object descriptors need to be handled differently to object
**	    descriptors generated via user-defined ADT's.
**	07-Feb-90 (seiwald)
**	    New GCA_CS_BEDCHECK, for name server to probe registered servers.
**	25-Jan-93 (brucek)
**	    Moved GCA_CS_* to gca.h for better visability by other groups.
**	22-Mar-02 (gordy)
**	    Added GCN internal message and data object definitions.
*/

#ifndef GCAIMSG_H_INCLUDED
#define GCAIMSG_H_INCLUDED

/*
** GCC related internal message types
*/

#define		GCA_IT_DESCR	0x50	/* Deprecated */
#define		GCA_TO_DESCR	0x51	/* Tuple Object Descriptor */
#define		GCA_GOTOHET	0x52	/* Heterogeneous state transmission */


/*
** GCN related internal message types
*/

#define		GCN_NS_RESOLVE		0x40	/* Resolve target server */
#define		GCN_RESOLVED		0x41	/* GCN_NS_RESOLVE responce */
	     /* GCN_NS_OPER		0x42	   Name Server operation */
	     /* GCN_RESULT		0x43	   Operation result */
#define		GCN_NS_AUTHORIZE	0x44	/* Request remote user access */
#define		GCN_AUTHORIZED		0x45	/* GCN_NS_AUTHORIZE responce */
#define		GCN_NS_2_RESOLVE	0x46	/* Resolve - remote uid/pwd */
#define		GCN2_RESOLVED		0x47	/* GCN_NS_2_RESOLVE responce */


/*
** GCN_LCL_ADDR: local connection info.
*/
typedef struct _GCN_LCL_ADDR		GCN_LCL_ADDR;

struct _GCN_LCL_ADDR
{
    GCN_VAL	gcn_host;
    GCN_VAL	gcn_addr;
    GCN_VAL	gcn_auth;
};

/*
** GCN_RMT_ADDR - remote node info.
*/
typedef struct _GCN_RMT_ADDR		GCN_RMT_ADDR;

struct _GCN_RMT_ADDR
{
    GCN_VAL	gcn_host;
    GCN_VAL	gcn_protocol;
    GCN_VAL	gcn_port;
};

/*
** GCN_RMT_DATA
*/
typedef struct _GCN_RMT_DATA		GCN_RMT_DATA;

struct _GCN_RMT_DATA
{
    i4		gcn_data_type;

#define		GCN_RMT_DB		1	/* Database[/class] */
#define		GCN_RMT_USR		2	/* User alias */
#define		GCN_RMT_PWD		3	/* Password */
#define		GCN_RMT_AUTH		4	/* Authentication */
#define		GCN_RMT_EMECH		5	/* Encryption mechanism */
#define		GCN_RMT_EMODE		6	/* Encryption mode */
#define		GCN_RMT_MECH		7	/* Authentication mechanism */

    GCN_VAL	gcn_data_value;
};

/*
** Data object for message type GCN_NS_RESOLVE
*/
typedef struct _GCN_D_NS_RESOLVE GCN_D_NS_RESOLVE;

struct _GCN_D_NS_RESOLVE
{
    GCN_VAL	gcn_install;
    GCN_VAL	gcn_uid;       /* this is local user id */
    GCN_VAL	gcn_ext_name;
    GCN_VAL	gcn_user;
    GCN_VAL	gcn_passwd;    
};

/*
** Data object for message type GCN_NS_2_RESOLVE
*/
typedef struct _GCN_D_NS_2_RESOLVE	GCN_D_NS_2_RESOLVE;

struct _GCN_D_NS_2_RESOLVE
{
    GCN_VAL	gcn_install;
    GCN_VAL	gcn_uid;       /* this is local user id */
    GCN_VAL	gcn_ext_name;
    GCN_VAL	gcn_user;
    GCN_VAL	gcn_passwd;    
    GCN_VAL	gcn_rem_username;
    GCN_VAL	gcn_rem_passwd;    
};

/*
** Data object for message type GCN_RESOLVED 
** (prior to GCA_PROTOCOL_LEVEL_40)
*/
typedef struct _GCN_D_RESOLVED GCN_D_RESOLVED;

struct _GCN_D_RESOLVED
{
    GCN_RMT_ADDR	gcn_rmt_addr;
    GCN_VAL		gcn_username;
    GCN_VAL		gcn_password;
    GCN_VAL		gcn_rmt_dbname;
    GCN_VAL		gcn_server_class;
    i4			gcn_lcl_count;
    GCN_VAL		gcn_lcl_addr[1];
};

/*
** Data object for message type GCN_RESOLVED 
** (GCA_PROTOCOL_LEVEL_40 and up)
*/
typedef struct _GCN1_D_RESOLVED	 GCN1_D_RESOLVED;

struct _GCN1_D_RESOLVED
{
    i4			gcn_rmt_count;
    GCN_RMT_ADDR	gcn_rmt_addr[1];
    GCN_VAL		gcn_username;
    GCN_VAL		gcn_password;
    GCN_VAL		gcn_rmt_dbname;
    GCN_VAL		gcn_server_class;
    i4			gcn_lcl_count;
    GCN_VAL		gcn_lcl_addr[1];
};

/*
** Data object for message type GCN2_RESOLVED 
*/
typedef struct _GCN2_D_RESOLVED	 GCN2_D_RESOLVED;

struct _GCN2_D_RESOLVED
{
    GCN_VAL		gcn_lcl_user;
    i4			gcn_l_local;
    GCN_LCL_ADDR	gcn_local[1];
    i4			gcn_l_remote;
    GCN_RMT_ADDR	gcn_remote[1];
    i4			gcn_l_data;
    GCN_RMT_DATA	gcn_data[1];
};

/*
** Data object for message type GCN_NS_AUTHORIZE
*/
typedef struct _GCN_D_NS_AUTHORIZE GCN_D_NS_AUTHORIZE;

struct _GCN_D_NS_AUTHORIZE
{
    i4		gcn_auth_type;
    GCN_VAL	gcn_username;
    GCN_VAL	gcn_client_inst;
    GCN_VAL	gcn_server_inst;
    i4		gcn_rq_count;
};

/*
** Data object for message type GCN_AUTHORIZED
*/
typedef struct _GCN_D_AUTHORIZED GCN_D_AUTHORIZED;

struct _GCN_D_AUTHORIZED
{
    i4		gcn_auth_type;
    i4		gcn_l_tickets;
    GCN_VAL	gcn_tickets[1];
};

#endif

