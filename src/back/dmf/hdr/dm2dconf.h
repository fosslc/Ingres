/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM2DCONF.H - Database configuration file format.
**
** Description:
**      This file describes the structure of the configuration file and
**	the types configuration records that reside in the configuration file.
**
** History: $Log-for RCS$
**      03-dec-1985 (derek)
**          Created new for Jupiter.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _DM2D_CNF    DM2D_CNF;
typedef struct _DM2D_DSC    DM2D_DSC;
typedef struct _DM2D_EXT    DM2D_EXT;
typedef struct _DM2D_TAB    DM2D_TAB;

/*}
** Name: DM2D_CNF - Configuration file configuration entry.
**
** Description:
**      This structure describes the structure of a configuration parameter
**	entry.  Currently there are none.
**
** History:
**     03-dec-1985 (derek)
**          Created new for Jupiter.
*/
typedef struct _DM2D_CNF
{
    i4         length;             /* Length of entry. */
    i4	    type;		/* Type of entry. */
#define			DM2D_T_CNF	1L
    i4	    cnf_id;		/* Configuration parameter id. */
#define			CNF_NONE	1L
    i4	    cnf_value;		/* Parameter value. */
};

/*}
** Name: DM2D_DSC - Configuration file database decription entry.
**
** Description:
**      This structure defines the basic information known about a
**	database by DMF.
**
** History:
**     03-dec-1985 (derek)
**          Created new for Jupiter.
*/
typedef struct _DM2D_DSC
{
    i4         length;             /* Length of entry. */
    i4         type;               /* Type of entry. */
#define                 DM2D_T_DSC      2L
    DB_DB_NAME	    dsc_name;		/* Name of the database. */
    DB_OWN_NAME	    dsc_owner;		/* Owner of the database. */
    i4	    dsc_type;		/* Type of database. */
#define			DSC_PRIVATE	1L
#define			DSC_PUBLIC	2L
#define			DSC_DISTRIBUTED	3L
    i4	    dsc_access_mode;	/* Database access mode. */
#define			DSC_READ	1L
#define			DSC_WRITE	2L
    i4	    dsc_c_version;	/* Version of DMF that createdd this
					** database. */
#define			DSC_PREHISTORIC	0L
#define			DSC_V1		1L
    i4	    dsc_version;	/* Current version. */
    i4	    dsc_create;		/* Creation date. */
    i4	    dsc_sysmod;		/* Sysmod date. */
    i4	    dsc_checkpoint;	/* Checkpoint date. */    
    u_i4       dsc_last_table_id;  /* Last table id used. */
};

/*}
** Name: DM2D_EXT - Configuration file extent entry.
**
** Description:
**      This structure describes the a configuration file extent entry.
**      The extent entries describes disk location that can be used to
**	locate tables.
**
** History:
**     03-dec-1985 (derek)
**          Created new for Jupiter.
*/
typedef struct _DM2D_EXT
{
    i4         length;             /* The length of the entry. */
    i4	    type;		/* The type of the entry. */
#define			DM2D_T_EXT	3L
    DMP_LOC_ARRAY   ext_location;	/* Location information. */
};

/*}
** Name: DM2D_TAB - Configuration file table entry.
**
** Description:
**      This structure describes the configuration file format for
**	a table description that is stored in the configuration file.
**      Currently the relation, attribute, indexes and relation_idx
**	are stored in the configuration file.  This is needed because
**	there is no way to construct the information with access path
**	information for the relation and attribute tables.
**
** History:
**     03-dec-1985 (derek)
**          Created new for jupiter.
*/
typedef struct _DM2D_TAB
{
    i4         length;	        /* Length of this entry. */
    i4	    type;		/* Type of entry. */
#define                 DM2D_T_TAB	4L
    DMP_RELATION    tab_relation;	/* Relation record. */
    DMP_ATTRIBUTE   tab_attribute[1];	/* Attribute records. */
};
