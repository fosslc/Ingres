/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: SXAP.H - data definitions SXF physical audit file access routines
**
** Description:
**	This file contains the typdefs for internal data structures
**	used by the low level SXF audit file access routines. It defines
**	the interface with the physical audit fie access routines.
**	
**	The following data structures are defined:-
**
** History:
**	10-aug-1992 (markg)
**	   Initial creation.
**      31-dec-1992 (mikem)
**         su4_us5 port.  Fixed GLOBALREF of Sxap_itab to match actual
**         declaration (it is declared READONLY).
**	15-jan-1993 (markg)
**	    Removed READONLY definition for Sxap_itab, because this
**	    breaks on some ANSI compilers where READONLY is defined
**	    as const.
**	2-aug-93 (robf)
**	    Add sxap_alter entry point
**      11-jan-94 (stephenb)
**          Added new element to SXAP_ITAB for operating system auditing.
**	14-apr-94 (robf)
**          Added SXAP_C_AUDIT_WRITER alter operation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
** Name: SXAP_ITAB - audit file access routine initilalization table
**
** Description:
**	This typedef defines the structure of the SXAP initialization
**	table. The table contains an array of function pointers used to 
**	initialize the SXAP_VECTOR array described below. Each entry on
**	this table will be a pointer to the initialization function of the
**	low level audit file access routines.
**
**	The function prototype definition for the startup routine for each
**	of the SXAP subsystem startup routines must also be declared here.
**
** History:
**	10-aug-92 (markg)
**	    Initial creation.
**	18-feb-93 (walt)
**	    Changed "GLOBALREF READONLY SXAP_ITAB" to "GLOBALCONSTREF SXAP_ITAB"
**	    because the Alpha VMS compiler says it's illegal to apply READONLY
**	    to GLOBALREF. 
**	16-mar-1993 (markg)
**	    Backed out Walt's change above.
**	2-aug-93 (robf)
**	    Add sxap_alter entry point
**      11-jan-94 (stephenb)
**          Added new element for operating system audititng (saxpo).
*/
# define	SXAP_DEFAULT		1
FUNC_EXTERN DB_STATUS sxap_startup(
    SXF_RSCB		*rscb,
    i4		*err_code);

# define        SXAP_OSAUDIT            2
FUNC_EXTERN DB_STATUS sxapo_startup(
    SXF_RSCB            *rscb,
	i4             *err_code);

# define	SXAP_ITAB_MAX		2

typedef DB_STATUS (*SXAP_ITAB[SXAP_ITAB_MAX + 1])(
    SXF_RSCB		*rscb,
    i4		*err_code);

GLOBALREF SXAP_ITAB  Sxap_itab;

/*
** Name: SXAP_VECTOR - audit file access routines vector
**
** Description:
**	This typedef contains an array of function pointers that are
**	used to call the low level audit file access routines. All calls 
**	to the low level audit file access routines must use this vector. 
**	
**
** History:
**	10-aug-92 (markg)
**	    Initial creation.
**	2-aug-93 (robf)
**	    Add sxap_alter entry point
*/
typedef struct _SXAP_VECTOR
{
    DB_STATUS	(*sxap_init)(
	SXF_RSCB	*rscb,
	i4		*err_code);

    DB_STATUS	(*sxap_term)(
	i4		*err_code);

    DB_STATUS	(*sxap_begin)(
	SXF_SCB		*scb,
	i4		*err_code);

    DB_STATUS	(*sxap_end)(
	SXF_SCB		*scb,
	i4		*err_code);

    DB_STATUS	(*sxap_open)(
	PTR		filename,
	i4		mode,
	SXF_RSCB	*rscb,
	i4		*filesize,
	PTR		stamp,
	i4		*err_code);

    DB_STATUS	(*sxap_close)(
	SXF_RSCB	*rscb,
	i4		*err_code);

    DB_STATUS	(*sxap_pos)(
	SXF_SCB		*scb,
	SXF_RSCB	*rscb,
	i4		pos_type,
	PTR		pos_key,
	i4		*err_code);

    DB_STATUS	(*sxap_read)(
	SXF_SCB		*scb,
	SXF_RSCB	*rscb,
	SXF_AUDIT_REC	*audit_rec,
	i4		*err_code);

    DB_STATUS	(*sxap_write)(
	SXF_SCB		*scb,
	SXF_RSCB	*rscb,
	SXF_AUDIT_REC	*audit_rec,
	i4		*err_code);

    DB_STATUS	(*sxap_flush)(
        SXF_SCB         *scb,
        i4         *err_code);

    DB_STATUS	(*sxap_show)(
	PTR		filename,
	i4		*max_file,
        i4         *err_code);


    DB_STATUS   (*sxap_alter) (
	SXF_SCB	        *scb,
	PTR		filename,
	i4		flags,
#define  SXAP_C_CHANGE_FILE	0x01
#define  SXAP_C_STOP_AUDIT	0x02
#define  SXAP_C_RESTART_AUDIT	0x04
#define  SXAP_C_AUDIT_WRITER	0x08
	i4		*err_code);

} SXAP_VECTOR;
GLOBALREF  SXAP_VECTOR  *Sxap_vector;
