/*
**    Include file
*/

#include <compat.h>
#include <si.h>
#include <st.h>
#include <tc.h>
#include <lo.h>
#include <er.h>
#include <ex.h>
#include <termdr.h>

#define opncomm_c

#include <sepdefs.h>
#include <sepfiles.h>
#include <seperrs.h>
#include <fndefs.h>

/*
** History:
**	27-apr-1994 (donj)
**	    Created
**    17-may-1994 (donj)
**      VMS needs <si.h> for FILE structure declaration.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**  Reference global variables
*/

GLOBALREF    char          msg [] ;
GLOBALREF    char          SEPpidstr [] ;
GLOBALREF    char          SEPinname [] ;
GLOBALREF    char          SEPoutname [] ;
GLOBALREF    char         *ErrC ;
GLOBALREF    char         *ErrOpn ;

GLOBALREF    i4            tracing ;
GLOBALREF    i4            SEPtcsubs ;

GLOBALREF    STATUS        SEPtclink ;

GLOBALREF    LOCATION      SEPtcinloc ;
GLOBALREF    LOCATION      SEPtcoutloc ;

GLOBALREF    TCFILE       *SEPtcinptr ;
GLOBALREF    TCFILE       *SEPtcoutptr ;
GLOBALREF    FILE         *traceptr ;

GLOBALREF    bool          batchMode ;

#define disp_return(e,f,g)  \
 { disp_line(e,0,0); EXinterrupt(EX_ON); testGrade = g; return(f); }

STATUS
open_io_comm_files(bool display_it, STATUS tclink)
{
    /*
    **	    open I/O COMM-files	    **
    */
    char                  *getloc     = ERx("get location for ") ;
    char                  *incomfile  = ERx("input COMM-file") ;
    char                  *outcomfile = ERx("output COMM-file") ;

#ifdef hp9_mpe
    STprintf(SEPinname,ERx("in%s.sep"), SEPpidstr);
#else
    STprintf(SEPinname,ERx("in_%s.sep"),SEPpidstr);
#endif
    if (LOfroms(FILENAME & PATH,SEPinname,&SEPtcinloc) != OK)
    {
	STpolycat(3,ErrC,getloc,incomfile,msg);
	if (display_it)
	{
	    disp_return(msg,FAIL,SEPerr_Cant_Opn_Commfile);
	}
	else
	{
	    return_fail(msg,FAIL,SEPerr_Cant_Opn_Commfile);
	}
    }
    if (TCopen(&SEPtcinloc,ERx("w"),&SEPtcinptr) != OK)
    {
	STpolycat(2,ErrOpn,incomfile,msg);
	if (display_it)
	{
	    disp_return(msg,FAIL,SEPerr_Cant_Opn_Commfile);
	}
	else
	{
	    return_fail(msg,FAIL,SEPerr_Cant_Opn_Commfile);
	}
    }
#ifdef hp9_mpe
    STprintf(SEPoutname,ERx("out%s.sep"), SEPpidstr);
#else
    STprintf(SEPoutname,ERx("out_%s.sep"),SEPpidstr);
#endif
    if (LOfroms(FILENAME & PATH,SEPoutname,&SEPtcoutloc) != OK)
    {
	STpolycat(3,ErrC,getloc,outcomfile,msg);
	if (display_it)
	{
	    disp_return(msg,FAIL,SEPerr_Cant_Opn_Commfile);
	}
	else
	{
	    return_fail(msg,FAIL,SEPerr_Cant_Opn_Commfile);
	}
    }
    if (TCopen(&SEPtcoutloc,ERx("r"),&SEPtcoutptr) != OK)
    {
	STpolycat(2,ErrOpn,outcomfile,msg);
	if (display_it)
	{
	    disp_return(msg,FAIL,SEPerr_Cant_Opn_Commfile);
	}
	else
	{
	    return_fail(msg,FAIL,SEPerr_Cant_Opn_Commfile);
	}
    }

    /*
    **  initializations
    */

    SEPtcsubs = 0;
    SEPtclink = tclink;
    TCputc(TC_EOQ,SEPtcinptr);
    TCflush(SEPtcinptr);

    return (OK);
}
