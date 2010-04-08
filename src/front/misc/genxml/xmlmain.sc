/*
**  Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
**
**  Name: genxml - utility for exporting ingres table data into xml format.
**
**  Usage:
**	
**     genxml dbname [-uuser] [-P] [-GgroupID] 
**                   [-dest=dir] [-xmlfile=filename] [-dtdfile=filename] 
**		     [-title_doctype=title] [-metadata_only] [-internal_dtd] 
**		     [-referred_dtd] [-encode_off] [{tables ...}] 
**
**  Description:
**      This utility will allow bulk transfer of Ingres data to
**      XML formats from an Ingres database.
**	
**	The various input parameters are as follows:
**	
**	dbname		 : The data base being exported.
**	-uuser		 : Effective user for the session.
**	-P		 : Password if the session requires password.
**	-GgroupID	 : Group ID of the user.
**	-dest=dir	 : The directory where the xml file will be send to.
**	-xmlfile=filename: The name of the output xml file.
**	-dtdfile=filename: The name of the output dtd file.
**	-title_doctype=name : The title ie the Doctype name of the XML file.
**	-metadata_only	 : If this flag is specified then the metadata.
**			   information will only be printed out. 
**	-internal_dtd	 : This flag prints the DTD inline inside the XML doc. 
**	-referred_dtd	 : This flag will puts a reference to the DTD 
**			   in the install area.
**      -encode_off      : Turn 64-bit string data encoding off.
**	tables		 : Table names which the user wants the xml dump of. 
**			   Table names must be separated by spaces. by default
**			   if no tables are specified, all user tables are
**			   output. 
**
**  Exit Status:
**      OK      genxml succeeded.
**      FAIL    genxml failed.
**
**  History:
**      24-Apr-2001 (gupsh01)
**          Created.
**	06-June-2001 (gupsh01)
**	    Modified with the new flags for the genxml utility.
**	10-Oct-2001 (hanch04)
**	    remove call to writevalue and replaced with STprintf,xfwrite
**	21-Dec-2001 (hanch04)
**	    Tables that are passed in with reserved word name, must be quoted.
**       3-Dec-2008 (hanal04) SIR 116958
**          Added -no_rep option to exclude all replicator objects.
*/

# include	<compat.h>
# include	<pc.h>		
# include	<cv.h>
# include	<ex.h>
# include	<me.h>
# include	<st.h>
# include	<si.h>
# include	<lo.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include 	<er.h>
# include	<fe.h>
# include	<fedml.h>
# include	<uigdata.h>
# include	<ug.h>
# include	<ui.h>
# include       <adf.h>
# include       <afe.h>
# include	<xf.h>
# include 	"erxml.h"

/**
** Name:    xmlmain.sc - main procedure for genxml
**
** Description:
**	This file defines:
**
**	main		main procedure for genxml utility
**
** History:
**	30-aug-2000 (gupsh01) 
**	    Created.
**	24-Apr-2001 (gupsh01)
**          Modified as per the new DTD. 	
**      06-June-2001 (gupsh01)
**          Modified to use the new flags for the genxml utility.
**	12-June-2001 (gupsh01)
**	    Corrected column count calculation.
**      29-May-2002 (gupsh01)
**          Modified the parameter to xmlprintrows to tp instead of 
**          tp->name.
**      02-May-2005 (gilta01) bug 112138 INGCBT522
**          Added xfxmlwrite() which allows the caller to indicate
**          whether special characters should be expanded.
**      23-June-2005 (zhahu02)
**          Updated for a non-existing table (INGCBT566/b114505).  
**      27-Feb-2009 (coomi01) b121663
**         Add flag to turn 64bit encoding off.
*/

GLOBALREF bool With_distrib;
GLOBALREF bool Copy_row_labels;
FUNC_EXTERN void xmldtd(bool internal_dtd, char *title_doctype);
/*{
** Name:	xmlmain - main procedure for genxml
**
** Description:
**	genxml - main program
**	-----------------------
**
** Input params:
**
** Output params:
**
** Returns:
**
** Exceptions:
**
** Side Effects:
**
** History:
/*
**	MKMFIN Hints
**
PROGRAM = genxml

NEEDLIBS =	XFERDBLIB \
		GNLIB SHFRAMELIB SHQLIB SHCOMPATLIB SHEMBEDLIB 

NEEDLIBSW = 	SHADFLIB 

UNDEFS =	II_copyright
*/
void
main(argc, argv)
i2	argc;
char	**argv;
{
    char	*dbname = ERx("");
    char	*user = ERx("");
    char	*directory = ERx("");
    char	*sourcedir = NULL;
    char	*destdir = NULL;
    char        *password = ERx("");
    char        *groupid = ERx("");
    char	*progname = ERx("Genxml");
    char	*filenamein = ERx("xmlin.xml");
    char        *filenameout = ERx("xmlout.xml");
    char        *filenamedtd = ERx("ingres.dtd");
    char        *title_doctype= ERx("IngresDoc");
    bool	portable = FALSE;
    i2		numobj;
    EX_CONTEXT	context;
    EX		FEhandler();
    i4          output_flags = 0;
    bool	metadata_only = FALSE;
    bool	internal_dtd  = FALSE;
    bool	referred_dtd  = FALSE;
    bool        use64bitEncoding = TRUE;
    char 	*owner;
    XF_TABINFO  *tlist;
    XF_TABINFO  *tp;
    i4 tcount = 0;
    STATUS      stat;
    i4 colcnt = 0;
    XF_COLINFO *cp;
    char  *Col_names[DB_MAX_COLS + 1] = { NULL };
    char tbuf[256];


    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise(ME_INGRES_ALLOC);

    /* Call IIUGinit to initialize character set attribute table */
    if ( IIUGinit() != OK)
	PCexit(FAIL);

     FEcopyright(progname, ERx("2000"));
 
    /*
    ** Get arguments from command line
    **
    ** This block of code retrieves the parameters from the command
    ** line.  The required parameters are retrieved first and the
    ** procedure returns if they are not.
    ** The optional parameters are then retrieved.
    */
    {
	ARGRET	rarg; 
	i4     	pos;

	if (FEutaopen(argc, argv, ERx("genxml")) != OK)
	    PCexit(FAIL);

	/* database name is required */
	if (FEutaget(ERx("database"), 0, FARG_PROMPT, &rarg, &pos) != OK)
	    PCexit(FAIL);
	dbname = rarg.dat.name;

    if (FEutaget(ERx("user"), 0, FARG_FAIL, &rarg, &pos) == OK)
      user = rarg.dat.name;

    /*
    ** bug 3594
    ** Check the return status from FEningres to determine
    ** if the DBMS was connected to before continuing.
    */
    if (FEningres(NULL, (i4) 0, dbname, user, ERx(""), password,
          groupid, (char *) NULL) != OK)
    {
        PCexit(FAIL);
    }

    if (FEutaget(ERx("xmlfile"), 0, FARG_FAIL, &rarg, &pos) == OK)
    {
         filenameout = ERx(rarg.dat.name);
    }

   if (FEutaget(ERx("dtdfile"), 0, FARG_FAIL, &rarg, &pos) == OK)
    {
         filenamedtd = ERx(rarg.dat.name);
    }

   if (FEutaget(ERx("title_doctype"), 0, FARG_FAIL, &rarg, &pos) == OK)
    {
         title_doctype = ERx(rarg.dat.name);
    }

    numobj = 0;
    while (FEutaget(ERx("objname"), numobj, FARG_FAIL, &rarg, &pos) == OK) 
    {
	if (STcompare(rarg.dat.name, ERx(".")) == 0)
	    break;
	/* Add to our list of objects to be unloaded. */
	if (IIUGdlm_ChkdlmBEobject(rarg.dat.name, NULL, TRUE))
	{
	    char	nbuf[((2 * FE_MAXNAME) + 2 + 1)];
	    IIUGrqd_Requote_dlm(rarg.dat.name, nbuf);
	    xfaddlist(nbuf);
	}
	else
	{
	    xfaddlist(rarg.dat.name);
	}
	numobj++;
    }

   /* Get optional arguments */

    if (FEutaget(ERx("dest"), 0, FARG_FAIL, &rarg, &pos)==OK)
        directory = rarg.dat.name;

    if (FEutaget(ERx("groupid"), 0, FARG_FAIL, &rarg, &pos) == OK)
	groupid = rarg.dat.name;

    if (FEutaget(ERx("password"), 0, FARG_FAIL, &rarg, &pos) == OK)
    {
	char *IIUIpassword();

	if ((password = IIUIpassword(ERx("-P"))) == NULL)
	{
	    FEutaerr(BADARG, 1, ERx(""));
	    PCexit(FAIL);
	}
    }
   
   /* get the other flags */
   if (FEutaget(ERx("metadata_only"), 0, FARG_FAIL, &rarg, &pos) == OK)
	 metadata_only = TRUE;
   if (FEutaget(ERx("internal_dtd"), 0, FARG_FAIL, &rarg, &pos) == OK)
	internal_dtd = TRUE;
   if (FEutaget(ERx("referred_dtd"), 0, FARG_FAIL, &rarg, &pos) == OK)
	referred_dtd = TRUE;
   if (FEutaget(ERx("encode_off"), 0, FARG_FAIL, &rarg, &pos) == OK)
	use64bitEncoding = FALSE;

    FEutaclose();
    }/* end of parsing the command line flags. */

    if (EXdeclare(FEhandler, &context) != OK)
    {
	EXdelete();
	PCexit(FAIL);
    }

    xfcapset();

    if (!With_distrib)
    {
        EXEC SQL SET LOCKMODE SESSION WHERE READLOCK = NOLOCK;
    }

    if (xfsetdirs(directory, sourcedir, destdir) != OK)
	PCexit(FAIL);

    /* open xml file */
    if ((Xf_out = xfopen(filenameout,0)) == NULL) 
    {
        PCexit (FAIL);
    }

    /* open the dtd file */
    if (internal_dtd)
	Xf_xml_dtd = Xf_out; 
    else	
    {
       if ((Xf_xml_dtd = xfopen(filenamedtd,0)) == NULL) 
       {
            PCexit (FAIL);
       }
    }

    /* get session data */
    owner = IIUIdbdata()->suser;

    /* We assume that the owner name has already been normalized. */
    Owner = (owner == NULL ? ERx("") : owner);

    
    /* DTD specification.
    ** ------------------
    ** Ingres DTD is a controlled document describing the ingres data, 
    ** and ingres docs will refer to it. DTD can be internal, or external. 
    ** External DTD's may be printed in the same directory as the xml file, 
    ** or they may be a referenced in the xml file to the DTD location, 
    ** This location will be either a URL or a static location like 
    ** $II_SYSTEM/ingres/files/ingres.dtd. Reference to the ingres dtd 
    ** will be made using the DOCTYPE declaration. 
    ** When the DTD is placed at the public URL location, it could be 
    ** referred to using the declaration.
    **
    **  <!DOCTYPE root_element_name SYSTEM "system_identifier">
    **  <!DOCTYPE root_element_name PUBLIC "public_identifier" 
				    SYSTEM "system_identifier">
    **  The public_identifier and the system_identifier are URIs.
    **
    ** for all practice purpose in case of Ingres external DTD will
    ** be declared as follows.
    **  <!DOCTYPE IngresDoc SYSTEM "path-to II_SYSTEM/ingres/files/ingres.dtd">
    */

    if (internal_dtd != TRUE)
      print_docType(filenamedtd, referred_dtd, title_doctype);

    xmldtd(internal_dtd, title_doctype);

    /* 
    ** Get all the metadata 
    ** Read table information into a linked list. 
    ** This will return all the details about tables and columns 
    */

    tlist = xffilltable(&tcount, 0); 

    /* print the outermost <IngresDoc> tag for the document.  */
    xfwrite(Xf_out, ERx("<"));
    xfwrite(Xf_out, ERx(title_doctype));
    if (tlist == NULL)
    xfwrite(Xf_out, ERx(" version=\'1.0\'/"));
    else
    xfwrite(Xf_out, ERx(" version=\'1.0\' "));
    xfwrite(Xf_out, ERx(">\n"));
    if (tlist == NULL)
         return;

    /* print the meta_ingres info */ 	
    xfwrite(Xf_out, ERx("<meta_ingres \t class=\"INGRES\"\n"));
    xmldbinfo(dbname , Owner);	
    for (tp = tlist; tp != NULL; tp = tp->tab_next)
    {
        xfread_id(tp->name);
        if (xfselected(tp->name))
        {
	/* print the meta_table information */
	colcnt = xmltabinfo(tp);
        }
     }
     xfwrite(Xf_out, ERx("</meta_ingres>\n"));

    /* 
    ** print the table information don't print this if 
    ** metadata_only flag is provided 
    */
    if ( metadata_only != TRUE )
    {
      for (tp = tlist; tp != NULL; tp = tp->tab_next)
      {
	colcnt = get_colcnt(tp);

        if (xfselected(tp->name))
        {
	xfwrite(Xf_out, ERx("<table \t table_name=\""));
	xfxmlwrite((PTR)tp->name, STlength(tp->name), XML_EXPANDED);

	/* table_nr */
	STprintf( tbuf, ERx("\"\n\t table_nr=\"%d\">\n"),
	    tp->table_reltid);
	xfwrite(Xf_out, tbuf);

	/* print the resultset information */

	/* row_start */
	xfwrite(Xf_out, ERx("<resultset \t row_start=\"1\"\n"));
	xmlprintresultset(tp);

	/* print data information from the rows.  */
	xmlprintrows( colcnt, tp, dbname, use64bitEncoding);	

	/* end of resultset */
	xfwrite(Xf_out, ERx("</resultset>\n"));
	/* end of table data */
	xfwrite(Xf_out, ERx("</table>\n"));
        }
      }
    }

   /* end of xml document write the closing tag */
    xfwrite(Xf_out, ERx("</"));
    xfwrite(Xf_out, ERx(title_doctype));
    xfwrite(Xf_out, ERx(">\n"));

   /* close files */
    xfclose(Xf_out);

    if (internal_dtd != TRUE)
      xfclose(Xf_xml_dtd);

    FEing_exit();

    EXdelete();

    PCexit(OK);
    /* NOTREACHED */
}
