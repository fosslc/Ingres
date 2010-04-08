/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: icehtml.c
**
** Description:
**      HTML related macros and structs.
**
** History:
**      01-dec-1996 (wilan06)
**          Created
**      21-feb-1997 (harpa06)
**          Kill the carriage returns and extra newline characters on all 
**          tags. The carriage returns add CTRL-M's to files which is incorrect.
**          The extra newline characters are not needed. Also fixed the <PRE>
**          tags.
**      21-mar-1997 (harpa06)
**          Added TAG_PARAGRAPH_BREAK. Added newline characters to some HTML 
**          tags
**      31-mar-1997 (harpa06)
**          Eliminated additional "\n\r" in TAG_HTML_DTD. This unnecessarily
**          increased the file size of any file that needs this HTML tag.
**      30-apr-1997 (harpa06)
**          Added TAG_SELECT, TAG_OPTION_WITH_ATTR, TAG_CENTER and 
**          END_TAG_CENTER.
**
**          Removed all '\r''s from definitions. They're not required.
**      20-jan-98 (harpa06)
**          Added TAG_PARAGRAPH_BREAK_NN
*/

# ifndef ICEHTML_H_INCLUDED
# define ICEHTML_H_INCLUDED

# define TAG_HTML               ERx("<HTML>\n")
# define TAG_END_HTML           ERx("</HTML>\n")

# define TAG_HEAD_TITLE         ERx("<HEAD><TITLE>")
# define TAG_END_HEAD_TITLE     ERx("</TITLE></HEAD>\n")

# define TAG_BODY               ERx("<BODY>\n")
# define TAG_END_BODY           ERx("</BODY>\n")

# define TAG_HEADER1            ERx("<H1><CENTER>\n")
# define TAG_END_HEADER         ERx("</CENTER></H1>\n")

# define TAG_HORZ_RULE          ERx("<HR size=5>\n")

# define TAG_PARAGRAPH          ERx("<P>\n")
# define TAG_PARAGRAPH_BREAK    ERx("<BR>\n")
# define TAG_PARAGRAPH_BREAK_NN ERx("<BR>")
# define TAG_PREFORM            ERx("<PRE>\n")
# define TAG_END_PREFORM        ERx("</PRE>\n")

# define TAG_HTML_DTD           ERx("<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\">\n")

# define TAG_COMMENT_BEGIN      ERx("<!--")
# define TAG_COMMENT_END        ERx("-->")

# define TAG_TABLE              ERx("<TABLE>\n")
# define TAG_TABLE_WITH_ATTR    ERx("<TABLE ")
# define TAG_END_TABLE          ERx("</TABLE>\n")

# define TAG_TABROW             ERx("<TR>\n")
# define TAG_TABROW_WITH_ATTR   ERx("<TR ")
# define TAG_END_TABROW         ERx("</TR>\n")

# define TAG_TABHEAD            ERx("<TH>\n")
# define TAG_TABHEAD_WITH_ATTR  ERx("<TH ")
# define TAG_END_TABHEAD        ERx("</TH>\n")

# define TAG_TABCELL            ERx("<TD>\n")
# define TAG_TABCELL_WITH_ATTR  ERx("<TD ")
# define TAG_END_TABCELL        ERx("</TD>\n")

# define TAG_SELECT             ERx ("<SELECT")
# define TAG_SELECT_WITH_ATTR   ERx("<SELECT ")
# define TAG_END_SELECT         ERx("</SELECT>\n")

# define TAG_CENTER             ERx("<CENTER>")
# define TAG_END_CENTER         ERx("</CENTER>")

# define TAG_OPTION             ERx("<OPTION>")
# define TAG_OPTION_WITH_ATTR   ERx("<OPTION ")
# define TAG_END_OPTION         ERx("</OPTION>\n")

# define TAG_IMGSRC             ERx("<IMG SRC=\"")
# define TAG_END_IMGSRC         ERx("\">")

# define END_TAG                ERx(">")

# define TAG_STRONG             ERx("<STRONG>")
# define  TAG_END_STRONG         ERx("</STRONG>")

# define TAG_LINK               ERx("<A HREF=\"")
# define TAG_END_LINK_URL       ERx("\">")
# define TAG_END_LINK           ERx("</A>")

/* don't use ERx to ensure that the preprocessor combines adjacent strings */
# define ATTR_BORDER            "border "
# define ATTR_DEF_CELLSPACING   "cellspacing=2 "

typedef enum tagHTMLHALIGN
{
    HALIGN_LEFT, HALIGN_RIGHT, HALIGN_CENTER
} HTMLHALIGN;

typedef enum tagHTMLVALIGN
{
    VALIGN_TOP, VALIGN_MIDDLE, VALIGN_BOTTOM
} HTMLVALIGN;

ICERES ICEHTMLPageOpen (PICECLIENT pic, char* pszTitle, char* pszHeader, char* pszMsg);
ICERES ICEHTMLPageClose (PICECLIENT pic);
ICERES ICEHTMLTableBegin (PICECLIENT pic, PICEHTMLFORMATTER pihf);
ICERES ICEHTMLTableEnd (PICECLIENT pic);
ICERES ICEHTMLTableRowBegin (PICECLIENT pic, PICEHTMLFORMATTER pihf,
                             bool fHeader);
ICERES ICEHTMLTableRowEnd (PICECLIENT pic, bool fHeader);
ICERES ICEHTMLTableCell (PICECLIENT pic, int col, char* data,
                         PICEHTMLFORMATTER pihf, bool fHeader);
ICERES ICEHTMLTableCellImage (PICECLIENT pic, int col, char* szFile,
                              PICEHTMLFORMATTER pihf, bool fHeader);
ICERES ICEHTMLSelectBegin (PICECLIENT pic, PICEHTMLFORMATTER pihf);
ICERES ICEHTMLSelectEnd (PICECLIENT pic);
ICERES ICEHTMLSelectRowBegin (PICECLIENT pic, PICEHTMLFORMATTER pihf,
                              bool fHeader);
ICERES ICEHTMLSelectRowEnd (PICECLIENT pic, bool fHeader);
ICERES ICEHTMLSelectItem (PICECLIENT pic, int col, char* data,
                          PICEHTMLFORMATTER pihf, bool fHeader);
ICERES ICEHTMLSelectItemBin (PICECLIENT pic, int col, char* file,
                             PICEHTMLFORMATTER pihf, bool fHeader);

# endif
