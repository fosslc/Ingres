/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**  Substitute in file (SIF)
**
**  Description:
**
**      Simple substitution in file - used by ICE and gateway.
**      Only used under NT. Unix has sed, etc.
**
**      This program scans the given input file and searches for the given
**      string and replaces all the occurances with the replacement string.
**      The program reads in segments of READ_BUF_SIZE and performs a
**      comparison from each position in the buffer. If the string is not
**      found in the buffer the buffer is written to the output file otherwise
**      the buffer is written up to the point where a match is found the
**      replacement string is written to the output file and the matched string
**      in the buffer is skipped.
**
**      SIF <filename> "search string" "replace string"
**
**      filename        name of the file to be scanned for search string
**                      The original file is saved to a .bak file.
**      search string   The string to be searched for enclosed in double
**                      quote characters.
**      replace string  The string to replace enclosed in double
**                      quote characters.
**
**  History:
**      11-Aug-95 (fanra01)
**          Created.
**      26-Apr-96 (musro02)
**          Add a UNIX dependent include of unistd.h to resolve 
**	    undefined symbol SEEK_SET.
**	    Add include of mf.h so UNIX can be determined
**	08-may-96 (emmag)
**	    Don't include mf.h if we're on NT. 
**	14-feb-97 (toumi01)
**          Move #include <unistd.h> to column 1 to satisfy Digital and
**	    other compilers.
**	28-nov-2000 (somsa01)
**	    Files are now renamed, as is, to '.bak'. All supported platforms
**	    support long filenames.
**	14-Jan-2004 (clach04)
**	    Bug 111618 / INGGAT0381
**	    Replaced tmpnam() calls with tempnam calls. tmpnam() was returning
**	    file/path names that where on the root drive of the current drive,
**	    the root drive may not be writeable. tempnam() returns a
**	    full/usable name. Removed old string buffers for temp filenames
**	    and replaced tmpfilename string ptr's with string buffers.
**	    Changed size of filename string buffers to max supported 
**	    filename size.
**	    Removed unused variables to remove compile warnings.
**	07-Aug-2009 (drivi01)
**	    Add pragma to disable deprecated POSIX functions warning 4996.
*/
/* Turn off POSIX warning for this file until Microsoft fixes this bug */
#ifdef NT_GENERIC
#pragma warning(disable: 4996)
#endif /*NT_GENERIC*/

#include <stdio.h>
#include <string.h>
#ifndef NT_GENERIC
#include <mf.h>
#endif /* NT_GENERIC */
#ifdef UNIX
#include <unistd.h>
#endif

#define READ_BUF_SIZE   1024

#define SIF_MAX_FNAME_LEN FILENAME_MAX
/* Prefix to use for temporary filenames */
#define SIF_TEMPNAME_PREFIX "siftmp_"

unsigned char buffer[READ_BUF_SIZE];

int main (int argc, char * * argv)
{
int i, j;
FILE * infd, * outfd;
int size;
char * tmpstr=NULL;
char * temp_dir=NULL;
char tmpname[SIF_MAX_FNAME_LEN]="";
char tmpname1[SIF_MAX_FNAME_LEN]="";
unsigned long bufferoffset = 0;
unsigned long fileoffset = 0;
char  bak[SIF_MAX_FNAME_LEN];
int count;

    if (argc == 4)
    {
        size = strlen(argv[2]);
        strcpy(bak,argv[1]);

        
        /*
        ** Set temp dir location
        **
        ** _tempnam ignores the DIR param IF the OS temp dir variable is set
        ** which it usually is by default under Windows, it is usually set
        ** to:  "C:\Documents and Settings\USERNAME\Local Settings\Temp"
        ** so for now just set to NULL.
        */
        temp_dir=NULL;

        strcat(bak,".bak");
        tmpstr = tempnam(temp_dir, SIF_TEMPNAME_PREFIX);
        strcpy(tmpname, tmpstr);
        rename (argv[1], tmpname);
        if((infd = fopen (tmpname,"rb")) != NULL)
        {
            tmpstr = tempnam(temp_dir, SIF_TEMPNAME_PREFIX);
            strcpy(tmpname1, tmpstr);
            if((outfd = fopen(tmpname1,"w+b")) != NULL)
            {
                memset(buffer,0,READ_BUF_SIZE);
                if ((count = fread ( buffer, 1, READ_BUF_SIZE, infd)) > 0)
                {
                    do
                    {
                        if(count > size)
                        {
                            for (bufferoffset=0, i=count, j=0; i > size; i--, j++)
                            {
                                if(memcmp(argv[2],&buffer[j], size))
                                    continue;
                                else
                                {
                                    fwrite(&buffer[bufferoffset],1,(j-bufferoffset),outfd);
                                    fwrite(argv[3],1,strlen(argv[3]),outfd);
                                    j+=size;
                                    i-=size;
                                    if (i < size) i++;  /* compensate for leaving loop early */
                                    bufferoffset=j;
                                }
                            }
                            fwrite(&buffer[bufferoffset],1,((count - i) - bufferoffset) , outfd);
                            fileoffset+=(count - i);
                            fseek(infd,fileoffset,SEEK_SET);
                        }
                        else
                        {
                            if (count)
                            {
                                fwrite(buffer,1,count, outfd);
                            }
                        }
                        memset(buffer,0, READ_BUF_SIZE);
                    }
                    while( (count = fread ( buffer, 1, READ_BUF_SIZE , infd)) > 0);
                }
                fclose(infd);
                fclose(outfd);
                _unlink(bak);
                rename(tmpname,bak);
                rename(tmpname1,argv[1]);
            }
            else
            {
                perror(tmpname1);
            }
        }
        else
        {
            perror(argv[1]);
        }
    }
    else
    {
        printf ("Substitute In File\n\n");
        printf ("Usage: %s <filename> <\"search string\"> <\"replace string\"> \n",argv[0]);
    }
}
