/*
** Copyright (c) 2004 Ingres Corporation
*/
/* static char	*Sccsid = "@(#)ablist.h	1.3  10/28/83"; */

/*
** list.h
** contains declaraions for list used throughout editor
*/

typedef	union
{
	char			*lt_vname;
	struct abfrmType	*lt_vfrm;
	char			*lt_vrec;
	struct abfileType	*lt_vfile;
	struct abprocType	*lt_vproc;
	struct abobjType	*lt_vobj;
	struct abdepType	*lt_vdep;
	struct abformType	*lt_vfo;
} ABLISTUNION;

struct ablistType
{
	struct ablistType	*lt_next;
	ABLISTUNION	 lt_var;
};

# define		lt_name		lt_var.lt_vname
# define		lt_form		lt_var.lt_vfo
# define		lt_frm		lt_var.lt_vfrm
# define		lt_rec		lt_var.lt_vrec
# define		lt_file		lt_var.lt_vfile
# define		lt_op		lt_var.lt_vop
# define		lt_proc		lt_var.lt_vproc
# define		lt_obj		lt_var.lt_vobj
# define		lt_dep		lt_var.lt_vdep

typedef struct ablistType	ABLIST;
	
# define	LNIL	((ABLIST *) 0)

# define	ablisnext(lt)	((lt)->lt_next)
# define	ablisnaddr(lt)	(&((lt)->lt_next))
