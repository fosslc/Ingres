/******************************************************************************
**
** Copyright (c) 1989, Ingres Corporation
**
******************************************************************************/

#include <compat.h>
#include <cv.h>
#include <cs.h>
#include <di.h>
#include <lo.h>
#include <pc.h>
#include <si.h>
#include <st.h>
#include <tr.h>

/******************************************************************************
** Name: DIDUMP.C		- dump a page from a file.
**
** Description:
**	This program accepts the name of a DI file, and a page number in
**	that file, and reads that page number from that file and dumps it
**	out.
**
**	Calling format:
**
**	<change directory to the directory holding the database>
**	didump [-pnn/mm] file1 [file2...]
**	didump [-pnn]    file1 [file2...]
**
**	In the first form, pages nn through mm are dumped for each file.
**	In the second, only page nn is dumped. If -p is ommitted, the entire
**	file is dumped, but don't do this unless you want to see a LOT of
**	information.
**
**	Example:
**
**	didump -p54/56 aaaaaabf.t00
**
**	Dumps pages 54 through 56 of aaaaaabf.t00.
**
**	Alternatively, you may provide the '-check' flag before any filenames.
**	If this flag is given, then instead of dumping the pages to the screen,
**	the pages are internally checked for obvious corruption (this checking
**	is perhaps release specific, and is not guaranteed to work with any
**	release other than 6.2). The '-check' flag may be provided with or
**	without the '-' flag to restrict the pages to be checked.
**
******************************************************************************/

# define PAGE_SIZE 2048

/******************************************************************************
** This structure is a faked-up copy of DMPP_PAGE (in dmpp.h). So if that
** one changes, this must change as well.
******************************************************************************/

/* incomplete, but these are the only fields we check. */

typedef struct {
	i4              page_main;
	i4              page_ovfl;
	i4              page_page;	/* the page number of this page. */
	u_i2            page_stat;
	u_i2            page_seq_number;
	char            page_tran_id[8];
	char            page_log_addr[8];
	u_i2            page_version;
	u_i2            page_next_line;	/* Next available line number in
					 * table. Also, the page_line_tab
					 * [page_next_line] contains the
					 * number of free bytes left on the
					 * page. */
	u_i2            page_line_tab[1];	/* variable sized. */
	/*
	 * Array of offsets from beginning of page to the first byte of the
	 * record.
	 */
} DMPP_PAGE;

/******************************************************************************
** A line table entry is composed of these parts:
**	bits 0-14	Offset to record from beginning of page.
**	bit  15		Record new indicator
******************************************************************************/
#define dmpp_get_offset_macro( line_tab, index )	\
		(((u_i2 *)line_tab)[index] & 0x7fff)

static bool     CheckFlag = FALSE;	/* was '-check' given? */
static STATUS   do_dump(char *, i4, i4);
static STATUS   do_check(char *, i4, i4);
static STATUS   check_page(DMPP_PAGE *, i4);



/******************************************************************************
** Name: main		- main routine for didump.
**
** Description:
**	Called by the O/S.
**
** History:
**	13-Nov-89 (bryanp)
**	    Created.
**	01-may-1996 (canor01)
**	    Added call to Csinitiate() to set up an scb (which is
**	    used in DI calls).
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**
******************************************************************************/
i4
main(argc, argv)
	i4              argc;
	char           *argv[];
{
	i4              low_page;
	i4              high_page;
	CL_ERR_DESC     err_desc;
	i4              arg_num, arg_id;
	char           *ptr, *slash_pos;
	char            low_buf[10];

	if (TRset_file(TR_T_OPEN, "stdio", 5, &err_desc) != OK) 
	{
		SIfprintf(stderr, "DIdump: Could not open TR output.\n");
		PCexit(FAIL);
	}
	/*
	 * Get low and high page numbers from '-p' argument
	 */
	arg_num = 1;
	low_page = 0;
	high_page = -1;

	if (argc <= 1) {
		TRdisplay(
			  "DIdump: usage -- 'didump [-pn/m] [-check] file1 file2...'\n");
		PCexit(FAIL);
	} else {
		for (arg_id = 1; arg_id < argc; arg_id++) {
			ptr = argv[arg_id];
			if (*ptr == '-' && *(ptr + 1) == 'p') {
				/*
				 * -p given. parse into low_page, high_page &
				 * skip to file_names.
				 */
				arg_num++;
				ptr += 2;
				slash_pos = STindex(ptr, "/", 0);
				if (slash_pos) {
					/*
					 * '-pnn/mm'
					 */
					STlcopy(ptr, low_buf, (slash_pos - ptr));
					if (CVal(low_buf, &low_page) != OK)
						TRdisplay("DIdump: Bad number -- %s\n", low_buf);
					if (CVal(slash_pos + 1, &high_page) != OK)
						TRdisplay("DIdump: Bad number -- %s\n", low_buf);
				} else {
					/*
					 * '-pnn'
					 */
					if (CVal(ptr, &low_page) != OK)
						TRdisplay("DIdump: Bad number -- %s\n", ptr);
					high_page = low_page;
				}
			} else if (STbcompare(ptr, STlength(ptr), "-check", 6, TRUE) == 0) {
				arg_num++;
				CheckFlag = TRUE;
			} else {
				/*
				 * THis is a file name. Done checking
				 * arguments.
				 */
				break;
			}
		}
	}

	if (arg_num >= argc) {
		TRdisplay(
			  "DIdump: usage -- 'didump [-pn/m] [-check] file1 file2...'\n");
	}
	CSinitiate(0,NULL,NULL);
	while (arg_num < argc) {
		if (low_page != high_page)
			TRdisplay("DIdump: %s pages %d-%d from %s\n",
				  (CheckFlag ? "Checking" : "Dumping"),
			    low_page, (high_page == -1) ? 99999 : high_page,
				  argv[arg_num]);
		else
			TRdisplay("DIdump: %s page %d from %s\n",
				  (CheckFlag ? "Checking" : "Dumping"),
				  low_page, argv[arg_num]);

		if (CheckFlag) {
			if (do_check(argv[arg_num], low_page, high_page) != OK)
				TRdisplay("DIdump: Check of %s failed\n", argv[arg_num]);
		} else {
			if (do_dump(argv[arg_num], low_page, high_page) != OK)
				TRdisplay("DIdump: Display of %s failed\n", argv[arg_num]);
		}

		arg_num++;
	}

	TRdisplay("DIdump: Successful end.\n");
	PCexit(OK);
}


static          STATUS
do_dump(file_name, low_page, high_page)
	char           *file_name;
	i4              low_page;
	i4              high_page;
{
	STATUS          status = OK;
	DI_IO           f;
	i4              one_page = 1;
	i4              page_num;
	CL_ERR_DESC     err_desc;
	bool            file_opened = FALSE;
	char           *path;
	LOCATION        loc;
	char            loc_buf[MAX_LOC];
	char            buf[PAGE_SIZE];
	char           *ptr;

	LOgt(loc_buf, &loc);
	LOtos(&loc, &path);

	/*
	 * open the file
	 */
	status = DIopen(&f, path, (u_i4) STlength(path),
			file_name, (u_i4) STlength(file_name),
			(i4) PAGE_SIZE, (i4) DI_IO_READ,
			(u_i4     ) DI_SYNC_MASK, &err_desc);
	if (status == OK)
		file_opened = TRUE;
	else
		TRdisplay("**** Status $%x, (%d) opening %s in %s\n",
			  status, status, file_name, path);

	/*
	 * Show the pages.
	 */
	for (page_num = low_page;
	     status == OK && (page_num <= high_page || high_page == -1);
	     page_num++) {
		status = DIread(&f, &one_page, (i4     ) page_num, buf, &err_desc);
		if (status == OK) {
			TRdisplay("Page %d of file %s contains:\n", page_num, file_name);
			for (ptr = buf; (ptr - buf) < PAGE_SIZE; ptr += 16) {
				TRdisplay("   0x%x: %4.4{%x %}%2< >%16.1{%c%}\n",
					  (ptr - buf), ptr, 0);
			}
		} else if (status == DI_ENDFILE) {
			TRdisplay("**** End of file reached at page %d in %s\n",
				  page_num, file_name);
			status = OK;
			break;
		} else {
			TRdisplay("**** Status $%x, (%d) reading page %d from %s\n",
				  status, status, page_num, file_name);
		}
	}

	if (file_opened) {
		(VOID) DIclose(&f, &err_desc);
	}
	return (status);
}


static          STATUS
do_check(file_name, low_page, high_page)
	char           *file_name;
	i4              low_page;
	i4              high_page;
{
	STATUS          status = OK;
	DI_IO           f;
	i4              one_page = 1;
	i4              page_num;
	CL_ERR_DESC     err_desc;
	bool            file_opened = FALSE;
	char           *path;
	LOCATION        loc;
	char            loc_buf[MAX_LOC];
	char            buf[PAGE_SIZE];
	char           *ptr;

	LOgt(loc_buf, &loc);
	LOtos(&loc, &path);

	/*
	 * open the file
	 */
	status = DIopen(&f, path, (u_i4) STlength(path),
			file_name, (u_i4) STlength(file_name),
			(i4) PAGE_SIZE, (i4) DI_IO_READ,
			(u_i4) DI_SYNC_MASK, &err_desc);
	if (status == OK)
		file_opened = TRUE;
	else
		TRdisplay("**** Status $%x, (%d) opening %s in %s\n",
			  status, status, file_name, path);

	/*
	 * Check the pages.
	 */
	for (page_num = low_page;
	     status == OK && (page_num <= high_page || high_page == -1);
	     page_num++) {
		status = DIread(&f, &one_page, (i4) page_num, buf, &err_desc);
		if (status == OK) {
			if (check_page((DMPP_PAGE *) buf, page_num) != OK) {
				TRdisplay("%10*! Bad page %d in file %s %10*!\n",
					  page_num, file_name);
				TRdisplay("Page %d of file %s contains:\n",
					  page_num, file_name);
				for (ptr = buf; (ptr - buf) < PAGE_SIZE; ptr += 16) {
					TRdisplay("   0x%x: %4.4{%x %}%2< >%16.1{%c%}\n",
						  (ptr - buf), ptr, 0);
				}
			}
		} else if (status == DI_ENDFILE) {
			TRdisplay("**** End of file reached at page %d in %s\n",
				  page_num, file_name);
			status = OK;
			break;
		} else {
			TRdisplay("**** Status $%x, (%d) reading page %d from %s\n",
				  status, status, page_num, file_name);
		}
	}

	if (file_opened) {
		(VOID) DIclose(&f, &err_desc);
	}
	return (status);
}


static          STATUS
check_page(page, page_num)
	DMPP_PAGE      *page;
	i4              page_num;
{
	i4              i, offset;

	if (page->page_page != page_num) {
		TRdisplay("***>Page num is %d, but was supposed to be %d\n",
			  page->page_page, page_num);
		return (FAIL);
	}
	if ((i4) page->page_next_line < 0 ||
	    (i4) page->page_next_line > 1000) {
		TRdisplay("***>Page next line is %d, which is clearly wrong\n",
			  page->page_next_line);
		return (FAIL);
	}
	for (i = 0; i < page->page_next_line; i++) {
		offset = dmpp_get_offset_macro(page->page_line_tab, i);

		if (offset < 0 || offset > PAGE_SIZE) {
			TRdisplay("***>Offset %d is %d, which is clearly wrong!\n",
				  i, offset);
			return (FAIL);
		}
	}

	return (OK);
}
