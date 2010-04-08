#include <compat.h>
#include <descrip.h>
#include <iledef.h>
#include <lnmdef.h>
#include <ssdef.h>
#include <psldef.h>
#include <er.h>
#include <st.h>

#include <logicals.h>

/*
** History:
**	??-???-1989 (eduardo)
**		Created.
**	09-Jul-1990 (owen)
**		Use angle brackets on sep header files in order to search
**		for them in the default include search path.
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**	20-jan-1994 (donj)
**	    Changed NMsetenv() and NMdelenv() to have only one exit point.
**	    Make tabledesc common since it's used in both functions. Also,
**	    created a function, NMcreate_table() void function to create
**	    the SEP_TABLE logical name table if it doesn't exist.
**	13-mar-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from testtool code as the use is no longer allowed
**	09-sep-2003 (abbjo03)
**	    Use OS headers provided by HP.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
*/

    $DESCRIPTOR(tabledesc,ERx("SEP_TABLE")) ;
    void                   NMcreate_table () ;

/*
**  NMsetenv
*/

STATUS
NMsetenv(name,value)
char *name;
char *value;
{
    STATUS                 ret_val = OK ;
    i4                     syserr ;
    i4                     len ;
    unsigned char          acmode = PSL$C_SUPER ;
    char                   logname [80] ;
    char                   logvalue [80] ;
    ILE3		   lnm_item_list[2];
    bool                   try_again = TRUE ;
    
    $DESCRIPTOR(logdesc,ERx("NOTHING")) ;

    if ((name == NULL)||(value == NULL)||(*name == '\0')||(*value == '\0'))
    {
	ret_val = FAIL;
    }
    else
    {
	STcopy(name,logname);    
	STcopy(value,logvalue);
	CVupper(logname);
	CVupper(logvalue);
	logdesc.dsc$w_length = STlength(logdesc.dsc$a_pointer = logname);

	lnm_item_list[0].ile3$w_length  = STlength(logvalue);    
	lnm_item_list[0].ile3$w_code  = LNM$_STRING;    
	lnm_item_list[0].ile3$ps_bufaddr = logvalue;    
	lnm_item_list[0].ile3$ps_retlen_addr = &len;  
	lnm_item_list[1].ile3$w_length  = 0;    
	lnm_item_list[1].ile3$w_code  = 0;

	while (try_again)
	{
	    switch (syserr = sys$crelnm(0,&tabledesc,&logdesc,&acmode,lnm_item_list))
	    {
		case SS$_NORMAL:
		case SS$_SUPERSEDE:
					try_again = FALSE;
					break;
		case SS$_NOLOGTAB:
					NMcreate_table();
					break;
		default:
					lib$signal(syserr);
					break;
	    }
	}
    }
    return (ret_val);
}

/*
**  NMdelenv
*/

STATUS
NMdelenv(name)
char *name;
{
    i4                     syserr ;
    unsigned char          acmode = PSL$C_SUPER ;
    STATUS                 ret_val = OK ;
    char                   logname [80] ;

    $DESCRIPTOR(logdesc,ERx("NOTHING"));
    
    if ((name == NULL)||(*name == '\0'))
    {
	ret_val = FAIL;
    }
    else
    {
	STcopy(name,logname);    
	CVupper(logname);
	logdesc.dsc$w_length = STlength(logdesc.dsc$a_pointer = logname);

	syserr = sys$dellnm(&tabledesc,&logdesc,&acmode);
	if ((syserr != SS$_NORMAL)&&(syserr != SS$_NOLOGNAM))
	{
	    lib$signal(syserr);
	}
    }
    return (ret_val);
}

void
NMcreate_table()
{
    i4                     syserr ;
    unsigned char          acmode = PSL$C_SUPER ;
    char                   restable [32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                             0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } ;
    i4                     reslen = 0 ;
    short                  promsk = 0xffff ;

    $DESCRIPTOR(resnam,restable) ;
    $DESCRIPTOR(parent_table,ERx("LNM$PROCESS_DIRECTORY")) ;

    syserr = sys$crelnt(0,&resnam,&reslen,0,&promsk,&tabledesc,&parent_table,&acmode);
    if ((syserr != SS$_NORMAL)&&(syserr != SS$_LNMCREATED))
    {
	lib$signal(syserr);
    }
}
