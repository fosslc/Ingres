/*
**  Copyright (c) 2009 Ingres Corporation
*/

/*
**  Name: updcoffbf.c
**
**  Description:
**	This application is designed to update coffbase file
**  for rebase operation to keep the most current information
**  about base addresses and binary sizes.  The file is created
**  to get around the unflexible nature of rebase operations on
**  windows, it is designed to get around of inability to
**  rebase digitally signed binaries.
**	
**  History:
**		17-Feb-2009 (drivi01)
**			Created.
**		12-Mar-2009 (drivi01)
**			Replace CVahxl and CVlx functions with 
**			Microsoft strtol and ltoa to remove
**			dependency on compatibility library.
**			Don't update size when
**			the size in the binaddr.txt for a
**			shared library is larger than the library
**			itself, when the size needs to be updated
**			in the binaddr.txt because the file has
**			grown, fix the new size to be divisible
**			by 64K before calculating the difference.
**		07-Aug-2009 (drivi01)
**			Add pragma to clean up deprecated POSIX
**			functions warnings b/c it's a bug.
*/
/* Turn off POSIX warning for this file until Microsoft fixes this bug */
#pragma warning(disable: 4996)

#include <windows.h>
#include <stdio.h>


VOID printUsage();

int
main(int argc, char* argv[])
{
	if (argc!=4)
	{
		printUsage();
	}
	if (argc==4)
	{
		char cf_file[MAX_PATH];
		char cf_file_id[MAX_PATH];
		char b_name[MAX_PATH];
		char sz_file[MAX_PATH];
		FILE *fp = NULL;
		FILE *fp2 = NULL;
		FILE *fp3 = NULL;
		char line[MAX_PATH], szLine[MAX_PATH];
		int dwSize;
		OFSTRUCT opFile;
		BOOL bUpdated=FALSE;
		long sz_diff=0;
		
		strcpy(cf_file, argv[1]);
		if (GetFileAttributes(cf_file) == INVALID_FILE_ATTRIBUTES)
			return 2;
		strcpy(b_name, argv[2]);
		strcpy(sz_file, argv[3]);
		if (GetFileAttributes(sz_file) == INVALID_FILE_ATTRIBUTES)
			return 2;
		
		/*
		** We will read from old coffbase file and write to another
		** if something has changed. New file appends process id to
		** the end during execution of this program and then gets
		** copied to original file to update it.
		*/
		if (!fp) fp = fopen(cf_file, "r");
		sprintf(cf_file_id, "%s%d", cf_file, GetCurrentProcessId());
		if (!fp3) fp3 = fopen(cf_file_id, "w");
		if (fp)
		{
			dwSize=sizeof(line);
			ZeroMemory(&line, dwSize);
			while(fgets(line, dwSize, fp)!=NULL)
			{
				//check if the line contains a name of the dll we're looking for
				if (strstr(line, b_name)>0)
				{
					char *str, oldSz[MAX_PATH], sz[MAX_PATH];
					int position;
					char *strs[256];
					int count=0;

					//tokenize the line
					str=strtok(line, " ");
					count=0;
					while (str != NULL)
					{
						strs[count]=str;
						strcpy(oldSz, str);
						str=strtok(NULL, " ");
						count++;
					}
					while (oldSz[strlen(oldSz)-1]=='\r' || oldSz[strlen(oldSz)-1]=='\n')
						oldSz[strlen(oldSz)-1]='\0';
			
					//read the new size from a file specified
					if (!fp2) fp2 = fopen(sz_file, "r");
					if (fp2)
					{
						while(fgets(szLine, MAX_PATH, fp2)!=NULL)
						{
							if (strstr(szLine, "0x")>0)
								strcpy(sz, szLine);
						}
						if (strlen(sz)==0)
							return 1;
						sz[strlen(sz)-1]='\0';
					}
					if (fp2) fclose(fp2);
					
					//check if new and old sizes are the same, exit if they're
					//we will not need to update coffbase file then
					if (strcmp(oldSz, sz)==0)
						return 0;
					else
					{
						DWORD st;
						LONG old_size=0, new_size=0;
						int written;
						//update the file with new size
						old_size = strtol(&(oldSz[2]), NULL, 16);
						new_size = strtol(&(sz[2]), NULL, 16);
						if (old_size>new_size)
						{
							//As long as the size in the file is larger than the real size
							//we're in no danger of overlapping the address space.
							//we will not need to update the file.
							return 0;
						}
						else
						{
							int sz_tmp;
							char tmp_buf[MAX_PATH];
							char tmp_buf2[MAX_PATH];
							int count=0;
							sz_tmp = new_size % 65536;
							if (sz_tmp > 0)
								new_size = new_size + (65536 - sz_tmp);
							ltoa(new_size, &tmp_buf, 16);
							sprintf(sz, "0x%08s", tmp_buf);
							sprintf(line, "%s %s %s\n", strs[0], strs[1], sz);
							sz_diff=new_size-old_size;
							if (fp3) fprintf(fp3, line);
							printf("%s has grown to a new size %s, the old size was %s\r\n", strs[0], sz, oldSz);
							bUpdated=TRUE; //the file was modified
						}

					}
				}
				else
				{
					/* if the file wasn't modified then we can just copy the lines
					** if the file was modified then all base addresses will most likely 
					** change.
					*/
					if (!bUpdated)
					{
						if (fp3) fprintf(fp3, line);
					}
					else
					{
						char *strs[256], *str;
						int count;
						long old_loc;
						char oldLoc[MAX_PATH];
						long new_loc;
						char newLoc[MAX_PATH];
						int sz_tmp;
					
						str=strtok(line, " ");
						count=0;
						while (str != NULL)
						{
							strs[count]=str;
							str=strtok(NULL, " ");
							count++;
						}
						//fix offset for locations to be multiple of 64K
						sz_tmp = sz_diff % 65536;
						if (sz_tmp>0)
						{
						sz_diff=sz_diff + (65536-sz_tmp);
						sz_tmp = sz_diff % 65536;
						}

						//put together a new line with updated base location
						strcpy(oldLoc, strs[1]);
						old_loc = strtol(&(oldLoc[2]), NULL, 16);
						new_loc=old_loc+sz_diff;
						ltoa(new_loc, &newLoc, 16);
						sprintf(line, "%s 0x%s %s", strs[0], newLoc, strs[2]);
						if (fp3) fprintf(fp3, line);
					}
				}

			}
		}
		if (fp) fclose(fp);
		if (fp3) fclose(fp3);
		/*copy newly created coffbase file to overwrite the old file*/
		if (GetFileAttributes(cf_file_id)!=INVALID_FILE_ATTRIBUTES && GetFileAttributes(cf_file)!=INVALID_FILE_ATTRIBUTES)
			MoveFileEx(cf_file_id, cf_file, MOVEFILE_REPLACE_EXISTING);
	}
	return 0;
}


void
printUsage()
{
	printf("updcoffbf USAGE:\r\n");
	printf("	updcoffbf <coffbase file> <binary name> <file containing new size>\r\n");
	printf("\r\n");
}