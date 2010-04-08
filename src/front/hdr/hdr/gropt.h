/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	flags for editor options
*/

#define EO_GRAF 1
#define EO_CONF 2
#define EO_EXPL 4
#define EO_TPLOT 8
#define EO_HRDRW 16

/*
**	options structure.
**
**	CAUTION!!! - if this structure is changed, parallel definition
**	KLUDGE_OPT in source file options.qc must also be changed.  If
**	this is not done, options and profile forms will be editting
**	with a wrong template, with possibly drastic results.
*/
typedef struct
{
	i4	id;		/* object (database) id */
	char *plot_type;	/* plot device type */
	char *plot_loc;		/* plot device location */
	char fc_char[2];	/* font comparison char (string for equel) */
	i2 lv_main;		/* main layout level */
	i2 lv_edit;		/* edit preview level */
	i2 lv_lyr;		/* layering preview level */
	i2 lv_plot;		/* plotting preview level */
	i2 dt_trunc;		/* data truncation length */
	u_i2 o_flags;		/* y/n type options */
} GR_OPT;

# include	<grmap.h>
