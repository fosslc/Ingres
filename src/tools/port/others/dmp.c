/*
**Copyright (c) 2004 Ingres Corporation
*/
/****************************************************************************/
/* This is a  program that dumps files in hex and ASCII.                    */
/* Lisle Porting Group                                                      */
/****************************************************************************/
/* History:                                                                 */
/*      30-March-2000 (legru01)                                             */
/*      The changes fix bug number 101089.                                  */
/*      Page Breaks are printed on separate pages                           */
/*      17-April-2000 (legru01)                                             */
/*      Correct a problem that causes a sigsegv under some circumstances    */
/*      05-May-2000 (legru01)                                               */
/*      Correctly construct 2 comment lines and a mispelled word            */
/*      27-July-2000 (musro02)                                              */
/*      ensure main begins in column 1 so mkming builds MING correctly      */
/*	17-Nov-2010 (kschendel)
**	    No need to forward-declare main, caused compiler warnings.
*/
/****************************************************************************/
/*Define Dictionary*/                    /***********************************/
#include <stdio.h>                       /* Make includes first part of file*/
#include <string.h>                      /* For string functions.           */
#include <stdlib.h>                      /* Standard include items.   */
#include <time.h>                        /* For time information.                           */
#define  ARG_HELP    '?'
#define  ARG_SLASH   '/'
#define  ARG_DASH    '-'

/* su4_u42 Sun/OS failed to define SEEK_END via stdio.h */
#if !defined(SEEK_END)
#define SEEK_END   2
#endif

/****************************************************************************/
/*****************************Program Body***********************************/

main(int argc, char *argv[])
{

      FILE    *fpFilePointer;    /* Pointer to a file */
      long    lPosition;
      int     i;                 /* Loop control      */
      int     nNumberBytesRead;
      unsigned    int nHexNumber;
      char    szInputFile[128];
      char    szBuffer[80];
      char    sBuffer[75];
      char    ht[20];
      int iLineCount = 3;      /* initial line count */
      int ln    = 0;
      int cnt   = 0;
      int flag  = 0;
      int pages = 1;
      int pn    = 1;
      int pagePrinted = 1;
      int curl  = 0;
      int psize = 0;              /* Total number of printed lines on a single page   */
      int iMaxlines = 60;      /* maximal # of lines on a single page                   */
      int iLinesToPrint = 0;  /* remaining lines to be printed on a single page    */
      long off  = 0;
      int skip_lines = 0;
                                        /* Time Dictionary */
      time_t  tTime;             /* time_t structure  */
      struct  tm  *pTM;
  /***************************************************************************/
  /********************** SOURCE/SOURCE CODE *********************************/
  /***************************************************************************/
      strcpy(ht,"HEAD"); /* copies the string head into the char array ht    */
      if (argc <= 1)
         {                         /* checks for correct arguments                        */
         printf("No file name given.\n");
         exit(4); 
         }

      for (i = 1;  i < argc;  i++)
          {
          switch(i)
              {
               case 1: strcpy(szInputFile, argv[1]); break;
               case 2: flag       = atoi(argv[2]);   break;
               case 3: pages      = atoi(argv[3]);   break;
               case 4: psize      = atoi(argv[4]);
                  if (psize > iMaxlines)
                     {
                     printf("Maxamial number of lines per page is %d\n", iMaxlines);
                     exit(4);
                     } break;
               case 5: skip_lines = atoi(argv[5]);   break;
                       skip_lines = 0;
              default: break;
              } /* End of Switch*/
           }    /* End of For*/


       if ((fpFilePointer = fopen(szInputFile, "rb")) == NULL)
            {
            printf("Unable to open file: %s\n", szInputFile);
            exit(16);
            }

             lPosition = 0l;
             time(&tTime);
             pTM = localtime(&tTime); /* returns a pointer to a pointer time */

         /*format a time string, using strftime() (new with ANSI C) */
         strftime(szBuffer, sizeof(szBuffer), "%A %B %d, %Y at %H:%M:%S", pTM);
   
      if (flag == 1)
          {
printf("===%s===                              PAGE %d\n",ht,pn);
          printf("Dump of %s\n",  szInputFile);
          printf("%s\n", szBuffer);
          }
      else
          {
printf("                                                  PAGE %d\n",pn);
          printf("Source Code of %s \n", szInputFile);
          printf("%s\n",         szBuffer);
          if ((pages * psize) < 0) /* put back = */
               exit (4);

          ln = curl= iLineCount;
          while(fgets((char *)sBuffer, 75 -1 ,  fpFilePointer)
               &&( ln   <   ((pages * psize) + skip_lines)) && (pagePrinted <= pages)  )
               {
                printf("%s",sBuffer);
                curl++ ;
                ln++;

                if (curl == psize)
                   {
                  
                     pagePrinted++;
                     if (pagePrinted <=  pages)
                        {
                        printf("\f");
                        pn ++;
printf("                                                  PAGE %d\n",pn);
                        curl = 1;
                        }    /* End of IF      */
                   }      /* End of IF      */
                 }        /* End of while   */
     return (0);
     }                    /* End of if else */


    /*************************************************************************/
    /************************    DUMP SOURCE CODE   ************************/
    /*************************************************************************/

     curl = ln = iLineCount;
     pagePrinted = 1;
     for (cnt = 0; cnt < 2; cnt++)
         {
          while((nNumberBytesRead = fread((char *)sBuffer,sizeof(char), 16, fpFilePointer) ) > 0 && ln<pages*psize && pagePrinted <= pages )
          {
          printf(" %8.8X -", lPosition);
          for (i = 0; i < 16; i++)
              {
              if (i == 8)
                 printf("  -  ");
                 if (i == 0 || i == 4 || i == 12)
                     printf("  ");
                     if (i < nNumberBytesRead)
                        {
                        nHexNumber =  (unsigned char)sBuffer[i];
                        printf("%2.2X", (unsigned int)nHexNumber);
                        }
                     else
                        printf("  ");
           }/* End of for */

         for (i = 0; i < nNumberBytesRead; i++)
             {
             if (sBuffer[i] < ' ' || sBuffer[i] == '\xFF')
                 sBuffer[i] = '.';
             }/*End of for*/

             sBuffer[nNumberBytesRead] = '\0';
             printf(" : %s", sBuffer);
             printf("\n");
             lPosition = lPosition +24;
             ln++;
             curl++;

        if (curl == psize)
        {
          pn++;
          pagePrinted++;
          if (ln < pages*psize && pagePrinted <= pages)
              {
              printf("\f");
              
printf("===%s===                              PAGE %d\n\n",ht,pn);
              curl = 2;
              }/* End of inner if   */
            }  /*  End of outer if */
       }       /* End of while      */

       if (!cnt)
          {

          strcpy(ht,"TAIL");
          pn=1;
          pagePrinted =1;
          printf("\f");
printf("===%s===                              PAGE %d\n\n",ht,pn);

          if (fseek(fpFilePointer,-lPosition,SEEK_END) == -1)
             {
             printf("FSEEK error");
             exit (5);
             } /* End of inner if */
          off = ftell(fpFilePointer);
          lPosition = off;
          ln = 2;
          curl = 2;
          }  /*  End of outer if */
       }
    return(0);
}/* End of main */

