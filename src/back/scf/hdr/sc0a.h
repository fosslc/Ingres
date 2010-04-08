/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: sc0a.h - Function definitions/prototypes for security auditing routines
**
** History:
**      18-nov-1992 (robf)
**          Created for C2
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/
DB_STATUS sc0a_flush_audit(SCD_SCB *scb, i4 *cur_error);

DB_STATUS sc0a_write_audit (
SCD_SCB 	    *scb,	  /* SCF info */
SXF_EVENT           type,         /* Type of event */
SXF_ACCESS          access,       /* Type of access */
char                *obj_name,    /* Object name */
i4		    obj_len,	  /* Length of object */
DB_OWN_NAME         *obj_owner,   /* Object owner */
i4             msgid,        /* Message identifier */
bool                force_write,   /* Force write to disk */
i4	    	    user_priv,	  /* User privilege mask */
DB_ERROR             *err_code     /* Error info */
) ;


DB_STATUS sc0a_qrytext(
SCD_SCB	*scb,
char	*qrytext,
i4	qrysize,
i4	ops
#define SC0A_QT_START	0x01	/* This call is the first */
#define SC0A_QT_END	0x02	/* This call is the last */
#define SC0A_QT_MORE	0x04	/* This call has  more */
#define SC0A_QT_NEWREC  0x08	/* INTERNAL flag, start new record */
);

DB_STATUS sc0a_qrytext_data(
SCD_SCB 	*scb,
i4		num_param,
DB_DATA_VALUE	**param
);

DB_STATUS sc0a_qrytext_param(
SCD_SCB 	*scb,
char		*name,
i4		name_len,
DB_DATA_VALUE  *param
);


