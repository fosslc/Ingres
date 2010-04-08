/*
** Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<st.h>		/* 6-x_PC_80x86 */
#include	<er.h>
#include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<equel.h>
#include	<equtils.h>
#include	<eqsym.h>
#include	<eqgen.h>
#include	<eqrun.h>
#include	<eqtgt.h>
#include	<ereq.h>

/* {
** Filename:	eqtarget.c - Save and access target list elements
**
** Description:
**	This module contains the routines to save away information about
**	elements of target lists as they are parsed and to return that
**	information for code generation.
**
** Defines:
**	eqtl_init()	- Open target list processing; initialize structure
**	eqtl_add()	- Add a target list element to the list
**	eqtl_get()	- Return pointer to target list structure
**	eqtl_elem()	- Given an attribute ID return pointer to target list 
**			  element if specified, otherwise return null.
**			  
**
** Locals:
**
** History:
**	23-nov-1988	(barbara)
**	    Written for Phoenix/Alerters
**	15-mar-1991	(kathryn)
**	    Added tables set_attribs and inq_attribs for attributes of
**	    INQUIRE_SQL/SET_SQL statements.
**	22-mar-1991	(barbara)
**	    Added new table set and inq entries for copyhandler.
**	28-mar-1991	(kathryn)
**	    Corrected SAVESTATE tla_type to DB_CHR_TYPE.
**	01-apr-1991	(kathryn)
**	    Add Highstamp, Lowstamp and Eventasync to inq_attribs table.
**	15-apr-1991	(teresal)
**	    Add support for inquiring on event info: eventname,
**	    eventowner, eventdatabase, eventtime, eventtext.
**	    Also, removed special event attribute handling that
**	    used to occur when GET EVENT returned event info.
**	19-jul-1991	(teresal)
**	    Changed EVENT to DBEVENT.
**	22-sep-1992	(lan)
**	    Added columnname and columntype to table inq_attribs.
**	10-nov-1992	(kathryn)
**	    Added support for GET DATA and PUT DATA statements, including
**	    new tables getdat_attribs and putdat_attribs.
**	    Added IITL_???MAX defines for each of the tables, and global
**	    max_attribs.  Added function eqtl_elem().
**      21-dec-1992 (larrym)
**          Added support for CONNECTION_TARGET
**	04-mar-1992 (larrym)
**	    Added support for CONNECTION_NAME
**	15-sep-1993 (kathryn)
**	    Added COLTYPE as valid SET_SQL attribute for data handlers.
**	    Added T_VBYTE as character coercible datatype.
**	17-sep-1993 (kathryn)
**          Remove references to INQUIRE_SQL on error,copy,message, and dbevent
**          handlers.  This never was and will not be completed/supported.
**	9-nov-93 (robf)
**          Add internal support for dbprompthandler, savepassword
**	13-dec-1993 (mgw)
**	    Correct the max_attribs handling.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**      06-Feb-96 (fanra01)
**          Made data extraction generic.
**	18-dec-96 (chech02)
**	    fixed redefined symbols error : max_attribs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
** Name: tl_descriptor - descriptor of the target list being processed
**
** Description:
**	The target list descriptor consists of a "head" element containing
**	information about the target list as a whole, and a list of attribute
**	elements, each one of which contains information about a particular
**	attribute.  This descriptor is initialized by eqtl_init() and the
**	individual elements are assigned information by eqtl_add().  A pointer
**	to this descriptor is returned by eqtl_get().
*/
static TL_DESC	tl_descriptor;
GLOBALREF i4	max_attribs;	/* maximum attributes for this statement */

/*
** Name:  TL_ATTRS - Information on a target list attribute
**
** Description:
**      This structure describes an attribute.  It is used as a lookup to
**      validate the user's target list.  For example, for each element in
**      a target list, is the attribute valid in the enclosing statement,
**      is the type of the user var/val compatible?
**
** To Add a new attribute:
**	-) Define the attribute in front/hdr/hdr/eqrun.h
**      -) Add the attribute to the appropriate table.
**	-) Up the IITL_???MAX define to the number of elements in the table.
**	-) If the number of elements in a table exceed MAXTL_ELEMS as defined 
**	   in front/hdr/hdr/eqtgt.h then update it.
**	   
**
**  TLA_ATTRS is defined in eqtgt.h as follows:
**	struct { char        *tla_string;    ** Attribute string **
**    		 i4          tla_id;         ** Attr id defined in eqrun.h **
**      	 i4          tla_type;       ** Attribute type **
**             } TL_ATTRS;
**
**	tla_string		tla_id		tla_type
*/

# define IITL_GETMAX	4
static TL_ATTRS getdat_attribs[] =
{
    {ERx("segment"),        	gdSEGMENT,      DB_CHR_TYPE},
    {ERx("maxlength"),          gdMAXLENG,      DB_INT_TYPE},
    {ERx("segmentlength"),      gdSEGLENG,      DB_INT_TYPE},
    {ERx("dataend"),		gdDATAEND,      DB_INT_TYPE},
    {(char *)0,                 0,              0          }
};

# define IITL_PUTMAX	3
static TL_ATTRS putdat_attribs[] =
{
    {ERx("segment"),            gdSEGMENT,      DB_CHR_TYPE},
    {ERx("segmentlength"),      gdSEGLENG,      DB_INT_TYPE},
    {ERx("dataend"),            gdDATAEND,      DB_INT_TYPE},
    {(char *)0,                 0,              0          }
};

# define IITL_SETMAX	26
static TL_ATTRS set_attribs[] =
{
    {ERx("columntype"),         isCOLTYPE,      DB_INT_TYPE},
    {ERx("connection_name"),    siCONNAME,      DB_CHR_TYPE},
    {ERx("copyhandler"),        siCPYHDLR,      DB_NODT},
    {ERx("dbeventdisplay"),     ssDBEVDISP,     DB_INT_TYPE},
    {ERx("dbeventhandler"),     siDBEVHDLR,     DB_NODT},
    {ERx("dbmserror"),          siDBMSER,       DB_INT_TYPE},
    {ERx("dbprompthandler"),    siDBPRMPTHDLR,  DB_NODT},
    {ERx("errordisp"),          ssERDISP,       DB_INT_TYPE},
    {ERx("errorhandler"),       siERHDLR,       DB_NODT},
    {ERx("errormode"),          ssERMODE,       DB_INT_TYPE},
    {ERx("errorno"),            siERNO,         DB_INT_TYPE},
    {ERx("errortype"),          siERTYPE,       DB_CHR_TYPE},
    {ERx("gcafile"),            ssGCAFILE,      DB_CHR_TYPE},
    {ERx("messagehandler"),     siMSGHDLR,      DB_NODT},
    {ERx("prefetchrows"),       siPFROWS,       DB_INT_TYPE},
    {ERx("printtrace"),         ssPRTTRC,       DB_INT_TYPE},
    {ERx("programquit"),        siPRQUIT,       DB_INT_TYPE},
    {ERx("printcsr"),           ssPRTCSR,       DB_INT_TYPE},
    {ERx("printgca"),           ssPRTGCA,       DB_INT_TYPE},
    {ERx("printqry"),           ssPRTQRY,       DB_INT_TYPE},
    {ERx("qryfile"),            ssQRYFILE,      DB_CHR_TYPE},
    {ERx("rowcount"),           siROWCNT,       DB_INT_TYPE},
    {ERx("savequery"),          siSAVEQRY,      DB_INT_TYPE},
    {ERx("savepassword"),       siSAVEPASSWD,   DB_INT_TYPE},
    {ERx("savestate"),          siSAVEST,       DB_CHR_TYPE},
    {ERx("session"),            siSESSION,      DB_INT_TYPE},
    {ERx("tracefile"),          ssTRCFILE,      DB_CHR_TYPE},
    {(char *)0,                 0,              0          }
};

# define IITL_INQMAX	32
static TL_ATTRS inq_attribs[] =
{
    {ERx("columnname"),         isCOLNAME,      DB_CHR_TYPE},
    {ERx("columntype"),         isCOLTYPE,      DB_INT_TYPE},
    {ERx("connection_name"),    siCONNAME,      DB_CHR_TYPE},
    {ERx("connection_target"),  isCONTARG,      DB_CHR_TYPE},
    {ERx("dbeventasync"),       isDBEVASY,      DB_INT_TYPE},
    {ERx("dbeventdatabase"),    isDBEVDB,       DB_CHR_TYPE},
    {ERx("dbeventname"),        isDBEVNAME,     DB_CHR_TYPE},
    {ERx("dbeventowner"),       isDBEVOWNER,    DB_CHR_TYPE},
    {ERx("dbeventtext"),        isDBEVTEXT,     DB_CHR_TYPE},
    {ERx("dbeventtime"),        isDBEVTIME,     DB_CHR_TYPE},
    {ERx("dbmserror"),          siDBMSER,       DB_INT_TYPE},
    {ERx("diffarch"),           isDIFARCH,      DB_INT_TYPE},
    {ERx("endquery"),           isENDQRY,       DB_INT_TYPE},
    {ERx("errorno"),            siERNO,         DB_INT_TYPE},
    {ERx("errorseverity"),      isERSEV,        DB_INT_TYPE},
    {ERx("errortext"),          isERTEXT,       DB_CHR_TYPE},
    {ERx("errortype"),          siERTYPE,       DB_CHR_TYPE},
    {ERx("highstamp"),          isHISTMP,       DB_INT_TYPE},
    {ERx("iiret"),              isIIRET,        DB_INT_TYPE},
    {ERx("lowstamp"),           isLOSTMP,       DB_INT_TYPE},
    {ERx("messagenumber"),      isMSGNO,        DB_INT_TYPE},
    {ERx("messagetext"),        isMSGTXT,       DB_CHR_TYPE},
    {ERx("object_key"),         isOBJKEY,       DB_LOGKEY_TYPE},
    {ERx("prefetchrows"),       siPFROWS,       DB_INT_TYPE},
    {ERx("programquit"),        siPRQUIT,       DB_INT_TYPE},
    {ERx("querytext"),          isQRYTXT,       DB_CHR_TYPE},
    {ERx("rowcount"),           siROWCNT,       DB_INT_TYPE},
    {ERx("savequery"),          siSAVEQRY,      DB_INT_TYPE},
    {ERx("savestate"),          siSAVEST,       DB_CHR_TYPE},
    {ERx("session"),            siSESSION,      DB_INT_TYPE},
    {ERx("table_key"),          isTABKEY,       DB_TABKEY_TYPE},
    {ERx("transaction"),        isTRANS,        DB_INT_TYPE},
    {(char *)0,                 0,              0          }
};

/*
** The following diagram attempts to show how the data structures are
** used to process a target list.
**
** tl_descriptor -- Data structure to describe a target list
**           ---------------------
**   HEAD    |stmtid             |  Head info applies to tgt list as a whole
**	     |no. of elements    |
**	     |ptr to attrib table| 
**           |error?             | 
**           ---------------------
** ELEMENTS  |var/val info       |  Each element in list describes a specific 
**	     |attrib name        |  item in target list.  Element info is
**	     --------------------   inserted in eqtl_add, validated by  
**           |var/val info       |  lookups to tl_attribs. 
**	     |attrib name        |
**	     --------------------   
**	     | .                 |  
**	       .
**	       .
**	      (MAXTL_ELEMS elements)
*/


/* {
** Name:	eqtl_init - Start of processing a target list.
**
** Description:
**	This routine is called before parsing a target list in an SQL
**	statement.  It validates the statement id and initializes
**	the tl_descriptor.  This descriptor will be assigned information
**	about the target list elements by the eqtl_add routine.
**
** Inputs:
**	stmt_id		Internal id of generated statement
** Outputs:
**	None.
**	Returns:
**	    VOID.
**	Errors:
**	    E_EQ0080_TL_STMTID - Illegal internal statement id.
**
** History
**	13-nov-1989	(barbara)
**	    Writtten for Phoenix/Alerters
**	15-mar-1991	(kathryn)
**	    Added support for SET/INQUIRE_SQL statements.
**	    Set new tl_descriptor fields tld_attrtbl and tld_nelems.
**	01-nov-1992	(kathryn)
**	    Added support for GET DATA and PUT DATA statements.
**	    Added intialization of global max_attribs.
**	23-sep-93 (essi)
**	    Corrected typo in assignment.
*/
VOID
eqtl_init(stmt_id)
i4	stmt_id;
{

    /* Initialize the target list "head" and all the elements */
    tl_descriptor.tld_error = FALSE;
    tl_descriptor.tld_stmtid = stmt_id;
    tl_descriptor.tld_nelems = 0;
    switch (stmt_id)
    {
	case IISETSQL:
		tl_descriptor.tld_attrtbl = set_attribs;
		max_attribs = IITL_SETMAX;
		break;
	case IIINQSQL:
		tl_descriptor.tld_attrtbl = inq_attribs;
		max_attribs = IITL_INQMAX;
		break;
        case IIGETDATA:
		tl_descriptor.tld_attrtbl = getdat_attribs;
		max_attribs = IITL_GETMAX;
   		break;
	case IIPUTDATA:
		tl_descriptor.tld_attrtbl = putdat_attribs;
		max_attribs = IITL_PUTMAX;
                break;
	default:
		tl_descriptor.tld_error = TRUE;
		er_write( E_EQ0080_TL_STMTID, EQ_ERROR, 0 );
		return;
    }

}

/* {
** Name: eqtl_add - Add an element of a target list to the tl_descriptor
**
** Description:
**	This routine is called after parsing an element of a target list.
**	It assigns info about this element to the TL_ELEM array in the
**	tl_descriptor after performing the following checks:
**	 -  Is the type of the user's val/var compatible with the
**	      attribute type?
**	 -  Is this attribute legal in the current statement?
**	If an error occurs the TL_ELEM array element is not assigned
**	any information.  
**
** Inputs:
**	vname		User variable name or string representation of value
**	vsym		Pointer to caller's symbol table entry (may be null)
**	iname		User indicator name (may be null)
**	isym		Pointer to caller's symbol table entry (may be null)
**	vtype		Type of user's variable/value	 
**	attr		Name of target attribute
**
** Outputs:
**	None.
**	Returns:
**	VOID.
**	Errors:
**	    E_EQ0081_TL_ATTR_UNKNOWN - Unknown attribute name 
**	    E_EQ0082_TL_ATTR_INVALID - Attribute invalid in this statement
**	    E_EQ0083_TL_TYPE_INVALID - Non-coercible type of user's var or val
**
** History:
**	14-nov-1989	(barbara)
**	    Written for Phoenix/Alerters
**	15-mar-1991 	(kathryn)
**	    Added vtype "T_NONE" with dbtype "DB_NODT" for user defined
**	    handlers.
**	01-nov-1992	(kathryn)
**	    Use global "max_attribs" rather than MAXTL_ELEMS to determine
**	    valid number of elements for the statement.  Each statement has 
**	    a different maximum. MAXTL_ELEMS is the maximum number of elements 
**	    for any statement.
**	    Added DB_ALL_TYPE for attributes which may be of any type, for
**	    future use.  This type may be specified in the table when an
**	    attribute is a user variable (PTR), and any underlying type is
**	    acceptable.
**	20-apr-2001    (gupsh01)
**	    Added support for unicode data types.
**	03-aug-2006 (gupsh01)
**	    Added support for ANSI date/time types.
**	    
*/

VOID
eqtl_add(vname, vsym, iname, isym, vtype, attr)
char	*vname;
PTR	vsym;
char	*iname;
PTR	isym;
i4	vtype;
char	*attr;
{
    TL_ATTRS	*attrptr;
    bool	attr_convert = FALSE;
    i4		tl_index;
    /* Don't bother saving attrib info if error occurred on eqtl_init() */
    if (tl_descriptor.tld_error == TRUE)
	return;

    attrptr = tl_descriptor.tld_attrtbl;
    
    for (; attrptr->tla_string != (char *)0; attrptr++)
    {
	if (STbcompare(attr, 0, attrptr->tla_string, 0, TRUE) == 0)
	    break;
    }


    /* Illegal attribute? */
    if (attrptr->tla_string == (char *)0)
    {
	er_write( E_EQ0081_TL_ATTR_UNKNOWN, EQ_ERROR, 1, attr );
	return;
    }

    if ( tl_descriptor.tld_nelems < max_attribs)
	tl_index = tl_descriptor.tld_nelems;
    else
    {
	er_write(E_EQ0316_argMAX, EQ_ERROR, 1, er_na(max_attribs));
	return;
    }


    /* Types non-coercible? (should perhaps be calling ADI here) */
    switch (vtype)
    {
      case T_DBV:
      case T_UNDEF:		/* Error already given */
      case T_NUL:
	attr_convert = TRUE;
	break;
      case T_NONE:	
	if (attrptr->tla_type == DB_NODT)
		attr_convert = TRUE;
        break;
      case T_CHAR:
      case T_CHA:
      case T_VCH:
      case T_VBYTE:
	switch (attrptr->tla_type)
	{
	  case DB_LOGKEY_TYPE:
	  case DB_TABKEY_TYPE:
	  case DB_CHR_TYPE:
	  case DB_CHA_TYPE:
	  case DB_VCH_TYPE:
	  case DB_TXT_TYPE:
	  case DB_DTE_TYPE:
	  case DB_ADTE_TYPE:
	  case DB_TMWO_TYPE:
	  case DB_TMW_TYPE:
	  case DB_TME_TYPE:
	  case DB_TSWO_TYPE:
	  case DB_TSW_TYPE:
	  case DB_TSTMP_TYPE:
	  case DB_INDS_TYPE:
	  case DB_INYM_TYPE:
	  case DB_ALL_TYPE:
	    attr_convert = TRUE;
	    break;
	  default:
	    attr_convert = FALSE;
	}
	break;
      case T_WCHAR:
      case T_NVCH:	 
	attr_convert = TRUE;
	break;
      case T_INT:
      case T_PACKED:
      case T_FLOAT:
	switch (attrptr->tla_type)
	{
	  case DB_INT_TYPE:
	  case DB_FLT_TYPE:
	  case DB_MNY_TYPE:
	  case DB_NODT:
	  case DB_ALL_TYPE:
	    attr_convert = TRUE;
	    break;
	  default:
	    attr_convert = FALSE;
	}
	break;
      default:
	attr_convert = FALSE;
    }
    if (attr_convert == FALSE)
    {
	er_write( E_EQ0083_TL_TYPE_INVALID, EQ_ERROR, 1, attr );
	return;
    }

    tl_descriptor.tld_elem[tl_index].tle_val = vname;
    tl_descriptor.tld_elem[tl_index].tle_var = vsym;
    tl_descriptor.tld_elem[tl_index].tle_indname = iname;
    tl_descriptor.tld_elem[tl_index].tle_indvar = isym;
    tl_descriptor.tld_elem[tl_index].tle_type = vtype;
    tl_descriptor.tld_elem[tl_index].tle_attr = attrptr->tla_id;
    tl_descriptor.tld_nelems++;
}

/* {
** Name: eqtl_get - Return pointer to target list descriptor
**
** Description:
**	Returns a pointer to the tl_descriptor structure so that the
**	caller may generate code for target list processing.
**
** Inputs:
**	None.
** Outputs:
**	None.
**	Returns:
**	    Address of global TL_DESC descriptor
**
** History:
**	13-nov-1989	(barbara)
**	    Written for Phoenix/Alerters
*/
TL_DESC *
eqtl_get()
{
    return &tl_descriptor;
}

/* {
** Name: eqtl_elem - Find a particular attribute in the element list.
**
** Description:
**	Given an attribute id, this routine looks through the target list
**	elements to determine if the attribute was specified in the target
**	list of the statement.  If so, it returns a pointer to the element. 
**
** Inputs:
**      tlid:	Target list attribute id.
**
** Outputs:
**      None.
**      Returns:
**          If found, pointer to target list element containing attribute.
**	    If not found, NULL.
**
** Side Effects:
**	None.
**
** History:
**      01-nov-1992     (kathryn)
**          Written.
*/

TL_ELEM *
eqtl_elem(i4 tlid)
{
    i4		i;
    TL_ELEM	*elem_ptr;
    TL_DESC	*tl_descr;

    tl_descr = eqtl_get();

    i = 0;

    while (i < tl_descr->tld_nelems)
    {
	elem_ptr = &tl_descr->tld_elem[i];
        if (elem_ptr->tle_attr == tlid)
	    return elem_ptr;
	i++;
    }
    return ((TL_ELEM *)0);
}
