/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPSCFLIBDATA
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <cs.h>
#include    <pc.h>
#include    <tm.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <scf.h>
#include    <gca.h>

#include    <sc.h>
#include    <sc0m.h>
#include    <scfcontrol.h>

/**
**
**  Name: SCMAINCB.C - Contains the core data structure for SCF
**
**  Description:
**      This file contains the definition (GLOBALDEF) for the central 
**      data structure in the System Control Facility.  It is placed 
**      in a file alone so as to facilitate the linking of other 
**      facilities without regard to which modules of scf are
**      included (i.e. for various flavors of module testing --
**      both inside and outside of SCF)
**
**          sc_main_cb - The core data structure of SCF
**          scf_prmain() - Prints the aforementioned data structure
**			    (May not be available in all versions)
**
**
**  History:    $Log-for RCS$
**      15-Aug-1986 (fred)
**          Created on Jupiter.
**      20-Jul-1992 (rog)
**          Included ddb.h for Sybil.
**	29-Jun-1993 (daveb)
**	    include <tr.h>
**	2-Jul-1993 (daveb)
**	    prototyped.
**	11-oct-93 (johnst)
**	    Bug #56449
**	    Changed TRdisplay format args from %x to %p where necessary to
**	    display pointer types. We need to distinguish this on, eg 64-bit
**	    platforms, where sizeof(PTR) > sizeof(int).
**	18-feb-1997 (hanch04)
**	    define for Sc_main_cb is moved to one line.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPSCFLIBDATA
**	    in the Jamfile.
[@history_template@]...
**/

/*
** Definition of all global variables owned by this file.
*/

#ifdef VMS
GLOBALDEF
    {"iiscf_cb"} noshare
	     SC_MAIN_CB *Sc_main_cb ZERO_FILL; /* Core DS of SCF */
#else
GLOBALDEF SC_MAIN_CB *Sc_main_cb ZERO_FILL; /* Core DS of SCF */
#endif /* VMS */

/*{
** Name: scf_prmain()	- Print the sc_main_cb DS
**
** Description:
**      This routine prints the mail SCF data structure SC_MAIN_CB.
**
** Inputs:
**      none
**
** Outputs:
**      none
**	Returns:
**	    VOID
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      15-Aug-1986 (fred)
**          Created on Jupiter.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
[@history_template@]...
*/
#ifdef xDEBUG
VOID
scf_prmain(void)
{
    i4	    i;
    i4	    *p;
    TRdisplay(">>> SC_MAIN_CB <<<\n");
    TRdisplay("\tsc_state: %w%<(%2x)\tsc_pid: %8x\n",
	"SC_UNINIT,SCINIT,SCSHUTDOWN", Sc_main_cb->sc_state,
	Sc_main_cb->sc_pid);
    TRdisplay("\tsc_iversion: %t\tsc_scf_version: %t\n",
	sizeof(Sc_main_cb->sc_iversion), &Sc_main_cb->sc_iversion,
	sizeof(Sc_main_cb->sc_scf_version), &Sc_main_cb->sc_scf_version);
    TRdisplay("\tsc_facilities %v%<(%x)\n",
	"invalid,CLF,ADF,DMF,PSF,OPF,QEF,QSF,RDF,SCF,ULF,invalid",
	    Sc_main_cb->sc_facilities);
    TRdisplay("\tsc_acc: %d.\tsc_nousers: %d.\tsc_sname: %t\n",
	Sc_main_cb->sc_acc, Sc_main_cb->sc_nousers,
	sizeof(Sc_main_cb->sc_sname), Sc_main_cb->sc_sname);
    TRdisplay("\tsc_mcb_list: %p\tsc_soleserver %w%<(%2x)\n",
	Sc_main_cb->sc_mcb_list,
	"DMC_S_NORMAL,DMC_S_SYSUPDATE,DMC_S_SINGLE,DMC_S_MULTIPLE",
	Sc_main_cb->sc_soleserver);
    TRdisplay("\tsc_kdbl: (n) %p (p) %p\n",
	Sc_main_cb->sc_kdbl.kdb_next, Sc_main_cb->sc_kdbl.kdb_prev);
    p = (i4 *) &Sc_main_cb->sc_adbytes;
    for (i = DB_MIN_FACILITY; i <= DB_MAX_FACILITY; i++, p++)
    {
        TRdisplay("\t%w:\tsc_errors: %d.\tscb size: %d.\n",
	    "user,CLF,ADF,DMF,PSF,OPF,QEF,QSF,RDF,SCF,ULF,invalid", i,
	    Sc_main_cb->sc_errors[i],
	    *p);
    }
    TRdisplay(">>> End of SC_MAIN_CB <<<\n");
}
#endif
