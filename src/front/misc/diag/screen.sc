/*
**	Copyright (c) 2004 Ingres Corporation
*/
#include <compat.h>
#include <cm.h>
#include <tm.h>
#include <st.h>
#include <lo.h>
#include <si.h>
#include <er.h>
#include <evset.h>

/**
**  Name: screen - Screen mode part of idbg
**
**  Description:
**     This module supports the screen mode based operation of idbg
**
**     It contains the code for the following frames:-
**
**     main_frame    - The top level screen
**     view_frame    - Viewing files
**     export_frame  - Exporting evidence sets
**     add_frame     - Adding files to evidence sets
**     show_frame    - Displaying files
**
**  History:
**	22-feb-1996 - Initial port to Open Ingres
**	01-Apr-1996 - (prida01)
**			Add sunOS defines for port.
**      19-May-1998 - (horda03)
**          VMS port. In VMS Evidence Set Dump Stacks are
**          stored as binary files which must be opened
**          using SIfopen (bug 89423).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for view_frame(), export_frame(), add_frame(),
**	  show_frame(), delete_evset_frame(), delete_file_frame() &
**	  import_frame(). Also remove cast of arg 4 in ERlookup() call,
**	  both to prevent compiler errors with GCC 4.0.
**    16-Sep-2008 (horda03)
**        Added Find, FindNext, Top and Bottom key
**        definitions to help searching.
**    25-Jan-2010 (horda03) Bug 121811
**        Fix warnings (from Windows port).
*/

#define BUF_SIZE 80

EXEC SQL BEGIN DECLARE SECTION;
char search_str[BUF_SIZE+1];
EXEC SQL END DECLARE SECTION;

static VOID error();
static view_frame();
static export_frame();
static add_frame();
static show_frame();
static i4 delete_evset_frame();
static i4 delete_file_frame();
static import_frame();
static i4 find();
static i4 search_string();

main_frame()
{
    EXEC SQL BEGIN DECLARE SECTION;
        extern i4  main_screen;
        char buffer[BUF_SIZE];
        i4  id;
        i4  rows;
    EXEC SQL END DECLARE SECTION;

    EVSET_DETAILS details;
    STATUS status;
    i4  evset= -1;
    i4  going=TRUE;

    EXEC FRS FORMS;

    EXEC FRS ADDFORM :main_screen;

    EXEC FRS INITTABLE main_screen data READ (id=integer);

    while(going)
    {
       EXEC FRS DISPLAY main_screen READ;
   
          EXEC FRS INITIALIZE;
          EXEC FRS BEGIN;
             /* Load up table with details of evidence sets */
         
             while((status=EVSetList(&evset,&details))==0)
             {
                 char time[30];
                 SYSTIME temp;
         
                 temp.TM_secs=details.created;
                 TMstr(&temp,time);
                 
                 id=details.id;
                 STprintf(buffer,"%04d %s %s\n",details.id,time,
                    details.description);
                 EXEC FRS LOADTABLE main_screen data (id=:id,data=:buffer);
             }
             
             if(status!=E_EVSET_NOMORE)
             {
                error(status,0,NULL);
             }

             EXEC FRS SET_FRS FRS (ACTIVATE(menuitem)=0);
   
          EXEC FRS END;
   
          EXEC FRS ACTIVATE MENUITEM 'View';
          EXEC FRS BEGIN;
             EXEC FRS INQUIRE_FRS TABLE main_screen (:rows=DATAROWS(data));
             if(rows!=0)
             {
                EXEC FRS GETROW main_screen data (:id=id);
                view_frame(id);
             }
          EXEC FRS END;
   
          EXEC FRS ACTIVATE MENUITEM 'Export';
          EXEC FRS BEGIN;
             EXEC FRS INQUIRE_FRS TABLE main_screen (:rows=DATAROWS(data));
             if(rows!=0)
             {
                EXEC FRS GETROW main_screen data (:id=id);
                export_frame(id);
             }
          EXEC FRS END;
   
          EXEC FRS ACTIVATE MENUITEM 'Delete';
          EXEC FRS BEGIN;
             EXEC FRS INQUIRE_FRS TABLE main_screen (:rows=DATAROWS(data));
             if(rows!=0)
             {
                EXEC FRS GETROW main_screen data (:id=id);
                if(delete_evset_frame(id))
                   EXEC FRS DELETEROW main_screen data;
             }
          EXEC FRS END;
   
          EXEC FRS ACTIVATE MENUITEM 'Import';
          EXEC FRS BEGIN;
             import_frame();
             EXEC FRS BREAKDISPLAY;
          EXEC FRS END;
   
          EXEC FRS ACTIVATE MENUITEM 'Quit',FRSKEY3;
          EXEC FRS BEGIN;
              going=FALSE;
              EXEC FRS BREAKDISPLAY;
          EXEC FRS END;
    
    }

    EXEC FRS ENDFORMS;
}

static
view_frame(evset)
i4  evset;
{
   EXEC SQL BEGIN DECLARE SECTION;
      extern i4  view_screen;
      char *type;
      char *comment;
      char version[80];
      i4  evidenceset;
      char created[30];
      char *title;
      i4  current_file;
      i4  flags;
#define FLAG_CAN_SHOW 1
#define FLAG_CAN_DELETE 2
      i4  rows;
   EXEC SQL END DECLARE SECTION;
   static i4  done_addform=0;
   STATUS status;
   EVSET_DETAILS details;
   EVSET_ENTRY fdetails;
   SYSTIME temp;
   i4  my_evset=evset;
   i4  file;
   bool going=TRUE;

   if(!done_addform)
   {
      EXEC FRS ADDFORM :view_screen;
      done_addform=1;
   }

   /* Get in details of evidence set */

   EVSetList(&my_evset,&details);

   evidenceset=details.id;
   temp.TM_secs=details.created;
   TMstr(&temp,created);
   title=details.description;

   /* Get version information */

   EVSetLookupVar(evidenceset,"VERSION",version,sizeof(version)-1);

   while(going)
   {
      EXEC FRS DISPLAY view_screen READ;
   
          EXEC FRS INITIALIZE(evidenceset=:evidenceset,created=:created,
             title=:title,version=:version);
          EXEC FRS BEGIN;
             EXEC FRS INITTABLE view_screen data 
                READ (file=integer,flags=integer);
   
             /* Load table with list of files */
             file=0;
   
             while((status=EVSetFileList(evset,&file,&fdetails))==0)
             {
                 flags=(fdetails.flags & EVSET_TEXT)?FLAG_CAN_SHOW:0;
                 if((fdetails.flags & EVSET_SYSTEM)==0 )
                    flags|=FLAG_CAN_DELETE;
                 type=(fdetails.flags & EVSET_TEXT)?"TEXT":"BINARY";
                 comment=fdetails.description;
                 current_file=file-1;
                 EXEC FRS LOADTABLE view_screen data (file=:current_file,
                    flags=:flags,type=:type,comment=:comment);
             }

             if(status!=E_EVSET_NOMORE)
             {
                error(status,0,NULL);
             }

          EXEC FRS END;
   
          EXEC FRS ACTIVATE BEFORE COLUMN data ALL;
          EXEC FRS BEGIN;
             EXEC FRS GETROW view_screen data (:flags=flags);

             if(flags & FLAG_CAN_SHOW)
                EXEC FRS SET_FRS FRS (ACTIVATE(menuitem)=1);
             else
                EXEC FRS SET_FRS FRS (ACTIVATE(menuitem)=0);
             if(flags & FLAG_CAN_DELETE)
                EXEC FRS SET_FRS FRS (ACTIVATE(menuitem)=1);
             else
                EXEC FRS SET_FRS FRS (ACTIVATE(menuitem)=0);

          EXEC FRS END;

          EXEC FRS ACTIVATE MENUITEM 'Show';
          EXEC FRS BEGIN;
             EXEC FRS GETROW view_screen data (:current_file=file);
             show_frame(evset,current_file);
          EXEC FRS END;
   
          EXEC FRS ACTIVATE MENUITEM 'Add';
          EXEC FRS BEGIN;
             add_frame(evset);
             /* Break out and redisplay the screen */
             EXEC FRS BREAKDISPLAY;
          EXEC FRS END;
   
          EXEC FRS ACTIVATE MENUITEM 'Delete';
          EXEC FRS BEGIN;
             EXEC FRS INQUIRE_FRS TABLE main_screen (:rows=DATAROWS(data));
             if(rows!=0)
             {
                EXEC FRS GETROW view_screen data (:current_file=file);
                if(delete_file_frame(evset,current_file))
                   EXEC FRS DELETEROW view_screen data;
             }
          EXEC FRS END;

          EXEC FRS ACTIVATE MENUITEM 'Quit',FRSKEY3;
          EXEC FRS BEGIN;
              going=FALSE;
              EXEC FRS BREAKDISPLAY;
          EXEC FRS END;
   }
}

static
export_frame(evset)
i4  evset;
{
   EXEC SQL BEGIN DECLARE SECTION;
      extern i4  export_screen;
      char filename[80];
      short xnull;
   EXEC SQL END DECLARE SECTION;
   static i4  done_addform=0;
   STATUS status;

   if(!done_addform)
   {
      EXEC FRS ADDFORM :export_screen;
      done_addform=1;
   }

   EXEC FRS DISPLAY export_screen UPDATE;

       EXEC FRS ACTIVATE MENUITEM 'Accept';
       EXEC FRS BEGIN;
          EXEC FRS GETFORM export_screen (:filename:xnull=data);
          EXEC FRS MESSAGE 'Exporting data...';
          if((status=EVSetExport(evset,filename))==0)
          {
             EXEC FRS BREAKDISPLAY;
          }
          else
          {
             error(E_EXPORT_CREATE,STlength(filename),(PTR)filename);
          }
       EXEC FRS END;

       EXEC FRS ACTIVATE MENUITEM 'Quit',FRSKEY3;
       EXEC FRS BEGIN;
           EXEC FRS BREAKDISPLAY;
       EXEC FRS END;
}

static
add_frame(evset)
i4  evset;
{
   EXEC SQL BEGIN DECLARE SECTION;
      extern i4  add_screen;
      char description[40];
      char type[10];
      char filename[80];
      short xnull;
   EXEC SQL END DECLARE SECTION;
   static i4  done_addform=0;
   char new_file[EVSET_MAX_PATH];
   STATUS status;

   if(!done_addform)
   {
      EXEC FRS ADDFORM :add_screen;
      done_addform=1;
   }

   EXEC FRS DISPLAY add_screen UPDATE;

       EXEC FRS INITIALIZE(type='TEXT');

       EXEC FRS ACTIVATE MENUITEM 'Accept';
       EXEC FRS BEGIN;
          EXEC FRS GETFORM add_screen(:description:xnull=description,
             :type:xnull=type,:filename:xnull=filename);

          if((status=EVSetCreateFile(evset,
              type[0]=='T'?EVSET_TEXT:EVSET_BINARY,description,new_file,
              sizeof(new_file)))==OK)
          {
             LOCATION from_loc;
             LOCATION to_loc;
             FILE *fdes;

             if((status=LOfroms(PATH&FILENAME,filename,&from_loc))!=OK ||
                (status=LOfroms(PATH&FILENAME,new_file,&to_loc))!=OK ||
                (status=SIopen(&to_loc,"w",&fdes))!=OK ||
                (status=SIcat(&from_loc,fdes))!=OK)
             {
                error(E_COPY_FAIL,STlength(filename),(PTR)filename);
             }

             SIclose(fdes);
         }
         else
         {
             error(status,0,NULL);
         }

         EXEC FRS BREAKDISPLAY;

       EXEC FRS END;

       EXEC FRS ACTIVATE MENUITEM 'Quit',FRSKEY3;
       EXEC FRS BEGIN;
           EXEC FRS BREAKDISPLAY;
       EXEC FRS END;
}

static
show_frame(evset,file)
i4  evset;
i4  file;
{
   EXEC SQL BEGIN DECLARE SECTION;
      extern i4  show_screen;
      char *title;
      char buffer[BUF_SIZE+1];
      char find_str[BUF_SIZE+1];
      char file_str[BUF_SIZE+1];
      i4   record,cur_row;
   EXEC SQL END DECLARE SECTION;
   EVSET_ENTRY fdetails;
   static i4  done_addform=0;
   LOCATION my_loc;
   FILE *fdes;
   STATUS status;

   if((status=EVSetFileList(evset,&file,&fdetails))!=OK)
   {
      error(status,0,NULL);
   }
#ifndef VMS 
  if (fdetails.flags & EVSET_BINARY)
   {
      error(E_VIEW_BINARY_FAIL,0,NULL);
   }
   else 
   {
#endif
      if(!done_addform)
      {
         EXEC FRS ADDFORM :show_screen;
         done_addform=1;
      }

      title=fdetails.description;

      EXEC FRS DISPLAY show_screen READ;

      EXEC FRS INITIALIZE(title=:title);
      EXEC FRS BEGIN;

#ifdef VMS
        if (fdetails.flags & EVSET_TEXT)
        {
#endif
         if(LOfroms(PATH&FILENAME,fdetails.value,&my_loc)==OK &&
            SIopen(&my_loc,"r",&fdes)==OK)
         {
            EXEC FRS INITTABLE show_screen data READ;

            EXEC FRS MESSAGE 'Reading data...';

            while(SIgetrec(buffer,BUF_SIZE,fdes)==OK)
            {
               EXEC FRS LOADTABLE show_screen data (data=:buffer);
            }

            SIclose(fdes);
         }

#ifdef VMS
      }
      else
      {
      EV_STACK_ENTRY StackRec;
      char           *string;
      char           prev_session [BUF_SIZE+1];
      i4             first = 1;

      /* Binary File. */

      string = & StackRec.Vstring[2];

      if(LOfroms(PATH&FILENAME,fdetails.value,&my_loc)==OK &&
            SIfopen(&my_loc, "r", SI_RACC, (i4) sizeof(EV_STACK_ENTRY), &fdes)==OK)
         {
            i4 count;

            EXEC FRS INITTABLE show_screen data READ;

            EXEC FRS MESSAGE 'Reading data...';

            while(SIread( fdes, sizeof(EV_STACK_ENTRY), &count, &StackRec)==OK)
            {
               STprintf( buffer, "%-80s", string);

               if (STscompare( buffer, BUF_SIZE * sizeof(*buffer), prev_session, BUF_SIZE * sizeof(*prev_session)) )
               {
                  STprintf( prev_session, "%-80s", string);

                  if (! first)
                  {
                     EXEC FRS LOADTABLE show_screen data (data=' ');
                  }
                  else first = 0;

                  EXEC FRS LOADTABLE show_screen data (data=:buffer);
               }

               STprintf( buffer, "[ pc: %08x      %8s          %8s      sp: %08x",
                         StackRec.pc,
                         " ",                  /* Would like to be PV */
                         " ",                  /* Would like to be FP */
                         StackRec.sp );

               EXEC FRS LOADTABLE show_screen data (data=:buffer);

               STprintf( buffer, "  r16-r21: %08x %08x %08x %08x %08x %08x ]",
                         StackRec.args[0],
                         StackRec.args[1],
                         StackRec.args[2],
                         StackRec.args[3],
                         StackRec.args[4],
                         StackRec.args[5] );

               EXEC FRS LOADTABLE show_screen data (data=:buffer);
            }

            SIclose(fdes);
         }
      }
#endif

      EXEC FRS END;

      EXEC FRS ACTIVATE MENUITEM 'Quit',FRSKEY3;
      EXEC FRS BEGIN;
          EXEC FRS BREAKDISPLAY;
      EXEC FRS END;

      EXEC FRS ACTIVATE MENUITEM 'Bottom';
      EXEC FRS BEGIN;
          EXEC FRS SCROLL show_screen data TO END;
      EXEC FRS END;

      EXEC FRS ACTIVATE MENUITEM 'Top';
      EXEC FRS BEGIN;
          EXEC FRS SCROLL show_screen data TO 1;
      EXEC FRS END;

      EXEC FRS ACTIVATE MENUITEM 'Find';
      EXEC FRS BEGIN;
         EXEC FRS PROMPT ('Find: ', :find_str);

         if (find_str [0] != '\0')
         {
            STcopy( find_str, search_str );

            find( &record );

            if (record != -1)
            {
               EXEC FRS SCROLL show_screen data to :record;
               EXEC FRS RESUME FIELD data;
            }
         }
      EXEC FRS END;

      EXEC FRS ACTIVATE MENUITEM 'FindNext';
      EXEC FRS BEGIN;
         if (search_str [0] != '\0')
         {
            find( &record );

            if (record != -1)
            {
               EXEC FRS SCROLL show_screen data to :record;
               EXEC FRS RESUME FIELD data;
            }
         }
         else
         {
            EXEC FRS MESSAGE 'Nothing to find';
            EXEC FRS SLEEP 2;
         }
      EXEC FRS END;

      EXEC FRS ACTIVATE MENUITEM 'SaveToFile';
      EXEC FRS BEGIN;
         EXEC FRS PROMPT ('File: ',:file_str);

         while(file_str [0] != '\0')
         {
            if (LOfroms(PATH&FILENAME, file_str, &my_loc) != OK)
            {
               EXEC FRS MESSAGE 'Bad file specified';
               EXEC FRS SLEEP 2;
               break;
            }

            if (SIopen( &my_loc, "w", &fdes) != OK)
            {
               EXEC FRS MESSAGE 'Unable to open file';
               EXEC FRS SLEEP 2;
               break;
            }

            EXEC FRS GETROW show_screen data (:cur_row = _record);
            EXEC FRS UNLOADTABLE show_screen data (:buffer=data, :record=_record);
            EXEC FRS BEGIN;
               SIwrite(STlength(buffer), buffer, &record, fdes);
               SIwrite(1, "\n", &record, fdes);
            EXEC FRS END;

            SIclose( fdes );

            break;
         }
      EXEC FRS END;

#ifndef VMS
   }
#endif
}
   
static i4
find( row_no )
i4  *row_no;
{
   EXEC SQL BEGIN DECLARE SECTION;

      char buffer[BUF_SIZE+1];
      i4  record, cur_row;

   EXEC SQL END DECLARE SECTION;

   *row_no = -1;

   if (search_str [0] != '\0')
   {
      EXEC FRS GETROW show_screen data (:cur_row = _record);
      EXEC FRS UNLOADTABLE show_screen data (:buffer=data, :record=_record);
      EXEC FRS BEGIN;
         if ( (cur_row < record) &&
              search_string( &buffer[0], &search_str[0] ) )
         {
            *row_no = record;
            break;
         }
      EXEC FRS END;

      if (*row_no == -1)
      {
         EXEC FRS MESSAGE 'String not found';
         EXEC FRS SLEEP 2;
      }
   }

   return OK;
}

static i4 
search_string( string, match )
char * string;
char * match;
{
   char *search_pos;
   char *check_search;
   char *check_match;
   i4   len_match = STlength( match );
   i4   match_char;
   i4   result    = 1;

   for( search_pos = STindex( string, match, STlength( string ) );
        result && search_pos;
        search_pos = STindex( search_pos, match, STlength( search_pos)) )
   {
      /* First char matched. */

      result = 0;  
      check_search = search_pos;
      check_match  = match;

      for (match_char = 1;
           match_char < len_match;
           match_char ++)
      {
         CMnext(check_search);
         CMnext(check_match);

         if (result = CMcmpcase( check_search, check_match ) ) break;
      }
         
      CMnext(search_pos);
   }

   return result ? 0 : 1;
}

static i4
delete_evset_frame(evset)
i4  evset;
{
   EXEC SQL BEGIN DECLARE SECTION;
      char xprompt[50];
      char reply[2];
   EXEC SQL END DECLARE SECTION;
   STATUS status;

   STprintf(xprompt,"Delete evidence set %04d - Are you sure ?",evset);

   EXEC FRS PROMPT(:xprompt,:reply) 
      WITH STYLE=POPUP(COLUMNS=40,STARTCOLUMN=10,STARTROW=10);

   if(reply[0]=='y' || reply[0]=='Y')
   {
      if((status=EVSetDelete(evset))==0)
      {
         return(1);
      }
      else
      {
         error(status,0,NULL);
      }
   }

   return(0);
}

static i4
delete_file_frame(evset,file)
i4  evset;
i4  file;
{
   EXEC SQL BEGIN DECLARE SECTION;
      char xprompt[50];
      char reply[2];
   EXEC SQL END DECLARE SECTION;
   STATUS status;

   STprintf(xprompt,"Delete file - Are you sure ?");

   EXEC FRS PROMPT(:xprompt,:reply) 
      WITH STYLE=POPUP(COLUMNS=40,STARTCOLUMN=10,STARTROW=10);

   if(reply[0]=='y' || reply[0]=='Y')
   {
      if((status=EVSetDeleteFile(evset,file))==0)
      {
         return(1);
      }
      else
      {
         error(status,0,NULL);
      }
   }

   return(0);
}

static 
import_frame()
{
   EXEC SQL BEGIN DECLARE SECTION;
      char filename[EVSET_MAX_PATH];
   EXEC SQL END DECLARE SECTION;
   STATUS status;
   i4  dummy;

   EXEC FRS PROMPT('Enter filename of import file',:filename) 
      WITH STYLE=POPUP(COLUMNS=78,STARTCOLUMN=1,STARTROW=10);

   EXEC FRS MESSAGE 'Importing data...';

   if((status=EVSetImport(filename,&dummy))!=OK)
      error(status,0,NULL);
}

static VOID
error(status,len,value)
STATUS status;
i4     len;
PTR    value;
{
   EXEC SQL BEGIN DECLARE SECTION;
      char xmessage[ER_MAX_LEN];
   EXEC SQL END DECLARE SECTION;
   i4  langcode;
   i4  mylen;
   CL_ERR_DESC clerr;
   ER_ARGUMENT er_arg;

   er_arg.er_size=len;
   er_arg.er_value=value;

   ERlangcode(NULL,&langcode);

   if(ERlookup(status,NULL,ER_TEXTONLY,NULL,xmessage,sizeof(xmessage),
      langcode,&mylen,&clerr,1,&er_arg)!=OK)
   {
      STprintf(xmessage,"Failure to lookup error message 0x%x",status);
   }

   EXEC FRS MESSAGE :xmessage WITH STYLE=POPUP;
}
