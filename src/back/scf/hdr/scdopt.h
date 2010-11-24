/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: scdopt.h - sundry options for scd_options()
**
** Description:
**	Definition of SCD_CB.
**
** History:
**	1-Sep-93 (seiwald)
**	    Written.
**	6-jan-94 (robf)
**          Add new secure 2 values
**	24-apr-94 (cohmi01)
**	    Add items for iomaster server: writealong and readahead.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**          Added mllist for must log DB list
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Prototype scd_options()
**/

/*
** Name: SCD_CB - scd_options() control block
**
** Description:
**	scd_options() takes as parameters pointers to just about every
**	facility's control block.  Still, some of the options set by
**	scd_options() don't show up in these control blocks.  This
**	control holds the various orphaned options.
*/

typedef struct _SCD_CB SCD_CB ;

struct _SCD_CB {
    i4	max_dbcnt;	/* Sets DMC_C_DATABASE_MAX */
    i4	qef_data_size;	/* QEF Default - Used to compute a value in qef_cb */
    i4	num_logwriters;	/* Used all over scd_initiate() */
    i4	start_gc_thread;/* Used to indicate starting group-commit */
    i4	cp_timeout;	/* USed all over scd_initiate() */
    i4	sharedcache;	/* Used to set dmc.cache_name - cleared for star */
    char *cache_name;	/* Sets dmc.cache_name if sharedchache */
    char *server_class; /* gcr_cb */
    int	num_databases;	/* NIGHTMARE - remove */
    char *dblist;	/* New to scd_options */
    char *mllist;	/* Must Log DB List */
    bool c2_mode;	/* C2 security mode */
    char *secure_level; /* Security level */
    char *local_vnode;	/* local node name for errlog.log, star */
    i4  num_writealong; /* number of writealong threads, for IOMASTER only */
    i4  num_readahead;  /* number of readahead  threads, for IOMASTER only */
    i4  num_ioslaves;   /* number of slaves, for IOMASTER only */
} ;

FUNC_EXTERN STATUS scd_options(
	DMC_CHAR_ENTRY  *dca,
	i4		*dca_index,
	OPF_CB		*opf_cb,
	RDF_CCB		*rdf_ccb,
	DMC_CB          *dmc,
	PSQ_CB		*psq_cb,
	QEF_SRCB	*qef_cb,
	QSF_RCB		*qsf_rcb,
	SXF_RCB		*sxf_rcb,
	SCD_CB		*scd_cb );
