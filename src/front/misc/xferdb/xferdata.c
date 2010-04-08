/*
** Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include	<si.h>
# include       <gl.h>
# include       <iicommon.h>
# include       <lo.h>
# include       <fe.h>
# include       <ui.h>
# include       <ug.h>
# include       <adf.h>
# include       <afe.h>
# include       <uigdata.h>
# include       <xf.h>

/*
** Name:	xferdata.c
**
** Description:	Global data for xferdb facility.
**
** History:
**
**	23-sep-96 (mcgem01)
**	    Created.
**	20-dec-96 (chech02)
**	    Added #inlcude si.h.
**	22-sep-97 (nanpr01)
**	    bug 85565 : cannot unload a 1.2 database over net.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-may-2001 (abbjo03)
**	    Add CreateParallelIndexes.
**	19-oct-2001 (hanch04)
**	    Added Xf_xmlfile, Xf_xmlfile_data, Xf_xmlfile_sql, Xf_xml_dtd
**      20-jun-2002 (stial01)
**          Unloaddb changes for 6.4 databases (b108090)
**	29-jan-03 (inkdo01)
**	    Add sequence support.
**      31-Jul-2003 (hanal04) SIR 110651 INGDBA 236
**          Added new -journal option to allow fully journalled copy.in.
**	8-Jun-2004 (schka24)
**	    Release 3 catalog flag.
**	16-feb-2005 (thaju02)
**	    Added concurrent_updates for usermod.
**	7-sep-2005 (thaju02)
**	    Add with_tbl_list.
**	13-oct-05 (inkdo01)
**	    Add With_r302_catalogs.
**      17-Mar-2008 (hanal04) SIR 101117
**          Added GroupTabIdx.
**	16-nov-2008 (dougi)
**	    Added identity_columns, With_r930_catalogs.
**       3-Dec-2008 (hanal04) SIR 116958
**          Added db_replicated and with_no_rep globals.
**       8-jun-2009 (maspa05) SIR 122202
**          Added -nologging to put "set nologging" at the top of copy.in 
*/

/* xfchkcap.sc */

GLOBALDEF bool With_distrib = FALSE;
GLOBALDEF bool With_log_key = FALSE;
GLOBALDEF bool With_multi_locations = FALSE;
GLOBALDEF bool With_comments = FALSE;
GLOBALDEF bool With_UDT = FALSE;
GLOBALDEF bool With_registrations = FALSE;
GLOBALDEF bool With_dbevents = FALSE;
GLOBALDEF bool With_procedures = FALSE;
GLOBALDEF bool With_rules = FALSE;
GLOBALDEF bool With_synonyms = FALSE;
GLOBALDEF bool With_defaults = FALSE;
GLOBALDEF bool With_sec_audit= FALSE;
GLOBALDEF bool With_sequences= FALSE;
GLOBALDEF bool With_physical_columns= FALSE;
GLOBALDEF bool With_security_alarms = FALSE;
GLOBALDEF bool With_64_catalogs = FALSE;
GLOBALDEF bool With_65_catalogs = FALSE;
GLOBALDEF bool With_20_catalogs = FALSE;
GLOBALDEF bool With_r3_catalogs = FALSE;
GLOBALDEF bool With_r302_catalogs = FALSE;
GLOBALDEF bool With_r930_catalogs = FALSE;
GLOBALDEF bool CreateParallelIndexes = FALSE;
GLOBALDEF bool SetJournaling = FALSE;
GLOBALDEF bool Unload_byuser = FALSE;
GLOBALDEF bool concurrent_updates = FALSE;
GLOBALDEF bool with_tbl_list = FALSE;
GLOBALDEF bool with_no_rep = FALSE;
GLOBALDEF bool db_replicated = FALSE;
GLOBALDEF bool GroupTabIdx = FALSE;
GLOBALDEF bool NoLogging = FALSE;
GLOBALDEF bool identity_columns = FALSE;

/* xffileio.c */
GLOBALDEF LOCATION      Xf_dir;

/* xffilobj.sc */
GLOBALDEF       i4      Objcount = 0;

/* xfwrap.c */
GLOBALDEF       TXT_HANDLE     *Xf_in = NULL;
GLOBALDEF       TXT_HANDLE     *Xf_out = NULL;
GLOBALDEF       TXT_HANDLE     *Xf_both = NULL;
GLOBALDEF       TXT_HANDLE     *Xf_reload = NULL;
GLOBALDEF       TXT_HANDLE     *Xf_unload = NULL;

/* xfwrscrp.c */
GLOBALDEF char  *Owner;
GLOBALDEF bool  Portable;

/* xfgenxml.sc */
GLOBALDEF	TXT_HANDLE	*Xf_xmlfile;
GLOBALDEF	TXT_HANDLE	*Xf_xmlfile_data;
GLOBALDEF	TXT_HANDLE	*Xf_xmlfile_sql;
GLOBALDEF	TXT_HANDLE	*Xf_xml_dtd;

