/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cm.h>
# include	<cv.h>
# include	<er.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ui.h>
# include	<si.h>


	static	PTR		srwl_hash_tbl = (PTR)NULL;

#ifdef xDEBUG
	static	VOID	IIUGdht_DumpHashTable();
#endif

/*
** Name:	ugkeyword.c - Front-End Utility SQL Reserved Word Look-Up Module
**
** Description:
**	Contains routines to effect a hashed look-up of a string against the
**	list of ANSI/INGRES SQL reserved words.
**
** CAVEAT:
** -------
**	All entries in sql_reserved_word_list MUST be in lower case ONLY.
**	SRWL_CNT value MUST be exact, or empty strings and/or random garbage
**	will be included in the hash table.
**	SRWL_NEXT_PRIME value MUST be greater than or equal to SRWL_CNT, and
**	should reflect the least prime number which is greater than or equal
**	to SRWL_CNT.
** 
** History:
**	4/17/90 (martym)
**		Derived from "iskeyword()" in OSL.
**	17-aug-91 (leighb)
**		DeskTop Porting Change: added cv.h
**	02-jul-1992	(rdrane)
**		Created from ISO 9075:1992, and replaces sqlkwd.roc and its attendant
**		sqlkwd.h.  Includes SQL reserved words which are not part of ANSI, not
**		QUEL specific, not FRS specific, and not 3GL specific.  Note that what
**		started life as a delimited identifier specific routine has now come
**		to replace the previous IIUGIsSQLKeyWord().  Main features are that the
**		keyword list is internal, and the look-up is via a hash mechanism
**		rather than a binary search.  Additionaly, each keyword is tagged with
**		the INGRES OpenSQL level in which it became a keyword, starting with
**		UI_LEVEL_65.
**	15-oct-1992
**		Fix INGRES OpenSQL level for pre-6.5 entries.  Tag as UI_LEVEL_NONE
**		instead of UI_LEVEL_64 so we'll work for all Versions.
**	20-oct-1993
**		Add "byref" and "initial_user" as 6.5 entries.  Deleted words which
**		the ANSI spec considers delimited, but the DBMS did not implement.
**	20-apr-1994 (rdrane)
**		Containerize the debugging code (dump the hash table) as per the
**		latest coding standards.
**	15-Dec-1994 (brogr02)
**		Added 'const' to declarations of 2 vars to eliminate 
**		compiler warnings.
**	01-sep-1995 (abowler)
**		Removed 'comment' from the keyword list. The server doesn't
**		think it should be there, we don't use it, and its not in
**		the SQL Standards book (B70409).
**      04-Dec-1995 (walro03/mosjo01)
**		'const' from 15-Dec-1994 change causes compiler errors on
**		DG/UX (dg8_us5).  Changed to READONLY.
**      04-Dec-1995 (rosga02)
**		Changed READONLY back to 'const' reversing the previous change
**              because it is causing AXP VMS compiling errors and not ANSI C
**              compliant. 
**	07-oct-1997 (nanpr01)
**		86139 : Added new reserved words for OI 2.0.
**	15-dec-1998 (nanpr01)
**		Added new reserved words for OI 2.5.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-sep-2001 (hanch04)
**	    added new reserved words:  endfor, endrepeat, leave, result, row
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**    26-Aug-2010 (thaju02) B123865
**        Add reserved word: grouping
*/


# define	SRWL_CNT			152
# define	SRWL_NEXT_PRIME		157
# define	SRWL_MAX_LGTH		19
								/*
								** Actually, current longest is 17, but
								** make each element a multiple of 4 bytes
								** (including the terminating NULL).
								*/
	typedef	struct	sql_kwrd
			{
				char	sk_word[(SRWL_MAX_LGTH + 1)];
				char	sk_cat_level[8];
			}		SQL_KEYWORD;


	GLOBALCONSTDEF SQL_KEYWORD	sql_reserved_word_list[SRWL_CNT] =
	{
		{
			ERx("abort"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("add"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("all"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("alter"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("and"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("any"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("as"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("asc"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("at"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("authorization"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("avg"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("begin"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("between"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("by"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("byref"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("callproc"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("cascade"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("check"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("close"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("column"),
			ERx(UI_LEVEL_800)
		},
		{
			ERx("commit"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("committed"),
			ERx(UI_LEVEL_800)
		},
		{
			ERx("constraint"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("constraints"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("continue"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("copy"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("count"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("create"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("current"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("current_user"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("cursor"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("declare"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("default"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("define"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("delete"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("describe"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("disable"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("distinct"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("do"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("drop"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("else"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("elseif"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("enable"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("end"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("endfor"),
			ERx(UI_LEVEL_860)
		},
		{
			ERx("endif"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("endloop"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("endrepeat"),
			ERx(UI_LEVEL_860)
		},
		{
			ERx("endwhile"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("escape"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("excluding"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("execute"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("fetch"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("for"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("foreign"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("from"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("full"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("global"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("grant"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("group"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("grouping"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("having"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("if"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("immediate"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("import"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("in"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("index"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("initial_user"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("inner"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("insert"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("integrity"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("into"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("is"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("isolation"),
			ERx(UI_LEVEL_800)
		},
		{
			ERx("join"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("key"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("leave"),
			ERx(UI_LEVEL_860)
		},
		{
			ERx("left"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("level"),
			ERx(UI_LEVEL_800)
		},
		{
			ERx("like"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("local"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("max"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("message"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("min"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("modify"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("module"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("natural"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("not"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("null"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("of"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("on"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("open"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("option"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("or"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("order"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("outer"),
			ERx(UI_LEVEL_800)
		},
		{
			ERx("permit"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("prepare"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("preserve"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("primary"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("privileges"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("procedure"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("public"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("raise"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("read"),
			ERx(UI_LEVEL_800)
		},
		{
			ERx("references"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("referencing"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("register"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("relocate"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("remove"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("repeat"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("repeatable"),
			ERx(UI_LEVEL_800)
		},
		{
			ERx("restrict"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("result"),
			ERx(UI_LEVEL_860)
		},
		{
			ERx("return"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("revoke"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("right"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("rollback"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("row"),
			ERx(UI_LEVEL_860)
		},
		{
			ERx("rows"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("save"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("savepoint"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("schema"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("select"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("serializable"),
			ERx(UI_LEVEL_800)
		},
		{
			ERx("session"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("session_user"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("set"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("some"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("sql"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("sum"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("system_maintained"),
			ERx(UI_LEVEL_850)
		},
		{
			ERx("system_user"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("table"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("temporary"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("then"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("to"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("uncommitted"),
			ERx(UI_LEVEL_800)
		},
		{
			ERx("union"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("unique"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("until"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("update"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("user"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("using"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("values"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("view"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("when"),
			ERx(UI_LEVEL_65)
		},
		{
			ERx("where"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("while"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("with"),
			ERx(UI_LEVEL_NONE)
		},
		{
			ERx("work"),
			ERx(UI_LEVEL_NONE)
		}
	};


/*
** Name:	IIUGIsSQLKeyword()
**
** Description:
**	Accepts a string and determines if it matches one of the ANSI/INGRES
**	SQL reserved words.  This indicates, among other things, whether or not
**	the string must be treated as a delimited identifier.
**
** Input:
**	name		Character pointer to NULL terminated string which is the
**				identifier to be checked.
** Output:
**	Nothing.
**
** Returns:
**	bool		TRUE if name matches an ANSI/INGRES SQL reserved word.
**				Otherwise, returns FALSE.
**
** Side Effects:
**	If the hash table hasn't been built yet, then IIUGhiHtabInit() will
**	be called to build it.
**
** History:
**	26-aug-1992 (rdrane)
**		Created.
**	20-apr-1994 (rdrane)
**		Use OpenSQL level to determine 6.5 syntax features support.
**		We missed this routine when we change the rest of UI and
**		friends.
*/

bool
IIUGIsSQLKeyWord(name)
		char	*name;
{
	register i4	i;
			const   char	*cur_sql_wrd_ptr;
			const   SQL_KEYWORD	*cur_sql_entry_ptr;
			char	w_name[(DB_MAXNAME + 1)];

	
	if  ((name == NULL) || (*name == EOS))
	{
		return(FALSE);
	}
					/*
					** Build the hash table if we haven't already.
					** Use the default memory allocation failure function
					** (it dies), comparison function (STcompare()), and
					** hash algorithm (ERx("string divide modulo table size").
					*/
	if  (srwl_hash_tbl == (PTR)NULL)
	{
		IIUGhiHtabInit(SRWL_NEXT_PRIME,NULL,NULL,NULL,&srwl_hash_tbl);
		i = SRWL_CNT;
		while (--i >= 0)
		{
			cur_sql_wrd_ptr = &sql_reserved_word_list[i].sk_word[0];
			cur_sql_entry_ptr = &sql_reserved_word_list[i];
			IIUGheHtabEnter(srwl_hash_tbl,cur_sql_wrd_ptr,
						    (PTR)cur_sql_entry_ptr);
		}

#ifdef xDEBUG
		IIUGdht_DumpHashTable(srwl_hash_tbl);
#endif

	}
						/*
						** Convert the input string to lower case, since all
						** ANSI/INGRES  SQL reserved words are case insensitive,
						** and our look-up table is lower case.  Also, strip
						** any trailing whitespace.
						*/
	STcopy(name,&w_name[0]);
	STtrmwhite(&w_name[0]);
	CVlower(&w_name[0]);
						/*
						** Not only do we need to match, but also we need
						** to ensure that the matched word is actually
						** reserved for our OpenSQL level!
						*/
	if ((IIUGhfHtabFind(srwl_hash_tbl,&w_name[0],&cur_sql_entry_ptr) == OK) &&
		(STcompare(IIUIcsl_CommonSQLLevel(),
				   &cur_sql_entry_ptr->sk_cat_level[0]) >= 0))
	{
			return(TRUE);
	}

	return(FALSE);
}


#ifdef xDEBUG
/*
** Name:	IIUGdht_DumpHashTable
**
** Description:
**	Dump the ANSI/INGRES SQL Reserved Word Hash Table to standard output.
**	This is a development debugging routine!
**
** Input:
**	Nothing.
**
** Output:
**	Nothing.
**
** Returns:
**	Nothing.
**
** History:
**	26-aug-1992 (rdrane)
**		Created.
*/

static
VOID
IIUGdht_DumpHashTable(srwl_hash_tbl)
			PTR		srwl_hash_tbl;
{
	register i4	i,j;
			double	collision_count[SRWL_CNT];
			double	tot_collisions;
			double	avg_collision_cnt;
			i4		max_collisions;
			i4		tot_words;
			i4		tot_empty;
			i4		cur_cell;
			i4		longest_word;
			bool 	cont_scan;
			char	*w_ptr;
			SQL_KEYWORD	*d_ptr;


	SIprintf(ERx("Dumping SQL Reserved Word hash Table\n\n"),NULL);

	i = SRWL_CNT;
	j = 0;
	while (--i >= 0)
	{
		collision_count[j++] = 0;
	}
	tot_words = 0;
	tot_empty = 0;
	longest_word = 0;

	cur_cell = 0;
	cont_scan = FALSE;
	while (TRUE)
	{
		i = IIUGhsHtabScan(srwl_hash_tbl,cont_scan,&w_ptr,&d_ptr);
		if  (i == 0)
		{
			break;
		}
		tot_words++;
		cont_scan = TRUE;
		j = i - cur_cell;
		if  (j > 1)
		{
			while (--j >= 0)
			{
				SIprintf(ERx("Entry: %d\n\t*** EMPTY ***\n"),cur_cell++);
				tot_empty++;
			}
			if  (!STequal(w_ptr,&d_ptr->sk_word[0]))
			{
				SIprintf(ERx("Mismatch: %s\t%s\n"),w_ptr,&d_ptr->sk_word[0]);
			}
			else
			{
				SIprintf(ERx("Entry: %d\n\t%s\t%s\n"),i,&d_ptr->sk_word[0],
						 &d_ptr->sk_cat_level[0]);
				if  ((j = STlength(w_ptr)) > longest_word)
				{
					longest_word = j;
				}
			}
		}
		else
		{
			if (i != cur_cell)
			{
				SIprintf(ERx("Entry: %d\n"),i);
				cur_cell = i;
			}
			else
			{
				collision_count[cur_cell]++;
			}
			if  (!STequal(w_ptr,&d_ptr->sk_word[0]))
			{
				SIprintf(ERx("Mismatch: %s\t%s\n"),w_ptr,&d_ptr->sk_word[0]);
			}
			else
			{
				SIprintf(ERx("Entry: %d\n\t%s\t%s\n"),i,&d_ptr->sk_word[0],
						 &d_ptr->sk_cat_level[0]);
				if  ((j = STlength(w_ptr)) > longest_word)
				{
					longest_word = j;
				}
			}
		}
	}

	max_collisions = 0;
	tot_collisions = 0;
	avg_collision_cnt = 0;
	i = SRWL_CNT;
	j = 0;
	while (--i >= 0)
	{
		tot_collisions += collision_count[j];
		if  (collision_count[j] > 0)
		{
			avg_collision_cnt++;
		}
		if  (collision_count[j] > max_collisions)
		{
			max_collisions = collision_count[j];
		}
		j++;
	}
	SIprintf(ERx("\n\nTotal Words: %d\n"),tot_words);
	SIprintf(ERx("\nLongest Word: %d Characters\n"),longest_word);
	SIprintf(ERx("\nTotal Empty Cells: %d\n"),tot_empty);
	SIprintf(ERx("Percent Empty Cells: %f\n"),
			 ((tot_empty * 100.0) / SRWL_CNT),'.');
	i = avg_collision_cnt;
	SIprintf(ERx("\nEntries with Collision Chains: %d\n"),i);
	i = tot_collisions;
	SIprintf(ERx("\nTotal Collisions: %d\n"),i);
	SIprintf(ERx("Percent Collisions: %f\n"),
			 ((tot_collisions * 100.0) / SRWL_CNT),'.');
	SIprintf(ERx("Longest Collision Chain: %d\n"),max_collisions);
	if  (avg_collision_cnt > 0)
	{
		SIprintf(ERx("Average Collision Chain: %f\n\n"),
			 (tot_collisions / avg_collision_cnt),'.');
	}

	return;
}
#endif
