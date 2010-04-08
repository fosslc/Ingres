# include	<stdio.h>		
# include	<stdlib.h>
# include	<sys/stat.h>
# include	<string.h>
# include	<time.h>

/*
** GENINGNT:	This is a simple C DOS program written to parse a master file
** containing a list of the OpenIngres programs for Windows NT, their
** product code, path under \ingres, CA-Installer file flag, icon and
** icon description. The program then attempts to find the file under
** the ingres uncompressed path name, obtains its size, obtains the
** compressed size from the compressed path name, and generates *.ISC
** files to update the CA-Installer scripts, dos BAT files to compress
** the files using CAZIP, create individal 3.5" diskettes, and create
** the CD-ROM disk on a hard drive. 	
** 
** Assumptions:
** 		This program assumes the master file is called
** 	INGRESNT.MAS and is found in the current directory
** 	where GENINGNT is executed from. The file is a simple
** 	text file which can be edited via NOTEPAD and has the 
** 	following format: 
**
**OpenIngres DBMS DBMS
**OpenIngres Net NET
**OpenIngres Star STAR
**OpenIngres ESQL/C ESQLC
**OpenIngres Tools TOOLS
**OpenIngres Vision VISION
**d:\ingreswk\output
**d:\oping12\ingres
**d:\ingreswk\compress\ingres
**n:\oping12\cdrom902
**: Lines 1 to 6 define each product name as it should appear on the disk   
**: followed by the product baseapp/isc name.
**: Line #7 of mas file is the output path. 
**: Line #8 of mas file is the ingres path.
**: Line #9 of mas file is the compressed ingres path.
**: Line #10 of mas file is the cdrom output path. 
**: List of Files to copy to Disk 1 of every product.
**: XXXXXX.ISC is needed to represent the individual product ISC files.
**: Note terminated by a closing phrase of END
**CAINLOGS.DL_
**CALM_W32.DL_
**CATOCFGE.EX_
**CATOPATH.EX_
**INSTALL.EXE
**OPINGNT_.DAT
**IINSTNT.DLL
**IPPRECAL.BAT
**INSTALL.ISC
**XXXXXXX.ISC
**END COPY FILES SECTION 
**: Rest of file has following format:
**: (Note colon in column 1 indicates comment line.)
**: {product code} {path under ingres} {file name} {flag} {icon} {icon desc} {icon parm}
**A \files\english\ icaccgr.hlp 2
**A \files\english\ icauthhi.hlp 2
**A \files\english\ iccats.hlp 2
**A \files\english\ icdbacc.hlp 2
**A \files\english\ icdbace.hlp 2
**A \files\english\ icdbfrm.hlp 2
**A \files\english\ icdblst.hlp 2
**A \files\english\ icdbpacc.hlp 2
**A \files\english\ icdbtbl.hlp 2
** ...
**
** The flag field is the CA-Installer Installation file flag which has the 
** following meaning:
**	0		If source older, dialog box prompts user.		
**	1		Dialog prompting for overwrite always displays.
**	2		Destination always replaced with the source. 
**	3		Destination will NOT be replacced with source.
**
** Side effects:	None.
**
** Notes:	
**	In the aforementioned master file example an Ingres work 
**	staging area directory was created called ingreswk under which
**	could be found two additional directories compress and output,
**	containing the ingres compressed files and the output from
**	GENINGNT respectively. GENINGNT.EXE and INGRESNT.MAS could be
**	found directly in the ingreswk directory. This situation worked
**	well since there was a central staging area. The orginal 
**	files could be found on the same D drive under oping\ingres
**	and the CDROM drive was attached via the network and usually 
**	had a local drive number of N.  
**				
**	When running the program for the first time, the compressed path
**	is simply an empty ingres tree structure and the only output
**	files from GENINGNT which should be used are COMPRESS.BAT,
**	OUTPUT.TXT and ERROR.TXT. GENINGNT will generate one line per
**	file in COMPRESS.BAT to be CAZIPed from the ingres path to the
**	compressed path. ERROR.TXT will report on missing programs in
**	the compressed path which will be everything and those missing
**	from the uncompressed path which should be resolved. Once 
**	COMPRESS.BAT has executed, GENINGNT should be executed again, this
**	time, checking COMPRESS.BAT for output, which contains a CAZIP
**	statement for each program that will span more than one 3.5" disk.
**	Re-run GENINGNT and COMPRESS.BAT until COMPRESS.BAT is empty, i.e.
**	there are no files needed compression. Check the
**	OUTPUT.TXT and ERROR.TXT files for any unaccountable errors. If
**	all checks out, CDROMFIN.BAT can be executed to create the CDROM
**	drive staging area, and DISKAUTO.BAT can be run to create all 
**	the individual floppy disks by calling the DISK*.BAT files. 
**
** Steps:
**	1.	Edit INGRESNT.MAS to have the currect path names.
**	2.	Run GENINGNT.EXE to create the COMPRESS.BAT file.
**	3.  	Run COMPRESS.BAT to create the compressed staging area.
**	4.	Re-run steps 2 and 3 until COMPRESS.BAT contains no files.
**	5.	Re-run GENINGNT.EXE to correctly update the *.ISC scripts
**		for the CA-Installer directly.
**	6.  	Run CDROMFIN.BAT to create the CDROM drive staging area.
**	7.  	Run DISKAUTO.BAT to create all the 3.5" floppies or run
**		DISKA1.BAT, DISKA2.BAT, etc. to manually create them.
**									
** Inputs:
**	INGRESNT.MAS	Text file contain entry for each file.
**	INSTALL.SC	CA-Installer script for DBMS, product A.
**	NET.ISC		CA-Installer script for NET, product B.
**	STAR.ISC	CA-Installer script for Star, product C.
**	ESQLC.ISC	CA-Installer script for ESQLC, product D.
**	TOOLS.ISC	CA-Installer script for Tools, product E.
**	VISION.ISC	CA-Installer script for Vision, product F.
**
** Note: *.ISC scripts should be located in the compressed path.
**
**	Outputs:
**	COMPRESS.BAT	Bat file to CAZIP Ingres files.
**	OUTPUT.TXT	Standard Output from GENINGNT piped to 
**			OUTPUT.TXT via "GENINGNT.EXE > OUTPUT.TXT"
**	ERROR.TXT	Error file listing missing programs.
**	DISK*.BAT	Individual bat files to create each 3.5" disk.
**	DISKAUTO.BAT	Bat file to call each DISK*.BAT file.
**	CDROMFIN.BAT	Bat file to create the CDROM staging area.
**	*.ISC		CA-Installer ISC files updated respectively.
** 	DIR*.ISC	CA-Installer [directories] section for each
**			product (for reference only).
**	DISKS.ISC	CA-Installer [disks] section as merged into
**			the INSTALL.ISC script (for reference only).
**	FILE.ISC	CA-Installer [files] section for each file
**			concatenated together (for reference only).
**	PRODUCT.ISC	CA-Installer [products] section as merged into
**			each *.ISC script (for reference only).
**
**
** History:
**	17-jun-96 (somsa01)
**		Created from geningnt.cpp for CA-OpenIngres install.
**	16-jul-96 (somsa01)
**		Cleaned up disk creation.
**	29-jul-97 (mcgem01)
**		Updated for OpenIngres 2.0.
**	
*/

# define	NUM_PRODUCTS	1
# define	MAXSTRING	256
# define	DISK_MAX_SIZE	1457664L
# define 	MAXNUMFILES	2000

/* Globals */

char 	dirpath[100][MAXSTRING], filename[100][MAXSTRING],
	prod_name[NUM_PRODUCTS][MAXSTRING], baseapp[NUM_PRODUCTS][MAXSTRING],
	ingres_wk[MAXSTRING]  = "C:\\TEST\\INGRESWK\\",
	cdrom_path[MAXSTRING] = "C:\\TEST\\OPING\\",
	unc_ingres[MAXSTRING] = "C:\\TEST\\UNCOMPRS\\INGRES",
	cmp_ingres[MAXSTRING] = "C:\\TEST\\COMPRESS\\INGRES";

int  	dirprod[100][NUM_PRODUCTS], prod_disk[NUM_PRODUCTS], filename_cnt = 0;

struct 	MASTERFILE
{
	char prod, path[MAXSTRING], name[MAXSTRING], dosname[MAXSTRING],
	     flag, icon[MAXSTRING], icon_dsc[MAXSTRING], icon_parm[MAXSTRING];
};

/*
** ext_to_underscore:		privfunc
**
** Description:
**	Function to set the last character of the extension
**	of a file (path) name to an underscore in order
**	to denote a compressed file.
**
** Inputs:
**	upath	pointer to file path/name string
**
** Outputs:
**	upath => string updated directly.
**
** Returns:
**	None.
*/
void
ext_to_underscore(char *upath)
{
	int		i, ext_period = 0;

	for ( i = (strlen(upath)-1); (i > 0) && (upath[i] != '\\'); i-- )
		if (upath[i] == '.')
			ext_period = i;
	if (ext_period)
			upath[strlen(upath) - 1 ] = '_';
}


/*
** ext_to_spannum:		privfunc
**
** Description:
**	Function to set the extension of a file name
**	the span number.
**
** Inputs:
**	uname		pointer to file name string
**	span_num 	number for the extension,
**					i.e ABC.001, XYZ.002
**
** Outputs:
**	uname =>	string updated directly.
**
** Returns:
**	None.
*/
void
ext_to_spannum(char *uname, int span_num)
{
	int		i, ext_period = 0;
	char	extension[4] = "000";

	if (span_num < 10)
		 itoa(span_num, &extension[2], 10);
	else
		 itoa(span_num, &extension[1], 10);
	ext_period = strlen(uname);
	for ( i = 0; i < abs(strlen(uname)); ++i )
		if (uname[i] == '.')
			 ext_period = i;
	for ( i = 0; i < 4; ++i )
		uname[ext_period + 1 + i] = extension[i];
}


/*
** init_file_read:		privfunc
**
** Description:
**      Function to open a file in the *ingres_wk directory for reading.
**
** Inputs:
**	fname		pointer to string containing name of the file.
**	ingres_wk	global string containing output path name.
**
** Outputs:
**	None.
**
** Returns:
**	Returns FILE type pointer to opened file else NULL on error.
*/
FILE *
init_file_read(char *fname)
{
	char		 path_name[MAXSTRING] = "\0";
	FILE		 *fp = NULL;	
	
	sprintf(path_name, "%s%s", ingres_wk, fname);
	if ((fp = fopen(path_name, "r")) == NULL)
		printf("Error opening File: %s\n", path_name);
	return(fp);	
}


/*
** init_file_wrt:		privfunc
**
** Description:
**      Function to open a file in the *ingres_wk directory for output.
**
** Inputs:
**	fname		pointer to string containing name of the file.
**	ingres_wk	global string containing output path name.
**
** Outputs:
**	None.
**
** Returns:
**	Returns FILE type pointer to opened file else NULL on error.
*/
FILE *
init_file_wrt(char *fname)
{
	char		 path_name[MAXSTRING] = "\0";
	FILE		 *fp = NULL;	
	
	sprintf(path_name, "%s%s", ingres_wk, fname);
	if ((fp = fopen(path_name, "w")) == NULL)
		printf("Error opening File: %s\n", path_name);
	return(fp);	
}


/*
** cat_char:		privfunc
**
** Description:
**      Function to concatenate a character to the end of a string.
**
** Inputs:
**	catstr		pointer to the string.
**	catchar		character to be concatenated to the end of catstr.
**
** Outputs:
**	Catstr modifed to contain catchar plus a null character.
**
** Returns:
**	Pointer to string, i.e. catstr.
*/
char *
cat_char(char *catstr, char catchar)
{
	char	temp[2];
	
	temp[0] = catchar;
	temp[1] = '\0';
	if (strlen(catstr) < (MAXSTRING - 2))
		strcat(catstr,temp);
	return (catstr);
}		


/*
** filesize:		privfunc
**
** Description:
**      Function to return size of a file. 
**
** Inputs:
**	fingres		pointer to either unc_ingres or cmp_ingres path name
**	fpath		pointer to path under \ingres for file
**	fname		pointer to file name
**	under		flag to indicate whether filename needs to end with
**			an underscore to denote compression.
**
** Outputs:
**	None.
**
** Returns:
**	Returns long interger with file size or zero if not found.
*/
long
filesize(char *fingres, char *fpath, char *fname, int under)
{
	struct stat	 stbuf;
	char		 true_path[MAXSTRING] = "\0";
	
	sprintf(true_path, "%s%s%s", fingres, fpath, fname);
	if (fingres == cmp_ingres && under)
		  ext_to_underscore(true_path);
	if (stat(true_path, &stbuf))
	{
		printf("Error sizing file %s\n",true_path);
		return 0;
	}
	else
		return (stbuf.st_size);
}

/*
** file_exists:		privfunc
**
** Description:
**      Function to return 1 if file exists in specified path. 
**
** Inputs:
**	fingres		pointer to either unc_ingres or cmp_ingres path name
**	fpath		pointer to path under \ingres for file
**	fname		pointer to file name
**
** Outputs:
**	None.
**
** Returns:
**	Returns 1 if file found, 0 otherwise.
*/
int
file_exists(char *fingres, char *fpath, char *fname)
{
	struct stat	stbuf;
	char		true_path[MAXSTRING] = "\0";
	
	sprintf(true_path, "%s%s%s", fingres, fpath, fname);
	if (stat(true_path, &stbuf))
		return (0);
	return (1);
}


/*
** delete_file:		privfunc
**
** Description:
**      Function to delete file in ingreswk path. 
**
** Inputs:
**	fname		pointer to file name
**
** Outputs:
**	None.
**
** Returns:
**	1 if successfull, 0 otherwise..
*/
int
delete_file(char *fname)
{
	char	remvd[MAXSTRING] = "\0";

	sprintf(remvd, "%s%s", ingres_wk, fname);
	if (remove(remvd))
	{							/* Delete fname File */ 	
		printf("Error deleting file %s.\n",remvd);
		return 0;
	}
	else
		printf("Deleted file %s.\n",remvd);
		return 1;
}


/*
** update_prod_isc:		privfunc
**
** Description:
**      Function to update the INSTALL.ISC, NET.ISC, etc. files with the
**	respective *.ISC result files. 
**
** Inputs:
**	Uses global array baseapp to determine *.ISC name for each product 
**	and PRODUCT.ISC, DISKS*.ISC, DIR*.ISC and FILE*.ISC files found in
**	*ingres_wk directory previously created by the program. 
**
** Outputs:
**	Updates *.ISC file for each product in *cmp_ingres directory.
**	Function will delete the DISK*.ISC and FILE*.ISC files in *ingres_wk
**	directory once it has merged them into the product ISC files. 
**
** Returns:
**		None.
*/
void
update_prod_isc()
{                        
	FILE	*isc_fp, *temp_fp, *merg_fp;
	char	isc[MAXSTRING], temp[MAXSTRING], merg[MAXSTRING],
			line[MAXSTRING], *name_temp;
	int		i, j, found, disk;

	for ( i = 0; i < NUM_PRODUCTS; ++i )
	{ 				/* Loop through NUM_PRODUCTS products */
		found = 0;
		strcpy(isc,cmp_ingres);
		cat_char(isc,'\\');
		if (!i)
			strcat(isc,"INSTALL");
		else
			strcat(isc,baseapp[i]);
		strcat(isc,".ISC");
		if ((isc_fp = fopen(isc, "r")) == NULL)
		{
			printf("Error opening file %s\n",isc);
			return;
		}
		sprintf(temp, "%s%s", cmp_ingres, "\\TEMP.ISC");
		if ((temp_fp = fopen(temp, "w")) == NULL)
		{
			printf("Error opening file %s\n",temp);
			return;
		}
		while (fgets(line,MAXSTRING - 1,isc_fp) != NULL)
		{
			if (strncmp(line,"[products]",10) == 0)
			{
				fprintf(temp_fp,"[products]\n");
				for ( j = 0; j < NUM_PRODUCTS; ++j)
				{
					name_temp = strrchr(prod_name[j],' ');
					++name_temp;
					if (j)
						fprintf(temp_fp,
						"\"%c\",\"%s\",\"%s\",\"OpenIngres for Microsoft Windows NT"
						" %s\",\"D\",\"%s.ISC\",\"%d\"\n", (j + 'A'),
						prod_name[j], baseapp[j], name_temp, baseapp[j], prod_disk[j]);
					else	  
						fprintf(temp_fp,
						"\"%c\",\"%s\",\"%s\",\"OpenIngres for Microsoft Windows NT"
						" %s\",\"D\"\n", (j + 'A'), prod_name[j], baseapp[j], name_temp);
				}
				found = 1;
			}
			else
				if (strncmp(line,"[disks]",7) == 0)
				{

					fprintf(temp_fp,"[disks]\n");
					for ( j = i; (j < NUM_PRODUCTS && !i) || (i && j <= i); ++j)
						for ( disk = prod_disk[j]; disk < prod_disk[j + 1]; ++disk)
							fprintf(temp_fp,"\"%d\",\"A:\\\",\"%s Disk #%d\"\n",
								disk, prod_name[j], disk + 1);
					found = 1;
				}
				else
					if (strncmp(line,"[directories]",13) == 0)
					{
						j = 0;
						fprintf(temp_fp,"[directories]\n");
						fprintf(temp_fp,"\"%c\",\"0\",\"%s\",\"OpenIngres %s\"\n",
							('A' + i), dirpath[0], dirpath[0]);
						while (dirpath[j][0] != NULL)
						{
							if (dirpath[j][strlen(dirpath[j])-1] == '\\')
								dirpath[j][strlen(dirpath[j])-1] = '\0';
							if (strcmp(dirpath[j],"\\ingres\\files\\dictfile") == 0)
								cat_char(dirpath[j],'s');
							if (strcmp(dirpath[j],"\\ingres\\files\\collatio") == 0)
								cat_char(dirpath[j],'n');
							if (dirprod[j][i])
								fprintf(temp_fp,
								"\"%c\",\"%c%c\",\"%s\",\"OpenIngres %s\"\n",
								('A' + i), ((j - 1) / 26) + 'A', ((j - 1) % 26) + 'A',
								dirpath[j], dirpath[j]);
							++j;
						}
						found = 1;
					}
					else
						if (strncmp(line,"[files]",7) == 0)
						{
							found = 1; 
							strcpy(merg,"FILE");
							strcat((cat_char(merg,'A' + i)),".ISC");
							if ((merg_fp = init_file_read(merg)) == NULL)
								return;
							while (fgets(line,MAXSTRING - 1,merg_fp) != NULL)
								fprintf(temp_fp,"%s", line);
							if (merg_fp)
							fclose(merg_fp);
							if (!delete_file(merg)) return; 
						}
			if (found)
			{
				fprintf(temp_fp,";END %s\n\n", baseapp[i]);
				while (fgets(line,MAXSTRING - 1,isc_fp) != NULL && found)
					if (strncmp(line,";END",4) == 0)
						found = 0;
			}				 	/* End if found */
			else
				fprintf(temp_fp,"%s", line);
		}  						/* End While isc_fp */
		fclose(temp_fp);
		fclose(isc_fp);
		if (remove(isc))
		{
			printf("Error deleting file %s\n",isc);
			return;
		};
		if (rename(temp,isc))
		{
			printf("Error renaming file %s to %s\n",temp,isc);
			return;
		}
	} 							/* End For Loop */
}								/* End function */


/*
** init_dir_arrays:		privfunc
**
** Description:
**      Function to initialize the directory product,
**		filename, and path arrays. 
**
** Inputs:
**		None.
**
** Outputs:
**		None.
**
** Returns:
**		None.
*/
void
init_dir_arrays()
{
	int		i = 0, j;

	filename[0][0] = '\0';
	strcpy(dirpath[i], "C:\\oping");
	for ( j = 0; j < NUM_PRODUCTS; j++ )
		dirprod[i][j] = 0;

	while ( ++i < 100 )
	{
		filename[i][0] = '\0';
		dirpath[i][0] = '\0';
		for ( j = 0; j < NUM_PRODUCTS; j++ )
			dirprod[i][j] = 0;
	}
}


/*
** find_dir_code:		privfunc
**
** Description:
**      Function to return index to dirpath array based on path. 
**	If the path is not found in the dirpath array, it is added
**	before returning. Also, the dirprod array is updated to
**	indicate the directory is being used by which product
**	before returning.
**
** Inputs:
**	dpath	pointer to the dirpath array
**	dprod	product character code
**
** Outputs:
**	None.
**
** Returns:
**	Integer index to the corresponding dirpath array entry.
*/
int 
find_dir_code(char *dpath, char dprod)
{
	int	 i = 0;
	
	if (strcmp(dpath,"\\") == 0)
		return (0);
	while ( i < 100 && dirpath[i][0] )
	{
		if ( strcmp(dirpath[i],dpath) == 0 )
		{
			dirprod[i][dprod - 'A'] = 1;
			return (i);
		}
		++i;
	}
	if (i < 100)
	{
		strcpy(dirpath[i],dpath);
		dirprod[i][dprod - 'A'] = 1;
		return (i);
	}
	return -1;
}


/*
** bld_diskauto_bat:	privfunc
**
** Description:
**      Function to build a bat file called DISKAUTO.BAT to call the
**	individual DISK*.BAT files in an automated fashion in order
**	to speed up the cutting of disks.
**
** Inputs:
**	disk_total		total number of disks for all 6 products
**
** Outputs:
**	Creates DISKAUTO.BAT file in *ingres_wk directory.
**
** Returns:
**	None.
*/
void
bld_diskauto_bat(int disk_total)
{
	FILE	*disk_fp;
	int		i, product = 0, prod_disk_cnt = 0;

	if ((disk_fp = init_file_wrt("DISKAUTO.BAT")) == NULL)
		return;
	fprintf(disk_fp,"@clear\n");
	for ( i = 0; i < disk_total; ++i )
	{
		if (i == prod_disk[product+1])
		{ 
			++product;
			prod_disk_cnt = 0;
		}
		++prod_disk_cnt;
		fprintf(disk_fp,"@echo.\n");
		fprintf(disk_fp,"@echo Enter Disk %c%d in Drive A:\n",
			('A'+product),prod_disk_cnt);
		fprintf(disk_fp,"@pause\n");
		fprintf(disk_fp,"@call DISK%c%d.BAT\n",('A'+product),prod_disk_cnt);
	}
	fclose(disk_fp);
}


/*
** file_disk_bat:		privfunc
**
** Description:
**      Function to build individual bat files to copy compressed files
**	to individual disks.
**
** Inputs:
**	ddsk_fp		pointer to current DISK*.BAT opened file
**	dpath		pointer to the path of the file under /ingres
**	dname		pointer to the name of the file
**	dmkdir		flag to denote whether a MKDIR statement is needed
**			to create the directory on the floppy for the file
**	under		flag to denote whether the filename extension should end
**			with an underscore to indicate compression
**	span		flag to denote whether the filename extension should be
**			a number to indicate a spanned-compressed file.
**	cdrom_fp	pointer to cdrom build file CDROMBLD.BAT.
**					
**				
** Outputs:
**	Updates file pointed to by ddsk_fp with approprate DOS COPY commands.
**
** Returns:
**	None.
*/
void
file_disk_bat(FILE *ddsk_fp, char *dpath, char *dname, 
	char *dmkdir, int under, int span, FILE *cdrom_fp)
{
	char	disk_path[MAXSTRING] = "\0", encod_path[MAXSTRING],
			flop_path[MAXSTRING] = "A:\0", *temp_ext;
	int		i;

	sprintf(disk_path, "%s%s%s", cmp_ingres, dpath, dname);
	if (under && !span)
		ext_to_underscore(disk_path);
	if (!span)
	{
		for ( i = (strlen(dpath)-2); (i > 0) && (dpath[i] != '\\'); i-- );
		if (i < 0) i = 0;
		strcat(flop_path,&dpath[i]);
		flop_path[strlen(flop_path)-1] = '\0';
	}
	else
		dmkdir = NULL;
	if (dmkdir && strlen(flop_path) > 3)
		fprintf(ddsk_fp, "MKDIR %s\n", flop_path);
	strcat(flop_path,"\\\0");
	strcat(flop_path,dname);
	if (under && !span)
		ext_to_underscore(flop_path);
	temp_ext = strchr(dname,'.');
	if (temp_ext && strcmp(temp_ext,".ISC") == 0)
	{
		strcpy(encod_path,disk_path);
		ext_to_underscore(disk_path);
		fprintf(ddsk_fp, "ENCODINF %s %s\n", encod_path, disk_path);
		fprintf(cdrom_fp, "ENCODINF %s %s\n", encod_path, disk_path);
	}
	fprintf(ddsk_fp, "COPY %s %s\n", disk_path, flop_path);
	fprintf(cdrom_fp, "COPY %s %s%s\n", disk_path, cdrom_path,
			&flop_path[2]);
}


/*
** file_disk_init:		privfunc
**
** Description:
**      Function to initialize the disk copy script built by
**	calling function file_disk_bat.
**
** Inputs:
**	icdrom_fp	pointer to current CDROMBLD.BAT opened file
**	idisk_num	current product diskette number
**	new_prod	first diskette of product so copy CA-Installer programs
**	iprod		character denoted product (i.e. A thru F for 6 products)
**			to create the directory on the floppy for the file
**	cmp_size	pointer to the disk compressed size variable of function
**			read_master.
**	filename	Array containing list of files to copy to disk 1 of
**			every product.
**	filename_cnt	Count of files in filename array.
**				
** Outputs:
**	Updates *cmp_size field with total size of CA-Installer programs.
**
** Returns:
**	File pointer to newly init DISK*.BAT file in the *ingres_wk directory.
*/
FILE *
file_disk_init(FILE *icdrom_fp, int idisk_num, int new_prod, 
	char iprod, long *cmp_size)
{
	FILE		*idsk_fp;
	int			i;
	char		dsk_fnm[MAXSTRING] = "\0" , *dsk_fnum = "00";

	strcpy(dsk_fnm,"DISK");
	dsk_fnum = itoa(idisk_num, dsk_fnum, 10);
	strcat((cat_char(dsk_fnm,iprod)),dsk_fnum);
	strcat(dsk_fnm,".BAT");
	if ((idsk_fp = init_file_wrt(dsk_fnm)) == NULL)
		return (idsk_fp);
	fprintf(idsk_fp,"LABEL A:INGRES %c%s\n", iprod, dsk_fnum);
	if (new_prod && idsk_fp != NULL)
	{
		for ( i = 0; i < filename_cnt; i++ )
			if (strncmp(filename[i],"XXXXXX",6) == 0)
			{
				sprintf(dsk_fnm,"%s.ISC",baseapp[iprod-'A']);
				if (iprod != 'A')
				{  
					file_disk_bat(idsk_fp, "\\", dsk_fnm, NULL, 0, 0, icdrom_fp);
					*cmp_size += filesize(cmp_ingres,"\\", dsk_fnm, 0);
				}
			}
			else 
			{			
				file_disk_bat(idsk_fp, "\\", filename[i], NULL, 0, 0, icdrom_fp);
				*cmp_size += filesize(cmp_ingres,"\\",filename[i],0);
			}
	}
	printf("\nProduct = %c  %s\t Disk #%d \t "
    	"Size of Installion Files = %ld\n", iprod,
		prod_name[iprod - 'A'],idisk_num, *cmp_size);

	return (idsk_fp);
}


/*
** bld_file_isc:		privfunc
**
** Description:
**      Function to build the file section of the CA-Installer
**	for each product.
**
** Inputs:
**	fiisc_fp	pointer to current FILE*.ISC opened file
**	fpgm		pointer to current MASTERFILE line structure
**	fdisk_num   current product diskette number
**	fdisk_size	compressed size of current file
**	span_num	0 if not spanned file, otherwise span number.
**				
** Outputs:
**	Writes line to FILE*.ISC file for current file pointed to by fpgm.
**
** Returns:
**	1 if error condition found updating file, 0 if successfull.
*/
int
bld_file_isc(FILE *fisc_fp, struct MASTERFILE *fpgm, int fdisk_num, 
	long fdisk_size, int span_num)
{
	char	*disk_num = "01", disk_dir[MAXSTRING] = "\0",
			*idir, flag[MAXSTRING] = "RCO", temp[MAXSTRING] = "\0";
	int  	i, dir_num;

	if ((dir_num = find_dir_code(fpgm->path, fpgm->prod)) != -1)
	{
		for ( i = (strlen(fpgm->path)-2); (i > 0) && 
			(fpgm->path[i] != '\\'); i-- );
		idir = strcat(disk_dir, &fpgm->path[++i]);
		disk_dir[strlen(disk_dir)-1] = '\0';
		cat_char(flag, fpgm->flag);
		if (fpgm->icon_dsc[0] != '\0')
		{
			cat_char(flag,'I');
			if (fpgm->prod != 'D')
				cat_char(flag,fpgm->prod);
			else
				cat_char(flag,'Z');
		}
		if (span_num)
		{
			--span_num;
			fdisk_num -= span_num;
			itoa(span_num, temp, 10);
			strcat(temp,flag);
			strcpy(flag,temp);
		}
		fdisk_size = (fdisk_size + 500) / 1000;
		if (fdisk_size == 0)
			fdisk_size = 1;
		for ( i = 0; i <= span_num; ++i )
		{
			disk_num = itoa((fdisk_num + i), disk_num, 10);
			if (i == 1)
			{
				strcpy(flag,"SCO");
				cat_char(flag,fpgm->flag);
				fdisk_size = 1;
			}
			strcpy(temp,fpgm->dosname);
			if (!span_num)
			{
			    if (!strcmp(idir, "WINSYS"))
			    {
				ext_to_underscore(temp);
				fprintf(fisc_fp,"\"%s\",\"%c\",\"%s\",\"%%2%%\",\"WINSYS\",\"%s\",\""
					"%s\",\"%s\",\"\",\"\",\"%ld\",\"%ld\",\"%s\",\"%s\"\n",
					disk_num, fpgm->prod, flag, fpgm->name, temp,
					fpgm->icon_dsc, fdisk_size, fdisk_size, fpgm->icon, fpgm->icon_parm );
			    }
			    else
			    {
				ext_to_underscore(temp);
				fprintf(fisc_fp,"\"%s\",\"%c\",\"%s\",\"%c%c\",\"%s\",\"%s\",\""
					"%s\",\"%s\",\"\",\"\",\"%ld\",\"%ld\",\"%s\",\"%s\"\n",
					disk_num, fpgm->prod, flag, ((dir_num - 1) / 26) + 'A',
					((dir_num - 1) % 26) + 'A', idir, fpgm->name, temp,
					fpgm->icon_dsc, fdisk_size, fdisk_size, fpgm->icon, fpgm->icon_parm );
			    }
			}
			else
			{
				ext_to_spannum(temp, i + 1);
				fprintf(fisc_fp,"\"%s\",\"%c\",\"%s\",\"%c%c\",\"\",\"%s\",\""
					"%s\",\"%s\",\"\",\"\",\"%ld\",\"%ld\",\"%s\",\"%s\"\n",
					disk_num, fpgm->prod, flag, ((dir_num - 1) / 26) + 'A',
					((dir_num - 1) % 26) + 'A', fpgm->name, temp,
					fpgm->icon_dsc, fdisk_size, fdisk_size, fpgm->icon, fpgm->icon_parm );
			}
		} 	/* End for */
	}  		/* End if directory found*/
	else
	{
		printf("Error finding directory code for file: %s path: %s\n",
			fpgm->name, fpgm->path );
		return 1;
	}
	return 0;
}


/*
** comp_file_time:		privfunc
**
** Description:
**      Function to return 0 if the file last modified time of the compressed
**	file is greater than or equal to the uncompressed version or else 1.
**
** Inputs:
**	fpath		pointer to the path under \ingres of the file
**	fname    	name of the file (NTFS structure)
**	fdosname    name of the file in DOS (FAT structure)
**	span 		span number
**				
** Outputs:
**	None.
**
** Returns:
**	1 always unless the uncompressed last modified time of the file is less
**	than or equal to the compressed time and the last modified time of the
**	dos/FAT named file equals the NTFS named file.
*/
int
comp_file_time(char *fpath, char *fname, char *fdosname, int span)
{
	struct stat	file_stbuf;
	char	 	file_path[MAXSTRING] = "\0";
	time_t		cmp_real_time, unc_real_time, unc_dos_real_time;
		
	sprintf(file_path, "%s%s%s", cmp_ingres, fpath, fdosname);
	if (span)
		ext_to_spannum(file_path, span);
	else
		ext_to_underscore(file_path);
	if (stat(file_path, &file_stbuf))
	{
		printf("Error finding time of file %s\n",file_path);
		return 1;
	}
	cmp_real_time = file_stbuf.st_mtime;
	sprintf(file_path, "%s%s%s", unc_ingres, fpath, fname);
	if (stat(file_path, &file_stbuf))
	{
		printf("Error finding time of file %s\n",file_path);
		return 1;
	}
	unc_real_time = file_stbuf.st_mtime;
	if (!span)
		sprintf(file_path, "%s%s%s", unc_ingres, fpath, fdosname);
	else
	{
		sprintf(file_path, "%s%s%s", cmp_ingres, fpath, fname);
		ext_to_underscore(file_path);
	}
	if (stat(file_path, &file_stbuf))
	{
		printf("Error finding time of file %s\n",file_path);
		return 1;
	}
	unc_dos_real_time = file_stbuf.st_mtime;
	if ((unc_real_time <= cmp_real_time) && 
		(unc_dos_real_time == unc_real_time) && !span)
		return 0;
	if ((unc_real_time <= cmp_real_time) && 
		(unc_dos_real_time == cmp_real_time) && span)
		return 0;
	printf("Error %s modification times do not match.\n", fname);	
	return 1;
}


/*
** file_comp_bat:		privfunc
**
** Description:
**      Function to build the bat file COMPRESS.BAT to CAZIP files
**	from the uncompressed to the compressed staging areas if
**	the last modified time of the file differs.
**
** Inputs:
**	ccmp_fp			pointer to the COMPRESS.BAT opened file
**	ppgm			pointer to Masterfile structure variable
**	span			flag to denote spanned file when compressed
**				
** Outputs:
**	Updates COMPRESS.BAT or COMPSPAN.BAT files in *ingres_wk directory.
**
** Returns:
**	1 if file needs compression, 0 otherwise.
*/
int
file_comp_bat(FILE *ccmp_fp, struct MASTERFILE *ppgm, int span)
{
	char	cmp_fnm[MAXSTRING] = "\0",
		  	unc_fnm[MAXSTRING] = "\0";

	if (comp_file_time(ppgm->path,ppgm->name,ppgm->dosname,span))
	{
		sprintf(unc_fnm, "%s%s", unc_ingres, ppgm->path);
		if (strcmp(ppgm->name,ppgm->dosname) && !span)
			fprintf(ccmp_fp, "COPY %s%s %s%s\n",
				unc_fnm, ppgm->name, unc_fnm, ppgm->dosname);
		if (!span)
			strcat(unc_fnm,ppgm->dosname);
		else
			strcat(unc_fnm,ppgm->name);
		sprintf(cmp_fnm, "%s%s%s", cmp_ingres, ppgm->path, ppgm->dosname);
		ext_to_underscore(cmp_fnm);
		if (span)
		{
			ext_to_spannum(cmp_fnm,span);
			fprintf(ccmp_fp, "CAZIP -c -b -3 %s %s\n", unc_fnm, cmp_fnm);
			sprintf(cmp_fnm, "%s%s%s", cmp_ingres, ppgm->path, ppgm->name);
			ext_to_underscore(cmp_fnm);
			fprintf(ccmp_fp, "CAZIP -c -b %s %s\n", unc_fnm, cmp_fnm);
		}
		else
			if ((strlen(cmp_fnm) + strlen(unc_fnm)) > 115)
			{
				sprintf(cmp_fnm, "%s\\%s", cmp_ingres, ppgm->dosname);
				ext_to_underscore(cmp_fnm);
				if ((strlen(cmp_fnm) + strlen(unc_fnm)) > 115)
				{
					printf("Error unable to compress %s, CAZIP parm limit exceeded.\n",
						ppgm->dosname);
					return 1;
				}
				fprintf(ccmp_fp, "CAZIP -c -b %s %s\n", unc_fnm, cmp_fnm);
				fprintf(ccmp_fp, "COPY %s ", cmp_fnm);
				sprintf(cmp_fnm, "%s%s%s", cmp_ingres, ppgm->path, ppgm->dosname);
				ext_to_underscore(cmp_fnm);
				fprintf(ccmp_fp,"%s\n",cmp_fnm);
				return 1;
			}
			else
				fprintf(ccmp_fp, "CAZIP -c -b %s %s\n", unc_fnm, cmp_fnm);
		return 1;
	}
	return 0;
}


/*
** file_isc_init:		privfunc
**
** Description:
**      Function to initialize the FILE*.ISC file for the CA-Installer
**	for each product.
**
** Inputs:
**	iprod		character denoting product code
**	idisknum	current diskette number
**	filename	Array containing list of files to copy to disk 1 of
**			every product.
**	filename_cnt	Count of files in filename array.
**				
** Outputs:
**	Updates the FILE*.ISC file in *ingres_wk directory.
**
** Returns:
**	Pointer to newly created FILE*.ISC file in the *ingres_wk directory.
*/
FILE *
file_isc_init(char iprod, int idisknum)
{
	FILE	*iiscp_fp;
	char	isc_fname[MAXSTRING] = "\0";
	int		i;

	strcpy(isc_fname,"FILE");
	strcat((cat_char(isc_fname,iprod)),".ISC");
	if ((iiscp_fp = init_file_wrt(isc_fname)) == NULL)
		return (NULL);
	else
		fprintf(iiscp_fp,"[files]\n");
	if (iprod == '\0') iprod = 'A';
	for (i = 0; i < filename_cnt; ++i)
	{
		if (filename[i][strlen(filename[i]) - 1] == '_')
		{ 
			strcpy(isc_fname,"%5%\",\"\",\"");
			strcat(isc_fname,filename[i]);
			if (isc_fname[(strlen(isc_fname))-2] == 'L')
				isc_fname[(strlen(isc_fname))-1] = 'L';
			else
				isc_fname[(strlen(isc_fname))-1] = 'E';
			strcat(isc_fname,"\",\"");
			strcat(isc_fname,filename[i]);
			strcat(isc_fname,"\",\"\",\"\",\"\",\"1\",\"1\",\"\",\"\"");
			fprintf(iiscp_fp,"\"%d\",\"%c\",\"RCO0\",\"%s\n",
				idisknum, iprod, isc_fname);
		}
		else if (filename[i][strlen(filename[i]) - 1] == '$')
		{ 
			strcpy(isc_fname,"%5%\",\"\",\"");
			strcat(isc_fname,filename[i]);
			if (isc_fname[(strlen(isc_fname))-2] == 'L')
				isc_fname[(strlen(isc_fname))-1] = '$';
			else
				isc_fname[(strlen(isc_fname))-1] = 'E';
			strcat(isc_fname,"\",\"");
			strcat(isc_fname,filename[i]);
			strcat(isc_fname,"\",\"\",\"\",\"\",\"1\",\"1\",\"\",\"\"");
			fprintf(iiscp_fp,"\"%d\",\"%c\",\"RCO0\",\"%s\n",
				idisknum, iprod, isc_fname);
		}
	}
	return (iiscp_fp);
}


/*
** bld_cdromfin_bat:	privfunc
**
** Description:
**      Function to create the CDROMFIN.BAT file by extracting the COPY
**	statements from the temp CDROMBLD.BAT file created by function
**	file_disk_bat.
**
** Inputs:
**	Reads and then deletes CDROMBLD.BAT file in *ingres_wk directory.
**	Uses global array dirpath to generate appropriate MKDIR statements.
**				
** Outputs:
**	Creates new CDROMFIN.BAT file in *ingres_wk directory.
**
** Returns:
**	None.
*/
void
bld_cdromfin_bat()
{
	FILE	*cdrom_fp, *cdrom_final_fp;
	char	mline[MAXSTRING] = "\0", mline_array[MAXNUMFILES][MAXSTRING];
	int		i, j, file_match = 0, count = 0;

	for ( i = 0; i < MAXNUMFILES; ++i )
		mline_array[i][0] = '\0';

	if ((cdrom_fp = init_file_read("CDROMBLD.BAT")) == NULL)
		return;
	if ((cdrom_final_fp = init_file_wrt("CDROMFIN.BAT")) == NULL) 
		return;
	for ( j = 0; j < 100 && dirpath[j][0] != NULL; ++j )
	{
		if (strcmp(dirpath[j],"\\ingres\\files\\dictfiles") == 0 ||
			strcmp(dirpath[j],"\\ingres\\files\\collation") == 0)
			dirpath[j][(strlen(dirpath[j]) - 1)] = '\0';
		for ( i = (strlen(dirpath[j])-2); (i > 0) &&
			(dirpath[j][i] != '\\'); i--);
		sprintf(mline, "MKDIR %s", cdrom_path);
		if (j) 
			strcat(mline, &dirpath[j][i]);
		fprintf(cdrom_final_fp,"%s\n",mline);
	}
	while (fgets(mline,MAXSTRING - 1,cdrom_fp))
	{
		file_match = 0;
		for (i = 0; i < count; ++i)
				if (strcmp(mline_array[i], mline) == 0)
					file_match = 1;
		if (!file_match)
			{ 
			fprintf(cdrom_final_fp,"%s",mline);
			if (count < MAXNUMFILES)
				strcpy(mline_array[count++],mline); 
			}
	}	
	fclose(cdrom_fp);
	fprintf(cdrom_final_fp,"ATTRIB +R %s\\* /s\n",cdrom_path);
	fprintf(cdrom_final_fp,"ATTRIB +H %s\\CAIN*EXE /s\n",cdrom_path);
	fprintf(cdrom_final_fp,"ATTRIB +H %s\\*.ISC /s\n",cdrom_path);
	fprintf(cdrom_final_fp,"ATTRIB +H %s\\*.BMP /s\n",cdrom_path);
	fprintf(cdrom_final_fp,"ATTRIB +H %s\\*.00* /s\n",cdrom_path);
	fprintf(cdrom_final_fp,"ATTRIB +H %s\\*.HLP /s\n",cdrom_path);
	fprintf(cdrom_final_fp,"ATTRIB +H %s\\*.BAT /s\n",cdrom_path);
	fprintf(cdrom_final_fp,"ATTRIB +H %s\\CICHKCAP.EXE /s\n",cdrom_path);
	fprintf(cdrom_final_fp,"ATTRIB +H %s\\OPINGNT_.DAT /s\n",cdrom_path);
	fprintf(cdrom_final_fp,"ATTRIB +H %s\\IINSTNT.DLL  /s\n",cdrom_path);
	fprintf(cdrom_final_fp,"ATTRIB +H %s\\DLLBMP.DLL   /s\n",cdrom_path);
	fprintf(cdrom_final_fp,"ATTRIB +H %s\\SPLASH.DLL   /s\n",cdrom_path);
	fprintf(cdrom_final_fp,"ATTRIB +H %s\\SLIDE.DLL    /s\n",cdrom_path);
	fclose(cdrom_final_fp);
	delete_file("CDROMBLD.BAT");
}

/*
** bld_compress_bat:	privfunc
**
** Description:
**      Function to create the COMPRESS.BAT file by removing the  
**	duplicate statements from the temp COMPSTMP.BAT file created by
**	function read_master.
**
** Inputs:
**	Reads and then deletes COMPSTMP.BAT file in *ingres_wk directory.
**				
** Outputs:
**	Creates new COMPRESS.BAT file in *ingres_wk directory.
**
** Returns:
**	None.
*/
void
bld_compress_bat()
{
	FILE	*comp_fp, *comp_final_fp;
	char	mline[MAXSTRING] = "\0", mline_array[MAXNUMFILES][MAXSTRING];
	int		i, file_match = 0, count = 0;

	for ( i = 0; i < MAXNUMFILES; ++i )
		mline_array[i][0] = '\0';

	if ((comp_fp = init_file_read("COMPSTMP.BAT")) == NULL)
		return;
	if ((comp_final_fp = init_file_wrt("COMPRESS.BAT")) == NULL) 
		return;
	
	while (fgets(mline,MAXSTRING - 1,comp_fp))
	{
		file_match = 0;
		for (i = 0; i < count; ++i)
				if (strcmp(mline_array[i], mline) == 0)
					file_match = 1;
		if (!file_match)
			{ 
			fprintf(comp_final_fp,"%s",mline);
			if (count < MAXNUMFILES)
				strcpy(mline_array[count++],mline); 
			}
	}	
	fclose(comp_fp);
	fclose(comp_final_fp);
	delete_file("COMPSTMP.BAT");
}


/*
** proc_mline:		privfunc
**
** Description:
**      Function to process one line of the INGRESNT.MAS file and
**	fill in the MASTERFILE stucture from it.
**
** Inputs:
**	pmline	pointer to currently read raw MASTERFILE line
**	ppgm	pointer to Masterfile structure variable
**
** Outputs:
**	Updates the passed ppgm MASTERFILE structure directly.
**
** Returns:
**	Number of fields processed from input line.
*/
int
proc_mline(char *pmline, struct MASTERFILE *ppgm)
{
	int		i, j = 1, k = 0, extension = 0, quote_active = 0;


	for ( i = 0; i < MAXSTRING; ++i )
	{
		ppgm->path[i] = '\0';
		ppgm->name[i] = '\0';
		ppgm->icon[i] = '\0';
		ppgm->icon_dsc[i] = '\0';
		ppgm->icon_parm[i] = '\0';
	}
		ppgm->prod = '\0';
		ppgm->flag = '\0';
	if (*pmline == ':')
		return 0;
	while (*pmline != '\n' && *pmline != NULL && k < (MAXSTRING - 1))
	{
		if (*pmline == ' ' && !quote_active)
		{
			if ( j < 7 ) j++;
			k = 0;
		}
		else
			switch(j)
			{
			
			case 1:
				if (k)
					goto mline_error;
				ppgm->prod = *pmline;
				++k;
				break;
				
			case 2:
				ppgm->path[k] = *pmline;
				ppgm->path[++k] = '\0';
				break;
				
			case 3:
				ppgm->name[k] = *pmline;
				ppgm->name[++k] = '\0';
				break;
				
			case 4:
				if (k)
					goto mline_error;
				ppgm->flag = *pmline;
				++k;
				break;
				
			case 5:
				ppgm->icon[k] = *pmline;
				ppgm->icon[++k] = '\0';
				break;
			
			case 6:
				if (*pmline == '\"')
				{
				 	if (quote_active)
				 		quote_active = 0;
					else 
						quote_active = 1;
				}
				else
				{
					ppgm->icon_dsc[k] = *pmline;
					ppgm->icon_dsc[++k] = '\0';
				}
				break;

			case 7:
				if (*pmline == '\"')
				{
				 	if (quote_active)
				 		quote_active = 0;
					else 
						quote_active = 1;
				}
				else
				{
					ppgm->icon_parm[k] = *pmline;
					ppgm->icon_parm[++k] = '\0';
				}
				break;

			}
		++pmline;
	}
	strcpy(ppgm->dosname,ppgm->name);
	for ( i = 0; i < abs(strlen(ppgm->dosname)); ++i )
		if (ppgm->dosname[i] == '.')
			extension = i;
	if (((strlen(ppgm->dosname) - extension) > 4) && extension != 0)
		ppgm->dosname[extension + 4] = '\0';
	if (extension > 8)
		strcpy( &ppgm->dosname[8], &ppgm->dosname[extension]);

	if (!strcmp(ppgm->icon, "\"\"")) ppgm->icon[0] = '\0';

	if (ppgm->icon[0] == ' ')
	{
		ppgm->icon[0] = '\0';
		if (j > 4)
			j = 4;
	}
	if (ppgm->icon_dsc[0] == ' ')
	{
		ppgm->icon_dsc[0] = '\0';
		if (j > 4)
		j = 4;
	}
	if (*pmline != NULL && (j == 4 || j == 6 || j == 7))
		return (j);
		
mline_error:
	printf("Error reading line of Master File (%d fields found).\n", j);
	printf("Line being skipped:\n");
	printf("Product: %c Path: %s Name: %s Icon: %s Icon Desc: %s\n",
		ppgm->prod, ppgm->path, ppgm->name, ppgm->icon, ppgm->icon_dsc); 
	return 0;
}


/*
** init_master:		privfunc
**
** Description:
**      Function to initialize/open the INGRESNT.MAS file and read in
**	and establish the default global variables from the top of the file.
**
** Inputs:
**	INGRESNT.MAS masterfile found in the same local directory as
**	as the GENINGNT.EXE program is executing from.
**
** Outputs:
**	Updates the global arrays BASEAPP and PROD_NAME and sets up the
**	global strings ingres_wk, unc_ingres, cmp_ingres and cdrom_path.
**
** Returns:
**	Pointer to currently opened INGRESNT.MAS file.
*/
FILE *
init_master(long *cmp_total_size)
{
	FILE	*ifp;
	char 	*temp, mline[MAXSTRING] = "\0";
	int		i;

	strcpy(mline,"INGRESNT.MAS");
	if ((ifp = fopen(mline, "r")) == NULL)
	{				 /* Open master file for reading */
		printf("Error opening Ingres NT Master Input File: %s\n", mline);
		return NULL;
	}
	else
	{
		for ( i = 0; i < NUM_PRODUCTS; ++i )
			if (fgets(mline,MAXSTRING - 1, ifp) == NULL)
			{		/* Get product names */
				printf("Error reading default Ingres Product names.\n");
				return NULL;
			}
			else
			{
				if (mline[strlen(mline)-1] == '\n')
					mline[strlen(mline)-1] = '\0';
				temp = strrchr(mline,' ');
				if (temp <= 0)
				{ 
					printf("Error reading BASEAPP from Product Names.\n");
					return NULL;
				}
				++temp;
				strcpy(baseapp[i],temp);
				--temp;
				*temp = '\0';
				strcpy(prod_name[i],mline);
			}
		if (fgets(mline,MAXSTRING - 1, ifp) == NULL)
		{			/* Get output path */
			printf("Error reading default Ingres Directory output path.\n");
			return NULL;
		}
		else
		{
			if (mline[strlen(mline)-1] == '\n')
				mline[strlen(mline)-1] = '\0';
			if (mline[strlen(mline)-1] != '\\')
				strcat(mline,"\\");
			strcpy(ingres_wk,mline);
		}
		if (fgets(mline,MAXSTRING - 1, ifp) == NULL)
		{			/* Get uncompressed path */
			printf("Error reading Uncompressed Ingres Directory path.\n");
			return NULL;
		}
		else
		{
			if (mline[strlen(mline)-1] == '\n')
				mline[strlen(mline)-1] = '\0';
			strcpy(unc_ingres,mline);
		}
		if (fgets(mline,MAXSTRING - 1, ifp) == NULL)
		{			/* Get compressed path */
			printf("Error reading Compressed Ingres Directory path.\n");
			return NULL;
		}
		else
		{
			if (mline[strlen(mline)-1] == '\n')
				mline[strlen(mline)-1] = '\0';
			strcpy(cmp_ingres,mline);
		}
		if (fgets(mline,MAXSTRING - 1, ifp) == NULL) 
		{			/* Get cdrom path */
			printf("Error reading CDROM Ingres Directory path.\n");
			return NULL;
		}
		else
		{
			if (mline[strlen(mline)-1] == '\n')
				mline[strlen(mline)-1] = '\0';
			strcpy(cdrom_path,mline);
		}
	}
	while ((fgets(mline,MAXSTRING - 1, ifp) != NULL) && 
		strncmp(mline,"END",3) != 0 && filename_cnt < 100)
		if (mline[0] != ':')
		{
			if (mline[strlen(mline)-1] == '\n')
				mline[strlen(mline)-1] = '\0';
			strcpy(filename[filename_cnt++],mline);
			if (mline[strlen(mline)-1]=='_')
			  *cmp_total_size += filesize(cmp_ingres,"\\",mline,1);
			else
			  *cmp_total_size += filesize(cmp_ingres,"\\",mline,0);
		}
	if (filename_cnt == 100)
	{	
		printf("Error reading list of files to automatically copy"
			" to disk one for each product.\n");
		return NULL;
	}
	printf("Ingres Output Directory Path: %s\n"
		   "Ingres Uncompressed Path:     %s\n"
		   "Ingres Compressed Path:       %s\n"
		   "Ingres CDROM Path:            %s\n\n",
			ingres_wk, unc_ingres, cmp_ingres, cdrom_path);
	printf("%d files to copy to disk #1 of every product.\n\n", filename_cnt);
	return (ifp);
}


/*
** readmaster:		privfunc
**
** Description:
**      Function to parse the master file line by line calling
**	individual functions to create the various *.ISC entries
**	([products],[directories],[files], and [disks]) and BAT files
**	needed to create installation diskettes and a cdrom for
**	OpenIngres 1.1/04 for Windows NT.
**
** Inputs:
**	INGRESNT.MAS masterfile found in the same local directory as
**	as the GENINGNT.EXE program is executing from.
**
** Outputs:
**	Creates various files in *ingres_wk directory.
**
** Returns:
**	1 if parse successfull, 0 on error.
*/
int
readmaster()
{
	FILE	*fp, *cmp_fp, *cdrom_fp, *error_fp,	*iscp_fp = NULL, *dsk_fp = NULL;
	struct 	MASTERFILE pgm;
	char 	*mkdir, disk_prod = 'A', prev_path[50][MAXSTRING],
			mline[MAXSTRING] = "\0";
	int		i, span_file, new_prod = 1, disk_num = 1, disk_total_num = 0,
			read_error = 0, recompress = 0;
	long	unc_file_size, unc_total_size = 0, unc_prod_size = 0, 
			cmp_file_size, cmp_total_size = 0, cmp_prod_size = 0, 
			file_num = 0, dsk_cmp_size = 0;

/* initialize files */
	if ((fp			= init_master(&cmp_total_size)) == NULL)
		return 0;
	if ((cmp_fp		= init_file_wrt("COMPSTMP.BAT")) == NULL)
		return 0;
	if ((cdrom_fp	= init_file_wrt("CDROMBLD.BAT")) == NULL)
		return 0;
	if ((error_fp	= init_file_wrt("ERROR.TXT")) == NULL)
		return 0;
	dsk_fp = file_disk_init(cdrom_fp, disk_num, new_prod, 
						 disk_prod, &dsk_cmp_size);
	prod_disk[disk_prod - 'A'] = disk_total_num;
	iscp_fp = file_isc_init(disk_prod,disk_total_num);
	
/* Main Parsing Loop */
	while (fgets(mline,MAXSTRING - 1,fp) != NULL)
	{
		if (mline[0] != ':' && proc_mline(mline, &pgm) >= 4)
		{	/* Make sure at least 4 fields per line & not comment line */
			unc_total_size += (unc_file_size = 
				filesize(unc_ingres, pgm.path, pgm.dosname, 0));
			cmp_total_size += (cmp_file_size =
				filesize(cmp_ingres, pgm.path, pgm.dosname, 1));
			cmp_prod_size  +=  cmp_file_size;
			unc_prod_size  +=  unc_file_size;
			if (span_file = (cmp_file_size >= DISK_MAX_SIZE))
				span_file = (int)(cmp_file_size / DISK_MAX_SIZE) + 1;
			new_prod  = (pgm.prod != disk_prod); 
			if ( new_prod || ((dsk_cmp_size + cmp_file_size + 25000) 
				> DISK_MAX_SIZE))
			{ 									/* New Disk Number Setup */
				++disk_total_num;
				if (disk_total_num)			/* Print previous disk total */
					printf("Total Disk Size = %ld\n", dsk_cmp_size);
				dsk_cmp_size = 0;
				for ( i = 0; i < 50; i++ )
					prev_path[i][0] = '\0';
				if (new_prod)
				{								/* New product & disk Reinit */
					if (disk_num)
					{
						printf("Total Uncompressed size of Prod %c is = %ld\n",
							disk_prod, unc_prod_size);
						printf("Total Compressed size of Prod %c is   = %ld\n",
							disk_prod, cmp_prod_size);
					}
					unc_prod_size = 0;
					cmp_prod_size = 0;
					disk_prod = pgm.prod;
					disk_num = 1;
					prod_disk[disk_prod - 'A'] = disk_total_num;
					if (iscp_fp)
						fclose(iscp_fp);
					iscp_fp = file_isc_init(disk_prod,disk_total_num);
				}
				else
					++disk_num;		/* Same product: bump up disk number */ 
				if (span_file)		
				{					/* Make SPANCOMP.BAT entry */
					for ( i = 1; i < span_file; ++i )
					{
						if (dsk_fp)
							fclose(dsk_fp);
						dsk_fp = file_disk_init(cdrom_fp, disk_num, 0,
								 disk_prod, &dsk_cmp_size);
						ext_to_spannum((strcpy(mline,pgm.dosname)), i);
						file_disk_bat(dsk_fp, pgm.path, mline, 0, 0, 1, cdrom_fp);
						printf("Disk Span File = %s\t Disk #%d\n", mline, disk_num);
						++disk_num;
						++disk_total_num;
					}
					ext_to_spannum(pgm.dosname, span_file);
					cmp_file_size = filesize(cmp_ingres,pgm.path,pgm.dosname,0);
				}
				/* Initialize new DISK*.BAT file for each product */
				if (dsk_fp)
					fclose(dsk_fp);
				dsk_fp = file_disk_init(cdrom_fp, disk_num, new_prod, 
						 disk_prod, &dsk_cmp_size);
			} 				/* End new/full disk condition */
			++file_num; 	/* Counter of files read from INGRESNT.MAS */
			dsk_cmp_size   +=  cmp_file_size;
			/* Make COMPRESS.BAT entry for each file */
			recompress += file_comp_bat(cmp_fp, &pgm, span_file);
			if (cmp_file_size)
			{
				/* Make FILE*.ISC entry for each file */
				if (bld_file_isc(iscp_fp, &pgm, disk_total_num,	unc_file_size, span_file))
					break;
				/* Make DISK*.BAT entry for each file */
				mkdir = NULL;
				for ( i = 0; strcmp(prev_path[i], pgm.path) &&
					prev_path[i][0] && i < 50; i++ );
				if (i == 50)
				{
					printf("Error max number of directories reached.");
					return 0;
				}
				if (!prev_path[i][0])
				{ 								/* Save directory */
					mkdir = strcpy(prev_path[i], pgm.path);
					dsk_cmp_size += 512;		/* Count space on disk*/
				}
				file_disk_bat(dsk_fp, pgm.path, pgm.dosname, mkdir, 1, span_file, cdrom_fp);
				printf("File: %-17s DosName: %-12s  Cmp Size:%8ld Path: %-25s\t%s\n",
					   pgm.name, pgm.dosname, cmp_file_size, pgm.path, pgm.icon);
			}		/* end if cmp_file_size */
		  	else
		  	{
		  		fprintf(error_fp, "Compressed File: %s \tPath: %s\n",
		  			pgm.name, pgm.path);
				++read_error;
			}
			if (!file_exists(unc_ingres, pgm.path, pgm.name))
			{
				fprintf(error_fp, "Uncompressed File: %s \tPath: %s\n",
					pgm.name, pgm.path);
				++read_error;
			}
		} 			/* if not colon and 4 or more fields per line */
	}  				/* End While Loop */

	/* Close all open files */
	fclose(fp);		
	fclose(iscp_fp);
	fclose(cmp_fp);
	fclose(cdrom_fp);
	fclose(dsk_fp);
	fclose(error_fp);

	/* Build DISKS*.ISC files and totals for last product. */
	prod_disk[NUM_PRODUCTS] = ++disk_total_num;
	printf("Total Disk Size = %ld\n", dsk_cmp_size);
	printf("Total Uncompressed size of Product %c is = %ld\n",
		disk_prod, unc_prod_size);
	printf("Total Compressed size of Product %c is   = %ld\n\n",
		disk_prod, cmp_prod_size);

	/* Totals for Ingres */
	printf("Total # of Ingres Files              = %ld\n",file_num);
	printf("Total # of Files needing Compression = %d\n",recompress);
	printf("Total # of Read Errors               = %d\n",read_error);
	printf("Total Uncompressed size              = %ld\n",unc_total_size);
	printf("Total Compressed size                = %ld\n",cmp_total_size);
	printf("Total # of disks needed              = %d\n\n",disk_total_num);
	if (!read_error)
		delete_file("ERROR.TXT");
	return 1;
}


/*
**	Main Loop of program GENINGNT.
*/
main() 
{
	init_dir_arrays();	     /* Initialize Directory Arrays */
	if (readmaster())            /* Parse Master File building output */
	{				 				
	    update_prod_isc();	     /* Update the 6 individual *.ISC files */
	    bld_cdromfin_bat();      /* Create CDROMFIN.BAT from CDROMBLD.BAT */
	    bld_compress_bat();	     /* Remove duplicate compress statements */ 
	    bld_diskauto_bat(prod_disk[NUM_PRODUCTS]);							 	     /* Build DISKAUTO.BAT file */
	    return 0;                /* Terminating program successfully */	
	
	}
	else return 1;		     /* Terminating program with an error */
}		
