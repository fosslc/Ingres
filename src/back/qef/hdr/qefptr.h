/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: QEFPTR.H - Pointer coercion type.
**
** Description:
**      This file contains a pointer coercion type.
**
** History: $Log-for RCS$
**      30-sep-88 (carl)
**          written
**/


/*}
** Name: QEP_PTR_UNION - Pointer coercion type.
**
** Description:
**      This structure is used to coerce the PTR type to another without
**  upsetting LINT.
**      
[@comment_line@]...
**
** History:
**      30-sep-88 (carl)
**          written
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
[@history_line@]...
[@history_template@]...
*/

typedef struct _QEP_PTR_UNION
{
    union
    {
	PTR		 qep_1_ptr;		
	QED_DDL_INFO	*qep_2_ddl_info_p;		
	DMU_CB		*qep_3_dmu_cb_p;
	DMF_ATTR_ENTRY	**qep_4_dmu_attr_pp;
    }	qep_ptr_u;
}   QEP_PTR_UNION;

