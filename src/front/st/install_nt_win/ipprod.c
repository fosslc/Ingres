# include	<compat.h>
# include	<lo.h>
# include	<si.h>
# include	<st.h>
# include	<tm.h>
/*
**  Name: ipprod -- Creates a batch file, called ipsetup.bat located in 
**			%II_SYSTEM%\ingres\bin, that contains calls to 
**			the installation setup batch files.
**
**  History:
**	19-jul-95 (reijo01)
**		Created.
**	08-aug-95 (reijo01)
**		Added logic to change the drive and working directory before
**		call the iisu batch files. This is because of problems in 
** 		the calling programs with setting the current working directory.
**	09-aug-1999 (mcgem01)
**		Changed nat to i4.
*/
static	LOCATION 	location;
void 
main(int argc, char **argv)
{
	int		i;
	char		buf[MAX_LOC];
	FILE		*fp;
	STATUS		status;
	char		*callstmt;
	i4 		casef;
	char		timestr[512];
	SYSTIME		timeval;
	char		path[512];
	char		drive[2];
	char		current_path[512];

	if (argc < 3)
	{
		SIfprintf(stdout,"Not enough parameters\n");
		PCexit(FAIL);
	}

	/*
	**
	*/
	if (!STbcompare(*++argv, STlength("Net"),
			 "Net", STlength("Net"), casef ))

		callstmt = "@call %II_SYSTEM%\\ingres\\utility\\iisunet";

	else if (!STbcompare(*argv, STlength("Dbms"),
			       "Dbms", STlength("Dbms"), casef ))

		callstmt = "@call %II_SYSTEM%\\ingres\\utility\\iisudbms";

	else if (!STbcompare(*argv, STlength("Star"), 
				"Star", STlength("Star"), casef ))

		callstmt = "@call %II_SYSTEM%\\ingres\\utility\\iisustar";

	else if (!STbcompare(*argv, STlength("Vision"),
				 "Vision", STlength("Vision"), casef ))

		callstmt = "@call %II_SYSTEM%\\ingres\\utility\\iisuabf";

	else if (!STbcompare(*argv, STlength("ESQL"),
				 "ESQL", STlength("ESQL"), casef ))

		callstmt = "@call %II_SYSTEM%\\ingres\\utility\\iisutm";

	else 
		PCexit(FAIL);

	/*
	** 	Set location and file name for the subsequent open.
	*/
	strcpy(path,*++argv);
	strcpy(current_path,*argv+2);
	strcat(path,"\\ingres\\bin\\ipsetup.bat");
	LOfroms(PATH & FILENAME, path, &location);

	
	/*
	** 	Open ipsetup.bat so we can append another line.
	*/
	if ((status = SIopen(&location, "a", &fp)) != OK)
	{
		printf("error opening file\n");
	}

	/*
	** 	Append another line that will call the setup bat files.
	*/
	TMnow(&timeval);
	TMstr(&timeval,&timestr);
	drive[0] = path[0];
	drive[1] = '\0';

	/*
	** Write a statement in the batch file that will set
	** the current drive to the drive passed to us.
	*/
	SIfprintf(fp,"@%s:\n",drive);

	/*
	** Write a statement in the batch file that will set
	** the current path to the path passed to us.
	*/
	SIfprintf(fp,"@cd %s\n",current_path);

	/* 
	** Write a Remark as a primitive log.
	*/
	SIfprintf(fp,"@REM %s installed at %s\n",*argv,timestr);
	
	/*
	** Now write the call statement that will call the appropiate
	** iisu batch file.
	*/
	SIfprintf(fp,"%s\n",callstmt);
}
