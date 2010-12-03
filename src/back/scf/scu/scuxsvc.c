/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <er.h>
#include    <ex.h>
#include    <pc.h>
#include    <cv.h>
#include    <cs.h>
#include    <tm.h>
#include    <st.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <gca.h>

/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <dmf.h>
#include <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <scu.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <scfcontrol.h>

#include    <ci.h>
#include    <cm.h>

/**
**
**  Name: SCUXSVC.C - miscellaneous scu utilities
**
**  Description:
**	This file contains various utilities that are made available
**	to all facilities (thru scf_call), but don't fit anywhere else.
**	They include:
**
**          scu_xencode() - encrypt a character string, like passwords.
**
**  History:    $Log-for RCS$
**	24-mar-89 (ralph)
**          Written for terminator
**	20-may-89 (ralph)
**	    Changed encryption to use separate key
**	06-jun-89 (ralph)
**	    Fixed unix compile problems
**	13-jul-92 (rog)
**	    Included ddb.h for Sybil, and er.h because of a change to scs.h.
**	12-Mar-1993 (daveb)
**	    Add include <st.h> <me.h>
**	06-may-1993 (ralph)
**	    DELIM_IDENT:
**	    Translate key seed to lower case prior to encryption.
**	    This is done because the key seed is the associated
**	    authorization identifier, but in some cases the auth
**	    id may not be in the same case in which it was defined.
**	    So, we make the keey seed case insensitive -- always
**	    lower case (for backward compatibility reasons).
**	2-Jul-1993 (daveb)
**	    prototyped.
**	7-Jul-1993 (daveb)
**	    fixed headers for prototypes.
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Jan-2001 (jenjo02)
**	    Added (SCD_SCB*)scb parameter to prototypes for
**	    scu_information(), scu_idefine(), scu_malloc(),
**	    scu_mfree(), scu_xencode().
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value scf_error structure.
**	    Use new form of sc0ePut().
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Include scu.h to get scu_xencode() prototype.
**/

/*
**  Forward and/or External function references.
*/

GLOBALREF SC_MAIN_CB	*Sc_main_cb;

/*{
** Name: scu_xencode - encrypt a character string
**
** Description:
**      This function uses CI routines to encrypt a character string.
**	Since the character string is used to generate the key schedule,
**	the encryption is essentially one-way (you'd need to know the
**	password to decode the password....)  This routine was designed
**	to encrypt application_id passwords.
**
** Inputs:
**      SCU_XENCODE			the opcode to scf_call()
**      scf_cb                          control block in which is specified
**          .scf_ptr_union.scf_xpassword
**			                pointer to buffer to be encrypted
**          .scf_nbr_union.scf_xpasskey
**			                pointer to seed for key schedule
**          .scf_len_union.scf_xpwdlen
**					length of password and key seed
**
** Outputs:
**      scf_cb                          the same control block
**          .error                      the error control area
**              .err_code               E_SC_OK or ...
**					E_SC0261_XENCODE_BAD_PARM
**					E_SC0262_XENCODE_BAD_RESULT
**	Returns:
**	    E_DB_{OK, WARNING, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-mar-89 (ralph)
**          Written for terminator
**	20-may-89 (ralph)
**	    Changed encryption to use separate key
**	06-jun-89 (ralph)
**	    Fixed unix compile problems
**	06-may-1993 (ralph)
**	    DELIM_IDENT:
**	    Translate key seed to lower case prior to encryption.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	12-Sep-2007 (drivi01)
**	    Modified scu_xencode function to fix numerous bugs.
**	    The buffers for password manipulation shouldn't exceed
**	    the size of scb_xpassword field in SCF control block,
**	    otherwise the data will be truncated.
*/
DB_STATUS
scu_xencode(SCF_CB *scf_cb, SCD_SCB *scb )
{
    STATUS	status;
    CI_KS	KS;
    char	inbuffer[DB_PASSWORD_LENGTH+1];
    char	outbuffer[DB_PASSWORD_LENGTH+1];
    char	keybuffer[DB_PASSWORD_LENGTH];
    u_i2	i2_size;
    i4		longnat_size;
    i4		nat_size;
    char	*char_ptr;

#define	PASSINIT    "hjodvwHOJHOJhodh498032&*&*#)$&*jpkshghjlg58925fjkdjkpg"

    status = E_DB_OK;
    CLRDBERR(&scf_cb->scf_error);

    /* Ensure input parameter is okay */

    if ((scf_cb->scf_len_union.scf_xpwdlen <= 0)		||
	(scf_cb->scf_len_union.scf_xpwdlen >= sizeof(inbuffer)) ||
	(scf_cb->scf_nbr_union.scf_xpasskey  == NULL)		||
	(scf_cb->scf_ptr_union.scf_xpassword == NULL))
    {
	sc0ePut(NULL, E_SC0261_XENCODE_BAD_PARM, NULL, 0);
	SETDBERR(&scf_cb->scf_error, 0, E_SC0261_XENCODE_BAD_PARM);
	return(E_DB_ERROR);
    }


    /* Copy string to input buffer */

    MEmove(scf_cb->scf_len_union.scf_xpwdlen,
		 (PTR)scf_cb->scf_ptr_union.scf_xpassword,
		 (char)'\0',
		 sizeof(inbuffer),
		 (PTR)inbuffer);

    /* Copy key to key buffer */

    MEmove(scf_cb->scf_len_union.scf_xpwdlen,
		 (PTR)scf_cb->scf_nbr_union.scf_xpasskey,
		 (char)'?',
		 sizeof(keybuffer),
		 (PTR)keybuffer);

    /* Fold the key to lower case */

    for (nat_size = sizeof(keybuffer), char_ptr = keybuffer;
	 nat_size > 0;
	 nat_size = CMbytedec(nat_size, char_ptr), char_ptr = CMnext(char_ptr))
    {
	CMtolower(char_ptr, char_ptr);
    }

	 

    /* Remove white space from input string */

    nat_size = STzapblank(inbuffer, outbuffer);

    /* Check size */

    if ((nat_size <= 0) ||
	(nat_size > sizeof(outbuffer)-1))
    {
	sc0ePut(NULL, E_SC0261_XENCODE_BAD_PARM, NULL, 0);
	SETDBERR(&scf_cb->scf_error, 0, E_SC0261_XENCODE_BAD_PARM);
	return(E_DB_ERROR);
    }

    /* Initialize input buffer to "garbage" */

    MEmove(sizeof(PASSINIT), (PTR)PASSINIT, (char)'?',
		 sizeof(inbuffer), (PTR)inbuffer);

    /* Normalize the string back into input buffer */

    MEcopy((PTR)outbuffer, nat_size, (PTR)inbuffer);

    /* Reset output buffer to blanks */

    MEfill(sizeof(outbuffer),
		 (u_char)' ',
		 (PTR)outbuffer);

    /*
    ** First, encrypt the key seed using the string to encode.
    ** Then,  encrypt the string using the encrypted seed.
    ** This is done to prevent two roles with the same password
    ** from having the same encrypted value.
    ** Note that this makes the encryption one-way, since
    ** the password must be provided to decrypt the password!
    */

    /* Generate the key schedule to encrypt the key seed */

    (VOID)CIsetkey((PTR)inbuffer, KS);

    /* Encrypt the key seed */

    longnat_size = DB_PASSWORD_LENGTH;
    (VOID)CIencode((PTR)keybuffer, longnat_size, KS, (PTR)outbuffer);

    /* Generate the second key schedule */

    (VOID)CIsetkey((PTR)keybuffer, KS);

    /* Encode the string */

	longnat_size = DB_PASSWORD_LENGTH;
    (VOID)CIencode((PTR)inbuffer, longnat_size, KS, (PTR)outbuffer);

    /* Make sure it was really encoded */

    if ((char *)STskipblank(outbuffer, (i4)sizeof(outbuffer))
		!= NULL)
    {
	/* It was; copy result to caller's area */

	i2_size = scf_cb->scf_len_union.scf_xpwdlen;
	MEmove(sizeof(outbuffer), (PTR)outbuffer, (char)' ',
		     i2_size, (PTR)scf_cb->scf_ptr_union.scf_xpassword);
    }
    else
    {
	/* The encryption did not work; return an error */

	sc0ePut(NULL, E_SC0262_XENCODE_BAD_RESULT, NULL, 0);
	SETDBERR(&scf_cb->scf_error, 0, E_SC0262_XENCODE_BAD_RESULT);
	status = E_DB_ERROR;
    }

    return(status);
}
