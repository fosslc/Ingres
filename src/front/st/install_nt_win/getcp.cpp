/*
**  Copyright (c) 2008 Ingres Corporation
*/

/*
**	Convert the current Windows OS-specific current code page
**	to a valid Ingres charset name.
**
**	Copied from code in dbmsnetmm.c with additions.
**
**  History:
**	21-Nov-2008 (whiro01)
**	    Created.
*/

#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <string.h>


// Mapping from Windows code page to Ingres II_CHARSET values
// Table taken from "dbmsnetmm.c" in the Installer code,
// with additions.
static struct _charmap {
	unsigned codepage;
	char *ingcodepage;
} charmap[] =
{
	{	437,		"IBMPC437"	},
	{	720,		"DOSASMO"	},
	{	737,		"PC737"		},
	{	850,		"IBMPC850"	},
	{	852,		"SLAV852"	},
	{	855,		"ALT"		},
	{	857,		"PC857"		},
	{	866,		"IBMPC866"	},
	{	869,		"GREEK"		},
	{	874,		"WTHAI"		},
	{	932,		"SHIFTJIS"	},
	{	936,		"CSGBK"		},
	{	949,		"KOREAN"	},
	{	950,		"CHTBIG5"	},
	{	1200,		"WIN1252"	},
	{	1250,		"WIN1250"	},
	{	1251,		"CW"		},
	{	1252,		"WIN1252"	},
	{	1253,		"WIN1253"	},
	{	1254,		"ISO88599"	},
	{	1255,		"WHEBREW"	},
	{	1256,		"WARABIC"	},
	{	1257,		"WIN1252"	},
	{	1258,		"WIN1252"	},
	{	28605,		"THAI"		},
	{	28598,		"PCHEBREW"	},
	{	28599,		"ISO88599"	},
	{	28597,		"ISO88597"	},
	{	28595,		"ISO88595"	},
	{	28592,		"ISO88592"	},
	{	28591,		"ISO88591"	},
	{	28605,		"IS885915"	},
	{	38598,		"HEBREW"	},
	{	28596,		"ARABIC"	},
	{	20866,		"KOI8"		},
	{	65001,		"UTF8"		}
};

static void ErrorOut()
{
	DWORD err;
	LPVOID lpMsgBuf;

	err = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	fprintf(stderr, "Error %d: %s\n", err, lpMsgBuf);
	LocalFree(lpMsgBuf);
	exit(err);
}

int main(int argc, char **argv)
{
	CPINFOEX cpinfo;
	char name[MAX_PATH];
	char *env;

	// To start with, look for "II_CHARSET=" in the environment and do a reverse
	// lookup in the above table
	if ((env = getenv("II_CHARSET")) != NULL)
	{
		for (int i = 0; i < sizeof(charmap) / sizeof(charmap[0]); i++)
		{
			if (_stricmp(env, charmap[i].ingcodepage) == 0)
			{
				cpinfo.CodePage = charmap[i].codepage;
				strcpy(name, env);
				break;
			}
		}
	}
	else
	{
		if (GetCPInfoEx(CP_ACP, 0, &cpinfo) == 0)
			ErrorOut();

		if (WideCharToMultiByte(CP_UTF8, 0, cpinfo.CodePageName, -1, name, sizeof(name), NULL, NULL) == 0)
			ErrorOut();
	}

	// Look up the code page and try to match to a valid Ingres choice
	// HOT: don't know what to do if it doesn't match any
	for (int i = 0; i < sizeof(charmap) / sizeof(charmap[0]); i++)
	{
		if (cpinfo.CodePage == charmap[i].codepage)
		{
			printf("%s\n", charmap[i].ingcodepage);
			return 0;
		}
	}

	fprintf(stderr, "Could not map Windows code page %d (%s)!\n", cpinfo.CodePage, name);
	printf("%d\n", cpinfo.CodePage);
	return 1;
}
