/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/* external routine declarations definitions */
#define        OPV_GRTINIT      TRUE
#include    <me.h>
#include    <opxlint.h>

/**
**
**  Name: OPVGRTINIT.C - initialize the global range table 
**
**  Description:
**      File contains procedures to initialize the global range table
**
**  History:    
**      30-jun-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opv_grtinit(
	OPS_STATE *global,
	bool rangetable);

/*{
** Name: opv_grtinit	- initialize the global range table
**
** Description:
**      This routine will initialize the global range table structure
**      "ops_rangetab".
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**      global->ops_rangetab            global range table initialized
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-jun-86 (seputis)
**          initial creation
**	23-jan-91 (seputis)
**	    moved RDF_CB initialization to ops_init so OPC can use it
**	    added parameter to not allocate range table space,
**	    support for OPF ACTIVE flag.
**      18-sep-92 (ed)
**          - bug 44850 - changed mechanism for global to local mapping
**          so create new structure for this purpose
**      12-jun-2009 (huazh01)
**          init opv_lrvbase[] to OPV_NOVAR. Instead of allowing OPF
**          to read a random value from an uninitialized array, this will
**          cause OPF to hit an E_OP0082 error. (b120508)
[@history_line@]...
*/
VOID
opv_grtinit(
	OPS_STATE          *global,
	bool		   rangetable)
{
    if (rangetable)
    {
	i4	    lrvsize;

	lrvsize = sizeof(OPV_IVARS) * OPV_MAXVAR;
	global->ops_rangetab.opv_base = 
	    (OPV_GRT *) opu_memory(global, sizeof(OPV_GRT)+lrvsize); /* allocate an
					    ** array of ptrs to global range
                                            ** table elements */
	MEfill( sizeof(OPV_GRT), (u_char) 0, (PTR)global->ops_rangetab.opv_base);
                                            /* initialize all ptrs to NULL since
                                            ** this indicates whether a global
                                            ** range table element is "free"
                                            */
	global->ops_rangetab.opv_lrvbase = (OPV_IVARS *)(&global->ops_rangetab.opv_base[1]);

        /* b120508: init opv_lrvbase[] to OPV_NOVAR */
	MEfill((u_i2)lrvsize, (u_char)OPV_NOVAR, 
                   (PTR)global->ops_rangetab.opv_lrvbase);


    }
    else
	global->ops_rangetab.opv_base = NULL;
    global->ops_rangetab.opv_gv = 0;        /* number of range vars defined */
    MEfill( sizeof(OPV_GBMVARS), (u_char)0, 
	(PTR)&global->ops_rangetab.opv_mrdf); /* initialize map of global range 
                                            ** variables which have fixed RDF
                                            ** information */
    MEfill( sizeof(OPV_GBMVARS), (u_char)0, 
	(PTR)&global->ops_rangetab.opv_msubselect); /* initialize map of global 
					    ** range variables which are
                                            ** subselects */
}
