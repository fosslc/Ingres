/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : cddsload.c
//    Load tables used by cdds
//
********************************************************************/

//
// Includes
//

// esql and so forth management
#include "dba.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "dbadlg1.h"  // dialog boxes part one
#include "cddsload.h"
#include "tools.h"

struct locallinkedlist {
   UCHAR buf       [MAXOBJECTNAME];
   UCHAR buffilter [MAXOBJECTNAME];
   UCHAR extradata [EXTRADATASIZE];
   struct locallinkedlist *pnext;
   DD_REGISTERED_TABLES RegistTbl;
};

int CDDSLoadAll(LPREPCDDS lpcdds,LPUCHAR lpVirtNode)
{
   UCHAR buf      [MAXOBJECTNAME];  
   UCHAR buffilter[MAXOBJECTNAME];
   UCHAR bufparenttbl[MAXOBJECTNAME];
   UCHAR extradata[EXTRADATASIZE];
   LPUCHAR aparents[MAXPLEVEL];
   LPOBJECTLIST  lpOL;
   int iret,iReplicVersion,iType,level;
   struct locallinkedlist * pllfirst, *pllcur,*pllcurprev;
   BOOL bFirst = TRUE;
   HCURSOR hOldCursor = SetCursor(LoadCursor(NULL,IDC_WAIT));
   pllfirst=(struct locallinkedlist *)0;

   aparents[0]= lpcdds->DBName;
   aparents[1]= '\0';
   level=1;


       // Get everything from dd_connections 
   iReplicVersion=GetReplicInstallStatus(GetVirtNodeHandle(lpVirtNode),lpcdds->DBName,
                                                     lpcdds->DBOwnerName);
   // Get additionnal information on current CDDS: name,error ,collision
   if (iReplicVersion == REPLIC_V11)  {
      iret = ReplicCDDSV11( lpcdds );
      if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
          return RES_ERR;
   }

   if (iReplicVersion == REPLIC_V10 ||
       iReplicVersion == REPLIC_V105  )
      iType = OT_REPLIC_CONNECTION;
   else {
      char buff[20];
      ZEROINIT(buff);
      iType = OT_REPLIC_CDDSDBINFO_V11;
      if (lpcdds->bAlter)
          _itoa(lpcdds->cdds,buff,10);
      else
          _itoa(-5,buff,10);
      aparents[1]=buff;
      aparents[2]='\0';
      level=2;

     }

   for ( iret = DBAGetFirstObject (lpVirtNode,
                      iType,
                      level,
                      aparents,
                      TRUE,
                      buf,
                      buffilter,extradata);

   iret == RES_SUCCESS ;
    iret = DBAGetNextObject(buf,buffilter,extradata)
   )
      {
	  LPDD_CONNECTION lp1;
      lpOL = AddListObjectTail(&lpcdds->connections, sizeof(DD_CONNECTION));
      if (!lpOL)
          break;
      fstrncpy( ((LPDD_CONNECTION)lpOL->lpObject)->szVNode,
              buf,MAXOBJECTNAME);

      fstrncpy( ((LPDD_CONNECTION)lpOL->lpObject)->szDBName,
              buffilter,MAXOBJECTNAME);
      lp1= ((LPDD_CONNECTION)lpOL->lpObject);
      lp1->dbNo = getint(extradata);
      lp1->nTypeRepl = getint(extradata+STEPSMALLOBJ);
      lp1->nServerNo = getint(extradata+(2*STEPSMALLOBJ));
      lp1->CDDSNo    = getint(extradata+(3*STEPSMALLOBJ));
      lp1->bTypeChosen   = FALSE;
      lp1->bServerChosen = FALSE;
	   if (lpcdds->bAlter && lp1->CDDSNo>=0) {
         lp1->bTypeChosen   = TRUE;
         lp1->bServerChosen = TRUE;
	  }

   }

 //  if (lpcdds->bAlter)  {
       // and from dd_distribution

      for (
        iret = DBAGetFirstObject (lpVirtNode,
                       GetRepObjectType4ll(OT_REPLIC_DISTRIBUTION,iReplicVersion),
                           1,
                           aparents,
                           TRUE,
                           buf,
                           buffilter,extradata);
        iret == RES_SUCCESS ;
        iret = DBAGetNextObject(buf,buffilter,extradata)
       )
		{   // now get also the paths of other CDDS for some checks
         if (TRUE  /*(UINT)getint(extradata) == lpcdds->cdds*/)  {
			 LPDD_DISTRIBUTION lp1;
             lpOL = AddListObjectTail(&lpcdds->distribution, sizeof(DD_DISTRIBUTION));

             if (!lpOL)
             break;
			 lp1=(LPDD_DISTRIBUTION)lpOL->lpObject;
			 x_strcpy( lp1->RunNode,buf);
             lp1->CDDSNo =  getint(extradata);
			 lp1->localdb = getint(extradata+STEPSMALLOBJ);
             lp1->source =  getint(extradata+(STEPSMALLOBJ*2));
             lp1->target =  getint(extradata+(STEPSMALLOBJ*3));
         }    
      }
 //  }

//   if (lpcdds->bAlter) {
      //struct locallinkedlist * pllfirst, *pllcur,*pllcurprev;
      //BOOL bFirst = TRUE;
      //pllfirst=(struct locallinkedlist *)0;
      // for each table
      if (iReplicVersion == REPLIC_V10 ||
          iReplicVersion == REPLIC_V105  )   {
         for (
             iret = DBAGetFirstObject (lpVirtNode,
                          GetRepObjectType4ll(OT_REPLIC_REGTBLS,iReplicVersion),
                                 1,
                                 aparents,
                                 TRUE,
                                 buf,
                                 buffilter,extradata);
             iret == RES_SUCCESS ;
             iret = DBAGetNextObject(buf,buffilter,extradata)
             )
             {
            if (bFirst) {
               pllfirst=ESL_AllocMem(sizeof(struct locallinkedlist));
               if (!pllfirst)
                  return RES_ERR;
               pllcur=pllfirst;
               bFirst=FALSE;
            }
            else {
               pllcurprev=pllcur;
               pllcur=ESL_AllocMem(sizeof(struct locallinkedlist));
               pllcurprev->pnext=pllcur;
               if (!pllcur)
                  return RES_ERR;
            }
            memcpy(pllcur->buf,      buf,      MAXOBJECTNAME);
            memcpy(pllcur->buffilter,buffilter,MAXOBJECTNAME);
            memcpy(pllcur->extradata,extradata,EXTRADATASIZE);
         }
      }  else  {
         DD_REGISTERED_TABLES RegTmp;
            for (
                iret = DBAGetFirstObject (lpVirtNode,
                             GetRepObjectType4ll(OT_REPLIC_REGTBLS,iReplicVersion),
                                    1,
                                    aparents,
                                    TRUE,
                                    (char *)&RegTmp,
                                    NULL,NULL);
                iret == RES_SUCCESS ;
                iret = DBAGetNextObject((char *)&RegTmp,NULL,NULL))
            {
               if (bFirst) {
                  pllfirst=ESL_AllocMem(sizeof(struct locallinkedlist));
                  if (!pllfirst)
                     return RES_ERR;
                  pllcur=pllfirst;
                  bFirst=FALSE;
               }
               else {
                  pllcurprev=pllcur;
                  pllcur=ESL_AllocMem(sizeof(struct locallinkedlist));
                  pllcurprev->pnext=pllcur;
                  if (!pllcur)
                     return RES_ERR;
               }
               memset(pllcur->buf      , '\0', MAXOBJECTNAME);
               memset(pllcur->buffilter, '\0', MAXOBJECTNAME);
               memset(pllcur->extradata, '\0', EXTRADATASIZE);
               pllcur->RegistTbl = RegTmp;

            }
      }

      pllcur=pllfirst;
      while (pllcur) {

         UCHAR buf2[MAXOBJECTNAME];  
         UCHAR buffilter2[MAXOBJECTNAME];
         UCHAR extradata2[EXTRADATASIZE];
         int   iret2;

         memcpy(buf,      pllcur->buf,      MAXOBJECTNAME);
         memcpy(buffilter,pllcur->buffilter,MAXOBJECTNAME);
         memcpy(extradata,pllcur->extradata,EXTRADATASIZE);

         lpOL = AddListObjectTail(&lpcdds->registered_tables, sizeof(DD_REGISTERED_TABLES));

         if (!lpOL)
         break;
         if (iReplicVersion == REPLIC_V10 ||
             iReplicVersion == REPLIC_V105   )  {
            x_strcpy(((LPDD_REGISTERED_TABLES)lpOL->lpObject)->tablename,buf);
            x_strcpy(((LPDD_REGISTERED_TABLES)lpOL->lpObject)->rule_lookup_table,buffilter);
            x_strcpy(((LPDD_REGISTERED_TABLES)lpOL->lpObject)->priority_lookup_table,extradata);

            ((LPDD_REGISTERED_TABLES)lpOL->lpObject)->cdds = getint(extradata+MAXOBJECTNAME);
                                             
            ((LPDD_REGISTERED_TABLES)lpOL->lpObject)->table_created[0]    =getint(extradata+MAXOBJECTNAME+(STEPSMALLOBJ));
            ((LPDD_REGISTERED_TABLES)lpOL->lpObject)->table_created_ini[0]=getint(extradata+MAXOBJECTNAME+(STEPSMALLOBJ));
            ((LPDD_REGISTERED_TABLES)lpOL->lpObject)->colums_registered[0]=getint(extradata+MAXOBJECTNAME+(2*STEPSMALLOBJ));
            ((LPDD_REGISTERED_TABLES)lpOL->lpObject)->rules_created[0]    =getint(extradata+MAXOBJECTNAME+(3*STEPSMALLOBJ));
            ((LPDD_REGISTERED_TABLES)lpOL->lpObject)->rules_created_ini[0]=getint(extradata+MAXOBJECTNAME+(3*STEPSMALLOBJ));
            ((LPDD_REGISTERED_TABLES)lpOL->lpObject)->table_created[1]    =0;
            ((LPDD_REGISTERED_TABLES)lpOL->lpObject)->table_created_ini[1]=0;
            ((LPDD_REGISTERED_TABLES)lpOL->lpObject)->colums_registered[1]=0;
            ((LPDD_REGISTERED_TABLES)lpOL->lpObject)->rules_created[1]    =0;
            ((LPDD_REGISTERED_TABLES)lpOL->lpObject)->rules_created_ini[1]=0;
            aparents[1] = StringWithOwner(buf, buffilter, bufparenttbl);
            aparents[2] = '\0';

         }
         else {
            LPDD_REGISTERED_TABLES lptable = ((LPDD_REGISTERED_TABLES)lpOL->lpObject);
            
            memcpy(lpOL->lpObject,&pllcur->RegistTbl,sizeof(DD_REGISTERED_TABLES));
            aparents[1] = StringWithOwner((LPTSTR)Quote4DisplayIfNeeded(pllcur->RegistTbl.tablename),pllcur->RegistTbl.tableowner, bufparenttbl);
            aparents[2] = '\0';
         }
         // now get the columns for this table

         for ( iret2 = DBAGetFirstObject (lpVirtNode,
               GetRepObjectType4ll(OT_REPLIC_REGCOLS,iReplicVersion),
                                   2,
                                   aparents,
                                   TRUE,
                                   buf2,
                                   buffilter2,extradata2);
               iret2 == RES_SUCCESS ;
               iret2 = DBAGetNextObject(buf2,buffilter2,extradata2)
             )  {
             LPOBJECTLIST lpOL2;
             lpOL2 = AddListObjectTail(&(((LPDD_REGISTERED_TABLES)lpOL->lpObject)->lpCols), sizeof(DD_REGISTERED_COLUMNS));
             if (!lpOL2)
                 break;
            if (iReplicVersion == REPLIC_V10 ||
                iReplicVersion == REPLIC_V105   )  {
                x_strcpy(((LPDD_REGISTERED_COLUMNS)lpOL2->lpObject)->columnname,buf2);
                x_strcpy(((LPDD_REGISTERED_COLUMNS)lpOL2->lpObject)->key_colums,extradata2);
                x_strcpy(((LPDD_REGISTERED_COLUMNS)lpOL2->lpObject)->rep_colums,extradata2+2);
                x_strcpy(buffilter2,buf);
            }
            else {
                x_strcpy(((LPDD_REGISTERED_COLUMNS)lpOL2->lpObject)->columnname,buf2);
                ((LPDD_REGISTERED_COLUMNS)lpOL2->lpObject)->key_sequence=getint(extradata2);
                ((LPDD_REGISTERED_COLUMNS)lpOL2->lpObject)->column_sequence=getint(extradata2+STEPSMALLOBJ);
                if (((LPDD_REGISTERED_COLUMNS)lpOL2->lpObject)->column_sequence==0)
                  ((LPDD_REGISTERED_COLUMNS)lpOL2->lpObject)->rep_colums[0]=0;
                else
                  ((LPDD_REGISTERED_COLUMNS)lpOL2->lpObject)->rep_colums[0]='R';

                if (((LPDD_REGISTERED_COLUMNS)lpOL2->lpObject)->key_sequence==0)
                  ((LPDD_REGISTERED_COLUMNS)lpOL2->lpObject)->key_colums[0]=0;
                else
                  ((LPDD_REGISTERED_COLUMNS)lpOL2->lpObject)->key_colums[0]='K';
                
                ((LPDD_REGISTERED_COLUMNS)lpOL2->lpObject)->key_colums[1]=0;

                ((LPDD_REGISTERED_COLUMNS)lpOL2->lpObject)->rep_colums[1]=0;
                x_strcpy(buffilter2,buf);
            }
         }
         pllcurprev=pllcur;
         pllcur=pllcur->pnext;
         ESL_FreeMem(pllcurprev);
      }
   SetCursor(hOldCursor);

   return RES_SUCCESS;
}
