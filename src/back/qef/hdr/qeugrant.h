/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: QEUGRANT.H - QEF data structures and definitions used in processing of
**		      GRANT statement
**
** Description:
**	This file contains QEF data structures and definitions used in
**	 processing of GRANT statement
**
**
** History:
**
**	25-aug-92 (andre)
**	    written
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**  Forward and/or External typedef/struct references.
*/
typedef struct _QEU_REM_PERMS		QEU_REM_PERMS;
typedef struct _QEU_NEWPERM_INFO	QEU_NEWPERM_INFO;

/*
** Name: QEU_REM_PERMS - describe permit to be removed because it will be
**			 subsumed by a newly created permit.
**
** Description:
**	When creating a new permit, we may find that one or more existing
**	permits represent a true subset of the new permit.  All such permits
**	will be removed, and this structure will contain information necessary
**	to remove one subsumed permit
**
** History:
**
**  09-jul-92 (andre)
**	created
*/
struct _QEU_REM_PERMS
{
    QEU_REM_PERMS	*qeu_next;
    i2			qeu_permno;	/* permit number */
};

/*
** Name: QEU_NEWPERM_INFO - this structure will describe permits which will
**			    be subsumed by newly created permit(s) + identify
**			    existing privilege descriptors describing privileges
**			    on which newly created permit(s) will depend.
**
** Description:
**  In the course of creating new permit(s), this structure will be used to keep
**  track of permits subsumed by the permit(s) to be created + of existing
**  (if any) privilege descriptors which describe privileges on which new
**  permit(s) depend.
**
** History:
**
**	09-jul-92 (andre)
**	    written
**	17-jul-92 (andre)
**	    added qeu_flags and defined QEU_SET_ALL_TO_ALL and
**	    QEU_SET_RETR_TO_ALL.  The new masks will be used by qeu_scan_perms()
**	    to communicate to its caller whether PUBLIC will posess ALL or
**	    RETRIEVE as a result of creation fo new privileges.  Note that if
**	    the masks will be set only if the already exsiting permit tuples did
**	    not convey these privileges to PUBLIC but the UNION of existing
**	    permits and of the newly created permits will.
*/
struct _QEU_NEWPERM_INFO
{
    QEU_REM_PERMS	*qeu_perms_to_remove;
#define	    QEU_DELETE_DESCR	    0
#define	    QEU_INSERT_DESCR	    1
#define	    QEU_UPDATE_DESCR	    2
#define	    QEU_NUM_DESCRS	    3
    u_i2		qeu_descr_nums[QEU_NUM_DESCRS];
    i4			qeu_flags;
#define	    QEU_SET_ALL_TO_ALL	    0x01
#define	    QEU_SET_RETR_TO_ALL	    0x02
};
