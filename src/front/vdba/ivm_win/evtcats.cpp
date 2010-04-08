/***************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Source   : evtcats.cpp
**
**
**  Project  : Ingres II / Visual Manager
**  Author   : noifr01
**
**  Purpose  : predefined categories and misc associated functions
**
**  History:
**	03-mar-2000 (mcgem01)
**	    Corrected miscellaneous typos in the messages, completed
**	    message category descriptions.  Removed obsolete categories.
**	    Removed front-end message categories as these will not appear
**	    in the error log.
**	29-May-2000 (noifr01)
**	    bug 99242 cleanup/check for DBCS compliance
**	18-oct-2000 (somsa01)
**	    In getMsgStringClassAndId(), we were not gathering info for
**	    informational messages ('I_').
**  18-Jan-2001 (noifr01)
**    (SIR 103727) added the JD category for JDBC messages
**  14-Feb-2001 (noifr01)
**    (BUG 103978) taking into account the 2 high bytes of the 4 bytes parsed
**     errcode for CL subcategories was redundant with the lclasssubmask
**    struvcture member, the corresponding value being counted twice,
**    resulting in the bug
**  14-Fev-2001 (noifr01)
**     (bug 104001) cleaned up categories, the way they are managed for some
**     specific ones (such as CL sub-categories with the same mask), the
**     E_HANDY_MASK submask, etc... added a couple of missing categories into
**     a generic "Misc Messages" category and applied a fix so that messages
**     with multiple masks can appear in a common category (this is the first
**     time it is used, for the "Misc Messages" category)
**  20-Nov-2002 (noifr01)
**    (bug 109143) management of gateway messages
**  04-Mar(2003 (noifr01)
**    (sir 109773) manage also front-end messages (for parsing correctly the
**    messages.txt file, and allowing future display of the explanation of
**    front-end messages)
**  06-Mar-2003 (noifr01)
**    (bug 109772) find CL message facility from range of message number
**    rather than from the facility substring, which is missing for a few
**    messages
**  10-Mar-2003 (noifr01)
**    (sir 109773) temporarily don't propose front-end categories in the
**    window that displays the message explanation, until the messages.txt
**    file has been cleaned up at least because of the ZZ messages, and until 
**    a decision is taken about which front end categories will be documented
**    in that file, and in IVM
****************************************************************************/

#include "stdafx.h"
#include "evtcats.h"
#include "ivm.h"
#include "getinst.h"

extern "C"
{
# include <compat.h>
# include <cm.h>
}
// ingres categories gotten from Factab[], in common!hdr!hdr!erfac.h
// CL sub-categories gotten from ercl.h and compat.h

INGERRCLASS IngErClasses[] =
{
	// interface only categories
// next line left for the future, but not used for now
	{ FALSE, "",  CLASS_INTERFACE_MISC	,""  ,          0,  0  ,TRUE, CLASS_INTERFACE_MISC, "Misc Messages"},

// manually added class for messages with noE_XXnnnn code
	{ FALSE, "ZZ",CLASS_OTHERS				,""  ,	        0,1    ,TRUE , CLASS_SAME,				"<Messages with no Error Code>"},
  // categories where interface is mapped to exactly one ingres category
	{ FALSE, "US",0L * OFFSET_CAT_MASK		,""  ,          0,65535,TRUE , CLASS_SAME,              "User Facility"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"BT", 0x00000100,255  ,TRUE , CLASS_SAME,              "BT (Bit Manipulation)"},
// 2 next lines for the example for grouping categories
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"CH", 0x00000200,255  ,FALSE, CLASS_INTERFACE_MISC,  "CH_MASK	CL"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"CK", 0x00000300,255  ,TRUE , CLASS_SAME,				"CK (Checkpoint)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"CP", 0x00000400,255  ,TRUE , CLASS_SAME,              "CP (Switch Protections)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"CV", 0x00000500,255  ,TRUE , CLASS_SAME,              "CV (Conversion)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"DI", 0x00000600,255  ,TRUE , CLASS_SAME,              "DI (Database I/O)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"DS", 0x00000700,255  ,TRUE , CLASS_SAME,              "DS (Data Structures)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"DY", 0x00000800,255  ,TRUE , CLASS_SAME,              "DY (Dynamic Linking) and DL"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"ER", 0x00000900,255  ,TRUE , CLASS_SAME,              "ER (Error Messages)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"EX", 0x00000A00,255  ,TRUE , CLASS_SAME,              "EX (Exception Handling)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"GV", 0x00000B00,255  ,TRUE , CLASS_SAME,              "GV (Global Variables)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"ID", 0x00000C00,255  ,TRUE , CLASS_SAME,              "ID (Object IDs)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"II", 0x00000D00,255  ,FALSE, CLASS_INTERFACE_MISC,    "II"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"IN", 0x00000E00,255  ,FALSE, CLASS_INTERFACE_MISC,    "IN"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"LG", 0x00000F00,255  ,TRUE , CLASS_SAME,              "LG (Logging)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"LK", 0x00001000,255  ,TRUE , CLASS_SAME,              "LK (Locking)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"LO", 0x00001100,255  ,TRUE , CLASS_SAME,              "LO (Locations)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"ME", 0x00001200,255  ,TRUE , CLASS_SAME,              "ME (Memory Allocation)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"MH", 0x00001300,255  ,TRUE , CLASS_SAME,              "MH (Math) and FP"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"NM", 0x00001400,255  ,TRUE , CLASS_SAME,              "NM (Logical Symbols)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"OL", 0x00001500,255  ,TRUE , CLASS_SAME,              "OL (4 GL Support)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"PC", 0x00001600,255  ,TRUE , CLASS_SAME,              "PC (Process Control)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"PE", 0x00001700,255  ,TRUE , CLASS_SAME,              "PE (Permissions)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"QU", 0x00001800,255  ,TRUE , CLASS_SAME,              "QU (Queue Manipulation)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"SI", 0x00001900,255  ,TRUE , CLASS_SAME,              "SI (Stream I/O)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"SM", 0x00001A00,255  ,FALSE, CLASS_INTERFACE_MISC,    "SM"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"SR", 0x00001B00,255  ,TRUE , CLASS_SAME,              "SR (Sorting)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"ST", 0x00001C00,255  ,TRUE , CLASS_SAME,              "ST (String Handling)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"SX", 0x00001D00,255  ,TRUE , CLASS_SAME,              "SX (Security Extensions)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"TE", 0x00001E00,255  ,TRUE , CLASS_SAME,              "TE (Terminal Driver)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"TM", 0x00001F00,255  ,TRUE , CLASS_SAME,              "TM (Timing Services)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"TO", 0x00002000,255  ,FALSE, CLASS_INTERFACE_MISC,    "TO"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"TR", 0x00002100,255  ,TRUE , CLASS_SAME,              "TR (Tracing)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"UT", 0x00002200,255  ,TRUE , CLASS_SAME,              "UT (Utility Invocation)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"NT", 0x00002300,255  ,FALSE, CLASS_INTERFACE_MISC,    "NT"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"JF", 0x00002400,255  ,TRUE , CLASS_SAME,              "JF (Journal Files)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"CS", 0x00002500,255  ,TRUE , CLASS_SAME,              "CS (Central System)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"CI", 0x00002600,255  ,TRUE , CLASS_SAME,              "CI (Licensing)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"GC", 0x00002700,255  ,TRUE , CLASS_SAME,              "GC (GCA CL)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"SD", 0x00002800,255  ,TRUE , CLASS_SAME,              "SD (Smart Disc)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"SL", 0x00002900,255  ,TRUE , CLASS_SAME,              "SL (Security Labels)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"CM", 0x00002A00,255  ,TRUE , CLASS_SAME,              "CM (Character Manipulation)"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"SA", 0x00002B00,255  ,TRUE , CLASS_SAME,              "SA (Security Auditing)"},
	{ TRUE,  "CL",1L * OFFSET_CAT_MASK		,"ZZ", 0x00007800,255  ,TRUE , CLASS_SAME,              "ZZ1"},
	{ TRUE,  "CL",1L * OFFSET_CAT_MASK		,"ZZ", 0x00007900,255  ,TRUE , CLASS_SAME,              "ZZ2"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"BS", 0x0000FE00,255  ,FALSE , CLASS_INTERFACE_MISC,   "BS"},
	{ FALSE, "CL",1L * OFFSET_CAT_MASK		,"??", 0x0000FF00,255  ,FALSE , CLASS_INTERFACE_MISC,   "HANDY MASK"},
	{ FALSE, "AD",2L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "ADF (Abstract Data Type Facility)"},
	{ FALSE, "DM",3L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "DMF (Data Manipulation Facility)"},
	{ FALSE, "OP",4L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "OPF (Optimizer Facility)"},
	{ FALSE, "PS",5L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "PSF (Parser Facility)"},
	{ FALSE, "QE",6L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "QEF (Query Execution Facility)"},
	{ FALSE, "QS",7L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "QSF (Query Storage Facility)"},
	{ FALSE, "RD",8L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "RDF (Relation Description Facility)"},
	{ FALSE, "SC",9L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "SCF (System Control Facility)"},
	{ FALSE, "UL",10L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "ULF (DBMS Utility Functions)"},
	{ FALSE, "DU",11L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "DUF (Database Utility Facility)"},
	{ FALSE, "GC",12L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "GCF (General Communication Facility)"},
	{ FALSE, "RQ",13L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "RQF (Remote Query Facility)"},
	{ FALSE, "TP",14L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "TPF (Transaction Process Facility)"},
	{ FALSE, "GW",15L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "GWF (Gateway Facility)"},
	{ FALSE, "SX",16L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "SXF (Security Extensions Facility)"},
#ifdef INCLUDE_ALL_FRONTEND_MESSAGES
	{ TRUE,  "AF",32L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "AF"},
	{ TRUE,  "FM",33L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "FM"},
	{ TRUE,  "UI",34L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "UI"},
	{ TRUE,  "AB",40L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "AB"},
	{ TRUE,  "AR",41L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "Archiver Facility"},
	{ TRUE,  "AS",42L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "AS"},
	{ TRUE,  "CA",44L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "CA"},
	{ TRUE,  "AM",45L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "AM"},
	{ TRUE,  "OR",47L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "OR"},
	{ TRUE,  "IB",48L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "IB (ABF Image Build)"},
	{ TRUE,  "IT",49L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "IT"},
	{ TRUE,  "O2",52L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "O2"},
	{ TRUE,  "OS",53L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "OS"},
	{ TRUE,  "FC",54L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "FC"},
	{ TRUE,  "FD",55L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "FD"},
	{ TRUE,  "FR",57L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "FR"},
	{ TRUE,  "TB",58L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "TB"},
	{ TRUE,  "FV",60L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "FV"},
	{ TRUE,  "FT",61L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "FT"},
	{ TRUE,  "GT",62L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "GT"},
	{ TRUE,  "LS",63L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "LS"},
	{ TRUE,  "TD",66L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "TD"},
	{ TRUE,  "VT",67L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "VT"},
	{ TRUE,  "GP",69L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "GP"},
	{ TRUE,  "GR",72L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "GR"},
	{ TRUE,  "RG",75L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "RG"},
	{ TRUE,  "VG",76L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "VG"},
	{ TRUE,  "CD",78L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "CD"},
	{ TRUE,  "CF",79L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "CF"},
	{ TRUE,  "DE",80L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "DE"},
	{ TRUE,  "IC",83L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "IC"},
	{ TRUE,  "IM",84L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "IM"},
	{ TRUE,  "PF",87L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "PF"},
	{ TRUE,  "TU",88L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "TU"},
	{ TRUE,  "UD",89L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "UD"},
	{ TRUE,  "XF",90L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "XF"},
	{ TRUE,  "QF",91L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "QF"},
	{ TRUE,  "RC",92L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "RC"},
	{ TRUE,  "RF",93L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "RF"},
	{ TRUE,  "RW",94L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "RW"},
	{ TRUE,  "SR",96L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "SR"},
	{ TRUE,  "MF",97L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "MF"},
	{ TRUE,  "MO",98L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "MO"},
	{ TRUE,  "QR",99L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE , CLASS_SAME,              "QR"},
	{ TRUE,  "DF",100L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "DF (Data Dictionary Facility)"},
	{ TRUE,  "WS",101L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "WS (Web Server Facility)"},
	{ TRUE,  "QG",134L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "QG"},
	{ TRUE,  "OO",135L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "OO"},
	{ TRUE,  "UF",136L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "UF"	},
	{ TRUE,  "UG",137L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "UG"	},
	{ TRUE,  "VF",139L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "VF"	},
	{ TRUE,  "EQ",140L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "EQ"	},
	{ TRUE,  "E0",141L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "E0"	},
	{ TRUE,  "E1",142L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "E1"	},
	{ TRUE,  "E2",143L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "E2"	},
	{ TRUE,  "E3",144L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "E3"	},
	{ TRUE,  "E4",145L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "E4"	},
	{ TRUE,  "E5",146L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "E5"	},
	{ TRUE,  "E6",147L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "E6"	},
	{ TRUE,  "LQ",151L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "LQ"	},
	{ TRUE,  "LC",152L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "LC"	},
	{ TRUE,  "C6",160L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "C6"	},
	{ TRUE,  "FE",161L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "FE"	},
	{ TRUE,  "DC",162L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "DC"	},
	{ TRUE,  "VQ",169L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "VQ"	},
	{ TRUE,  "CG",170L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "CG"	},
	{ TRUE,  "FG",173L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "FG"	},
	{ TRUE,  "AI",174L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "AI"	},
	{ TRUE,  "A6",175L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "A6"	},
	{ TRUE,  "TL",178L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "TL"	},
	{ TRUE,  "M1",179L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "M1"	},
	{ TRUE,  "MC",180L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "MC"	},
	{ TRUE,  "FI",181L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "FI"	},
	{ TRUE,  "CU",182L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "CU"	},
	{ TRUE,  "AC",183L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "AC"	},
	{ TRUE,  "CO",184L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "CO"	},
	{ TRUE,  "MT",185L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "MT"	},
	{ TRUE,  "OM",189L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "OM (OpenROAD)"	},
	{ TRUE,  "DO",191L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "DO (OpenROAD)"	},
	{ TRUE,  "PW",192L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "PW (OpenROAD)"	},
	{ TRUE,  "XO",193L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "XO (OpenROAD)"	},
	{ TRUE,  "WN",194L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "WN (OpenROAD)"	},
	{ TRUE,  "WE",195L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "WE (OpenROAD)"	},
	{ TRUE,  "SY",197L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "SY (OpenROAD)"	},
	{ TRUE,  "SE",199L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "SE (OpenROAD)"	},
	{ TRUE,  "EO",200L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "EO (OpenROAD)"	},
#endif // INCLUDE_ALL_FRONTEND_MESSAGES		
	{ TRUE,  "AP",201L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "AP"	},	/* API */
#ifdef INCLUDE_ALL_FRONTEND_MESSAGES
	{ TRUE,  "DD",205L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "DD"	},
	{ TRUE,  "WR",206L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "WR (OpenROAD)"	},
	{ TRUE,  "WM",207L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "WM (OpenROAD)"	},
	{ TRUE,  "WF",209L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "WF (OpenROAD)"	},
	{ TRUE,  "WT",210L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "WT (OpenROAD)"	},
	{ TRUE,  "W4",211L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "W4 (OpenROAD)"	},
#endif // INCLUDE_ALL_FRONTEND_MESSAGES		
	{ TRUE,  "GL",213L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "GL"	},
#ifdef INCLUDE_ALL_FRONTEND_MESSAGES
	{ TRUE,  "CX",215L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "CX"	},
	{ TRUE,  "G4",216L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "G4"	},
	{ TRUE,  "WB",225L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "WB"	},
	{ TRUE,  "WC",220L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "WC (OpenROAD)"	},
	{ TRUE,  "DG",229L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "DG"	},
	{ TRUE,  "GU",202L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "GU (Gateway Utility)"	},	/* Gateway utility */
	{ TRUE,  "GX",203L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "GX (Gateway Error)"	},	/* Gateway error */
	{ TRUE,  "ST",212L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "ST "	},
	{ TRUE,  "IE",217L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "IE"	},
	{ TRUE,  "WA",226L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "WA"	},
	{ TRUE,  "RP",230L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "Replicator Common "	},	/* Replicator common */
	{ TRUE,  "RS",231L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "Replicator Server "	},	/* Replicator Server */
	{ TRUE,  "RM",232L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "Replicator Manager"	},	/* Replicator Manager */
	{ TRUE,  "VS",233L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "VS"	},	/* VSE */
	{ TRUE,  "SW",234L * OFFSET_CAT_MASK		,""  ,          0, -1  ,TRUE, CLASS_SAME,               "SW (Sync OpenAPI)"	},	/* Sync API wrapper */
#endif // INCLUDE_ALL_FRONTEND_MESSAGES		
	{ FALSE,  "OD",235L * OFFSET_CAT_MASK	,""  ,          0, -1  ,TRUE, CLASS_SAME,               "OD (ODBC)"	},	/* ODBC error */
	{ FALSE,  "RE",236L * OFFSET_CAT_MASK	,""  ,          0, -1  ,TRUE, CLASS_SAME,               "RE (Remote Command Server)"	},	/* ODBC error */
	{ FALSE,  "JD",237L * OFFSET_CAT_MASK	,""  ,          0, -1  ,TRUE, CLASS_SAME,               "JD (JDBC Server)"	},

#if 0 // exclude rosetta messages for now
	{ FALSE,  "EX",257L * OFFSET_CAT_MASK	,""  ,          0, -1  ,TRUE, CLASS_SAME,               "EX"	},	/* Rosetta generic exceptions */
	{ FALSE,  "RX",258L * OFFSET_CAT_MASK	,""  ,          0, -1  ,TRUE, CLASS_SAME,               "RX"      },      /* Rosetta utilities */
	{ FALSE,  "SY",259L * OFFSET_CAT_MASK	,""  ,          0, -1  ,TRUE, CLASS_SAME,               "SY"      },      /* Rosetta system manager */
	{ FALSE,  "SQ",260L * OFFSET_CAT_MASK	,""  ,          0, -1  ,TRUE, CLASS_SAME,               "SQ"      },      /* Rosetta rssql wrapper */
	{ FALSE,  "RI",261L * OFFSET_CAT_MASK	,""  ,          0, -1  ,TRUE, CLASS_SAME,               "RI"      },      /* Rosetta RMI facility */
	{ FALSE,  "DA",262L * OFFSET_CAT_MASK	,""  ,          0, -1  ,TRUE, CLASS_SAME,               "DA"      },      /* Rosetta primary data types */
	{ FALSE,  "OL",263L * OFFSET_CAT_MASK	,""  ,          0, -1  ,TRUE, CLASS_SAME,               "OL"      },      /* Rosetta rsoledb wrapper */
#endif
  
  // NULLS for detecting end of array
	{ FALSE, NULL,  0          ,NULL  ,          0,  0  ,TRUE, CLASS_SAME,               NULL },

};

typedef struct
{
	char * immediatesuffix;
	char * otherstringinmsg;
	long   lmsg;
} GW_MSG;

GW_MSG GW_StartStopMessages[]=
{
	{"ORA",    "Gateway Server start-up",        GW_ORA_START                   },
	{"ORA",    "Connect limit",                  GW_ORA_START_CONNECTLIMIT      },
	{"MSSQL",  "Gateway Server start-up",        GW_MSSQL_START                 },
	{"MSSQL",  "Connect limit",                  GW_MSSQL_START_CONNECTLIMIT    },
	{"INF",    "Gateway Server start-up",        GW_INF_START                   },
	{"INF",    "Connect limit",                  GW_INF_START_CONNECTLIMIT      },
	{"SYB",    "Gateway Server start-up",        GW_SYB_START                   },
	{"SYB",    "Connect limit",                  GW_SYB_START_CONNECTLIMIT      },
	{"DB2UDB", "Gateway Server start-up",        GW_DB2UDB_START                },
	{"DB2UDB", "Connect limit",                  GW_DB2UDB_START_CONNECTLIMIT   },
	{"ORA",    "Gateway Server normal shutdown", GW_ORA_STOP                    },
	{"MSSQL",  "Gateway Server normal shutdown", GW_MSSQL_STOP                  },
	{"INF",    "Gateway Server normal shutdown", GW_INF_STOP                    },
	{"SYB",    "Gateway Server normal shutdown", GW_SYB_STOP                    },
	{"DB2UDB", "Gateway Server normal shutdown", GW_DB2UDB_STOP                 },
	{NULL, NULL, 0L}
};

static int iClass4Interface = -1;


static BOOL GetNextIngClass4Interface1(long &lClassRef, CString &csDesc, BOOL bIncludeFrontEndClasses)
{
	INGERRCLASS * pclass;
	while ( TRUE) {
		iClass4Interface++;
		pclass = & IngErClasses[iClass4Interface];
		if (!pclass->classprefix) // end of array has empty pointer
			return FALSE;
		if (pclass->bIsClass4Interface && (pclass->bNotBackend == bIncludeFrontEndClasses || bIncludeFrontEndClasses)) {
			lClassRef=pclass->lclassnum + pclass->lclasssubmask;
			csDesc = pclass->classdescription;
			return TRUE;
		}
	}
	return FALSE;
}
BOOL GetFirstIngClass4Interface(long &lClassRef, CString &csDesc)
{
	iClass4Interface = -1;
	return GetNextIngClass4Interface1(lClassRef, csDesc, FALSE);
}
BOOL GetNextIngClass4Interface(long &lClassRef, CString &csDesc)
{
	return GetNextIngClass4Interface1(lClassRef, csDesc, FALSE);
}

#define DONTSHOWFRONTEND

BOOL GetFirstIngClass4MessageExplanation(long &lClassRef, CString &csDesc)
{
	iClass4Interface = -1;
	return GetNextIngClass4Interface1(lClassRef, csDesc,
#ifdef DONTSHOWFRONTEND		
		FALSE
#else
		TRUE
#endif		
		);
}
BOOL GetNextIngClass4MessageExplanation(long &lClassRef, CString &csDesc)
{
	return GetNextIngClass4Interface1(lClassRef, csDesc,
#ifdef DONTSHOWFRONTEND		
		FALSE
#else
		TRUE
#endif		
		);
}


static int iIngresCat;
static long lInterfaceCat;

static long llastfoundinIngresCat;

BOOL GetFirstIngCatMessage(long lcatID, long &lmsgID, CString &csDesc)
{
	lInterfaceCat	=lcatID;
	iIngresCat	= -1L;
	return GetNextIngCatMessage( lmsgID,  csDesc);


}
BOOL GetNextIngCatMessage( long &lmsgID, CString &csDesc)
{
	long lcurmsg = llastfoundinIngresCat;
	char buf[10000];
	long lclass;

	while (TRUE) { 
		if (iIngresCat>=0) {  // skip this step at initialization
			lclass = IngErClasses[iIngresCat].lclassnum + IngErClasses[iIngresCat].lclasssubmask;
			while (TRUE) { // look for next message in prev category
				lcurmsg++;
				if (lcurmsg>IngErClasses[iIngresCat].lmaxnum)
					break;
				if (GetAndFormatIngMsg(lclass + lcurmsg, buf, sizeof(buf))) {
					char * p = buf;
					unsigned char c;
					while ( (c=*p) !='\0') { 
						if (c>'\0' && c<' ') /* DBCS compliant given the range of trailing bytes */    
							*p=' ';
						p = _tcsinc(p);
					}
					llastfoundinIngresCat = lcurmsg;
					lmsgID = lclass + lcurmsg;
					csDesc = buf;
					return TRUE;
				}
			}
			if ((long)IngErClasses[iIngresCat].lclassnum== CLASS_GW_STARTSTOP) {
				int i,i2use;
				for (i=0,i2use=-1;GW_StartStopMessages[i].immediatesuffix;i++) {
					if (GW_StartStopMessages[i].lmsg == CLASS_GW_STARTSTOP + llastfoundinIngresCat) {
						i2use=i+1;
						break;
					}
				}
				if (i2use<0)
					i2use=0;
				if (GW_StartStopMessages[i2use].immediatesuffix) {
					lmsgID = GW_StartStopMessages[i2use].lmsg;
					llastfoundinIngresCat = lmsgID-CLASS_GW_STARTSTOP;
					csDesc.Format("%s  %s",GW_StartStopMessages[i2use].immediatesuffix,GW_StartStopMessages[i2use].otherstringinmsg);
					return TRUE;
				}
			}
			IngErClasses[iIngresCat].lmaxnum = llastfoundinIngresCat;
		}


		while (TRUE) { // search next ingres category that fits with the interface category
			iIngresCat++;
			if (!IngErClasses[iIngresCat].classprefix)
				return FALSE; // all ingrescats scanned
			lclass = IngErClasses[iIngresCat].lclassnum + IngErClasses[iIngresCat].lclasssubmask;
			if ( IngErClasses[iIngresCat].lInterfaceClass == lInterfaceCat || lclass == lInterfaceCat)
				break;
		}
		// valid ingres class found for the interface class

		// if max messageno unknown, set to the maximum
		if (IngErClasses[iIngresCat].lmaxnum <0L) {
			if (!stricmp(IngErClasses[iIngresCat].classprefix,"CL"))
				IngErClasses[iIngresCat].lmaxnum = 0xFF;
			else
				IngErClasses[iIngresCat].lmaxnum = 0xFFFF;
		}
		lcurmsg = 0L;
		llastfoundinIngresCat = 0L;



	}
	return FALSE; // no more message
}

#define NUMFAC (sizeof(Factab)/sizeof(Factab[0]))

// returns -1 if error
static inline int getintfromhexsinglechar(char c)
{

	if (c>='0' && c<='9')
		return (c-'0');
	if (c>='A' && c<='F')
		return 10 + (int)(c-'A');
	return -1;
}

MSGCLASSANDID getMsgStringClassAndId(char * pmsg, char * pbufreturnedsubstring, int ibufsize)
{
	char c = *pmsg;
	char bufcat[3];
	int ihexcharcode;
	long lerrcode = 0L;
	MSGCLASSANDID result;
	
	BOOL bresult = FALSE;
	if (_tcsncmp(pmsg,"EA ", 3)==0) {
		/* temporary workaround by mapping to virtual messge ID's of the GWF*/
		/* category, until these messages are formatted like Ingres ones    */
		int i;
		for (i=0;GW_StartStopMessages[i].immediatesuffix;i++) {
			if (!(_tcsncmp(pmsg+3, 
				           GW_StartStopMessages[i].immediatesuffix,
				          _tcslen(GW_StartStopMessages[i].immediatesuffix)))
				&& _tcsstr(pmsg+3, GW_StartStopMessages[i].otherstringinmsg)
			   ) {
					strcpy(pbufreturnedsubstring,""); /* sigle byte chars string*/
					char * pdatestart= strchr(pmsg+3,_T(' '));
					if (pdatestart) {
						pdatestart = strchr(pdatestart+1,_T(' '));
						if (pdatestart) {
							while (*pdatestart==' ')
								pdatestart++;
							char *pdateend = strstr(pdatestart,": ");
							if (pdateend) {
								int idatelen=(pdateend-pdatestart);
								if ( idatelen+1<=ibufsize) {
									strncpy(pbufreturnedsubstring,pdatestart, idatelen+1);
									pbufreturnedsubstring[idatelen]='\0';
								}
							}
						}
					}
					result.lMsgClass  = CLASS_GW_STARTSTOP;
					result.lMsgFullID = GW_StartStopMessages[i].lmsg;
					return result;
			}
		}
		strcpy(pbufreturnedsubstring,""); /* sigle byte chars string*/
		result.lMsgClass  = CLASS_GW_STARTSTOP;
		result.lMsgFullID = GW_OTHER_STARTSTOP_MSG;
		return result;
	}

	while (TRUE) {
		if (_tcslen(pmsg)<8)
			break;
		if ( (c!='E' && c!='W' && c!='S' && c!='I') || pmsg[1]!='_')
			break;
		
		pmsg+=2;
		if (CMbytecnt(pmsg)!=1 || CMbytecnt(pmsg+2)!=1)
			break;
		bufcat[0]=*(pmsg++);
		bufcat[1]=*(pmsg++);
		bufcat[2]='\0';

		for (int iloc=0;iloc<4;iloc++) {
			ihexcharcode =getintfromhexsinglechar(*(pmsg++)); // DBCS checked. OK given the ranges of leading and trailing bytes
			if (ihexcharcode<0)
				break;
			ihexcharcode = ihexcharcode << (3-iloc)*4;
			lerrcode += (long)ihexcharcode;
		}

		bresult = TRUE;
		break;
	}
	if (!bresult) {
		result.lMsgClass  = CLASS_OTHERS;
		result.lMsgFullID = MSG_NOCODE;
		return result;
	}

	for (int icat=0;IngErClasses[icat].classprefix;icat++) {
		if (!strcmp(IngErClasses[icat].classprefix,bufcat)) {
			// category found, look for subcategory
			if (IngErClasses[icat].lclasssubmask !=0L) {
				long lsubcat = lerrcode&0xFF00;
				if (lsubcat!=IngErClasses[icat].lclasssubmask)
					continue;
				lerrcode &=0xFF;
			}
			long lbaseclass = IngErClasses[icat].lclassnum   +
								IngErClasses[icat].lclasssubmask;
			result.lMsgFullID = lbaseclass + lerrcode;
			long lInterfaceClass = IngErClasses[icat].lInterfaceClass;
			if ( lInterfaceClass != CLASS_SAME)
				result.lMsgClass = lInterfaceClass;
			else
				result.lMsgClass = lbaseclass;
			return result;
		}
	}

	// not found, -> error
	result.lMsgClass  = CLASS_OTHERS;
	result.lMsgFullID = MSG_NOCODE;
	return result;

}
