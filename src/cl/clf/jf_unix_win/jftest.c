#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <me.h>
#include    <di.h>
#include    <jf.h>
#include    <si.h>
#include    <pc.h>
#ifdef MVS
#include    <to.h>
#endif
#include    <errno.h>
 
/*
**Copyright (c) 2004 Ingres Corporation
*/
 
#define DEBUG
 
 
/**
**
**  Name: JFtest.C - Test routines for JF.
**
**  Description:
**      This file contains all the test routines to test
**      the JF (Journal File) routines.
**
**          main - Runs the test.
**          JFtest - tests file all journal functions including
**                 creating, opening, reading, writing, closing,
**                 and deleting.
**
**
**  History:
**      27-Feb-87 (DaveS)
**          Modified from tSR for JF testing.
**	18-jun-90 (mikem)
**	    Fixed bugs to make it run against the current CL.  Also added
**	    test to test that we would not read past the "JF" end of file even
**	    if the disk file had more in it.
**	12-mar-91 (rudyw)
**	    Comment out text following #endif
**	22-apr-91 (rudyw)
**	    Add open comment syntax to text after two #endif. The incomplete
**	    version I submitted was still OK to the compiler since it
**	    considered everything past the #endif to be extraneous text.
**	06-jul-93 (shailaja)
**	    Fixed prototype mismatches.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-aug-93 (ed)
**	    add missing include
**	19-apr-95 (canor01)
**	    added <errno.h>
**      27-may-97 (mcgem01)
**          Cleaned up compiler warnings.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**/

/*
**
NEEDLIBS =      COMPATLIB 

PROGRAM =       jftest

*/
 
 
/*{
** Name: JFtest - Test the JF file I/O functions.
**
** Description:
**      This file contains tests for the JF I/O routines
**      such as JFopen, JFread, JFwrite, and JFclose.
**
** Inputs:
**      path                        Pointer to the pathname used
**                                  as default path for tests.
**      pathlength                  Length of path name.
**      filename                    Pointer to the filename to use
**                                  for these tests.
**      filelength                  Length of filename.
**
** Outputs:
**      none
** Returns:
**     OK
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
** 	27-Feb-87 (DaveS)
**          Created (started with tSR)
**	18-jun-90 (mikem)
**	    Fixed bugs to make it run against the current CL.  Also added
**	    test to test that we would not read past the "JF" end of file even
**      05-mar-93 (smc)
**          Undefined PAGESIZE for axp_osf as it is defined in 
**          sys/rt_limits.h.
**      08-apr-93 (smc)
**          Backed out previous change when DEC removed the definition
**          in O/S release BL12.
**	24-feb-99 (matbe01)
**	    PAGESIZE is defined in sys/limits.h for AIX (rs4_us5).  Added a
**	    check for the define and an undefine.
*/
STATUS
JFtest(defpath, defpathlength, filename, filelength)
char		*defpath;
u_i4	defpathlength;
char		*filename;
u_i4	filelength;
{
#define                     MAX_PAGES   18
#define                     MAX_ALLOC   20

#ifdef PAGESIZE
#undef PAGESIZE
#endif
#define                     PAGESIZE    8192

   STATUS              s;              /* Return value from JFxxx. */
   JFIO                f;              /* File control block. */
   i4             page;           /* Logical page number for write . */
   unsigned char       buffer[PAGESIZE];  /* Page buffer for I/O. */
   i4                  i;
   i4             eofblock;      /* last block number updated */
   CL_ERR_DESC         err_code;
 
#ifdef MVS
    DIinit();
#endif
 
    /* First see if it catches a bad blocksize */
 
    printf ("TJF: issuing JFcreate (testing bad blksize)\n");
    s = JFcreate(&f, defpath, defpathlength, filename, filelength,
                 16000, MAX_ALLOC, &err_code);
    if (s != JF_BAD_CREATE)
    {
        SIprintf("JFtest: Created file %s ",filename);
        SIprintf("in directory %s ",defpath);
        SIprintf("with bad block size.\n");
        return (FAIL);
    }
 
    /* Now create the file at the path (directory specified). */
 
    printf ("TJF: issuing JFcreate (real create)\n");
    s = JFcreate(&f, defpath, defpathlength, filename, filelength,
                 PAGESIZE, MAX_ALLOC, &err_code);
    if (s != OK)
    {
        SIprintf("JFtest: Cannot create file %s ",filename);
        SIprintf("in directory %s.\n",defpath);
        SIprintf("       Status returned is:  %08.4x \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
        return (FAIL);
    }
 
#ifdef DEBUG
   printf("JFtest: JFcreate successful\n");
#endif
 
   /* try to open with bad blocksize */
 
   s = JFopen(&f, defpath, defpathlength, filename, filelength,
              PAGESIZE/2, &eofblock, &err_code);
   if (s != JF_BAD_OPEN)
   {
      SIprintf("JFtest: Open for file %s ",filename);
      SIprintf("in directory %s \n",defpath);
      SIprintf("succeeded with mismatched block sizes.\n");
      return (FAIL);
   }
 
   /* Now open for real */
 
   s = JFopen(&f, defpath, defpathlength, filename, filelength,
              PAGESIZE, &eofblock, &err_code);
   if (s != OK)
   {
      SIprintf("JFtest: Cannot open (1) file %s ",filename);
      SIprintf("in directory %s.\n",defpath);
      SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
      return (FAIL);
   }
 
   if (eofblock != 0)
   {
      SIprintf("JFtest: Empty file open of file %s \n",filename);
      SIprintf("in directory %s.\n ",defpath);
      SIprintf("returned wrong EOF: x'%08.4x' \n",eofblock);
      return (FAIL);
   }
 
#ifdef DEBUG
   printf("JFtest: JFopen successful\n");
#endif
 
   /* Try to write page 0 */
      MEfill(sizeof(buffer), 0, buffer);
      s = JFwrite(&f, (PTR) buffer, 0, &err_code);
      if (s == OK)
       { 
        SIprintf("JFtest: Page 0 write allowed\n");
        return (FAIL);
       } 
 
   /* Now really write pages. */
   for (i = 1; i < MAX_PAGES; i++)
   {
      MEfill(sizeof(buffer), i, buffer);
      s = JFwrite(&f, (PTR) buffer, page = i, &err_code);
      if (s != OK)
      {
         SIprintf("JFtest: Error writing page %d. \n", page);
         SIprintf("In file %s in directory %s.\n",filename,defpath);
         SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
         return (FAIL);
      }
#ifdef DEBUG
     printf("JFtest: JFwrite of page %d successful\n",i);
#endif
   }
 
   /* Try to read page 0 */
      s = JFread(&f, (PTR) buffer, 0, &err_code);
      if (s == OK)
       { 
        SIprintf("JFtest: Page 0 read succeeded\n");
        return (FAIL);
       } 
 
   /* Read the file. */
 
   for (i = 1; i < MAX_PAGES; i = i + 10)
   {
      MEfill(sizeof(buffer), MAX_PAGES + 1, buffer);
      s = JFread(&f, (PTR) buffer, page = i, &err_code);
      if (s != OK)
      {
         SIprintf("JFtest: Error reading page %d. \n", page);
         SIprintf("In file %s in directory %s.\n",filename,defpath);
         SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
         return (FAIL);
      }
      if (buffer[i] != i)
      {
         SIprintf("JFtest: Error reading page %d. \n", page);
         SIprintf("In file %s in directory %s.\n",filename,defpath);
         SIprintf("Has value %d in page should be %d. \n",buffer[i],i);
        return (FAIL);
      }
#ifdef DEBUG
   printf("JFtest: JFread of page %d successful\n",i);
#endif
   }
 
   /* Now try to read past end of file. */
 
   MEfill(sizeof(buffer), 1, buffer);
   s = JFread(&f, (PTR) buffer, page = 5 * MAX_ALLOC, &err_code);
   if (s == OK)
   {
      SIprintf("JFtest: Page %d read from \n", page);
      SIprintf("file %s in directory %s \n",filename,defpath);
      SIprintf("End of file was %d, read illegal. \n",MAX_PAGES);
      return (FAIL);
   }
 
   /* Close file. */
 
   s = JFclose(&f, &err_code);
   if (s != OK)
   {
      SIprintf("JFtest: Cannot close (1) file %s \n", filename);
      SIprintf("in directory %s.\n", defpath);
      SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
      return (FAIL);
   }
 
   /* Try to read closed file, should be error. */
 
   s = JFread(&f, (PTR) buffer, page = 1, &err_code);
   if (s == OK)
   {
      SIprintf("JFtest: Error reading file %s \n", filename);
      SIprintf("in directory %s.\n", defpath);
      SIprintf("File was closed; should be illegal to read. \n");
      return (FAIL);
   }
 
   /*
   ** Now open file again.
   */
 
   s = JFopen(&f, defpath, defpathlength, filename, filelength,
              PAGESIZE, &eofblock, &err_code);
   if (s != OK)
   {
      SIprintf("JFtest: Cannot open (2) file %s \n", filename);
      SIprintf("in directory %s.\n", defpath);
      SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
      return (FAIL);
   }
 
   if (eofblock != 0)
   {
      SIprintf("JFtest: Non-updated file open of file %s \n",filename);
      SIprintf("in directory %s ",defpath);
      SIprintf("returned wrong EOF: x'%08.4x' \n",eofblock);
      return (FAIL);
   }
#ifdef DEBUG
   printf("JFtest: Reopen EOF is correct.\n",i);
#endif
 
   /*
   ** Now try to write past the end of file.
   ** Should allocate and write.
   */
 
   MEfill(sizeof(buffer), 33, buffer);
   s = JFwrite(&f, (PTR) buffer, page = MAX_ALLOC + MAX_PAGES, &err_code);
   if (s != OK)
   {
      SIprintf("JFtest: Error writing page %d. \n", page);
      SIprintf("In file %s in directory %s.\n",filename,defpath);
      SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
      return (FAIL);
   }
#ifdef DEBUG
   printf("JFtest: Write extend is OK.\n",i);
#endif
 
   /* Now call JFupdate to update the logical end of file */
   s = JFupdate(&f, &err_code);
   if (s != OK)
   {
       SIprintf("JFtest: Error setting extended EOF ");
       SIprintf("in file %s in directory %s.\n",filename,defpath);
       SIprintf("       Status returned is:  %08.4x \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
       return (FAIL);
   }
 
   /* Close file again */
 
   s = JFclose(&f, &err_code);
   if (s != OK)
   {
      SIprintf("JFtest: Cannot close (2) file %s \n", filename);
      SIprintf("in directory %s.\n", defpath);
      SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
      return (FAIL);
   }
 
   /*
   ** Now open one last time
   */
 
   s = JFopen(&f, defpath, defpathlength, filename, filelength,
              PAGESIZE, &eofblock, &err_code);
   if (s != OK)
   {
      SIprintf("JFtest: Cannot open (3) file %s \n", filename);
      SIprintf("in directory %s.\n", defpath);
      SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
      return (FAIL);
   }
 
   if (eofblock != page)
   {
      SIprintf("JFtest: Updated file open (3) of file %s \n",filename);
      SIprintf("in directory %s\n ",defpath);
      SIprintf("returned wrong EOF: x'%08.4x', ",eofblock);
      SIprintf("should be x'%08.4x'.\n",page);
      return (FAIL);
   }
 
 
    s = JFdelete(&f, defpath, defpathlength, filename, filelength,
                 &err_code);
#ifdef UNIX
    /* at least for now UNIX will allow deletes - seems like a CL issue */
    if (s != OK)
    {
        SIprintf("JFtest: Delete error - open journal delete disallowed.");
        return (FAIL);
    }
#else
    if (s == OK)
    {
        SIprintf("JFtest: Delete error - open journal delete allowed.");
        return (FAIL);
    }
 
   /* Close file one last time - will allow delete */
 
   s = JFclose(&f, &err_code);
   if (s != OK)
   {
      SIprintf("JFtest: Cannot close (3) file %s \n", filename);
      SIprintf("in directory %s after delete.\n", defpath);
      SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
      return (FAIL);
   }
 
    /* delete the file */
    s = JFdelete(&f, defpath, defpathlength, filename, filelength,
                 &err_code);
    if (s != OK)
    {
        SIprintf("JFtest: Delete error - file %s ", filename);
        SIprintf("in directory %s.\n", defpath);
        SIprintf("       Status returned is:  %08.4x \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
        return (FAIL);
    }

#endif /* UNIX */

#ifdef DEBUG
   printf("JFtest: File delete successful.\n",i);
#endif
 
 
   /* Now try to open the deleted file. */
 
   s = JFopen(&f, defpath, defpathlength, filename, filelength,
              PAGESIZE, &eofblock, &err_code);
   if (s == OK)
   {
      SIprintf("JFtest: Opened deleted file %s \n", filename);
      SIprintf("in directory %s.\n", defpath);
      SIprintf(" File should have been deleted, illegal open.\n");
      s = JFclose(&f, &err_code); /* try to close it */
      return (FAIL);
   }

   /* test to make sure that a file closed before update, does not update
   ** the JF end of file.
   */

    /* Now create the file at the path (directory specified). */
 
    printf ("TJF: issuing JFcreate (real create)\n");
    s = JFcreate(&f, defpath, defpathlength, filename, filelength,
                 PAGESIZE, MAX_ALLOC, &err_code);
    if (s != OK)
    {
        SIprintf("JFtest: Cannot create file %s ",filename);
        SIprintf("in directory %s.\n",defpath);
        SIprintf("       Status returned is:  %08.4x \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
        return (FAIL);
    }

   /* Now open for real */
 
   s = JFopen(&f, defpath, defpathlength, filename, filelength,
              PAGESIZE, &eofblock, &err_code);
   if (s != OK)
   {
      SIprintf("JFtest: Cannot open (1) file %s ",filename);
      SIprintf("in directory %s.\n",defpath);
      SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
      return (FAIL);
   }

   /* Now really write pages. */
   for (i = 1; i <= 5; i++)
   {
      MEfill(sizeof(buffer), i, buffer);
      s = JFwrite(&f, (PTR) buffer, page = i, &err_code);
      if (s != OK)
      {
         SIprintf("JFtest: Error writing page %d. \n", page);
         SIprintf("In file %s in directory %s.\n",filename,defpath);
         SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
         return (FAIL);
      }
#ifdef DEBUG
     printf("JFtest: JFwrite of page %d successful\n",i);
#endif
   }

   /* Now call JFupdate to update the logical end of file */
   s = JFupdate(&f, &err_code);
   if (s != OK)
   {
       SIprintf("JFtest: Error setting extended EOF ");
       SIprintf("in file %s in directory %s.\n",filename,defpath);
       SIprintf("       Status returned is:  %08.4x \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
       return (FAIL);
   }

   /* Now really write pages. */
   for (i = 6; i < 10; i++)
   {
      MEfill(sizeof(buffer), i, buffer);
      s = JFwrite(&f, (PTR) buffer, page = i, &err_code);
      if (s != OK)
      {
         SIprintf("JFtest: Error writing page %d. \n", page);
         SIprintf("In file %s in directory %s.\n",filename,defpath);
         SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
         return (FAIL);
      }
#ifdef DEBUG
     printf("JFtest: JFwrite of page %d successful\n",i);
#endif
   }

   /* close and open the file to simulate crash while writing file */

   s = JFclose(&f, &err_code);
   if (s != OK)
   {
      SIprintf("JFtest: Cannot close (3) file %s \n", filename);
      SIprintf("in directory %s after delete.\n", defpath);
      SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
      return (FAIL);
    }

   /* Now open for real */
 
   s = JFopen(&f, defpath, defpathlength, filename, filelength,
              PAGESIZE, &eofblock, &err_code);
   if (s != OK)
   {
      SIprintf("JFtest: Cannot open (1) file %s ",filename);
      SIprintf("in directory %s.\n",defpath);
      SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
      return (FAIL);
   }


   /* Read the file. */
 
   for (i = 1; i <= 5; i++)
   {
      MEfill(sizeof(buffer), MAX_PAGES + 1, buffer);
      s = JFread(&f, (PTR) buffer, page = i, &err_code);
      if (s != OK)
      {
         SIprintf("JFtest: Error reading page %d. \n", page);
         SIprintf("In file %s in directory %s.\n",filename,defpath);
         SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
         return (FAIL);
      }
      if (buffer[i] != i)
      {
         SIprintf("JFtest: Error reading page %d. \n", page);
         SIprintf("In file %s in directory %s.\n",filename,defpath);
         SIprintf("Has value %d in page should be %d. \n",buffer[i],i);
        return (FAIL);
      }
#ifdef DEBUG
   printf("JFtest: JFread of page %d successful\n",i);
#endif
   }

   /* Make sure read past JF eof (but within DI eof) returns EOF */
 
  MEfill(sizeof(buffer), MAX_PAGES + 1, buffer);
  s = JFread(&f, (PTR) buffer, 6, &err_code);
  if (s != JF_END_FILE)
  {
     SIprintf("JFtest: Error reading page %d. \n", 6);
     SIprintf("In file %s in directory %s.\n",filename,defpath);
     SIprintf("Status returned is x'%08.4x' \n",s);
#ifdef UNIX
        SIprintf("       The system error is: %08.4x \n",err_code.errnum);
#endif /* UNIX */
     return (FAIL);
  }
  if (buffer[i] != MAX_PAGES + 1)
  {
     SIprintf("JFtest: Error reading page %d. \n", page);
     SIprintf("In file %s in directory %s.\n",filename,defpath);
     SIprintf("Has value %d in page should be %d. \n",buffer[i],i);
    return (FAIL);
  }
#ifdef DEBUG
   printf("JFtest: JFread of page %d successful\n",i);
#endif
 
   return (OK);
}
 
/*{
** Name: main - Main program to run JF tests.
**
** Description:
**      This routine is the main program for running the JF tests.
**      Individual sets of tests can be chosen by input parameters.
**      This program also will run all the tests.
**
** Inputs:
**      argc                      Count of the number of input
**                                call parameters.
**      argv                      Pointer to an array of pointers
**                                to call parameters.
**
** Outputs:
** Returns:
**          0 = success
**          8 = aborted
** Exceptions:
**          none
**
** Side Effects:
**     none
**
** History:
** 19-sep-85 (Jennifer)
**          Created (for SR)
** 27-Feb-87 (DaveS)
**          Modified for JF testing
*/
 
 
STATUS
main(argc, argv)
   int                argc;
   char               **argv;
{
#define             TEST_ALL            -1
#define             TEST_MAX            2
 
   STATUS             s;
   int                test;
   int                i;
   char               pathname[128];
   char               areaname[128];
   char               dirname[128];
   char               filename[16];
 
   i4            lpathname = 0;
   i4            lareaname = 0;
   i4            ldirname  = 0;
   i4            lfilename = 0;
#ifdef MVS
   TQE                tqe = {0};
#endif
   char               *test_array[2];

   test = TEST_ALL;
   test_array[0] = "jfio";
   test_array[1] = "jfall";
 
#ifdef MVS
   argv[2] = "TESTDB";
   argv[3] = "testdir1";
   argv[4] = "journal1.dat";
#endif

   MEadvise(ME_INGRES_ALLOC);
 
 
   /* Get type of test suite to run. */
 
   if (argc >= 2)
   {
      SIprintf ("args = %s, %s, %s\n", argv[2],argv[3],argv[4]);

      for (i = 0; i < TEST_MAX; i++)
      {
         if (strcmp (argv[1], test_array[i]) == 0)
         {
            test = i + 1;
            break;
         }
      }
   }
 
    /* Get path name to use for testing. */
 
   if (argc < 4 )
   {
        SIprintf("tJF: Usage is test area dir filename. \n");
	PCexit(8);
   }
 
   STcopy (argv[2],areaname);
   STcopy (argv[3],dirname);
   STcopy (argv[4],filename);
 
#ifdef MVS
   CVupper(areaname);
   CVlower(dirname);
   CVlower(filename);
#endif /* MVS */
 
   SIprintf("areaname = '%s'\n", areaname);
   SIprintf("dirname = '%s'\n", dirname);
   SIprintf("filename = '%s'\n", filename);
 
#ifdef UNIX
   STprintf(pathname,"%s/%s",areaname,dirname);
#endif /* UNIX */
#ifdef MVS
   STprintf(pathname,"%s:ja.%s",areaname,dirname);
#endif /* MVS */
 
#ifdef MVS
   TOsettqe (&tqe);
#endif
 
   lpathname = STlength(pathname);
   lareaname = STlength(areaname);
   ldirname  = STlength(dirname);
   lfilename = STlength(filename);
 
/* if (argc == 4)                      */
/* {                                   */
/*    strcpy(filename, argv[3]);       */
/* }                                   */
 
   switch (test)
   {
 
      case  1:
      case  2:
      case -1:
         {
            SIprintf (" testing with %s, %d, %s, %d\n",
                 pathname, lpathname, filename, lfilename);
            s = JFtest(pathname, lpathname, filename, lfilename);
            break;
         }
   }
 
   if (s != OK) exit (8);
   exit(0);
}
