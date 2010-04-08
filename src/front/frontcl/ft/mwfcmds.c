/*
**	MWfcmds.c
**	"@(#)mwfcmds.c	1.35"
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include <compat.h>
# include <st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <adf.h>
# include <fmt.h>
# include <ft.h>
# include <frame.h>
# include <fe.h>
# include "mwproto.h"
# include "mwmws.h"
# include "mwhost.h"
# include "mwform.h"
# include "mwintrnl.h"

/**
** Name:	MWfcmds.c - Commands and events for MWS Forms Director.
**
** Usage:
**	INGRES FE system and MWhost on Macintosh.
**
** Description:
**
**	This file contains the functions that send commands to and
**	receive events from the Form Director of the
**	MacWorkStation application running on the Macintosh.
**	
**	Routines defined here are:
**	- X101 IIMWafAddForm		create a new forms-mode window.
**	- X102 IIMWtfTossForm		remove the forms-mode window.
**	- X103 IIMWsfShowForm		draw the hidden forms-mode window.
**	- X104 IIMWhfHideForm		remove the forms window from the screen.
**	- X105 IIMWffFrontForm		bring the forms window to the front.
**	- X106 IIMWsftSaveFldsText	save the texts of all fields in the form.
**	- X107 IIMWrftRestoreFldsText	restore the texts of all fields in form.
**	- X108 IIMWfftFreeFldsText	discard structures used to save texts.
**	- X111 IIMWafiAddFieldItem	add a field item to the forms window.
**	- X112 IIMWdfiDeleteFormItem	delete an item from the forms window.
**	- X113 IIMWsiShowItem		redisplay a hidden item in the form.
**	- X114 IIMWhiHideItem		hide an item in the forms-mode window.
**	- X115 IIMWsiaSetItemAttr	change the attributes of the an item.
**	- X116 IIMWatiAddTableItem	add a table item to the window.
**	- X117 IIMWaciAddColumnItem	add a column item to a table item.
**	- X118 IIMWatrAddTRim		add a trim text, box or line item.
**	- X119 IIMWatcAddTableCell	set the text and attributes in a tbl cell.
**	- X120 IIMWgitGetItemText	retrieve the text of an item.
**	- X121 IIMWsitSetItemText	set the textual value of an item.
**	- X122 IIMWgciGetCurItem	determine the currently selected item.
**	- X123 IIMWsciSetCurItem	select the current item.
**	- X124 IIMWgttGetTbcellText	retrieve the textual value of a tbl cell.
**	- X125 IIMWsttSetTbcellText	set the textual value of a table cell.
**	- X126 IIMWgctGetCurTbcell	determine the current field of a TABLE.
**	- X127 IIMWsctSetCurTbcell	select the current cell of a TABLE item.
**	- X128 IIMWatgAddTblGroup	add a group of table cells
**	- X129 IIMWctgClearTblGroup	clear a group of table cells
**	- X131 IIMWstfScrollTblFld	scroll a table up/down.
**	-      IIMWprfPRstFld		Reset a form item.
**
**	The Form Director
**
**	The Dialog Director, which is a part of the current MWS release
**	does not provide the functionality necessary to support INGRES
**	forms mode. The display attributes of dialog items do not support
**	the full repertoire required by INGRES frontends. Additionally,
**	there needs to be a way of determining when a text edit field is 
**	entered and exited. Host-resident software needs to know this so
**	that it can get the value that the user entered into the field
**	and run a validation on it.
**	
**	Capturing Keystrokes
**	
**	One of the responsibilities of the Form Director is that it be
**	able to notify the host application when the user enters and
**	exits an item. This is necessary when the host application is an
**	INGRES frontend because the frontend may need to validate the
**	field being exited or notify the application which field is being
**	entered.
**	
**	Associating Mouse Clicks with Items in a Forms-Mode Window
**	
**	Converting mouse clicks can be a fairly simple process as long as
**	the Forms Director data structures are properly constructed. When
**	the host creates a new item, the Form Director should add an
**	entry (of type itemEntry) describing the item to a linked list.
**	This entry should include the pixel bounds of the item in local
**	coordinates.
**	
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	07/10/90 (dkh) - Integrated into r63 code line.
**	08/22/90 (nasser) - Replaced IIMWpc() with IIMWpxc().
**	09/16/90 (nasser) - Added col parameter to IIMWsi() & IIMWhi().
**	06/08/92 (fredb) - Enclosed file in 'ifdef DATAVIEW' to protect
**		ports that are not using MacWorkStation from extraneous
**		code and data symbols.  Had to include 'fe.h' to get
**		DATAVIEW (possibly) defined.
**    25-oct-1996(angusm) - add comments - 'text' is not
**            a null-terminated string in all cases.
**	18-apr-97 (cohmi01)   
**	    Added IIMWssbSendScrollButton for UPFRONT (bug 81574).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


# ifdef DATAVIEW
static i4 fix_flags();
GLOBALREF       MWSinfo IIMWVersionInfo;


/*{
** Name:	IIMWafAddForm	(X101) - creates a new forms-mode window.
**
** Description:
**	IIMWafAddForm creates a new forms window with alias given by
**	formAlias. It does not create forms-mode items: Use the
**	MWadd..Item commands to add items to a form.
**	
** Inputs:
**	formAlias	Integer (alias for form)
**	theFrame	FRAME (the frame data structure)
**
** Sends:
**	"X101	formAlias; posx, posy, maxx, maxy; flags; font, ptsz;
**		scrtype, mode; fldno, nsno, trimno; name"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWafAddForm(formAlias,theFrame)
i4  formAlias;
FRAME *theFrame;
{
	char params[512];

	_VOID_ STprintf(params,"%d;%d,%d,%d,%d;%x;%d,%d;%d,%d;%d,%d,%d;%s",
		formAlias,
		theFrame->frposx,
		theFrame->frposy,
		theFrame->frmaxx,
		theFrame->frmaxy,
		theFrame->frmflags,
		0,0,			/* default font and point size */
		theFrame->frscrtype,
		theFrame->frmode,
		theFrame->frfldno,
		theFrame->frnsno,
		theFrame->frtrimno,
		theFrame->frname);
	return(IIMWpxcPutXCmd("X101", params, 0));
}

/*{
** Name:	IIMWtfTossForm (X102) - removes the forms-mode window.
**
** Description:
**	IIMWtfTossForm removes from the forms list the forms-mode window
**	whose alias is formAlias. The forms-mode window must have been
**	previously created with the IIMWafAddForm (X101) command.
**	
**	This form may or may not be visible on the screen. If the form is
**	visible, it will be removed from the screen.
**	
**	Use IIMWtfTossForm when a forms-mode window is no longer needed,
**	such as when the application recognizes, through a menu item
**	selection or other mechanism, that the user has finished
**	interacting with the form.
**	
**	An alternative to IIMWtfTossForm is IIMWhfHideForm (X104), which
**	simply removes the forms-mode window from the MacWorkStation
**	screen. This technique may be more efficient when the application
**	calls the form frequently.
**	
** Input:
**	formAlias	Integer (alias of form)
**
** Sends:
**	"X102	formAlias"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWtfTossForm(formAlias)
i4  formAlias;
{
	char params[8];

	_VOID_ STprintf(params,"%d",formAlias);
	return(IIMWpxcPutXCmd("X102", params, 0));
}

/*{
** Name:	IIMWsfShowForm (X103) - draws the hidden forms-mode window.
**
** Description:
**	IIMWsfShowForm draws on the screen the hidden forms-mode window
**	whose alias is formAlias. If formAlias is already visible,
**	ShowForm does nothing.
**	
** Input:
**	formAlias	Integer (alias of form)
**
** Sends:
**	"X103	formAlias"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWsfShowForm(formAlias)
i4  formAlias;
{
	char params[8];

	_VOID_ STprintf(params,"%d",formAlias);
	return(IIMWpxcPutXCmd("X103", params, 0));
}

/*{
** Name:	IIMWhfHideForm (X104) - removes the forms window from the screen.
**
** Description:
**	IIMWhfHideForm removes from the screen the forms-mode window whose
**	alias is formAlias. If the form is already hidden, IIMWhfHideForm
**	does nothing.
**	
**	The form remains in the forms-mode window list and may be reshown
**	by calling IIMWsfShowForm (X103) or IIMWsfSelectForm (X105) commands.
**	
**	Note: You can use an alias of zero to refer to the front-most
**	forms-mode window.
**	
** Input:
**	formAlias	Integer (alias of form)
**
** Sends:
**	"X104	formAlias"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWhfHideForm(formAlias)
i4  formAlias;
{
	char params[8];

	_VOID_ STprintf(params,"%d",formAlias);
	return(IIMWpxcPutXCmd("X104", params, 0));
}

/*{
** Name:	IIMWffFrontForm (X105)	- brings the forms window to the front.
**
** Description:
**	IIMWffFrontForm brings the forms-mode window whose alias is
**	formAlias to the front of the MacWorkStation screen. If the form
**	is currently hidden, it will become visible.
**	
** Input:
**	formAlias	Integer (alias of form)
**
** Sends:
**	"X105	formAlias"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWffFrontForm(formAlias)
i4  formAlias;
{
	char params[8];

	_VOID_ STprintf(params,"%d",formAlias);
	return(IIMWpxcPutXCmd("X105", params, 0));
}

/*{
** Name:	IIMWsftSaveFldsText (X106) - save the text values of
**						all fields in the form.
**
** Description:
**	In the range mode of a form, the application may require
**	the saving or restoring of the texts of all fields in the
**	form.  IIMWsftSaveFldsText sends a commmand to MWS telling
**	it to save the text values of all the fields.
**	
** Input:
**	formAlias	Integer (alias of form)
**
** Sends:
**	"X106	formAlias"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05-dec-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWsftSaveFldsText(formAlias)
i4	formAlias;
{
	char	params[8];

	_VOID_ STprintf(params, "%d", formAlias);
	return(IIMWpxcPutXCmd("X106", params, 0));
}

/*{
** Name:	IIMWrftRestoreFldsText (X107) - restore the text values of
**						all fields in the form.
**
** Description:
**	In the range mode of a form, the application may require
**	the saving or restoring of the texts of all fields in the
**	form.  IIMWrftRestoreFldsText sends a commmand to MWS
**	telling it to restore the text values of all the fields.
**	
** Input:
**	formAlias	Integer (alias of form)
**
** Sends:
**	"X107	formAlias"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05-dec-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWrftRestoreFldsText(formAlias)
i4	formAlias;
{
	char	params[8];

	_VOID_ STprintf(params, "%d", formAlias);
	return(IIMWpxcPutXCmd("X107", params, 0));
}

/*{
** Name:	IIMWfftFreeFldsText (X108) - discard the structures used
**						to save the text values.
**
** Description:
**	In the range mode of a form, the application may require
**	the saving or restoring of the texts of all fields in the
**	form.  IIMWfftFreeFldsText sends a commmand to MWS
**	telling it to discard the structures used to save the
**	text values.
**	
** Input:
**	formAlias	Integer (alias of form)
**
** Sends:
**	"X108	formAlias"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05-dec-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWfftFreeFldsText(formAlias)
i4	formAlias;
{
	char	params[8];

	_VOID_ STprintf(params, "%d", formAlias);
	return(IIMWpxcPutXCmd("X108", params, 0));
}

/*{
** Name:	IIMWafiAddFieldItem (X111) - add a field item to the form.
**
** Description:
**	IIMWafiAddFieldItem adds an item to the forms-mode window whose
**	alias is formAlias. The alias of the item is assigned using the
**	value of itemAlias. Subsequent references to this item can be
**	made by using the value of formAlias and itemAlias. If an item
**	with the given alias already exists, the existing item is deleted
**	and the item is recreated having the attributes given in the
**	latest AddFieldItem Message.
**	
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of item)
**	theField	REGFLD * (pointer to info about field)
**	
** Sends:
**	"X111	formAlias, itemAlias; posx, posy, maxx, maxy; flags;
**		font, ptsz; titx, tity; datatype, datax, datay, length,
**		width, dataln; title; text"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWafiAddFieldItem(formAlias, itemAlias, theField)
i4  formAlias;
i4  itemAlias;
REGFLD *theField;
{
	FLDHDR	*theHdr;
	FLDTYPE *theType;
	FLDVAL	*theVal;
	char	 params[mwMAX_MSG_LEN + 1];
	i4	 flags;

	theHdr = &theField->flhdr;
	theType = &theField->fltype;
	theVal = &theField->flval;
	flags = fix_flags(theHdr, TRUE, FALSE);

	_VOID_ STprintf(params,
		"%d,%d;%d,%d,%d,%d;%x;%d,%d;%d,%d;%d,%d,%d,%d,%d,%d;%s;%.*s",
		formAlias, itemAlias,
		theHdr->fhposx,
		theHdr->fhposy,
		theHdr->fhmaxx,
		theHdr->fhmaxy,
		flags,
		theHdr->fhdfont,
		theHdr->fhdptsz,
		theHdr->fhtitx,
		theHdr->fhtity,
		theType->ftdatatype,	/* text data type */
		theType->ftdatax,	/* pos of data string in characters */
		theVal->fvdatay,	/* pos of data string in characters */
		theType->ftlength,	/* length of data type in characters */
		theType->ftwidth,	/* width of field in characters */
		theType->ftdataln,	/* max line length for data in chars */
		theHdr->fhtitle,	/* textual value of field title */
		((DB_TEXT_STRING*)theVal->fvdsdbv->db_data)->db_t_count,
					/* length of text of field data */
		theVal->fvbufr);	/* textual value of field data */

	return(IIMWpxcPutXCmd("X111", params, 0));
}

/*{
** Name:	IIMWdfiDeleteFormItem (X112)	- delete item from the form.
**
** Description:
**	IIMWdfiDeleteFormItem allows the host to delete items from the
**	forms-mode window whose alias is formAlias. ItemAlias specifies
**	the alias of the item to be deleted.
**	
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of item)
**
** Sends:
**	"X112	formAlias, itemAlias"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**		
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWdfiDeleteFormItem(formAlias,itemAlias)
i4  formAlias,itemAlias;
{
	char params[8];

	_VOID_ STprintf(params,"%d,%d",formAlias,itemAlias);
	return(IIMWpxcPutXCmd("X112", params, 0));
}

/*{
** Name:	IIMWsiShowItem (X113) - redisplay hidden items in the form.
**
** Description:
**	IIMWsiShowItem allows the host to redisplay items in the
**	forms-mode window whose alias is formAlias that were previously
**	hidden by a IIMWhiHideItem message. ItemAlias specifies the alias
**	of the item to be deleted. If the specified item is not hidden
**	when this message is received, then IIMWsiShowItem does nothing.
**	
** Inputs:
**	formAlias	alias of form
**	itemAlias	alias of item
**	col		column number
**
** Sends:
**	"X113	formAlias, itemAlias, col"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	09/12/90 (nasser) - Added column to the message.
*/
STATUS
IIMWsiShowItem(formAlias, itemAlias, col)
i4	formAlias;
i4	itemAlias;
i4	col;
{
    char	params[16];

    _VOID_ STprintf(params, "%d,%d,%d", formAlias, itemAlias, col + 1);
    return(IIMWpxcPutXCmd("X113", params, 0));
}

/*{
** Name:	IIMWhiHideItem (X114)	- hide items in the forms-mode window.
**
** Description:
**	IIMWhfHideForm Item allows the host to hide items in the
**	forms-mode window whose alias is formAlias. ItemAlias specifies
**	the alias of the item to be hidden. 
**	
** Inputs:
**	formAlias	alias of form
**	itemAlias	alias of item
**	col		column number
**
** Sends:
**	"X114	formAlias, itemAlias, col"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	09/12/90 (nasser) - Added column to the message.
*/
STATUS
IIMWhiHideItem(formAlias, itemAlias, col)
i4	formAlias;
i4	itemAlias;
i4	col;
{
    char	params[16];

    _VOID_ STprintf(params, "%d,%d,%d", formAlias, itemAlias, col + 1);
    return(IIMWpxcPutXCmd("X114", params, 0));
}

/*{
** Name: IIMWsiaSetItemAttr (X115) - change the attributes of the specified item.
**
** Description:
**	IIMWsiaSetItemAttr changes the value of the specified attributes of
**	an item. The form containing the item is given by formAlias. The
**	alias of the item is assigned using the value of itemAlias. 
**	
** Input command:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of item(s))
**	column		Integer (index of column, if a table)
**	row		Integer (index of row, if a table)
**	attributes	Compound (list of display attributes)
**
** Sends:
**	"X115	formAlias, itemAlias; column, row; attributes"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWsiaSetItemAttr(formAlias,itemAlias,column,row,flags)
i4	formAlias;
i4	itemAlias;
i4	column;
i4	row;
i4	flags;
{
	char params[32];

	_VOID_ STprintf(params, "%d,%d;%d,%d;%x",
		formAlias, itemAlias, column + 1, row + 1, flags);
	return(IIMWpxcPutXCmd("X115", params, 0));
}

/*{
** Name:	IIMWatiAddTableItem (X116) - add a table item to the form.
**
** Description:
**	IIMWatiAddTableItem adds table field to the forms-mode
**	window whose alias is formAlias. The alias of the item
**	is assigned using the value of itemAlias. Subsequent
**	references to this item can be made by using the value
**	of formAlias and itemAlias. If an item with the given
**	alias already exists, the existing item is deleted and
**	the item is recreated having the attributes given in the
**	latest AddFieldItem Message.
**	
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of item)
**	theTable	TBLFLD * (pointer to info about the table)
**	
** Sends:
**	"X116	formAlias, itemAlias; posx, posy, maxx, maxy; flags;
**		font, ptsz; titx, tity; rows, currow, cols, curcol;
**		start, width, lastrow; title"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	10/06/89 (nasser)
**		Included this header.
*/
STATUS
IIMWatiAddTableItem(formAlias, itemAlias, theTable)
i4  formAlias;
i4  itemAlias;
TBLFLD *theTable;
{
	FLDHDR	*theHdr;	
	char	 params[512];
	i4	 flags;

	theHdr = &theTable->tfhdr;
	flags = fix_flags(theHdr, TRUE, TRUE);

	_VOID_ STprintf(params,
		"%d,%d;%d,%d,%d,%d;%x;%d,%d;%d,%d;%d,%d,%d,%d;%d,%d,%d;%s",
		formAlias, itemAlias,
		theHdr->fhposx,
		theHdr->fhposy,
		theHdr->fhmaxx,
		theHdr->fhmaxy,
		flags,
		theHdr->fhdfont,
		theHdr->fhdptsz,
		theHdr->fhtitx,
		theHdr->fhtity,
		theTable->tfrows,	/* number of rows, if table */
		theTable->tfcurrow + 1,	/* selected row, if table */
		theTable->tfcols,	/* number of columns, if table */
		theTable->tfcurcol + 1,	/* selected column, if table */
    		theTable->tfstart + 1,	/* line number of start of data rows */
    		theTable->tfwidth,	/* number of physical lines per row */
    		theTable->tflastrow + 1,/* row nbr of last actual row displayed */
		theHdr->fhtitle);	/* textual value of table title */
	return(IIMWpxcPutXCmd("X116", params, 0));
}

/*{
** Name:	IIMWaciAddColumnItem (X117)- add a column item to a table.
**
** Description:
**	IIMWaciAddColumnItem adds a column to a table item.
**	
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of table item)
**	column		Integer (column number)
**	theColumn	FLDCOL * (pointer to info about column)
**	
** Sends:
**	"X117	formAlias, itemAlias, col; posx, posy, maxx, maxy; flags;
**		font, ptsz; titx, tity; datatype, datax, length,
**		width, dataln; title"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	10/06/89 (nasser)
**		Included this header.
*/
STATUS
IIMWaciAddColumnItem(formAlias, itemAlias, column, theColumn)
i4  formAlias;
i4  itemAlias;
i4  column;
FLDCOL *theColumn;
{
	FLDHDR	*theHdr;
	FLDTYPE *theType;
	char	 params[512];
	i4	 flags;

	theHdr = &theColumn->flhdr;
	theType = &theColumn->fltype;
	flags = fix_flags(theHdr, TRUE, FALSE);

	_VOID_ STprintf(params,
		"%d,%d,%d;%d,%d,%d,%d;%x;%d,%d;%d,%d;%d,%d,%d,%d,%d;%s",
		formAlias, itemAlias, column + 1,
		theHdr->fhposx,
		theHdr->fhposy,
		theHdr->fhmaxx,
		theHdr->fhmaxy,
		flags,
		theHdr->fhdfont,
		theHdr->fhdptsz,
		theHdr->fhtitx,
		theHdr->fhtity,
		theType->ftdatatype,	/* text data type */
		theType->ftdatax,	/* pos of data string in chars */
		theType->ftlength,	/* length of data type in chars */
		theType->ftwidth,	/* width of field in characters */
		theType->ftdataln,	/* max line length for data in chars */
		theHdr->fhtitle);	/* text of column title */
	return(IIMWpxcPutXCmd("X117", params, 0));
}

/*{
** Name:	IIMWatrAddTRim (X118) - add a trim item to the form.
**
** Description:
**	IIMWatrAddTRim adds a trim item to the forms-mode
**	window whose alias is formAlias. The alias of the
**	tirm item is assigned using the value of itemAlias.
**	Subsequent references to this item can be made by
**	using the value of formAlias and itemAlias. If an
**	item with the given alias already exists, the
**	existing item is deleted and the item is recreated.
**	
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of item)
**	theTrim		TRIM * (pointer to info about trimfield)
**	
** Sends:
**	"X118	formAlias, itemAlias; trmx, trmy; flags; font, ptsz; text"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	10/06/89 (nasser)
**		Included this header.
*/
STATUS
IIMWatrAddTRim(formAlias, itemAlias, theTrim)
i4  formAlias;
i4  itemAlias;
TRIM *theTrim;
{
	char params[512];

	_VOID_ STprintf(params,"%d,%d;%d,%d;%x;%d,%d;%s",
		formAlias, itemAlias,
		theTrim->trmx,
		theTrim->trmy,
		theTrim->trmflags,
		theTrim->trmfont,
		theTrim->trmptsz,
		theTrim->trmstr);	/* textual value of trim */
	return(IIMWpxcPutXCmd("X118", params, 0));
}

/*{
** Name:	IIMWatcAddTableCell (X119) - add a cell to a table.
**
** Description:
**	IIMWatcAddTableCell adds a cell to a table item in
**	the forms-mode window whose alias is formAlias. The
**	alias of the table item is itemAlias.
**	
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of item)
**	column		Integer (column number of cell)
**	row		Integer (row number of cell)
**	flags		Integer	
**	theVal		FLDVAL * (pointer to info about cell)
**	
** Sends:
**	"X119	formAlias, itemAlias; column, row; flags; text"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	10/06/89 (nasser)
**		Added this header.
*/
STATUS
IIMWatcAddTableCell(formAlias,itemAlias,column,row,flags,theVal)
i4  formAlias,itemAlias,column,row;
i4 flags;
FLDVAL *theVal;
{
	char params[mwMAX_MSG_LEN + 1];

	_VOID_ STprintf(params, "%d,%d;%d,%d;%x;%.*s",
		formAlias, itemAlias, column + 1, row + 1, flags,
		((DB_TEXT_STRING*)theVal->fvdsdbv->db_data)->db_t_count,
					/* length of text of field data */
		theVal->fvbufr);	/* textual value of field data */
	return(IIMWpxcPutXCmd("X119", params, 0));
}

/*{
** Name:	IIMWgitGetItemText (X120) - get the text of an item.
**
** Description:
**	IIMWgitGetItemText retrieves the textual value of an item.
**	FormAlias and itemAlias provide the forms-mode window alias
**	and item alias, respectively. If the item is of type TABLE,
**	then the title string of the table is returned.
**	
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of item)
**
** Sends:
**	"X120	formAlias, itemAlias"
** Receives:
**	"X420	formAlias, itemAlias; text"
**			formAlias	Integer (alias of form)
**			itemAlias	Integer (alias of item)
**			text		String (value or title)
**
** Outputs:
**	text		value or title
**	Returns:
**		OK/FAIL
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWgitGetItemText(formAlias,itemAlias,text)
i4	formAlias;
i4	itemAlias;
u_char *text;
{
	char params[256];
	TPMsg theMsg;

	_VOID_ STprintf(params,"%d,%d", formAlias, itemAlias);
	if ((IIMWpxcPutXCmd("X120", params, mwMSG_FLUSH) == OK) &&
		((theMsg = IIMWgseGetSpecificEvent('X',420)) != (TPMsg) NULL) &&
		(IIMWmgdMsgGetDec(theMsg, &formAlias, ',') == OK) &&
		(IIMWmgdMsgGetDec(theMsg, &itemAlias, ';') == OK) &&
		(IIMWmgsMsgGetStr(theMsg, (char *) text, NULLCHAR) == OK))
	{
		return(OK);
	}
	else
	{
		return(FAIL);
	}
}

/*{
** Name:	IIMWsitSetItemText (X121)	- set the text of an item.
**
** Description:
**	IIMWsitSetItemText sets the textual value of an item. FormAlias
**	and itemAlias provide the forms-mode window alias and item
**	alias, respectively. If the item is of type TABLE, then the
**	title string of the table is set.
**	
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of item)
**	count		Integer (number of characters to send)
**	text		StringPtr (text string)
**
** Sends:
**	"X121	formAlias, itemAlias; text"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWsitSetItemText(formAlias,itemAlias,count,text)
i4	 formAlias;
i4	 itemAlias;
u_i4	 count;
u_char	*text;
{
	char params[mwMAX_MSG_LEN + 1];

	_VOID_ STprintf(params,"%d,%d;%.*s",formAlias,itemAlias, count, text);
	return(IIMWpxcPutXCmd("X121", params, 0));
}

/*{
** Name: IIMWgciGetCurItem (X122) - find the currently selected item.
**
** Description:
**	IIMWgciGetCurItem allows the host to determine the currently
**	selected item of a form. FormAlias provides the forms-mode
**	window alias.  If there is no currently selected item, then
**	the itemAlias returned will be zero. A GetCurItemResp (X422)
**	event is sent in response.
**	
** Inputs:
**	formAlias	&Integer (alias of form)
**
** Sends:
**	"X122	formAlias"
** Receives:
**	"X422	formAlias, itemAlias; column, row"
**			formAlias	Integer (alias of form)
**			itemAlias	Integer (alias of item)
**			column		Integer (index of column, if a table)
**			row		Integer (index of row, if a table)
**
** Outputs:
**	formAlias	alias of form
**	itemAlias	alias of item
**	column		index of column, if a table
**	row		index of row, if a table
**	Returns:
**		OK/FAIL.
**
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWgciGetCurItem(formAlias, itemAlias, column, row)
i4  *formAlias,*itemAlias,*column,*row;
{
	char	params[256];
	TPMsg	theMsg;

	_VOID_ STprintf(params,"%d",*formAlias);
	if ((IIMWpxcPutXCmd("X122", params, mwMSG_FLUSH) == OK) &&
		((theMsg = IIMWgseGetSpecificEvent('X',422)) != (TPMsg) NULL) &&
		(IIMWmgdMsgGetDec(theMsg, formAlias, ',') == OK) &&
		(IIMWmgdMsgGetDec(theMsg, itemAlias, ';') == OK) &&
		(IIMWmgdMsgGetDec(theMsg, column, ',') == OK) &&
		(IIMWmgdMsgGetDec(theMsg, row, NULLCHAR) == OK))
	{
		(*column)--;
		(*row)--;
		return(OK);
	}
	else
	{
		return(FAIL);
	}
}

/*{
** Name: IIMWsciSetCurItem (X123) - select the current form item.
**
** Description:
**	IIMWsciSetCurItem allows the host to set the current form item
**	The item that was previously selected is deselected as this
**	message is processed. FormAlias and itemAlias provide the
**	forms-mode window alias and item alias, respectively.
**
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of item)
**	frm		FRAME	(Info for the frame)
**	fld		REGFLD	(Info for the field)
**
** Sends:
**	"X123	formAlias, itemAlias; frmMode; frmFlags, itemFlags"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWsciSetCurItem(formAlias, itemAlias, frm, fld)
i4	 formAlias;
i4	 itemAlias;
FRAME	*frm;
REGFLD	*fld;
{
	i4	 item_flags;
	char	 params[32];

	/* Turn off flags MWS doesn't need and move flags from fhd2flags */
	item_flags = fix_flags(&fld->flhdr, FALSE, FALSE);

	_VOID_ STprintf(params,"%d,%d;%d;%x,%x",formAlias,itemAlias,
		frm->frmode, frm->frmflags, item_flags);
	return(IIMWpxcPutXCmd("X123", params, 0));
}

/*{
** Name: IIMWgttGetTbcellText (X124) - retrieve the text of a table cell.
**
** Description:
**	IIMWgttGetTbcellText retrieves the textual value of a field in
**	a table item. FormAlias and itemAlias provide the forms-mode
**	window alias and item alias, respectively. The values of column
**	and row are 1-based (the first row is row 1) and identify the
**	specific field whose textual value is to be returned. If the 
**	value of row is zero, then the title string of the table is
**	returned. If the specified field does not exist a null string
**	is returned.  If the row_nbr_abs parameter is TRUE, then we
**	are dealing with a range mode table, and the row number is
**	absolute.
**	If the table cell specified has null text, then the next cell
**	in the table with a non-null value is returned.
**	
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of item)
**	column		Integer * (index of column)
**	row		Integer * (index of row)
**	row_nbr_abs	Integer (TRUE if range mode table)
**
** Sends:
**	"X124	formAlias, itemAlias; column, row; row_nbr_abs"
**Receives:
**	"X424	formAlias, itemAlias; column, row; text"
**			formAlias	Integer (alias of form)
**			itemAlias	Integer (alias of item)
**			column		Integer (index of column, if a table)
**			row		Integer (index of row, if a table)
**			text		String	(value)
**
** Outputs:
**	text		value
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
i4
IIMWgttGetTbcellText(formAlias, itemAlias, column, row, row_nbr_abs, text)
i4	formAlias;
i4	itemAlias;
i4     *column;
i4     *row;
i4	row_nbr_abs;
u_char *text;
{
	char	params[256];
	TPMsg	theMsg;

	_VOID_ STprintf(params,"%d,%d;%d,%d;%d", formAlias, itemAlias,
		*column + 1, *row + 1, row_nbr_abs);
	if ((IIMWpxcPutXCmd("X124", params, mwMSG_FLUSH) == OK) &&
		((theMsg = IIMWgseGetSpecificEvent('X',424)) != (TPMsg) NULL) &&
		(IIMWmgdMsgGetDec(theMsg, &formAlias, ',') == OK) &&
		(IIMWmgdMsgGetDec(theMsg, &itemAlias, ';') == OK) &&
		(IIMWmgdMsgGetDec(theMsg, column, ',') == OK) &&
		(IIMWmgdMsgGetDec(theMsg, row, ';') == OK) &&
		(IIMWmgsMsgGetStr(theMsg, (char *) text, NULLCHAR) == OK))
		/* not always null-terminated */
	{
		(*column)--;
		(*row)--;
		return(OK);
	}
	else
	{
		return(FAIL);
	}
}

/*{
** Name: IIMWsttSetTbcellText (X125) - set the text of a cell in a table.
**
** Description:
**	IIMWsttSetTbcellText sets the textual value of a cell in a table
**	item. FormAlias and itemAlias provide the forms-mode window
**	alias and item alias, respectively. The values of column and row
**	are 1-based (the first row is row 1) and identify the specific
**	field whose textual value is to be changed. Text is the textual
**	value to which the item should be changed. If the value of row is 
**	zero, the title string of the table is set. The new value is
**	displayed according to the display attributes of the table item.
**	If the specified field does not exist, then IIMWsttSetTbcellText
**	does nothing.
**	
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of item)
**	column		Integer (index of column)
**	row		Integer (index of row)
**	count		Integer (number of characters to send)
**	text		String	(textual value)
**
** Sends:
**	"X125	formAlias, itemAlias; column, row; text"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWsttSetTbcellText(formAlias,itemAlias,column,row,count,text)
i4	 formAlias;
i4	 itemAlias;
i4	 column;
i4	 row;
u_i4	 count;
u_char	*text;
{
	char params[mwMAX_MSG_LEN + 1];

	_VOID_ STprintf(params, "%d,%d;%d,%d;%.*s",
		formAlias, itemAlias, column + 1, row + 1, count, text);
	return(IIMWpxcPutXCmd("X125", params, 0));
}

/*{
** Name: IIMWgctGetCurTbcell (X126) - find the current field of a table.
**
** Description:
**	IIMWgctGetCurTbcell allows the host to determine the currently
**	selected cell of a table item. FormAlias and itemAlias
**	provide the forms-mode window alias and item alias,
**	respectively. If there is no currently selected cell, then the 
**	row and column number returned will both be zero. If the
**	rowHilite attribute of the table is TRUE, then the column
**	number returned will be zero and the entire row is selected.
**	
** Inputs:
**	formAlias	&Integer (alias of form)
**	itemAlias	&Integer (alias of item)
**
** Sends:
**	"X126	formAlias, itemAlias"
** Receives:
**	"X426	formAlias, itemAlias; column, row"
**			formAlias	Integer (alias of form)
**			itemAlias	Integer (alias of item)
**			column		Integer (column index of field)
**			row		Integer (row index of field)
**
** Outputs:
**	formAlias	alias of form
**	itemAlias	alias of item
**	column		column index of field
**	row		row index of field
**	Returns:
**		OK/FAIL.
**	Exceptions:
**	None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWgctGetCurTbcell(formAlias,itemAlias,column,row)
i4  *formAlias,*itemAlias,*column,*row;
{
	char params[256];
	TPMsg theMsg;

	_VOID_ STprintf(params,"%d,%d",*formAlias,*itemAlias);
	if ((IIMWpxcPutXCmd("X126", params, mwMSG_FLUSH) == OK) &&
		((theMsg = IIMWgseGetSpecificEvent('X',426)) != (TPMsg) NULL) &&
		(IIMWmgdMsgGetDec(theMsg, formAlias, ',') == OK) &&
		(IIMWmgdMsgGetDec(theMsg, itemAlias, ';') == OK) &&
		(IIMWmgdMsgGetDec(theMsg, column, ',') == OK) &&
		(IIMWmgdMsgGetDec(theMsg, row, NULLCHAR) == OK))
	{
		(*column)--;
		(*row)--;
		return(OK);
	}
	else
	{
		return(FAIL);
	}
}

/*{
** Name: IIMWsctSetCurTbcell (X127) - select the current cell of a TABLE item.
**
** Description:
**	IIMWsctSetCurTbcell allows the host to set the currently
**	selected field of a TABLE item. The item that was previously
**	selected is deselected as this message is processed. FormAlias
**	and itemAlias provide the forms-mode window alias and item
**	alias, respectively. The values of column and row are 1-based
**	(the first row is row 1) and identify the specific field that
**	is to be selected. If the rowHilite attribute of the table is
**	TRUE, then column of this message is ignored.
**	
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of item)
**	frm		FRAME	(Info for the frame)
**	tbl		TBLFLD	(Info for the table field)
**
** Sends:
**	"X127	formAlias, itemAlias; column, row; lastrow;
**		frmMode; frmFlags, tblFlags, colFlags"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWsctSetCurTbcell(formAlias, itemAlias, frm, tbl)
i4	 formAlias;
i4	 itemAlias;
FRAME	*frm;
TBLFLD	*tbl;
{
	i4	 tbl_flags;
	FLDCOL	*colstr;
	i4	 col_flags;
	char	 params[256];

	tbl_flags = fix_flags(&(tbl->tfhdr), FALSE, TRUE);
	colstr = tbl->tfflds[tbl->tfcurcol];
	col_flags = fix_flags(&(colstr->flhdr), FALSE, FALSE);
	_VOID_ STprintf(params, "%d,%d;%d,%d;%d;%d;%x,%x,%x",
		formAlias, itemAlias, tbl->tfcurcol + 1, tbl->tfcurrow + 1,
		tbl->tflastrow + 1, frm->frmode,
		frm->frmflags, tbl_flags, col_flags);
	return(IIMWpxcPutXCmd("X127", params, 0));
}

/*{
** Name:  IIMWatgAddTblGroup (X128) -	add a group of table cells
**
** Description:
**	Typically when a table is built, most of the cells are
**	empty and have the same flags.  IIMWatgAddTblGroup
**	combines cells with the same flags and no text and
**	sends one message to MWS.
**
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of table)
**	start_col	Integer (index of start column)
**	start_row	Integer (index of start row)
**	end_col		Integer (index of end column)
**	end_row		Integer (index of end row)
**	flags		Hex	(flags for the fields)
**
** Sends:
**	"X128	formAlias, itemAlias; start_col, start_row;
**		end_col, end_row; flags"
**
** Outputs:
** 	Returns:
**		OK/FAIL.
** 	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08-nov-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWatgAddTblGroup(formAlias, itemAlias, start_col, start_row,
	end_col, end_row, flags)
i4	formAlias;
i4	itemAlias;
i4	start_col;
i4	start_row;
i4	end_col;
i4	end_row;
i4	flags;
{
	char	params[256];

	_VOID_ STprintf(params, "%d,%d;%d,%d;%d,%d;%x",
		formAlias, itemAlias,
		start_col + 1, start_row + 1, end_col + 1, end_row + 1,
		flags);
	return(IIMWpxcPutXCmd("X128", params, 0));
}

/*{
** Name:  IIMWctgClearTblGroup (X129) -	clear a group of table cells
**
** Description:
**	Frequently, INGRES clears a group of cells in a table.
**	IIMWctgClearTblGroup sends one message to MWS to
**	clear multiple cells.
**
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of table)
**	start_col	Integer (index of start column)
**	start_row	Integer (index of start row)
**	end_col		Integer (index of end column)
**	end_row		Integer (index of end row)
**
** Sends:
**	"X129	formAlias, itemAlias; start_col, start_row;
**		end_col, end_row"
**
** Outputs:
** 	Returns:
**		OK/FAIL.
** 	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08-nov-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWctgClearTblGroup(formAlias, itemAlias, start_col, start_row,
	end_col, end_row)
i4	formAlias;
i4	itemAlias;
i4	start_col;
i4	start_row;
i4	end_col;
i4	end_row;
{
	char	params[256];

	_VOID_ STprintf(params, "%d,%d;%d,%d;%d,%d",
		formAlias, itemAlias,
		start_col + 1, start_row + 1, end_col + 1, end_row + 1);
	return(IIMWpxcPutXCmd("X129", params, 0));
}

/*{
** Name:	IIMWstfScrollTblFld (X131) - scroll a table up/down.
**
** Description:
**	Scroll a form table up or down. 
**
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of item(s))
**	count		Integer (number of rows to scroll)
**				(+ive if up, -ive if down)
**
** Sends:
**	"X131	formAlias, itemAlias; count"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08-nov-1989 (nasser)
**		Initial definition.
*/
STATUS
IIMWstfScrollTblFld(formAlias, itemAlias, count)
i4	formAlias;
i4	itemAlias;
i4	count;
{
	char	params[64];

	_VOID_ STprintf(params, "%d,%d;%d", formAlias, itemAlias, count);
	return(IIMWpxcPutXCmd("X131", params, 0));
}

/*{
** Name:	 IIMWprfPRstFld	-	Reset a form item.
**
** Description:
**	Reset a form item to original form attributes. Put cursor a
**	the left and justify the text. 
**	
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of item(s))
**	column		Integer (index of column, if a table)
**	row		Integer (index of row, if a table)
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWprfPRstFld(formAlias,itemAlias,column,row)
i4  formAlias,itemAlias,column,row;
{
	IIMWuaUnusedArg(formAlias, itemAlias, row, column);

	return(OK);	/** does nothing for now **/
}

/*{
** Name:  fix_flags -- combine tow flag words into one and remove
**			irrelevant flags.
**
** Description:
**	Some flags that are used by the front-ends are irrelevant
**	for MWS.  The purpose of this function is to reset these
**	flags in the flag word.  Also, this function combines
**	the two flag words in the header into one.
**
** Inputs:
**	hdr		pointer to header info
**	dispattr	if true, send dispattr part of the flags
**
** Outputs:
** 	Returns:
**		the modified flag.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	15-nov-89 (nasser)
**		Initial definition.
*/
static i4
fix_flags(hdr, dispattr, tblflags)
FLDHDR	*hdr;
i4	 dispattr;
i4	 tblflags;
{
	i4	flags;
	i4	mask;

	/* Turn off flags that MWS does not need */
	/* Bits that are not used at all or are reassigned */
	mask = ~mw2FLAGS & mwVALID;
	/* Turn off dispattr bits if so requested */
	if ( ! dispattr)
		mask &= ~fdDISPATTR;
	/* If table flags, put back table field flags */
	if (tblflags)
		mask |= fdtfMASK;
	flags = hdr->fhdflags & mask;

	/* Move flags from fhd2flags */
	if (hdr->fhd2flags & fdSCRLFD)
		flags |= mwSCRLFD;
	if (hdr->fhd2flags & fdREADONLY)
		flags |= mwREADONLY;
	
	return(flags);
}
# endif /* DATAVIEW */


/*{
** Name: IIMWssbSendScrollButton - Send Scroll Button position   
**
** Description:
**	Sends MWS message with info  needed to refresh the postion of a scroll 
**	bar button subsequent to a scrolling operation.
**
** Inputs:
**	formAlias	Integer (alias of form)
**	itemAlias	Integer (alias of table)
**	cur		Ordinal 1-based index of current row at top of display.
**	tot		Total number of rows in dataset.
**
** Outputs:
**	Sends X200 message.
**
**	Returns:
**		
**	VOID
**
** History:
**	18-apr-97 (cohmi01)   
**	    Added for Refresh of Scroll Button position (bug 81574).
*/

IIMWssbSendScrollButton(
i4	formAlias,
i4	itemAlias,
i4	cur,
i4	tot)
{
    char	params[64];

    /* Send X200 only if this new msg is supported */
    if (IIMWVersionInfo.version >= MWSVER_32)
    {
	_VOID_ STprintf(params, "%d,%d;%d,%d", formAlias, itemAlias, cur, tot);
	return(IIMWpxcPutXCmd("X200", params, 0));
    }
}
