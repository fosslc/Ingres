/*
**  omenu.h
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "%W%	%G%";
*/

# include	<ft.h>

struct omenuType {
	char		*mu_prline;
	struct ocom_tab	*mu_coms;
	i4		mu_flags;
	FTINTRN		*mu_prompt;
	FTINTRN		*mu_window;
};

struct ocom_tab {
	char	*ct_name;
	i4	(*ct_func)();
	i4	ct_enum;
};

typedef struct omenuType 	*OMENU;

# ifdef OLD_MENU
# define	NULLRET		1
# define	INPUT		2
# endif /* OLD_MENU */
