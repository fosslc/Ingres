/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: erfac.h	Facility codes
**
** Description:
**	The facility codes in this header files are used by errhelp
**	and the ICL diagnostics.
**
** History:
**	15-apr-97 (mcgem01)
**	    Add WB for OpenIngres ICE.
**	25-mar-1998 (canor01)
**	    Update with newer message classes, including VS.
**	16-apr-1998 (canor01)
**	    Add Gateway error classes.
**	16-apr-98 (joea)
**		Add Replicator common and server, synchronous API wrappers.
**      24-aug-1998 (canor01)
**          Add Rosetta utilities.
**       3-sep-1998 (mulwi01)
**          Add facility for rssql wrapper
**      16-sep-1998 (ma$ha01)
**          Add facility for Rosetta primary data types
**      16-sep-1998 (mulwi01)
**          Add facility for rsoledb wrapper
**	22-oct-98 (thoda04)
**		Add ODBC error class.
**	14-sep-99 (dansa02)
**	    Mainly to avoid conflicts with future allocations of class id
**	    numbers and 2-letter codes, I've added the classes that have 
**	    been used by OpenROAD for the past 9 years.  (These were not 
**	    previously visible in Ingres builds because their .msg files 
**	    lived under the w4gl source subtree.)
**
**		OM 189 (already in this file)
**		DO 191 
**		PW 192 
**		XO 193 
**		WN 194 
**		WE 195 
**		SY 197 
**		SE 199 
**		EO 200 
**		WR 206 
**		WM 207 
**		WF 209 
**		WT 210 
**		W4 211 
**		WC 220 
**
**	    (Note that the XO, WE, WM, WF classes have no corresponding .msg
**	    file at this time.  There are, however, active er*.h files in
**	    w4gl!hdr!hdr that contain defined constants under those classes.)
**
**	    I also noticed that several other classes (from front!hdr!hdr .msg
**	    files, and eraif.h) were missing, so I added those too:
**
**		AP 201 
**		CX 215 
**		G4 216 
**		DG 229 
**
**	    To be safe, always allocate new facility code numbers higher than 
**	    the highest one here (and search this list to avoid conflicts
**	    with existing two-letter mnemonics).
**	04-aug-2000 (yeema01) Bug 102264	
**	    Added SV (265) for OpenROAD Application Remote Server. Also
**	    changed OpenROAD SY message class to SM to avoid overlapping
**	    with the Rosetta message class in Ingres 2.5.
**	    
**
**      13-nov-1998 (canor01)
**          Synchronize Ingres/Rosetta changes.
**	30-jun-1999 (somsa01)
**	    Added "RE" for RMCMD messages.
**      17-Jan-2000 (fanra01)
**          Bug 100036
**          Added WU for ice utilities.
**	23-Mar-00 (gordy)
**	    Added JD (237) for JDBC.
**	12-jun-2001 (somsa01)
**	    Added "XM" for GENXML error messages.
*/

struct
{
	char	*fac;
	int	num;
}
 Factab[] =
{
	{ "US",0	},
	{ "CL",1	},
	{ "AD",2	},
	{ "DM",3	},
	{ "OP",4	},
	{ "PS",5	},
	{ "QE",6	},
	{ "QS",7	},
	{ "RD",8	},
	{ "SC",9	},
	{ "UL",10	},
	{ "DU",11	},
	{ "GC",12	},
	{ "RQ",13	},
	{ "TP",14	},
	{ "GW",15	},
	{ "SX",16	},
	{ "AF",32	},
	{ "FM",33	},
	{ "UI",34	},
	{ "AB",40	},
	{ "AR",41	},
	{ "AS",42	},
	{ "CA",44	},
	{ "AM",45	},
	{ "OR",47	},
	{ "IB",48	},
	{ "IT",49	},
	{ "O2",52	},
	{ "OS",53	},
	{ "FC",54	},
	{ "FD",55	},
	{ "FR",57	},
	{ "TB",58	},
	{ "FV",60	},
	{ "FT",61	},
	{ "GT",62	},
	{ "LS",63	},
	{ "TD",66	},
	{ "VT",67	},
	{ "GP",69	},
	{ "GR",72	},
	{ "RG",75	},
	{ "VG",76	},
	{ "CD",78	},
	{ "CF",79	},
	{ "DE",80	},
	{ "IC",83	},
	{ "IM",84	},
	{ "PF",87	},
	{ "TU",88	},
	{ "UD",89	},
	{ "XF",90	},
	{ "QF",91	},
	{ "RC",92	},
	{ "RF",93	},
	{ "RW",94	},
	{ "SR",96	},
	{ "MF",97	},
	{ "MO",98	},
	{ "QR",99	},
	{ "DF",100	},
	{ "WS",101	},
	{ "QG",134	},
	{ "OO",135	},
	{ "UF",136	},
	{ "UG",137	},
	{ "VF",139	},
	{ "EQ",140	},
	{ "E0",141	},
	{ "E1",142	},
	{ "E2",143	},
	{ "E3",144	},
	{ "E4",145	},
	{ "E5",146	},
	{ "E6",147	},
	{ "LQ",151	},
	{ "LC",152	},
	{ "C6",160	},
	{ "FE",161	},
	{ "DC",162	},
	{ "VQ",169	},
	{ "CG",170	},
	{ "FG",173	},
	{ "AI",174	},
	{ "A6",175	},
	{ "TL",178	},
	{ "M1",179	},
	{ "MC",180	},
	{ "FI",181	},
	{ "CU",182	},
	{ "AC",183	},
	{ "CO",184	},
	{ "MT",185	},
	{ "OM",189	},
	{ "DO",191	},
	{ "PW",192	},
	{ "XO",193	},
	{ "WN",194	},
	{ "WE",195	},
	{ "SM",197	},	/* OpenROAD SY message changed to SM */
	{ "SE",199	},
	{ "EO",200	},
	{ "AP",201	},	/* API messages */
	{ "GU",202	},	/* Gateway utility messages */
	{ "GX",203	},	/* Gateway error messages */
	{ "DD",205	},
	{ "WR",206	},
	{ "WM",207	},
	{ "WF",209	},
	{ "WT",210	},
	{ "W4",211	},
	{ "ST",212	},
	{ "GL",213	},
	{ "CX",215	},
	{ "G4",216	},
	{ "IE",217	},
	{ "WC",220      },
	{ "WB",225	},
	{ "WA",226      },
	{ "DG",229	},
	{ "RP",230	},	/* Replicator common */
	{ "RS",231	},	/* Replicator Server */
	{ "RM",232	},	/* Replicator Manager */
	{ "VS",233	},	/* VSE messages */
	{ "SW",234	},	/* Sync API wrapper */
	{ "OD",235	},	/* ODBC error messages */
	{ "RE",236	},	/* RMCMD error messages */
	{ "JD",237	},	/* JDBC Driver */
	{ "XM",238	},	/* GENXML */
	{ "EX",257	},	/* Rosetta generic exceptions */
        { "RX",258      },      /* Rosetta utilities */
        { "SY",259      },      /* Rosetta system manager */
        { "SQ",260      },      /* Rosetta rssql wrapper */
        { "RI",261      },      /* Rosetta RMI facility */
        { "DA",262      },      /* Rosetta primary data types */
        { "OL",263      },      /* Rosetta rsoledb wrapper */
        { "WU",264      },      /* ICE utilities */
	{ "SV",265      }       /* OpenROAD AppServer messages */
};

#define NUMFAC (sizeof(Factab)/sizeof(Factab[0]))
