/**
 ** acstart.c
 **
 ** Driver program for the achilles tool. Converted from UNIX acstart.sh.
 **
 **
 ** History:
 **	06-Feb-96 (bocpa01)
 **		Created.
 **
 **/

#include <process.h>
#include "acstart.h"

enum { KWD_ACTUAL, KWD_DEBUG, KWD_INTERACTIVE, KWD_OUTPUT, KWD_REPEAT, KWD_USERNAME };
enum {MSGLINELENGTH = 60};

char *pszKeyword[] = {"actual", "debug", "interactive", "output", "repeat", "username"}; 

VOID OutputMessage(enum MSGTYPE msgType, PCHAR pszMessage) {
	
	char	*pch = pszMessage;
	int		 nNumberOfBlanks, len;

	printf("|------------------------------------------------------------|\n");
	if (msgType == MSGTYPE_ERROR)
		printf("|                      E  R  R  O  R                         |\n");
	else
		printf("|                    W  A  R  N  I  N  G                      |\n");
	printf("|------------------------------------------------------------|\n");
	if ((len = strlen(pszMessage)) > MSGLINELENGTH) {
		char *pchCurrent, *pchMessage;
		int	 i;

		pch = calloc(1, strlen(pszMessage) + len / MSGLINELENGTH + 1);
		for (i = 0, pchCurrent = pch, pchMessage = pszMessage; *pchMessage; 
			 pchCurrent++, pchMessage++, i++) {
			if (i >= MSGLINELENGTH - 1) {
				*pchCurrent++ = '\n';
				i = 0;
			}
			*pchCurrent = *pchMessage;
		}
			
	}
	pch = strtok(pch, "\n");
	while (pch) {
		nNumberOfBlanks = (MSGLINELENGTH - strlen(pch)) / 2;
		printf("|%*s%s%*s|\n", nNumberOfBlanks, "", pch, 
				MSGLINELENGTH - strlen(pch) - nNumberOfBlanks, "");
		pch = strtok(NULL, "\n");
	}
	printf("|------------------------------------------------------------|\n");
}

VOID Message(enum MSGTYPE msgType, DWORD stringID, PCHAR pszArg) {
	
	char	chString[STRINGLENGTH];

	GetString(stringID, chString, sizeof(chString));
	if (pszArg) {
		char chMessage[STRINGLENGTH];

		sprintf(chMessage, chString, pszArg);
		strcpy(chString, chMessage);
	}
	OutputMessage(msgType, chString);
}


BOOL IsKeywordOnly(char *pszKeyword, char *pszArg) {

	int	  i = strlen(pszKeyword) + 2;
	char *pch = malloc(i);

	sprintf(pch, "-%s", pszKeyword);
	for (i -= 2; i && strcmp(pch, pszArg); i--)
		*(pch + i) = '\0';
	free(pch);
	return i;
}

char *IsKeyword(char *pszKeyword, char *pszArg) {

	int	  i = strlen(pszKeyword) + 3;
	char *pch = malloc(i);

	sprintf(pch, "-%s=", pszKeyword);	
	for (i--; i > 2 && strncmp(pch, pszArg, i); i--) 
		sprintf(pch, "-%.*s=", (i - 3), pszKeyword);
	free(pch);
	if (i > 2)
		return pszArg + i;
	return NULL;
}

int main(int argc, char *argv[]) {

	char 	*pszArgValue;
	UINT	 uID;
	DWORD	 dwProcessId;

	GetEnvironmentVariable("USER", user_val, sizeof(user_val));

	if (argc <= 1) {
		printf("Usage: acstart <test_case> <data_base_spec>\n\n");
		printf("\t-o[utput] path    Change output directory where all achilles\n");
		printf("\t                  files are written.\n\n");
		printf("\t-r[epeat] [n]     Change repeat tests parameter in config or\n");
		printf("\t                  template file. Defaults to 0. A value of\n");
		printf("\t                  zero means to repeat the test endlessly.\n\n");
		printf("\t-u[sername] user  Change effective user parameter in config\n");
		printf("\t                  or template file.\n\n");
		printf("\t-d[ebug]          Do all sanity checking and file creation\n");
		printf("\t                  without actually starting up achilles.\n\n");
		printf("\t-i[nteractive]    Start up achilles in this process rather\n");
		printf("\t                  than as a detached process.\n\n");
		printf("\t-a[actual]        Actual time for achilles to run if\n");
		printf("\t                  desired (note: overrides -r[epeat] option)\n");
		return 0;
	}

/*#############################################################################
#
# Check local Ingres environment
#
#############################################################################*/

	if ((uID = InitEnvironment())) {
		Message(MSGTYPE_ERROR, uID, NULL);
		return 1;
	}

/*#############################################################################
#
#	Break out command line arguments.
#
#############################################################################*/

	for (i = 1; i < argc; i++) {
		if (*argv[i] == '-') { 
			if (need_output) {
				Message(MSGTYPE_ERROR, IDS_BADOUTPUTFLAG, NULL);
				return 1;
			}
			if (need_user) {
				Message(MSGTYPE_ERROR, IDS_BADUSERNAMEFLAG, NULL);
				return 1;
			}
			if (need_repeat) {
				repeat_val = 9999;
				repeat_sw = TRUE;
				need_repeat = FALSE;
			}
			if (need_time) {
				time_val=9999;
				time_sw = TRUE;
				need_time = FALSE;
			} 
		} else {
			if (need_time) {
				if ((time_val = String2Integer(argv[i])) < 0) {
					Message(MSGTYPE_ERROR, IDS_BADTIMEVALUE, NULL);
					return 1;
				}
				time_sw = TRUE; 
				need_time = FALSE;
			} else if (need_repeat) {
				if ((repeat_val = String2Integer(argv[i])) < 0) {
					Message(MSGTYPE_ERROR, IDS_BADREPEATVALUE, NULL);
					return 1;
				}
				repeat_sw = TRUE; 
				need_repeat = FALSE;
			} else if (need_output) {
				strcpy(output_val, argv[i]); 
				output_sw = TRUE; 
				need_output = FALSE;
			} else if (need_user) {
				strcpy(user_val, argv[i]);   
				user_sw = TRUE;   
				need_user = FALSE;
			} else if (need_cfg) {
				need_cfg = FALSE; 
				strcpy(test_case, argv[i]);
			} else if (need_db) {
				need_db = FALSE;  
				strcpy(dbspec, argv[i]);
			}
		} 
		if (IsKeywordOnly(pszKeyword[KWD_ACTUAL], argv[i]))
			need_time = TRUE;
		else if ((pszArgValue = IsKeyword(pszKeyword[KWD_ACTUAL], argv[i]))) {
			time_sw = TRUE;       
			if ((time_val = String2Integer(pszArgValue)) < 0) {
				Message(MSGTYPE_ERROR, IDS_BADTIMEVALUE, NULL);
				return 1;
			}
		} else if (IsKeywordOnly(pszKeyword[KWD_DEBUG], argv[i]))
			debug_sw = TRUE;
		else if (IsKeywordOnly(pszKeyword[KWD_INTERACTIVE], argv[i]))
			interactive_sw = TRUE;
		else if (IsKeywordOnly(pszKeyword[KWD_REPEAT], argv[i]))
			need_repeat = TRUE;
		else if ((pszArgValue = IsKeyword(pszKeyword[KWD_REPEAT], argv[i]))) {
			repeat_sw = TRUE;
			if ((repeat_val = String2Integer(pszArgValue)) < 0) {
				Message(MSGTYPE_ERROR, IDS_BADREPEATVALUE, NULL);
				return 1;
			}
		} else if (IsKeywordOnly(pszKeyword[KWD_OUTPUT], argv[i]))
			need_output = TRUE;
		else if ((pszArgValue = IsKeyword(pszKeyword[KWD_OUTPUT], argv[i]))) {
			output_sw = TRUE;
			strcpy(output_val, pszArgValue);
		} else if (IsKeywordOnly(pszKeyword[KWD_USERNAME], argv[i]))
			need_user = TRUE;
		else if ((pszArgValue = IsKeyword(pszKeyword[KWD_USERNAME], argv[i]))) {
			user_sw = TRUE;
			strcpy(user_val, pszArgValue);
			printf("switch %d\n", user_sw);
			printf("user val is %s\n", user_val);
		} else if (*argv[i] == '-') {
			Message(MSGTYPE_ERROR, IDS_BADFLAG, argv[i]);
			return 1;
		}
	}

	if (need_time) {
		Message(MSGTYPE_ERROR, IDS_BADTIMEFLAG, NULL);
		return 1;
	}
	if (need_output) {
		Message(MSGTYPE_ERROR, IDS_BADOUTPUTFLAG, NULL);
		return 1;
	}
	if (need_user) {
		Message(MSGTYPE_ERROR, IDS_BADUSERNAMEFLAG, NULL);
		return 1;
	}
	if (need_cfg) {
		Message(MSGTYPE_ERROR, IDS_NOTESTGIVEN, NULL);
		return 1;
	}
	if (need_db) {
		Message(MSGTYPE_ERROR, IDS_NODATABASEGIVEN, NULL);
		return 1;
	}

	if (!ProcessParameters())
		return 1;

	printf("|------------------------------------------------------------|\n");
	printf("|                P  A  R  A  M  E  T  E  R  S                |\n");
	printf("|------------------------------------------------------------|\n");
	if (time_sw)
		printf("| time value = %d\n", time_val);
	if (debug_sw)
		printf("| debug is on\n");
	if (interactive_sw)
		printf("| interactive is on\n");
	if (repeat_sw)
		printf("| repeat value = %d\n", repeat_val);
	if (output_sw)
		printf("| output path  = %s\n", output_val);
	if (user_sw)
		printf("| username     = %s\n", user_val);
	printf("| test case    = %s\n", test_case);
	printf("| config file  = %s\n", cfg_file);
	printf("| rpt file     = %s\n", rpt_file);
	printf("| log file     = %s\n", log_file);
	printf("| db spec      = %s\n", dbspec);
	printf("|------------------------------------------------------------|\n");

	if (!PrepareRun())
		return 1;

	if (debug_sw) {
		printf("|------------------------------------------------------------|\n");
		printf("|        Here's the command line we would have used:         |\n");
		printf("|------------------------------------------------------------|\n");
		printf("achilles -l -s -d %s\n", dbspec);
        printf("         -f %s\n" ,cfg_file);
		printf("         -o %s\n", output_val);
        printf("          > %s\n", log_file);
		printf("|------------------------------------------------------------|\n");
	} else if (interactive_sw) {

		if (!StartAchilles(TRUE)) {

/*			CHAR chMessage[STRINGLENGTH], chTimeValue[64];

			itoa(time_val, chTimeValue, 10);
			execlp("achilles",  "-l", "-s", "-d", dbspec, "-f", cfg_file,
  				  "-o", output_val, "-a", chTimeValue, NULL);
			sprintf(chMessage, "achilles -l -s \n  -d %s \n  -f %s \n  -o %s \n  -a %s", 
				dbspec, cfg_file, output_val, chTimeValue);	*/
			printf("|------------------------------------------------------------|\n");
			printf("|                      E  R  R  O  R                         |\n");
			printf("|------------------------------------------------------------|\n");
			printf("| Can not exec achilles -l -s \n");
			printf("|     -d %s\n", dbspec);
			printf("|     -f %s\n", cfg_file);
			printf("|     -o %s\n", output_val);
			printf("|     -a %d\n", time_val);
			printf("|------------------------------------------------------------|\n");
			return 1;		/* this will happen only if execl is unsuccessfull */
		}
	} else {
		if ((dwProcessId = StartAchilles(FALSE)) == 0) 
			return 1;

		/* create report file for the user and let him know where it is */

		printf("	   please enter the file - %s\n", cfg_file);
	    printf("       when generating a Report File with acreport\n");
		printf("ACHILLES  %d > %s", dwProcessId, rpt_file);
	}
	return 0;
}