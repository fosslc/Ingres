/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project : Ingres Visual DBA
**
**  Source : dbamisc.c
**
**  History:
**	17-Jun-99 (noifr01) 
**	    bug #97482: the HasSystemObjects() was
**	    returning FALSE for databases against a gateway, resulting
**	    indirectly in bug #97482: 
**	    checking "system objects" in a gateway DOM window didn't
**	    refresh the databases (since databases were considered as
**	    having no possible system table)
**	05-Jan-2000 (noifr01)
**	    bug #99866: disabled the "drop" menu item for an ICE server
**	    variable, since the ICE server API functions don't provide
**	    the functionality  
**	17-feb-2000 (somsa01)
**	    On HP, we need to set the UNIX95 variable so that we can get
**	    the proper flags for the 'ps' command.
**	03-Aug-2000 (schph01)
**      (bug 99866) reenabled the "drop" menu item for an ICE server
**	    variable, since the ICE server API functions now provides
**      the functionality as per change 446504 for bug 102229
**  04-Aug-2000 (noifr01 and schph01)
**      (bug 102277) consider any table/view/dbevent with a name starting
**       with "ii" as a system object, which was previously the case only
**      against gateways
**  14-Sep-2000 (noifr01)
**      corrective fix for previous fix (bug 102277): the change had the side
**      effect of have the replicator "dd_" catalogs no more considered as
**      system tables (for ingres installations)
**  25-Sep-2000 (uk$so01)
**      bug 99242 (DBCS) modify the functions: UNIXProcessRunning()
**      NOTE: manipulating the characters like '\n', '/' is DBCS compliant
**            because these characters are not in the DBCS trailing bytes.
**  08-dec-2000 (somsa01)
**      Added include of tchar.h for MainWin.
**  20-Dec-2000 (noifr01)
**   (SIR 103548) removed obsolete IsProcessRunning() function, since it
**   didn't take into account the installation ID (in the case there are 
**   more than one)
**  21-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
**  30-Mar-2001 (noifr01)
**    (sir 104378) differentiation of II 2.6.
**  31-May-2001 (uk$so01)
**    SIR #104796 (Sql Assistant, put the single quote to surrounds the values
**    of the insert or update statement.
**  28-Jun-2001 (noifr01)
**    (SIR 103694) added unicode constants in IngGetStringType() function
**    (now needed for usage by the assistant)
**  10-Dec-2001 (noifr01)
**    (sir 99596) removal of obsolete code and resources
**  28-Mar-2002 (noifr01)
**    (sir 99596) removed additional unused resources/sources
**  18-Mar-2003 (schph01 and noifr01)
**    (sir 107523) manage sequences
**  27-Mar-2003 (noifr01)
**    (bug 109837) disable functionality against older versions of Ingres,
**    that would require to update rmcmd on these platforms
**  28-Mar-2002 (noifr01)
**    (sir 99596) removed additional unused resources/sources
**  12-May-2004 (schph01)
**    (SIR 111507) Add management for new column type bigint
**  10-Nov-2004 (schph01)
**    (bug 113426) disabled the "Add","drop" menu items for an ICE Static
**    branch (OT_ICE_SUBSTATICS or OT_ICE_MAIN), new type when no ice
**    server detected.
**  05-Oct-2006 (gupsh01)
**    Modified date to ingresdate.
**  12-Oct-2006 (wridu01)
**    (Sir 116835) Added support for Ansi Date/Time data types.
**  11-May-2010 (drivi01)
**    Added function CanObjectExistInVectorWise which returns
**    TRUE or FALSE depending on if the object supported by
**    VectorWise.
** 30-Jun-2010 (drivi01)
**    Bug #124006
**    Add new BOOLEAN datatype.
********************************************************************/

#include "dba.h" 
#include "dbaset.h"
#include "main.h"
#include "resource.h"
#include "dbaginfo.h"  // GetGWType
#include "dbadlg1.h" // PS
#include "error.h"
#include "dom.h"
#include "tree.h"

#if defined (MAINWIN)
#include <tchar.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

static LPUCHAR lpstaticdisppublic= "(public)";
static LPUCHAR lpstaticsyspublic = "public";
static LPUCHAR lpdefprofile      = "(default profile)";

static int strcmpignoringquotes(LPCTSTR pS1,LPCTSTR pS2)
{
   LPCTSTR ps11 = RemoveDisplayQuotesIfAny(pS1);
   LPCTSTR ps21 = RemoveDisplayQuotesIfAny(pS2);
   return lstrcmp(ps11,ps21);
}

BOOL IsSystemObject(iobjecttype, lpobjectname, lpobjectowner)
int     iobjecttype;
LPUCHAR lpobjectname;
LPUCHAR lpobjectowner;
{
   TCHAR   tchQuoteDollar [] = {'"', '$', EOS};
   if (HasOwner(iobjecttype)) {
      if (!x_stricmp(lpobjectowner,"$ingres"))
         return TRUE;
   }

   if (iobjecttype==OT_GROUPUSER && !x_stricmp(lpobjectname,"$ingres"))
      return FALSE;        // $ingres in a group should appear
                           // but not be expandable

   if (lpobjectname[0]=='$')
      return TRUE;
   if (! x_strncmp(lpobjectname,tchQuoteDollar,2)) // quoted (system) object name
       return TRUE; 

   switch (iobjecttype) {
      case OT_USER  :
      case OT_SCHEMAUSER  :
         if (!x_stricmp(lpobjectname,"$ingres"))
            return TRUE;
         else
            return FALSE;
         break;
      case OT_TABLE:
      case OT_VIEW:
      case OT_DBEVENT:
         if (!x_strnicmp(lpobjectname,"ii",2)) /* ii tables always considered as system objects */
            return TRUE;
         if (GetOIVers() == OIVERS_NOTOI) 
            return FALSE; /* generic gateways: don't change old logic where dd_ tables are not system objects */

         if (!x_strnicmp(lpobjectname,"dd_",3))  // TO BE DISCUSSED
            return TRUE;
         else
            return FALSE;
         break;
         
      case OT_LOCATION:
      case OT_TABLELOCATION:
         if (!x_strnicmp(lpobjectname,"ii_",3))  // TO BE DISCUSSED
            return TRUE;
         else
            return FALSE;
         break;
		case OT_DATABASE:
			if (!x_stricmp(lpobjectname,"iidbdb"))
				return TRUE;
			break;
		default:
			break;
   }

   if (!x_strnicmp(lpobjectname,"ii_",3))
      return TRUE;

   return FALSE;
}

BOOL HasSystemObjects(iobjecttype)
int     iobjecttype;
{
   if (HasOwner(iobjecttype))
      return TRUE;

   switch (iobjecttype) {
      /* noifr01 (17-Jun-99) (bug #97482): the "HasOwner()" criteria above*/
      /* was not sufficient for DATABASES in the case of a gateway (where */
      /* the owner is not available, or being understood as a system user,*/
      /* in the general case). Since at least iidbdb is known as a system */
      /* database, DATABASES are now added to the list of particular      */
      /* objects types that possibly can have system objects, regardless  */
      /* of the HasOwner() criteria                                       */
      case OT_DATABASE  :
      case OT_USER  :
      case OT_LOCATION:
      case OT_TABLELOCATION:
      case OT_SCHEMAUSER:
         return TRUE;
         break;
   }
   return FALSE;
}

LPUCHAR lpdefprofiledispstring()
{
   return lpdefprofile;
}


LPUCHAR lppublicdispstring()
{
   return lpstaticdisppublic;
}

LPUCHAR lppublicsysstring()
{
   return lpstaticsyspublic;
}

// defaults to TRUE
BOOL CanObjectBeDeleted(iobjecttype, lpobjectname, lpobjectowner)
int     iobjecttype;
LPUCHAR lpobjectname;
LPUCHAR lpobjectowner;
{
  // Very special
  if (iobjecttype == OT_STATIC_INSTALL_SECURITY)
    return FALSE;

   // Added Emb 17/3/95 to manage "< No xxx >" lines
   if (lpobjectname[0]=='\0')
    return FALSE;

   if (IsSystemObject(iobjecttype, lpobjectname, lpobjectowner))
      return FALSE;

   switch (iobjecttype) {
      case OT_ICE_SUBSTATICS :
      case OT_ICE_MAIN :
        return FALSE;
      case OTR_CDB:
        return FALSE;
      case OT_PROFILE:
		  if (!x_stricmp(lpobjectname,lpdefprofiledispstring()))
			  return FALSE;
      case OT_USER  :
         if (!x_stricmp(lpobjectname,"ingres"))
            return FALSE;
         if (!x_stricmp(lpobjectname,lppublicdispstring()))
            return FALSE;
         break;
      case OT_LOCATION:
         if (!x_strnicmp(lpobjectname,"ii_",3))
            return FALSE;
         break;
      case OT_RULEPROC     :
      case OT_SCHEMAUSER   :
      case OT_TABLELOCATION:
      case OT_VIEWTABLE    :
//      case OTR_TABLEVIEW   :
      case OTR_USERSCHEMA  :
         return FALSE;

      // new style alarms (with 2 sub-branches alarmee and launched dbevent)
      case OT_ALARMEE:
      //case OT_ALARMEE_SOLVED_USER:
      //case OT_ALARMEE_SOLVED_GROUP:
      //case OT_ALARMEE_SOLVED_ROLE:
      case OT_S_ALARM_EVENT:
        return FALSE;

      // All xref grants : not accepted
      case OTR_DBGRANT_ACCESY_DB:     // xref grants on databases
      case OTR_DBGRANT_ACCESN_DB:
      case OTR_DBGRANT_CREPRY_DB:
      case OTR_DBGRANT_CREPRN_DB:
      case OTR_DBGRANT_SEQCRY_DB:
      case OTR_DBGRANT_SEQCRN_DB:
      case OTR_DBGRANT_CRETBY_DB:
      case OTR_DBGRANT_CRETBN_DB:
      case OTR_DBGRANT_DBADMY_DB:
      case OTR_DBGRANT_DBADMN_DB:
      case OTR_DBGRANT_LKMODY_DB:
      case OTR_DBGRANT_LKMODN_DB:
      case OTR_DBGRANT_QRYIOY_DB:
      case OTR_DBGRANT_QRYION_DB:
      case OTR_DBGRANT_QRYRWY_DB:
      case OTR_DBGRANT_QRYRWN_DB:
      case OTR_DBGRANT_UPDSCY_DB:
      case OTR_DBGRANT_UPDSCN_DB:
      case OTR_DBGRANT_SELSCY_DB: 
      case OTR_DBGRANT_SELSCN_DB: 
      case OTR_DBGRANT_CNCTLY_DB: 
      case OTR_DBGRANT_CNCTLN_DB: 
      case OTR_DBGRANT_IDLTLY_DB: 
      case OTR_DBGRANT_IDLTLN_DB: 
      case OTR_DBGRANT_SESPRY_DB: 
      case OTR_DBGRANT_SESPRN_DB: 
      case OTR_DBGRANT_TBLSTY_DB: 
      case OTR_DBGRANT_TBLSTN_DB:
      case OTR_DBGRANT_QRYCPN_DB:
      case OTR_DBGRANT_QRYCPY_DB:
      case OTR_DBGRANT_QRYPGN_DB:
      case OTR_DBGRANT_QRYPGY_DB:
      case OTR_DBGRANT_QRYCON_DB:
      case OTR_DBGRANT_QRYCOY_DB:
      case OTR_GRANTEE_SEL_TABLE:     // xref grants on tables
      case OTR_GRANTEE_INS_TABLE:
      case OTR_GRANTEE_UPD_TABLE:
      case OTR_GRANTEE_DEL_TABLE:
      case OTR_GRANTEE_REF_TABLE:
      case OTR_GRANTEE_CPI_TABLE:
      case OTR_GRANTEE_CPF_TABLE:
      case OTR_GRANTEE_ALL_TABLE:
      case OTR_GRANTEE_SEL_VIEW:      // xref grants on views
      case OTR_GRANTEE_INS_VIEW:
      case OTR_GRANTEE_UPD_VIEW:
      case OTR_GRANTEE_DEL_VIEW:
      case OTR_GRANTEE_RAISE_DBEVENT: // xref grants on dbevents
      case OTR_GRANTEE_REGTR_DBEVENT:
      case OTR_GRANTEE_EXEC_PROC:     // xref grants on procedures
      case OTR_GRANTEE_ROLE:          // xref grants on roles
      case OTR_GRANTEE_NEXT_SEQU:     // xref grants on Sequences
        return FALSE;
      case OT_REPLIC_CDDS_DETAIL:
      case OTR_REPLIC_CDDS_TABLE:
      case OT_REPLIC_REGTABLE:
        return FALSE;

      case OT_REPLICATOR:
        return FALSE;

   }
   return TRUE;
}

// defaults to FALSE
BOOL CanObjectBeAltered(iobjecttype, lpobjectname, lpobjectowner)
int     iobjecttype;
LPUCHAR lpobjectname;
LPUCHAR lpobjectowner;
{
   // Added Emb 17/3/95 to manage "< No xxx >" lines
   if (lpobjectname[0]=='\0')
    return FALSE;

   if (IsSystemObject(iobjecttype, lpobjectname, lpobjectowner))
      return FALSE;

   switch (iobjecttype) {
      case OT_SYNONYM       :
      case OT_SYNONYMOBJECT :
      case OT_TABLELOCATION :
      case OT_VIEW          :
      case OT_VIEWTABLE     :
      case OT_GROUPUSER     :
      case OT_SCHEMAUSER    :
      case OT_INTEGRITY     :
      case OT_PROCEDURE     :
      case OT_RULE:
      case OT_S_ALARM_SELSUCCESS_USER :   // alarms on table
      case OT_S_ALARM_SELFAILURE_USER : 
      case OT_S_ALARM_DELSUCCESS_USER :
      case OT_S_ALARM_DELFAILURE_USER :
      case OT_S_ALARM_INSSUCCESS_USER :
      case OT_S_ALARM_INSFAILURE_USER :
      case OT_S_ALARM_UPDSUCCESS_USER :
      case OT_S_ALARM_UPDFAILURE_USER :
      case OT_TABLEGRANT_SEL_USER     :   // grantees on table
      case OT_TABLEGRANT_INS_USER     :
      case OT_TABLEGRANT_UPD_USER     :
      case OT_TABLEGRANT_DEL_USER     :
      case OT_TABLEGRANT_REF_USER     :
      case OT_TABLEGRANT_CPI_USER     :
      case OT_TABLEGRANT_CPF_USER     :
      case OT_TABLEGRANT_ALL_USER     :
      case OT_TABLEGRANT_INDEX_USER   :  // desktop
      case OT_TABLEGRANT_ALTER_USER   :  // desktop

      case OT_VIEWGRANT_SEL_USER      :   // grantees on view
      case OT_VIEWGRANT_INS_USER      :
      case OT_VIEWGRANT_UPD_USER      :
      case OT_VIEWGRANT_DEL_USER      :
      case OT_DBEGRANT_RAISE_USER     :   // grantees on dbevent
      case OT_DBEGRANT_REGTR_USER     :
      case OT_PROCGRANT_EXEC_USER     :   // grantees on procedure
      case OT_SEQUGRANT_NEXT_USER     :   // grantees on sequence
      case OT_ROLEGRANT_USER          :   // grantees on role
      case OT_S_ALARM_CO_SUCCESS_USER :   // alarms on database
      case OT_S_ALARM_CO_FAILURE_USER :
      case OT_S_ALARM_DI_SUCCESS_USER :
      case OT_S_ALARM_DI_FAILURE_USER :

         return FALSE;

      case OT_RULEPROC     :
         return FALSE;

      case OT_LOCATION:
      case OT_PROFILE:
      case OT_GROUP:
      case OT_ROLE:
      case OT_REPLIC_CDDS :
        return TRUE;

      case OT_USER:
         if (!x_stricmp(lpobjectname,lppublicdispstring()))
            return FALSE;
         else
            return TRUE;
         break;

      // new style alarms (with 2 sub-branches alarmee and launched dbevent)
      case OT_ALARMEE:
      //case OT_ALARMEE_SOLVED_USER:
      //case OT_ALARMEE_SOLVED_GROUP:
      //case OT_ALARMEE_SOLVED_ROLE:
      case OT_S_ALARM_EVENT:
        return FALSE;
      case OT_REPLIC_CONNECTION:
        return TRUE;

      // alter not available for desktop anymore,
      // available only on pure ingres nodes
      /* 27-Mar-2003 disabled against <2.5 versions since it would require*/
      /* to update rmcmd on those levels of the server */
      case OT_DATABASE:
        if (GetOIVers() < OIVERS_25)
          return FALSE;
        return TRUE;
        break;

      case OT_TABLE:
        {
            if (GetOIVers() >= OIVERS_20)
                return TRUE;
            return FALSE;
        }

      // level 2, child of "schemauser on database"
      case OT_SCHEMAUSER_TABLE:
        {
            if (GetOIVers() >= OIVERS_20)
                return TRUE;
            return FALSE;
        }

      // level 2, child of "schemauser on database"
      case OT_SCHEMAUSER_VIEW:
      case OT_SCHEMAUSER_PROCEDURE:
        return TRUE;

      case OT_ICE_WEBUSER_ROLE         :
      case OT_ICE_WEBUSER_CONNECTION   :
      case OT_ICE_PROFILE_ROLE         :
      case OT_ICE_PROFILE_CONNECTION   :
      case OT_ICE_BUNIT                :
      case OT_ICE_SERVER_APPLICATION   :
        return FALSE;

      case OT_ICE_ROLE                 :
      case OT_ICE_DBUSER               :
      case OT_ICE_DBCONNECTION         :
      case OT_ICE_WEBUSER              :
      case OT_ICE_PROFILE              :
      case OT_ICE_BUNIT_SEC_ROLE       :
      case OT_ICE_BUNIT_SEC_USER       :
      case OT_ICE_BUNIT_FACET          :
      case OT_ICE_BUNIT_PAGE           :
      case OT_ICE_SERVER_LOCATION      :
      case OT_ICE_SERVER_VARIABLE      :
        return TRUE;

      case OT_ICE_BUNIT_FACET_ROLE:
      case OT_ICE_BUNIT_FACET_USER:
      case OT_ICE_BUNIT_PAGE_ROLE :
      case OT_ICE_BUNIT_PAGE_USER :
      case OT_SEQUENCE      :
        return TRUE;

      default:
         return FALSE;  /* not implemented yed */
   }

   return FALSE;  /* not implemented yed */
}

// defaults to TRUE
BOOL CanObjectBeAdded(iobjecttype)
int     iobjecttype;
{
  // Very special
  if (iobjecttype == OT_STATIC_INSTALL_SECURITY)
    return FALSE;

  if (iobjecttype == OT_ICE_SUBSTATICS)
    return FALSE;

  if (iobjecttype == OT_ICE_MAIN)
    return FALSE;

   switch (iobjecttype) {
      case OT_ICE_SUBSTATICS:
      case OT_ICE_MAIN:
        return FALSE;
      case OTR_CDB:
        return FALSE;

      case OT_TABLELOCATION :
      case OT_VIEWTABLE     :
      case OT_SCHEMAUSER    :
      case OT_RULEPROC      :
      case OTR_TABLEVIEW    :
      case OTR_USERSCHEMA   :
      case OTR_USERGROUP    :
      case OTR_LOCATIONTABLE:
      case OTR_ALARM_SELSUCCESS_TABLE :
      case OTR_ALARM_SELFAILURE_TABLE :
      case OTR_ALARM_DELSUCCESS_TABLE :
      case OTR_ALARM_DELFAILURE_TABLE :
      case OTR_ALARM_INSSUCCESS_TABLE :
      case OTR_ALARM_INSFAILURE_TABLE :
      case OTR_ALARM_UPDSUCCESS_TABLE :
      case OTR_ALARM_UPDFAILURE_TABLE :
      case OT_SYNONYMOBJECT:
      case OTR_PROC_RULE   :
      case OT_REPLIC_REGTABLE:
      case OT_REPLIC_CDDS_DETAIL:
      case OTR_REPLIC_CDDS_TABLE :
      case OTR_REPLIC_TABLE_CDDS :
         return FALSE;

      // new style alarms (with 2 sub-branches alarmee and launched dbevent)
      case OT_ALARMEE:
      //case OT_ALARMEE_SOLVED_USER:
      //case OT_ALARMEE_SOLVED_GROUP:
      //case OT_ALARMEE_SOLVED_ROLE:
      case OT_S_ALARM_EVENT:
        return FALSE;

      case OT_REPLICATOR:
        return FALSE;

      // level 2, child of "schemauser on database"
      case OT_SCHEMAUSER_TABLE:
      case OT_SCHEMAUSER_VIEW:
      case OT_SCHEMAUSER_PROCEDURE:
        return FALSE;
   }
   return TRUE;  
}

// Says whether an object definition can be copied into the clipboard
BOOL CanObjectBeCopied(int iobjecttype, LPUCHAR lpobjectname, LPUCHAR lpobjectowner)
{
  // Emb Sep 25, 98: don't copy items such as <No xyz> or <Data Unavailable>
  if (lpobjectname[0] == '\0')
    return FALSE;

  // Emb July 3, 97: don't copy system objects
  if (IsSystemObject(iobjecttype, lpobjectname, lpobjectowner))
    return FALSE;


  // gwtype_none
  switch (iobjecttype) {
    // Masqued 9/5/95 Emb
    //case OT_ROLE:
    //  return FALSE;
    
    // Added Dec. 11, 96
    case OT_PROFILE:
      return FALSE;

    case OT_SCHEMAUSER_TABLE:
    case OT_SCHEMAUSER_VIEW:
    case OT_SCHEMAUSER_PROCEDURE:
      return TRUE;

    case OT_DATABASE:
      return FALSE;

    case OT_ICE_WEBUSER_ROLE         :
    case OT_ICE_WEBUSER_CONNECTION   :
    case OT_ICE_PROFILE_ROLE         :
    case OT_ICE_PROFILE_CONNECTION   :
    case OT_ICE_BUNIT                :
    case OT_ICE_SERVER_APPLICATION   :
    case OT_ICE_ROLE                 :
    case OT_ICE_DBUSER               :
    case OT_ICE_DBCONNECTION         :
    case OT_ICE_WEBUSER              :
    case OT_ICE_PROFILE              :
    case OT_ICE_BUNIT_SEC_ROLE       :
    case OT_ICE_BUNIT_SEC_USER       :
    case OT_ICE_BUNIT_FACET          :
    case OT_ICE_BUNIT_PAGE           :
    case OT_ICE_SERVER_LOCATION      :
    case OT_ICE_SERVER_VARIABLE      :
    case OT_ICE_BUNIT_FACET_ROLE:
    case OT_ICE_BUNIT_FACET_USER:
    case OT_ICE_BUNIT_PAGE_ROLE :
    case OT_ICE_BUNIT_PAGE_USER :
        return FALSE;

    default:
      return CanObjectBeAdded(iobjecttype);
  }
}

// says whether the structure of an object can be modified
// defaults to FALSE
BOOL CanObjectStructureBeModified(int iobjecttype)
{
  switch (iobjecttype) {
    case OT_TABLE:
    case OT_INDEX:
    case OT_SCHEMAUSER_TABLE:
      return TRUE;
    default:
      return FALSE;
  }
  return FALSE;
}

BOOL CanObjectExistInVectorWise(int iobjecttype)
{

	if (IsVW())
	{
		switch(iobjecttype)
		{
		case OT_TABLE:
		case OT_STATIC_TABLE:
		case OT_VIEW:
		case OT_STATIC_VIEW:
		case OT_SCHEMAUSER_TABLE:
		case OT_SCHEMAUSER_VIEW:
		case OT_SYNONYM:
			return FALSE;
		}
	}
	return TRUE;
}

// defaults to FALSE
BOOL HasProperties4display(iobjecttype)
int     iobjecttype;
{


   switch (iobjecttype) {
      case OTR_CDB:
        return TRUE;

      case OT_LOCATION :
      case OT_PROFILE  :
      case OT_USER:
      case OT_GROUP    :
      case OT_ROLE     :
      case OT_VIEW     :
      case OT_INTEGRITY:
      case OT_TABLE    :
      case OT_INDEX    :
      case OT_DATABASE :

      case OT_GROUPUSER:
      case OT_TABLELOCATION  :
      case OTR_PROC_RULE     :     
      case OTR_LOCATIONTABLE :     
      case OTR_TABLEVIEW     :     
      case OTR_USERGROUP     :     
      case  OTR_USERSCHEMA   :

                       
      case OTR_DBGRANT_ACCESY_DB  :
      case OTR_DBGRANT_ACCESN_DB  :
      case OTR_DBGRANT_CREPRY_DB  :
      case OTR_DBGRANT_CREPRN_DB  :
      case OTR_DBGRANT_SEQCRY_DB  :
      case OTR_DBGRANT_SEQCRN_DB  :
      case OTR_DBGRANT_CRETBY_DB  :
      case OTR_DBGRANT_CRETBN_DB  :
      case OTR_DBGRANT_DBADMY_DB  :
      case OTR_DBGRANT_DBADMN_DB  :
      case OTR_DBGRANT_LKMODY_DB  :
      case OTR_DBGRANT_LKMODN_DB  :
      case OTR_DBGRANT_QRYIOY_DB  :
      case OTR_DBGRANT_QRYION_DB  :
      case OTR_DBGRANT_QRYRWY_DB  :
      case OTR_DBGRANT_QRYRWN_DB  :
      case OTR_DBGRANT_UPDSCY_DB  :
      case OTR_DBGRANT_UPDSCN_DB  :
      case OTR_DBGRANT_SELSCY_DB  : 
      case OTR_DBGRANT_SELSCN_DB  : 
      case OTR_DBGRANT_CNCTLY_DB  : 
      case OTR_DBGRANT_CNCTLN_DB  : 
      case OTR_DBGRANT_IDLTLY_DB  : 
      case OTR_DBGRANT_IDLTLN_DB  : 
      case OTR_DBGRANT_SESPRY_DB  : 
      case OTR_DBGRANT_SESPRN_DB  : 
      case OTR_DBGRANT_TBLSTY_DB  : 
      case OTR_DBGRANT_TBLSTN_DB  : 
      case OTR_DBGRANT_QRYCPN_DB  :
      case OTR_DBGRANT_QRYCPY_DB  :
      case OTR_DBGRANT_QRYPGN_DB  :
      case OTR_DBGRANT_QRYPGY_DB  :
      case OTR_DBGRANT_QRYCON_DB  :
      case OTR_DBGRANT_QRYCOY_DB  :

      case OTR_GRANTEE_SEL_TABLE  :
      case OTR_GRANTEE_INS_TABLE  :
      case OTR_GRANTEE_UPD_TABLE  :
      case OTR_GRANTEE_DEL_TABLE  :
      case OTR_GRANTEE_REF_TABLE  :
      case OTR_GRANTEE_CPI_TABLE  :
      case OTR_GRANTEE_CPF_TABLE  :
      case OTR_GRANTEE_ALL_TABLE  :
      case OTR_GRANTEE_SEL_VIEW   :
      case OTR_GRANTEE_INS_VIEW   :
      case OTR_GRANTEE_UPD_VIEW   :
      case OTR_GRANTEE_DEL_VIEW   :
      case OTR_GRANTEE_EXEC_PROC  :
      case OTR_GRANTEE_ROLE       :

      case OTR_ALARM_SELSUCCESS_TABLE : 
      case OTR_ALARM_SELFAILURE_TABLE : 
      case OTR_ALARM_DELSUCCESS_TABLE : 
      case OTR_ALARM_DELFAILURE_TABLE : 
      case OTR_ALARM_INSSUCCESS_TABLE : 
      case OTR_ALARM_INSFAILURE_TABLE :
      case OTR_ALARM_UPDSUCCESS_TABLE :
      case OTR_ALARM_UPDFAILURE_TABLE :

      // added Emb 11/7/95
      case OT_RULEPROC:

      // added Emb 10/7/96
      case OT_REPLIC_REGTABLE:
      case OTR_REPLIC_CDDS_TABLE:

      case OT_RULE     :
      case OT_PROCEDURE:
      case OT_SEQUENCE:
         return TRUE;

      case OT_SCHEMAUSER_TABLE:
      case OT_SCHEMAUSER_VIEW:
      case OT_SCHEMAUSER_PROCEDURE:
        return TRUE;

   }
   return FALSE;  
}


BOOL NonDBObjectHasOwner(iobjecttype)
int iobjecttype;
{
   if (iobjecttype==OT_DATABASE)
      return FALSE;
   else
      return HasOwner(iobjecttype);
}

BOOL HasOwner(iobjecttype)
int iobjecttype;
{
   // a database has no owner under OpenIngres Desktop
	if (iobjecttype == OT_DATABASE) {
		if (GetOIVers() == OIVERS_NOTOI )
			return FALSE;
	}

   switch (iobjecttype) {
      case OT_DATABASE        :
      case OT_TABLE           :
      case OT_VIEWTABLE       :
      case OT_VIEW            :
      case OT_INDEX           :
      case OT_PROCEDURE       :
      case OT_SEQUENCE        :
      case OT_RULE            :
      case OT_SYNONYM         :
      case OT_DBEVENT         :
      case OT_RULEPROC        :

        case OT_S_ALARM_EVENT:

        case OT_REPLIC_REGTABLE:    // Added Emb Sep. 11, 95

        // new types for schemauser
        case OT_SCHEMAUSER_TABLE:
        case OT_SCHEMAUSER_VIEW:
        case OT_SCHEMAUSER_PROCEDURE:

         return TRUE;
      default:
         return FALSE;
   }
}


// defaults to FALSE
BOOL HasExtraDisplayString(iobjecttype)
int iobjecttype;
{
   switch (iobjecttype) {
      case OT_LOCATION:                  /* start of area name           */
      case OT_RULE :                     /* start of text of rule        */
      case OT_INDEX :                    /* storage structure            */
      case OT_PROCEDURE:                 /* start of procedure text      */
      case OT_DBGRANT_QRYIOY_USER     :  /* query_io_limit               */
      case OT_DBGRANT_QRYRWY_USER     :  /* query_row_limit              */
      case OT_DBGRANT_CNCTLY_USER     :
      case OT_DBGRANT_SESPRY_USER     :
      case OT_DBGRANT_IDLTLY_USER     :
      case OT_SYNONYM:                   /* Object sysnonym is defined on */
      case OT_REPLIC_CONNECTION:         /* connection type and server */
      case OT_REPLIC_REGTABLE:           /* CDDS flags */

      // replicator v11
      case OT_REPLIC_CONNECTION_V11:     /* DBMS type */

         return TRUE;
      default:
         return FALSE;
   }
}


int IngGetStringType(lpstring, ilen)
LPUCHAR lpstring;
int ilen;
{
   UCHAR buf[MAXOBJECTNAME];
   struct int2string *typestring;
   struct int2string typestrings[]= {
      { INGTYPE_C           , "c"               },
      { INGTYPE_CHAR        , "char"            },
      { INGTYPE_CHAR        , "character"       },
      { INGTYPE_TEXT        , "text",           },
      { INGTYPE_VARCHAR     , "varchar"         },
      { INGTYPE_LONGVARCHAR , "long varchar"    },
      { INGTYPE_BIGINT      , "bigint"          },
      { INGTYPE_BOOLEAN     , "boolean"         },
      { INGTYPE_INT8        , "int8"            },
      { INGTYPE_INT4        , "integer"         },
      { INGTYPE_INT2        , "smallint"        },
      { INGTYPE_INT1        , "integer1"        },
      { INGTYPE_DECIMAL     , "decimal"         },
      { INGTYPE_FLOAT8      , "float8"          },
      { INGTYPE_FLOAT8      , "float"           },
      { INGTYPE_FLOAT4      , "float4"          },
      { INGTYPE_FLOAT4      , "real"            },
      { INGTYPE_DATE        , "ingresdate"      },
      { INGTYPE_ADTE        , "ansidate"                       },
      { INGTYPE_TMWO        , "time without time zone"         },
      { INGTYPE_TMW         , "time with time zone"            },
      { INGTYPE_TME         , "time with local time zone"      },
      { INGTYPE_TSWO        , "timestamp without time zone"    },
      { INGTYPE_TSW         , "timestamp with time zone"       },
      { INGTYPE_TSTMP       , "timestamp with local time zone" },
      { INGTYPE_INYM        , "interval year to month"         },
      { INGTYPE_INDS        , "interval day to second"         },
      { INGTYPE_IDTE        , "ingresdate"                     },
      { INGTYPE_MONEY       , "money"           },
      { INGTYPE_BYTE        , "byte"            },
      { INGTYPE_BYTEVAR     , "byte varying"    },
      { INGTYPE_LONGBYTE    , "long byte"       },
      { INGTYPE_UNICODE_NCHR, "nchar"           },
      { INGTYPE_UNICODE_NVCHR,"nvarchar"        },
      { INGTYPE_UNICODE_LNVCHR,"long nvarchar"  },
      { INGTYPE_OBJKEY      , "??"              }, /* see comment */
      { INGTYPE_TABLEKEY    , "??"              },
      { INGTYPE_SECURITYLBL , "??"              },
      { INGTYPE_SHORTSECLBL , "??"              },
      { 0                   ,(char *)0          }  };

      /* Last entries are unused at this time. Bytes seem to map to chars.*/
      /* To be filled if needed according to the strings found for that need*/

   fstrncpy(buf,lpstring,sizeof(buf));
   suppspace(buf);
   for (typestring=typestrings;typestring->pchar;typestring++) {
      if (!x_stricmp(typestring->pchar,buf)) {
         switch (typestring->item) {
            case INGTYPE_INT8:
            case INGTYPE_INT4:
            case INGTYPE_INT2:
            case INGTYPE_INT1:
               switch (ilen) {
                  case 8:   return INGTYPE_INT8; break;
                  case 4:   return INGTYPE_INT4; break;
                  case 2:   return INGTYPE_INT2; break;
                  case 1:   return INGTYPE_INT1; break;
               }
               break;
            case INGTYPE_FLOAT8:
            case INGTYPE_FLOAT4:
               switch (ilen) {
                  case 8:   return INGTYPE_FLOAT8; break;
                  case 4:   return INGTYPE_FLOAT4; break;
               }
               break;
            default:
               break;
         }
         return typestring->item;
      }
   }
   return INGTYPE_ERROR;
}

BOOL CanObjectHaveSchema(iobjecttype)
int iobjecttype;
{
   switch (iobjecttype) {
      case OT_TABLE:
      case OT_VIEW:
      case OT_INDEX:
      case OT_PROCEDURE:
      case OT_SEQUENCE:
      case OT_RULE:
      case OT_DBEVENT:
         return TRUE;
         break;
      default:
         return FALSE;
         break;
   }
   return FALSE;
}

int DOMTreeCmpllGrants(p1,powner1,pextra1,p2,powner2,pextra2)
LPUCHAR p1;
LPUCHAR powner1;
LPUCHAR pextra1;
LPUCHAR p2;
LPUCHAR powner2;
LPUCHAR pextra2;
{
  int resutemp = DOMTreeCmpTableNames(p1,powner1,p2,powner2);
  if (!resutemp) 
    resutemp = x_strcmp(pextra1,pextra2);
  return resutemp;
}


int DOMTreeCmpStr(p1,powner1,p2,powner2,iobjecttype, bEqualIfMixed)
LPUCHAR p1;
LPUCHAR powner1;
LPUCHAR p2;
LPUCHAR powner2;
int iobjecttype;
BOOL bEqualIfMixed;/* TRUE if wish return 0 if one has dot, and other not*/
{
  char *pchar1,*pchar2;
  int icmp ;
  int icmp2;
  if (!CanObjectHaveSchema(iobjecttype))
    return x_strcmp(p1,p2);

  pchar1=GetSchemaDot(p1); //pchar1=strchr(p1,'.');
  pchar2=GetSchemaDot(p2); //pchar2=strchr(p2,'.');
  
  if (pchar1) {
    if (!pchar2) {
      icmp2=strcmpignoringquotes(pchar1+1,p2);
      if (bEqualIfMixed || icmp2)
        return icmp2;
      else 
        return strcmpignoringquotes(powner1,powner2);
    }
    else /* both have a dot */
      //return DOMTreeCmpDoubleStr(pchar1+1,p1,pchar2+1,p2);
      return DOMTreeCmpDoubleStr(pchar1+1,powner1,pchar2+1,powner2);
  }
  else {  /* pchar1 null */
    if (pchar2) {
      icmp2=strcmpignoringquotes(p1,pchar2+1);
      if (bEqualIfMixed || icmp2)
        return icmp2;
    }
  }
  icmp = strcmpignoringquotes(p1,p2);
  if (bEqualIfMixed || icmp)              /* both are NULL */
    return icmp;
  return strcmpignoringquotes(powner1,powner2); 

}

int DOMTreeCmpTableNames(p1,powner1,p2,powner2)
LPUCHAR p1;
LPUCHAR powner1;
LPUCHAR p2;
LPUCHAR powner2;
{
  char *pchar1,*pchar2;
  int icmp;
  int icmp2;
  char buf[MAXOBJECTNAME];

  pchar1=GetSchemaDot(p1);
  pchar2=GetSchemaDot(p2);
  if (pchar1) {
    if (!pchar2) {
      icmp2=strcmpignoringquotes(pchar1+1,p2);
      if (icmp2)
        return icmp2;
      else {
        x_strcpy(buf,p1);
        pchar1=GetSchemaDot(buf);
        *pchar1='\0';
        return strcmpignoringquotes(RemoveDisplayQuotesIfAny(buf),powner2);
      }
    }
    else /* both have a dot */
      return DOMTreeCmpDoubleStr(pchar1+1,p1,pchar2+1,p2);
  }
  else {  /* pchar1 null */
    if (pchar2) {
      icmp2=strcmpignoringquotes(p1,pchar2+1);
      if (icmp2)
        return icmp2;
      else {
        x_strcpy(buf,p2);
        pchar2=GetSchemaDot(buf);
        *pchar2='\0';
        return strcmpignoringquotes(powner1,RemoveDisplayQuotesIfAny(buf));
      }
    }
  }
  icmp = strcmpignoringquotes(p1,p2);
  if (icmp)              /* both are NULL */
    return icmp;
  return strcmpignoringquotes(powner1,powner2);

}

int DOMTreeCmpDoubleStr(p11,p12,p21,p22)
LPUCHAR p11;
LPUCHAR p12;
LPUCHAR p21;
LPUCHAR p22;
{
  int resutemp = strcmpignoringquotes(p11,p21);
  if (!resutemp) 
    resutemp = strcmpignoringquotes(p12,p22);
  return resutemp;
}

int DOMTreeCmpTablesWithDB(p11,p12,p13,p21,p22,p23)
LPUCHAR p11;
LPUCHAR p12;
LPUCHAR p13;
LPUCHAR p21;
LPUCHAR p22;
LPUCHAR p23;
{
  int resutemp = x_strcmp(p11,p21);
  if (!resutemp) 
    resutemp = DOMTreeCmpTableNames(p12,p13,p22,p23);
  return resutemp;
}


int  DOMTreeCmpSecAlarms(p1, l1, p2, l2)
LPUCHAR p1;
long l1;
LPUCHAR p2;
long l2;

{
  int resutemp = x_strcmp(p1,p2);
  if (!resutemp) {
    if (l1>l2)
      return 1;
    if (l1<l2)
      return (-1);
    return 0;
  }
  return resutemp;
}

int DOMTreeCmpRelSecAlarms(p11, p12, p1owner, l1, p21, p22, p2owner, l2)
LPUCHAR p11;
LPUCHAR p12;
LPUCHAR p1owner;
long l1;
LPUCHAR p21;
LPUCHAR p22;
LPUCHAR p2owner;
long l2;
{
  int resutemp;
  resutemp = x_strcmp(p11,p21);
  if (!resutemp) {
     resutemp=DOMTreeCmpTableNames(p12,p1owner,p22,p2owner);
     if (!resutemp) {
       if (l1>l2)
         return 1;
       if (l1<l2)
         return (-1);
       return 0;
     }
  }
  return resutemp;
}

int DOMTreeCmpllSecAlarms(p11, p1owner, usr1, l1, p21, p2owner, usr2, l2)
LPUCHAR p11;
LPUCHAR p1owner;
LPUCHAR usr1;
long    l1;
LPUCHAR p21;
LPUCHAR p2owner;
LPUCHAR usr2;
long    l2;
{
  int resutemp;
  resutemp=DOMTreeCmpTableNames(p11,p1owner,p21,p2owner);
  if (!resutemp) {
     resutemp=x_strcmp(usr1,usr2);
     if (!resutemp) {
       if (l1>l2)
         return 1;
       if (l1<l2)
         return (-1);
       return 0;
     }
  }
  return resutemp;
}

 
int DOMTreeCmp4ints(i11,i12,i13,i14,i21,i22,i23,i24)
int i11;
int i12;
int i13;
int i14;
int i21;
int i22;
int i23;
int i24;
{
  int i1[4],i2[4],i;
  i1[0]=i11; i1[1]=i12; i1[2]=i13; i1[3]=i14;
  i2[0]=i21; i2[1]=i22; i2[2]=i23; i2[3]=i24;

  for (i=0;i<4;i++) {
    if (i1[i]>i2[i])
      return 1;
    if (i1[i]<i2[i])
      return (-1);
  }
  return 0;
}


 // TO BE FINISHED: strings should not be loaded through LoadString()
 // as this code is environment independent, but loaded through a generic
 // function

//
//  This function fills the received buffer with a string that
//  will be used as a caption for the extra display string.
//  if the object type has no extra display string, we will return
//  an empty buffer.
//  The function returns a pointer to the received buffer
//  The received buffer is assumed to be BUFSIZE long
//
//  IMPORTANT: this function should be updated for every
//  new object type added in HasExtraDisplayString()!
//

//
// Receives the object type,
// and returns the level that must be passed to UpdateDOMData
//
// IMPORTANT NOTE : this function needs to be updated for every new type
//
int GetParentLevelFromObjType(int iobjecttype)
{
  switch (iobjecttype) {
    // level 0
    case OT_DATABASE:
    case OT_PROFILE:
    case OT_USER:
    case OT_GROUP:
    case OT_ROLE:
    case OT_LOCATION:
    case OTLL_DBGRANTEE:
      return 0;

    // level 1, child of database
    case OTR_CDB:
    case OT_TABLE:
    case OT_VIEW:
    case OT_PROCEDURE:
    case OT_SEQUENCE:
    case OT_SCHEMAUSER:
    case OT_SYNONYM:
    case OT_DBGRANT_ACCESY_USER:
    case OT_DBGRANT_ACCESN_USER:
    case OT_DBGRANT_CREPRY_USER:
    case OT_DBGRANT_CREPRN_USER:
    case OT_DBGRANT_CRETBY_USER:
    case OT_DBGRANT_CRETBN_USER:
    case OT_DBGRANT_DBADMY_USER:
    case OT_DBGRANT_DBADMN_USER:
    case OT_DBGRANT_LKMODY_USER:
    case OT_DBGRANT_LKMODN_USER:
    case OT_DBGRANT_QRYIOY_USER:
    case OT_DBGRANT_QRYION_USER:
    case OT_DBGRANT_QRYRWY_USER:
    case OT_DBGRANT_QRYRWN_USER:
    case OT_DBGRANT_UPDSCY_USER:
    case OT_DBGRANT_UPDSCN_USER:
    case OT_DBGRANT_SELSCY_USER: 
    case OT_DBGRANT_SELSCN_USER: 
    case OT_DBGRANT_CNCTLY_USER: 
    case OT_DBGRANT_CNCTLN_USER: 
    case OT_DBGRANT_IDLTLY_USER: 
    case OT_DBGRANT_IDLTLN_USER: 
    case OT_DBGRANT_SESPRY_USER: 
    case OT_DBGRANT_SESPRN_USER: 
    case OT_DBGRANT_TBLSTY_USER: 
    case OT_DBGRANT_TBLSTN_USER: 
    case OT_DBGRANT_QRYCPN_USER:
    case OT_DBGRANT_QRYCPY_USER:
    case OT_DBGRANT_QRYPGN_USER:
    case OT_DBGRANT_QRYPGY_USER:
    case OT_DBGRANT_QRYCON_USER:
    case OT_DBGRANT_QRYCOY_USER:
    case OT_DBGRANT_SEQCRN_USER:
    case OT_DBGRANT_SEQCRY_USER:

    case OTLL_GRANTEE:
    case OTLL_OIDTDBGRANTEE:
    case OTLL_SECURITYALARM:
    case OT_DBEVENT:
    case OT_S_ALARM_CO_SUCCESS_USER:
    case OT_S_ALARM_CO_FAILURE_USER:
    case OT_S_ALARM_DI_SUCCESS_USER:
    case OT_S_ALARM_DI_FAILURE_USER:

    // level 1, child of user
    case OTR_USERSCHEMA:
    case OTR_USERGROUP:
    case OTR_ALARM_SELSUCCESS_TABLE:
    case OTR_ALARM_SELFAILURE_TABLE:
    case OTR_ALARM_DELSUCCESS_TABLE:
    case OTR_ALARM_DELFAILURE_TABLE:
    case OTR_ALARM_INSSUCCESS_TABLE:
    case OTR_ALARM_INSFAILURE_TABLE:
    case OTR_ALARM_UPDSUCCESS_TABLE:
    case OTR_ALARM_UPDFAILURE_TABLE:

    case OTR_GRANTEE_RAISE_DBEVENT:
    case OTR_GRANTEE_REGTR_DBEVENT:
    case OTR_GRANTEE_EXEC_PROC:
    case OTR_GRANTEE_NEXT_SEQU:
    case OTR_DBGRANT_ACCESY_DB:
    case OTR_DBGRANT_ACCESN_DB:
    case OTR_DBGRANT_CREPRY_DB:
    case OTR_DBGRANT_CREPRN_DB:
    case OTR_DBGRANT_SEQCRY_DB:
    case OTR_DBGRANT_SEQCRN_DB:
    case OTR_DBGRANT_CRETBY_DB:
    case OTR_DBGRANT_CRETBN_DB:
    case OTR_DBGRANT_DBADMY_DB:
    case OTR_DBGRANT_DBADMN_DB:
    case OTR_DBGRANT_LKMODY_DB:
    case OTR_DBGRANT_LKMODN_DB:
    case OTR_DBGRANT_QRYIOY_DB:
    case OTR_DBGRANT_QRYION_DB:
    case OTR_DBGRANT_QRYRWY_DB:
    case OTR_DBGRANT_QRYRWN_DB:
    case OTR_DBGRANT_UPDSCY_DB:
    case OTR_DBGRANT_UPDSCN_DB:
    case OTR_DBGRANT_SELSCY_DB: 
    case OTR_DBGRANT_SELSCN_DB: 
    case OTR_DBGRANT_CNCTLY_DB: 
    case OTR_DBGRANT_CNCTLN_DB: 
    case OTR_DBGRANT_IDLTLY_DB: 
    case OTR_DBGRANT_IDLTLN_DB: 
    case OTR_DBGRANT_SESPRY_DB: 
    case OTR_DBGRANT_SESPRN_DB: 
    case OTR_DBGRANT_TBLSTY_DB: 
    case OTR_DBGRANT_TBLSTN_DB: 
    case OTR_DBGRANT_QRYCPN_DB:
    case OTR_DBGRANT_QRYCPY_DB:
    case OTR_DBGRANT_QRYPGN_DB:
    case OTR_DBGRANT_QRYPGY_DB:
    case OTR_DBGRANT_QRYCON_DB:
    case OTR_DBGRANT_QRYCOY_DB:
    case OTR_GRANTEE_SEL_TABLE:
    case OTR_GRANTEE_INS_TABLE:
    case OTR_GRANTEE_UPD_TABLE:
    case OTR_GRANTEE_DEL_TABLE:
    case OTR_GRANTEE_REF_TABLE:
    case OTR_GRANTEE_CPI_TABLE:
    case OTR_GRANTEE_CPF_TABLE:
    case OTR_GRANTEE_ALL_TABLE:
    case OTR_GRANTEE_SEL_VIEW:
    case OTR_GRANTEE_INS_VIEW:
    case OTR_GRANTEE_UPD_VIEW:
    case OTR_GRANTEE_DEL_VIEW:

    // level 1, child of user
    case OTR_GRANTEE_ROLE:
    // level 1, child of role
    case OT_ROLEGRANT_USER:
    // level 1, child of group
    case OT_GROUPUSER:
    // level 1, child of location
    case OTR_LOCATIONTABLE:
      return 1;

    // level 2, child of "table of database"
    case OT_INTEGRITY:
    case OT_RULE:
    case OT_INDEX:
    case OT_COLUMN:
    case OT_TABLELOCATION:
    case OT_S_ALARM_SELSUCCESS_USER:
    case OT_S_ALARM_SELFAILURE_USER:
    case OT_S_ALARM_DELSUCCESS_USER:
    case OT_S_ALARM_DELFAILURE_USER:
    case OT_S_ALARM_INSSUCCESS_USER:
    case OT_S_ALARM_INSFAILURE_USER:
    case OT_S_ALARM_UPDSUCCESS_USER:
    case OT_S_ALARM_UPDFAILURE_USER:

    case OT_TABLEGRANT_SEL_USER:
    case OT_TABLEGRANT_INS_USER:
    case OT_TABLEGRANT_UPD_USER:
    case OT_TABLEGRANT_DEL_USER:
    case OT_TABLEGRANT_REF_USER:
    case OT_TABLEGRANT_CPI_USER:
    case OT_TABLEGRANT_CPF_USER:
    case OT_TABLEGRANT_ALL_USER:
    case OT_TABLEGRANT_INDEX_USER:  // desktop
    case OT_TABLEGRANT_ALTER_USER:  // desktop

    case OTR_TABLESYNONYM:
    case OTR_TABLEVIEW:
    // level 2, child of "view of database"
    case OT_VIEWTABLE:

    case OT_VIEWGRANT_SEL_USER:
    case OT_VIEWGRANT_INS_USER:
    case OT_VIEWGRANT_UPD_USER:
    case OT_VIEWGRANT_DEL_USER:

    case OTR_VIEWSYNONYM:

    // level 2, child of "procedure of database"
    case OT_PROCGRANT_EXEC_USER:
    case OTR_PROC_RULE:

    // level 2, child of "Sequence of database"
    case OT_SEQUGRANT_NEXT_USER:

    // level 2, child of "dbevent of database"
    case OT_DBEGRANT_RAISE_USER:
    case OT_DBEGRANT_REGTR_USER:

    // level 2, child of "schemauser on database"
    case OT_SCHEMAUSER_TABLE:
    case OT_SCHEMAUSER_VIEW:
    case OT_SCHEMAUSER_PROCEDURE:

      return 2;

    // level 3, child of "index on table of database"
    case OTR_INDEXSYNONYM:

    // level 3, child of "rule on table of database"
    case OT_RULEPROC:
      return 3;

    // replicator, all levels mixed
    case OT_REPLIC_CONNECTION:
    case OT_REPLIC_CDDS:
    case OT_REPLIC_MAILUSER:
    case OT_REPLIC_REGTABLE:
      return 1;             // parent = database

    case OT_REPLIC_CDDS_DETAIL:
    case OTR_REPLIC_CDDS_TABLE:
      return 2;             // parents = database + cdds

    case OTR_REPLIC_TABLE_CDDS:
      return 2;             // parents = database + table

    // new style alarms (with 2 sub-branches alarmee and launched dbevent)
    case OT_ALARMEE:
    //case OT_ALARMEE_SOLVED_USER:
    //case OT_ALARMEE_SOLVED_GROUP:
    //case OT_ALARMEE_SOLVED_ROLE:
      return 0;
    case OT_S_ALARM_EVENT:
      return 1;   // same as OT_DBEVENT

    case OT_REPLICATOR:
      return 0;

    //
    // ICE
    //
    // Under "Security"
    case OT_ICE_ROLE                 :
      return 0;
    case OT_ICE_DBUSER               :
      return 0;
    case OT_ICE_DBCONNECTION         :
      return 0;
    case OT_ICE_WEBUSER              :
      return 0;
    case OT_ICE_WEBUSER_ROLE         :
      return 1;
    case OT_ICE_WEBUSER_CONNECTION   :
      return 1;
    case OT_ICE_PROFILE              :
      return 0;
    case OT_ICE_PROFILE_ROLE         :
      return 1;
    case OT_ICE_PROFILE_CONNECTION   :
      return 1;
    // Under "Bussiness unit" (BUNIT)
    case OT_ICE_BUNIT                :
      return 0;
    case OT_ICE_BUNIT_SEC_ROLE       :
      return 1;
    case OT_ICE_BUNIT_SEC_USER       :
      return 1;
    case OT_ICE_BUNIT_FACET          :
      return 1;
    case OT_ICE_BUNIT_PAGE           :
      return 1;

    case OT_ICE_BUNIT_FACET_ROLE:
    case OT_ICE_BUNIT_FACET_USER:
    case OT_ICE_BUNIT_PAGE_ROLE :
    case OT_ICE_BUNIT_PAGE_USER :
      return 2;

    case OT_ICE_BUNIT_LOCATION       :
      return 1;
    // Under "Server"
    case OT_ICE_SERVER_APPLICATION   :
      return 0;
    case OT_ICE_SERVER_LOCATION      :
      return 0;
    case OT_ICE_SERVER_VARIABLE      :
      return 0;

    default:
      return 0;   // UNKNOWN TYPE
  }
}


BOOL IsNT()
{
#ifdef MAINWIN
  // SHOULD NOT BE CALLED
  return FALSE;
#else
  OSVERSIONINFO vsinfo;
  
  vsinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  if (!GetVersionEx (&vsinfo)) {
     myerror(ERR_GW);
     return FALSE;
  }

  if (vsinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
     return TRUE;
  else
     return FALSE;
#endif
}

