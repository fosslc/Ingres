# ifdef VMS

/*
** LIBQSYS.H - Non-portable section of Libq written for VMS system only.
**
** History:	26-feb-1985 - Written to support the Equel Rewrite (ncg)
**
** Copyright (c) 2004 Ingres Corporation
*/

/* String descriptor routines */
char		*IIsd(), *IIsd_reg(), *IIsd_no(), *IIsd_notrim(), 
		*IIsd_keepnull();
void		IIsd_undo();
# endif /* VMS */
