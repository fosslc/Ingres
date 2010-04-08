/*
**    Copyright (c) 2004 Ingres Corporation
*/
 
# include <compat.h>
# include <me.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <fe.h>
# ifdef FT3270
# include <ftblock.h>
# else
# include <ft.h>
# endif
# include <fmt.h>
# include <adf.h>
# include <frame.h>
# include <er.h>
# include <st.h>
 
/**
**    Name: FTstrtch.c  -  Routine to stretch and squeeze a form
**                         to make room for 3270 attribute bytes.
**
**    Description:
**       This file defines the following external procedure:
**
**       IIFTsffaStretchFormForAttrs   stretch and squeeze a form to
**                                     make room for 3270 attr bytes
**
**       This file defines the following internal procedure:
**
**       reposition_features   Reposition the features of a form
**       process_features      Process the features of a form
**       clear_area            Clear an area of a form
**       draw_chfeat           Draw a character feature
**       draw_vline            Draw a vertical line
**
**    History:
**       05/16/90 (esd) Created
**       06/01/90 (esd) Fix bug: vifred was displaying character string
**                      formats based on old width of scrollable field.
**       06/17/90 (esd) Fix bug: fields with null titles were being
**                      padded too much if fhtitx < fhdatax.
**       08/29/90 (esd) Fix bug: width of box wasn't being updated
**                      when the number of digits in width increased.
**       09/03/90 (esd) Cleanup for porting to full-duplex world
**                      (e.g. don't use FT3270-specific routines
**                      to get memory and write diagnostic messages).
**       09/12/90 (esd) Fix bug in process_features: ftyp->ftfmtstr
**                      string was being updated even the string
**                      was shared with other data structures.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
[@history_template@]...
**/
 
/*
** For FT3270 environments, the FT_DIAGn macros write
** diagnostic messages to "diagnstc.output", *if* it exists.
** They are defined in ftblock.h; they all cause a call to FTdiag.
** For non-FT3270 environments, there is no ftblock.h and no FTdiag;
** for these environments, we'll make these FT_DIAGn macros no-ops
** (for now, anyway).
*/
# ifndef FT3270
# define FT_DIAG0( msg )
# define FT_DIAG1( msg, a1 )
# define FT_DIAG2( msg, a1, a2 )
# define FT_DIAG3( msg, a1, a2, a3 )
# define FT_DIAG4( msg, a1, a2, a3, a4 )
# define FT_DIAG5( msg, a1, a2, a3, a4, a5 )
# endif  /* FT3270 */
 
/*
** Information pertaining to the repositioning of a cell in a form.
*/
typedef  struct
{
   /*
   ** Flags for cell (see #defines below)
   */
   i2 flags;
 
   /*
   ** Amount (or minimum amount) by which the cell
   ** will be repositioned (from its original position on the form)
   ** to the right (or left, if amount is negative).
   */
   i2 dx;
 
   /*
   ** Amount (or minimum amount) by which the vertical bar
   ** running thru the cell would be repostioned to the right.
   ** If a vertical bar runs thru the cell and it's visible
   ** (not obscured by another feature), then vbar_dx = dx.
   ** If no vertical bar runs thru the cell, vbar_dx is used
   ** as a work area by reposition_features.
   */
   i2 vbar_dx;
 
   /*
   ** Make structure size a nice power of 2 to improve performance
   ** and to make things line up nicely when displaying storage
   ** during problem analysis.
   */
   i2 slack;
 
}  POS_INFO;
 
/*
** Definitions of flags for the cell.
** They are arranged in such a way that it's easy to "reverse polarity"
** of the flags (swap top and bottom, left and right).
** Note that only bits in the mask 0x7F7F are used.
*/
 
/*
** Indicate if cell contains part of a "character" (non-box) feature
** (excluding optional parts: portions of horizontally scrollable fields
** beyond the first 8 bytes).
*/
# define CHFEAT   0x0101
 
/*
** Indicate if the cell is on the left (or right)
** edge of a character feature (including optional parts).
*/
# define CHFEATLX 0x0200
# define CHFEATRX 0x0002
 
/*
** If CHFEAT set, indicate if the cell is on the left (or right)
** edge of a required part of the feature.
*/
# define CHFEATL  0x0400
# define CHFEATR  0x0004
 
/*
** If CHFEATL or CHFEATR set, indicate if the cell is on the top
** (or bottom) edge of a required part of the feature.
*/
# define CHFEATT  0x0800
# define CHFEATB  0x0008
 
/*
** Indicate if a vertical bar or an endpoint of a horizontal line
** is visible in the cell, or would be visible if one were drawn there.
*/
# define VBARV    0x1010
 
/*
** Indicate if a vertical bar runs thru the cell
** or an endpoint of a horizontal line is present in the cell
** (whether visible or not).
*/
# define VBAR     0x2020
 
/*
** If VBAR set, indicate if a vertical bar runs thru the cell
** (as opposed to just the endpoint of a horizontal line).
** VBART indicates that a spoke pokes thru the top of the cell
** (and into the cell above: VBARB is set for the cell above).
** VBARB indicates that a spoke pokes thru the bottom of the cell
** (and into the cell below: VBART is set for the cell below).
*/
# define VBART    0x4000
# define VBARB    0x0040
 
/*
** Macro to "reverse the polarity" of the flags for a cell
** (swap top and bottom, left and right).
** Input: flags to be reversed.
** Returns: reversed flags.
*/
# define REVERSE_POLARITY( x ) ( ( (x) << 8 ) | ( (x) >> 8 ) )
 
/*
** External references
*/
FUNC_EXTERN VOID FTpanic();
 
/*
** Forward references
*/
static   VOID  reposition_features();
static   VOID  process_features();
static   VOID  clear_area();
static   VOID  draw_chfeat();
static   VOID  draw_vline();
 
/*{
**    Name: IIFTsffaStretchFormForAttrs - Stretch and squeeze a form to
**                                        make room for 3270 attr bytes
**    Description:
**       The features of the passed-in form are repositioned
**       as needed to make room for 3270 attribute bytes.
**       All features are left in their original vertical position,
**       and as close to their original horizontal position as possible.
**       The features that may be repositioned are field titles,
**       field data windows, trim strings, and vertical lines.
**
**       For the purposes of this routine, "lines" include
**       sides of boxes, including boxes drawn around table fields
**       and boxed regular fields, and the lines drawn
**       between columns and rows of a table field.
**
**       Endpoints of horizontal lines are considered to be a degenerate
**       sort of vertical line (a vertical line of minimum height).
**       Endpoints of a horizontal line move independently, so that
**       horizontal lines may change in length as well as position.
**
**       The form is "stretched" to make room for an attribute byte
**       whenever two visible features abut in the original form.
**       Exception: if two vertical lines abut in the original form,
**       they may continue to do so in the modified form.
**
**       The form may be "squeezed" in two ways:
**
**       Two or more spaces between features may be reduced
**       to a smaller number of spaces (but always at least one).
**
**       A horizontally scrollable field may have its data window
**       reduced in size to as few as 8 bytes.  (The number 8 was
**       chosen because it's kind of a "magic number" in the IBM world:
**       for many kinds of objects, names tend to be unique
**       on the first 8 characters).
**
**       An attempt is made to leave the form at its original size.
**       In the special case where the original form was 80 bytes wide
**       (a common width, since most full-duplex terminals will display
**       exactly 80 bytes across), the form will be made 79 bytes wide
**       if possible (since the implementation of FT for 3270s
**       will display only 79 characters across on a typical
**       80-byte-wide 3270 - one column is reserved for attributes).
**
**       Any pieces of vertical lines or endpoints of horizontal lines
**       that were visible in the original form will remain visible
**       in the modified form.  (Other features are always visible).
**
**       The physical order of features will not be changed; that is,
**       if feature A is to the left of feature B in the original form,
**       it will remain so in the modified form.  However,
**       if a vertical line is obscured by feature B in the original
**       form, it may wind up to the left or right of feature B
**       in the modified form, which will cause it to become visible.
**       For purposes of positioning vertical lines, the title and
**       data window of a field are treated as a single unit.
**       For example, in the case where the title is to the left
**       of the data window of a field, a vertical line that ran
**       thru the data window could wind up running thru the title,
**       or even wind up to the left of the title.
**
**       Horizontal lines and horizontally scrollable fields aside,
**       no features will change in size or shape.
**
**       If two vertical lines overlap or touch at their endpoints,
**       they will be moved as single unit, even if the point at which
**       they touch is not visible (i.e. covered by a field or trim).
**       As a special case: if an endpoint of a horizontal line falls
**       within a vertical line or coincides with its endpoint, the two
**       features will be moved as single unit, even if the endpoint
**       is not visible.
**
**       If the title of a regular field is not on the same line as
**       any part of the data window for the field, the title and
**       data window will be moved as single unit; that is, each will
**       retain its horizontal offset relative to the other.
**       (This restriction may be removed in a future release;
**       it was imposed to simplify the implementation: if the title
**       and data window move independently, it's harder to enforce
**       the rule that no other field or trim may abut the smallest
**       rectangle containing the title and data window).
**
**       If the title of a regular field *is* on the same line as
**       all or part of the data window for the field, the title and
**       data window will be treated as separate features:
**       it may be necessary to insert an attribute byte between them,
**       or it may be advantageous to reduce the number of spaces
**       between them.
**
**    Inputs:
**       frm         pointer to FRAME structure for form to be stretched
**
**    Outputs:
**       various fields associated with frame structure may be updated:
**
**       for the frame:       frmaxx
**       for fields and cols: fhposx, fhmaxx, fhtitx, ftdatax, ftdataln
**       for trim and boxes:  trmx, and for boxes, the height and width
**                            encoded in trmstr (if the encoded string
**                            got longer, a new trmstr will be allocated
**                            in tagged memory with tag frm->frtag)
**
**    Returns:
**       VOID
**
**    History:
**       05/16/90 (esd) Created
**       09/03/90 (esd) Cleanup
**	15-nov-91 (leighb) DeskTop Porting Bug Fix:
**		Add '(' & ')' to call to MEfree().
*/
VOID
IIFTsffaStretchFormForAttrs( frm )
FRAME *frm;
{
   POS_INFO *cp, *frm_cp, *end_frm_cp, *end_dmy_cp;
   i4        dx, dmy_dx;
   i4        flags;
   i4        frmaxx, frmaxy;
   i4        frm_size;
 
   FT_DIAG1( ERx( "FTstrtch: stretching form %s\n" ), frm->frname );
 
   /*
   ** If form has already been stretched, return immediately.
   */
   if ( frm->frmflags & fdFRMSTRTCHD )
   {
      FT_DIAG0( ERx( "form already stretched; returning\n" ) );
      return;
   }
 
   /*
   ** If form has no features (e.g. a new form),
   ** mark it as stretched and return.
   */
   if ( frm->frfldno == 0 && frm->frnsno == 0 && frm->frtrimno == 0 )
   {
      frm->frmflags |= fdFRMSTRTCHD;
      FT_DIAG0( ERx( "form has no features; returning\n" ) );
      return;
   }
 
   /*
   ** Allocate a 2-dimensional array of POS_INFO structures,
   ** one for each cell (position) in the form (augmented by
   ** an extra dummy column on each side).
   */
   FT_DIAG1( ERx( "frmaxy   = %3d\n" ), frm->frmaxy );
   FT_DIAG1( ERx( "frmaxx   = %3d\n" ), frm->frmaxx );
   frmaxx = frm->frmaxx;
   frmaxy = frm->frmaxy;
   frm_size = frmaxy * ( frmaxx + 2 );
   cp = ( POS_INFO * ) MEreqmem(
      ( u_i4 ) 0,
      ( u_i4 ) frm_size * sizeof( POS_INFO ),
      ( bool ) FALSE,
      ( STATUS * ) NULL );
   if ( cp == NULL )
   {
      IIUGbmaBadMemoryAllocation( ERx( "IIFTsffa" ) );
   }
   frm_cp     = cp + frmaxy;
   end_dmy_cp = cp + frm_size;
   end_frm_cp = end_dmy_cp - frmaxy;
 
   /*
   ** Initialize POS_INFO for the dummy column on the left
   ** to indicate no features are present and the cells
   ** are not to be moved (dx = 0).
   */
   for ( ; cp < frm_cp; cp++ )
   {
      cp->flags = VBARV;
      cp->dx = cp->vbar_dx = 0;
      cp->slack = 0;
   }
 
   /*
   ** Initialize POS_INFO for the remaining columns
   ** to indicate no features are present and the cells
   ** are to be moved impossibly far to the left
   ** (so that the first call to reposition_features
   ** will move them as far to the left as possible).
   */
   for ( ; cp < end_dmy_cp; cp++ )
   {
      cp->flags = VBARV;
      cp->dx = cp->vbar_dx = MINI2 + 1;
      cp->slack = 0;
   }
 
   /*
   ** Set flags in POS_INFO for all the features on the form.
   */
   FT_DIAG0( ERx( "drawing features\n" ) );
   process_features( frm, frm_cp, ( bool ) TRUE );
 
   /*
   ** Set flags in POS_INFO for a dummy character feature
   ** encompassing the entire dummy column to the right of the form.
   */
   clear_area(  frm_cp, frmaxy, 0, frmaxx, frmaxy, 1 );
   draw_chfeat( frm_cp, frmaxy, 0, frmaxx, frmaxy, 1 );
 
   /*
   ** Reposition features as far to the left as possible
   ** (including the dummy feature to the right of the form).
   */
   FT_DIAG0( ERx( "positioning features as far left as possible\n" ) );
   reposition_features( frm_cp, end_dmy_cp - 1, 1, frmaxy );
 
   /*
   ** Prepare to reposition the dummy feature we constructed just to
   ** the right of the form.  (The logic below will set dmy_dx
   ** to the dx that's appropriate for the cells of the feature).
   **
   ** In most cases, we put the dummy feature
   ** one position to the right of its original position, if possible.
   ** (It's *not* possible if it got pushed further to the right).
   ** This has the effect of leaving the form in its original size,
   ** since we can discard not only the dummy feature
   ** but also the attributes to its left.
   **
   ** In the special case where the original form was 80 bytes wide
   ** (a common width, since most full-duplex terminals will display
   ** exactly 80 bytes across), we try to make the form 79 bytes wide
   ** (since our implementation of FT for 3270s will display only 79
   ** characters across on a typical 80-byte-wide 3270 - we reserve
   ** one column for attributes).  For this case, we try to reposition
   ** the dummy feature to its original position (just off
   ** the right edge of the form).
   **
   ** The foregoing applies if the form is not boxed.
   ** If the form is boxed, frmaxx doesn't include the 2 vertical lines.
   ** For full-duplex terminals, these 2 vertical bars require 2
   ** character postions, but for 3270's, they require 4
   ** (since columns of attribute bytes must be inserted between
   ** the vertical lines and the edges of the form).
   ** So if the form is boxed and 76, 77, or 78 bytes wide,
   ** we'll try to make it 75 bytes wide.
   */
   dmy_dx = 1;
   if ( frm->frmflags & fdBOXFR )
   {
      FT_DIAG0( ERx( "form is boxed\n" ) );
      if ( frmaxx >= 76 && frmaxx <= 78 )
      {
         dmy_dx = 76 - frmaxx;
      }
   }
   else
   {
      if ( frmaxx == 80 )
      {
         dmy_dx = 0;
      }
   }
   if ( end_frm_cp->dx > dmy_dx )
   {
      dmy_dx = end_frm_cp->dx;
   }
 
   /*
   ** Now we'll reposition features back as close to their original
   ** position as is possible.  To do this, we'll process the columns
   ** from right to left.  Each feature will be moved back to its
   ** original position on the form (dx = 0) except that:
   **
   ** (1) The dummy feature to the right of the form will be positioned
   **     as determined above.
   ** (2) If the first call to reposition_features moved a feature
   **     to the right (dx > 0) we'll leave it there (otherwise,
   **     we'd ultimately push features off the left edge of the form).
   ** (3) We may have to move a feature to the left to avoid touching
   **     a feature to its right that got put back in its original
   **     position.
   **
   ** In order to make use of the same subroutine that we used
   ** to reposition features from left to right, we will in effect
   ** rotate the form 180 degress (swapping top and bottom, and
   ** left and right).  Rather than rebuilding the POS_INFO array
   ** in reverse order (which would get a bit complicated unless
   ** we allocate another array of POS_INFO structures, and would also
   ** complicate life for the second call to process_features),
   ** we'll tell reposition_features to process cells in reverse order.
   ** But we still need to "reverse the polarity" of flags and
   ** negate dx and vbar_dx for each cell in the form and the dummy
   ** feature to the right.
   */
   for ( cp = frm_cp; cp < end_frm_cp; cp++ )
   {
      flags = cp->flags;
      cp->flags = REVERSE_POLARITY( flags );
      dx = cp->dx;
      if ( dx < 0 ) dx = 0;
      cp->dx = -dx;
      dx = cp->vbar_dx;
      if ( dx < 0 ) dx = 0;
      cp->vbar_dx = -dx;
   }
   for ( ; cp < end_dmy_cp; cp++ )
   {
      flags = cp->flags;
      cp->flags = REVERSE_POLARITY( flags );
      cp->dx = cp->vbar_dx = -dmy_dx;
   }
   FT_DIAG0( ERx( "repositioning features as close " ) );
   FT_DIAG0( ERx( "to their original positions as possible\n" ) );
   reposition_features( end_frm_cp - 1, frm_cp, -1, -frmaxy );
 
   /*
   ** Use POS_INFO to update information associated with the FRAME
   ** structure, for all the features on the form.
   */
   FT_DIAG0( ERx( "updating frame information\n" ) );
   process_features( frm, frm_cp, ( bool ) FALSE );
 
   /*
   ** Store the revised width of the form back into the frame structure.
   */
   frm->frmaxx -= end_frm_cp->dx + 1;
   FT_DIAG1( ERx( "frmaxy   = %3d\n" ), frm->frmaxy );
   FT_DIAG1( ERx( "frmaxx   = %3d\n" ), frm->frmaxx );
 
   /*
   ** Mark the form as stretched.
   */
   frm->frmflags |= fdFRMSTRTCHD;
 
   /*
   ** Free the array of POS_INFO structures.
   */
   _VOID_ MEfree( ( PTR ) (frm_cp - frmaxy) );	 
 
   /*
   ** Return.
   */
   FT_DIAG0( ERx( "form successfully stretched; returning\n" ) );
   return;
}
 
/*{
**    Name: reposition_features - Reposition the features of a form
**                                just as far to the right as necessary
**    Description:
**       The features of a form are repositioned just as far to the
**       right of their current positions as is necessary to satisfy
**       the constraints described for IIFTsffaStretchFormForAttrs.
**
**       The form is described by a (conceptually) 2-dimensional array
**       of POS_INFO structures (one for each cell in the form).
**       The structures for the cells of a column are assumed to be
**       contiguous (in contrast to the usual convention for
**       2-dimensional arrays, which is that elements of a *row*
**       are contiguous).
**
**       The caller is responsible for initializing POS_INFO structures
**       for a dummy column to the left of the leftmost column
**       of the form (e.g. to represent cells with no visible features,
**       at offset 0 from their original position).
**       This dummy column will constrain the placement of features
**       that have no features to the left of them: no feature
**       will placed on top of or to left of the cells in this column.
**
**       At entry, the dx and vbar_dx values for the POS_INFO structures
**       specify the current position of features (relative to their
**       original positions).  They can be just about anything:
**       for example, they can specify that features overlap or
**       fall outside the form.  There are only 4 restrictions:
**
**       (1) All dx's for a given character feature must be the same.
**       (2) All vbar_dx's for a given vertical line, together with
**           the dx's for the visible part of the line, must be the same
**       (3) No dx or vbar_dx may be so large that it will overflow
**           if this routine has to increment it (that is, move
**           the corresponding feature to the right).
**       (4) For each cell in the dummy column at the left,
**           dx must equal vbar_dx.
**
**       The dx values for cells without a visible feature and
**       the vbar_dx values for cells without a vertical line
**       are ignored at entry, except for the dummy column at the left.
**
**       At exit, the dx and vbar_dx values for the POS_INFO structures
**       specify each cell's new position (relative to its original
**       position on the form).  dx values are applicable primarily
**       to cells in which a feature is visible; vbar_dx values
**       are applicable primarily to cells which contain part of
**       a vertical line (whether visible or not).  They will adhere
**       to the constraints described for IIFTsffaStretchFormForAttrs.
**
**       The dx for a blank cell (a cell without visible features)
**       will specify that it is to be repositioned as far to the left
**       as is possible without landing on top of (or beyond)
**       a visible feature or a vertical bar obscured by the optional
**       (blank) part of a character feature that's entirely to the left
**       of the blank cell.  If there's no feature to the left
**       of the blank cell, its dx specifies that it be placed on top
**       of the cell in the same row of the dummy column to the left.
**
**       The vbar_dx for a cell without a part of a vertical line
**       will specify that it is to be repositioned as far to the left
**       as is possible without landing on top of (or beyond)
**       a cell containing a part of a vertical line.
**       If there's no vertical line to the left of the cell,
**       its vbar_dx specifies that it be placed on top
**       of the cell in the same row of the dummy column to the left.
**
**    Inputs:
**       start_cp    pointer to POS_INFO struct for first cell of form
**       end_cp      pointer to POS_INFO struct for last cell of form
**       incr_b      amount by which to increment a pointer to get
**                   to the cell below, or if at the bottom of a column,
**                   to get to the top cell of the column to the right
**                   (this amount will typically be 1 or -1)
**       incr_r      amount by which to increment a pointer to get
**                   to the cell to the right
**                   (this amount will typically be frmaxy or -frmaxy)
**
**    Outputs:
**       dx and vbar_dx are updated for the specified cells.
**
**    Returns:
**       VOID
**
**    History:
**       05/16/90 (esd) Created
**       09/03/90 (esd) Cleanup
*/
static
VOID
reposition_features( start_cp, end_cp, incr_b, incr_r )
POS_INFO *start_cp;
POS_INFO *end_cp;
i4        incr_b;
i4        incr_r;
{
   POS_INFO *cp, *vline_cp, *chfeat_cp;
   i4        dx,  vline_dx,  chfeat_dx,  vbar_dx;
   i4        flags, flags_l;
 
   /*
   ** Sweep thru the columns of the form from left to right;
   ** process each column from top to bottom.
   */
   end_cp += incr_b;
   for ( cp = start_cp; cp != end_cp; cp += incr_b )
   {
      /*
      ** Get the flags for the current cell and the cell to its left.
      */
      flags   = cp->flags;
      flags_l = ( cp - incr_r )->flags;
 
      /*
      ** Determine dx and vbar_dx for the current cell.
      ** In the case where the current cell and the cell to its left
      ** are completely empty, these will be one less than
      ** the dx and vbar_dx for the cell to the left of the current cell
      ** (because the cells can overlay each other).
      ** We'll start with these as tentative values,
      ** and then look for various features in the cells
      ** that will require these values to be adjusted.
      */
      dx      = ( cp - incr_r )->dx      - 1;
      vbar_dx = ( cp - incr_r )->vbar_dx - 1;
 
      /*
      ** If the cell to the left contains a vertical bar,
      ** vbar_dx must be incremented to account for it.
      ** If the vertical bar is visible, then dx will equal vbar_dx.
      */
      if ( flags_l & VBAR )
      {
         vbar_dx++;
         if ( flags_l & VBARV )
         {
            dx = vbar_dx;
         }
      }
 
      /*
      ** If the cell to the left contains a required part
      ** of a character feature,
      ** dx must be incremented to account for it.
      */
      if ( flags_l & CHFEAT )
      {
         dx++;
      }
 
      /*
      ** If the cell to the left is at the right edge
      ** of a character feature (including optional parts),
      ** then vbar_dx must be set to dx if it's bigger
      ** (to prevent vertical bars to the right from getting shifted
      ** up against, into, or through the required parts of
      ** the character feature).
      */
      if ( ( flags_l & CHFEATRX ) && ( dx > vbar_dx ) )
      {
         vbar_dx = dx;
      }
 
      /*
      ** If the current cell is at the left edge
      ** of a character feature (including optional parts),
      ** then dx must be set to vbar_dx if it's bigger
      ** (to prevent the required parts of the character feature
      ** from getting shifted up against, into, or through
      ** vertical bars to the left).
      */
      if ( ( flags & CHFEATLX ) && ( vbar_dx > dx ) )
      {
         dx = vbar_dx;
      }
 
      /*
      ** If the current cell is at the left edge
      ** of the required part a character feature,
      ** then dx must be incremented to account for an attribute byte.
      **
      ** Since all cells in the required part of the feature
      ** must have the same dx, we'll compute a worst-case dx
      ** as we traverse the left edge; when we get to the bottom
      ** we'll copy it into the POS_INFO structures for the entire edge.
      ** (Features, and the required parts of features, are always
      ** rectangular).
      */
      if ( flags & CHFEATL )
      {
         dx++;
 
         /*
         ** If we're at the top of the left edge,
         ** remember where the left edge starts,
         ** and initialize the worst-case dx for the feature
         ** to an impossibly good value.
         */
         if ( flags & CHFEATT )
         {
            chfeat_cp = cp;
            chfeat_dx = MINI2 + 1;
         }
 
         /*
         ** If the dx for the current cell is worse than
         ** the worst-case dx found so far for the feature,
         ** make it the worst-case dx.
         */
         if ( dx > chfeat_dx )
         {
            chfeat_dx = dx;
         }
 
         /*
         ** If we're at the bottom of the left edge,
         ** set the dx for all cells in the left edge
         ** to the worst-case dx for the feature.
         */
         if ( flags & CHFEATB && chfeat_dx > chfeat_cp->dx )
         {
            for ( ; chfeat_cp != cp + incr_b; chfeat_cp += incr_b )
            {
               chfeat_cp->dx = chfeat_dx;
            }
         }
      }
 
      /*
      ** If we're *not* at the left edge of a character feature,
      ** simply set the dx for the current cell equal to the dx
      ** we've computed.  Note that this will be overridden
      ** by logic below if a vertical bar is visible in the cell.
      */
      else
      {
         cp->dx = dx;
      }
 
      /*
      ** Now we handle the case where the current cell
      ** contains a vertical bar (or endpoint of a horizontal line).
      **
      ** If the cell to the left does *not* contain a vertical bar,
      ** vbar_dx must be incremented to account for an attribute byte
      ** or space to the right of the last relevant feature to the left
      ** (we'll need an attribute byte following a character feature
      ** and a space following a vertical bar, since we always want
      ** to leave at least one space between vertical bars if they
      ** started out with at least one space between them).
      **
      ** Since all parts of the vertical line
      ** must have the same vbar_dx, we'll compute a worst-case vbar_dx
      ** as we traverse the vertical line; when we get to the bottom
      ** we'll copy it into the POS_INFO structures for the entire line.
      */
      if ( flags & VBAR )
      {
         if ( ( flags_l & VBAR ) == 0 )
         {
            vbar_dx++;
         }
 
         /*
         ** If we're at the top of the vertical line,
         ** remember where the vertical line starts,
         ** and initialize the worst-case vbar_dx for the vertical line
         ** to an impossibly good value.
         */
         if ( ( flags & VBART ) == 0 )
         {
            vline_cp = cp;
            vline_dx = MINI2 + 1;
         }
 
         /*
         ** If the vbar_dx for the current cell is worse than
         ** the worst-case vbar_dx found so far for the vertical line,
         ** make it the worst-case vbar_dx.
         */
         if ( vbar_dx > vline_dx )
         {
            vline_dx = vbar_dx;
         }
 
         /*
         ** If we're at the bottom of the vertical line,
         ** set vbar_dx for all cells in the vertical line
         ** to the worst-case vbar_dx for the vertical line,
         ** and set the dx for all cells in which the vertical line
         ** is visible to the same thing.
         */
         if ( ( flags & VBARB ) == 0 )
         {
            if ( vline_dx > vline_cp->vbar_dx )
            {
               for ( ; vline_cp != cp + incr_b; vline_cp += incr_b )
               {
                  vline_cp->vbar_dx = vline_dx;
 
                  if ( vline_cp->flags & VBARV )
                  {
                     vline_cp->dx = vline_dx;
                  }
               }
            }
            else
            {
               for ( ; vline_cp != cp + incr_b; vline_cp += incr_b )
               {
                  if ( vline_cp->flags & VBARV )
                  {
                     vline_cp->dx = vline_cp->vbar_dx;
                  }
               }
            }
         }
      }
 
      /*
      ** If the current cell does *not* contain a vertical bar,
      ** simply set the vbar_dx for the current cell equal to
      ** the vbar_dx we've computed.
      */
      else
      {
         cp->vbar_dx = vbar_dx;
      }
   }
   return;
}
 
/*{
**    Name: process_features -    Process the features of a form
**
**    Description:
**       This routine runs thru all the features of a form,
**       doing one of 2 things:
**
**       (1) If 'draw' is TRUE (first call):
**
**       The routine sets flags in the associated POS_INFO structure
**       for each cell in each feature of the form.
**       (What constitutes a "feature" is discussed
**       in the description of IIFTsffaStretchFormForAttrs).
**
**       (2) If 'draw' is FALSE (second call):
**
**       The routine uses the POS_INFO structures
**       associated with cells in each feature of the form
**       to update information associated with the FRAME structure.
**
**    Inputs:
**       frm         ptr to FRAME structure for form to be processed
**       frm_cp      ptr to 2-dimensional array of POS_INFO structures
**                   for the cells of the form, with *columns* grouped
**                   together
**       draw        should we draw features (TRUE)
**                   or update them (FALSE)? (see description)
**
**    Outputs:
**       see description
**
**    Returns:
**       VOID
**
**    History:
**       05/16/90 (esd) Created
**       06/01/90 (esd) Fix bug: vifred was displaying character string
**                      formats based on old width of scrollable field.
**                      Fix is to update ftfmt and ftfmtstr.
**       06/17/90 (esd) Fix bug: fields with null titles were being
**                      padded too much if fhtitx < fhdatax.
**                      Fix is to set fhtitx = fhdatax in this case.
**       08/29/90 (esd) Fix bug: width of box wasn't being updated
**                      when the number of digits in width increased.
**       09/03/90 (esd) Cleanup
**       09/12/90 (esd) When changing ftyp->ftfmtstr, create a new copy
**                      of the string, because there are cases (such as
**                      when QBF creates a default form) where the strng
**                      is pointed to by other data structures.
*/
static
VOID
process_features( frm, frm_cp, draw )
FRAME    *frm;
POS_INFO *frm_cp;
bool     draw;
{
   i4      i, j, n;
   i4      y, x, datay, datax, tity, titx, featx;
   i4      h, w, datah, dataw,       titw, featw, neww, id;
   i4      tx, endx, enddatax, endtx;
   POS_INFO *cp;
   FIELD   *f;
   FIELD   **fp;
   i4      nfp;
   REGFLD  *rf;
   TBLFLD  *tf;
   FLDCOL  *fcol, **fcolp;
   i4      nfcolp;
   FLDHDR  *fhdr, *tfhdr;
   FLDTYPE *ftyp;
   FLDVAL  *fval;
   TRIM    *t, **tp;
   i4      ntp;
   i4      *vbarxp, vbarx[ DB_MAX_COLS + 1 ];
   i4      frmaxy = frm->frmaxy;
 
   /*
   ** Macro to return the address of the POS_INFO structure
   ** for the cell with coordinates y (vertical) and x (horizontal).
   */
#  define  CP( y, x ) ( frm_cp + ( frmaxy * (x) ) + (y) )
 
   /*
   ** Process trim and boxes.
   */
   ntp = frm->frtrimno;
   tp  = frm->frtrim;
   for ( i = 0; i < ntp; i++, tp++ )
   {
      t = *tp;
 
      /*
      ** Process a piece of trim:
      */
      if ( ( t->trmflags & fdBOXFLD ) == 0 )
      {
         /*
         ** On first call to process_features, draw the trim string.
         */
         if ( draw )
         {
            w = STlength( t->trmstr );
            clear_area(  frm_cp, frmaxy, t->trmy, t->trmx, 1, w );
            draw_chfeat( frm_cp, frmaxy, t->trmy, t->trmx, 1, w );
         }
 
         /*
         ** On second call to process_features,
         ** update the trim string's horizontal position.
         */
         else
         {
            t->trmx -= CP( t->trmy, t->trmx )->dx;
         }
         FT_DIAG3( ERx( "trim is at %3d, %3d: '%s'\n" ),
                   t->trmy, t->trmx, t->trmstr );
         continue;
      }
 
      /*
      ** Process a box:
      **
      ** Get size and id of box encoded in text string.
      ** For now, we ignore it if we can't find 3 numbers.
      */
      if ( STscanf( t->trmstr, ERx( "%d:%d:%d" ), &h, &w, &id ) != 3 )
      {
         continue;
      }
      endx = t->trmx + w - 1;
 
      /*
      ** On first call to process_features,
      ** draw the vertical sides of the box.
      */
      if ( draw )
      {
         draw_vline( frm_cp, frmaxy, t->trmy, t->trmx, h );
         draw_vline( frm_cp, frmaxy, t->trmy, endx,    h );
      }
 
      /*
      ** On second call to process_features,
      ** update the box's horizontal position and width.
      */
      else
      {
         char    buf[ 40 ];
 
         t->trmx -= CP( t->trmy, t->trmx )->vbar_dx;
         endx    -= CP( t->trmy, endx    )->vbar_dx;
         neww     = endx - t->trmx + 1;
         if ( neww != w )
         {
            w = neww;
            STprintf( buf, ERx( "%d:%d:%d" ), h, w, id );
            if ( STlength( t->trmstr ) < STlength( buf ) )
            {
               t->trmstr = ( char * ) FEreqmem(
                  ( u_i4 ) frm->frtag,
                  ( u_i4 ) ( STlength( buf ) + 1 ),
                  ( bool ) FALSE,
                  ( STATUS * ) NULL );
               if ( t->trmstr == NULL )
               {
                  IIUGbmaBadMemoryAllocation( ERx( "IIFTsffa" ) );
               }
            }
            STcopy( buf, t->trmstr );
         }
      }
      FT_DIAG5( ERx( "box %3d is at %3d, %3d; size %3d, %3d\n" ),
                id, t->trmy, t->trmx, h, w );
   }
 
   /*
   ** Process "display-only" fields and then "updateable" fields.
   */
   for ( i = 0, nfp = frm->frnsno,  fp = frm->frnsfld; i < 2;
         i++,   nfp = frm->frfldno, fp = frm->frfld
       )
   {
      FT_DIAG1( ERx( "processing %s fields\n" ),
                i ? ERx( "updateable" ) :
                ERx( "display-only" ) );
      /*
      ** Process each field of the given type.
      */
      for ( n = 0; n < nfp; n++, fp++ )
      {
         f = *fp;
 
         /*
         ** Process a table field.
         */
         if ( f->fltag == FTABLE )
         {
            tf = f->fld_var.fltblfld;
            tfhdr = &tf->tfhdr;
            FT_DIAG1( ERx( "processing table %s\n" ),
                      tfhdr->fhdname );
            y     = tfhdr->fhposy;
            tx    = tfhdr->fhposx;
            h     = tfhdr->fhmaxy;
            endtx = tfhdr->fhmaxx + tx - 1;
 
            /*
            ** Calculate the positions of vertical bars for table field.
            */
            nfcolp = tf->tfcols;
            vbarxp = &vbarx[ 0 ];
            fcolp  = tf->tfflds;
            for ( j = 0; j < nfcolp; j++, vbarxp++, fcolp++ )
            {
               fcol = *fcolp;
               fhdr = &fcol->flhdr;
               *vbarxp = fhdr->fhposx - 1 + tx;
            }
            *vbarxp = endtx;
 
            /*
            ** On the first call to process_features,
            ** draw the vertical lines for the table field.
            */
            if ( draw )
            {
               if ( vbarx[ 0 ] != tx )
               {
                  FTpanic( ERx( "FTstrch: first col fhposx not 1\n" ) );
               }
               for ( j = 0, vbarxp = vbarx; j <= nfcolp; j++, vbarxp++ )
               {
                  draw_vline( frm_cp, frmaxy, y, *vbarxp, h );
               }
            }
 
            /*
            ** On the second call to process_features,
            ** see where the table moved to and how wide it got.
            */
            else
            {
               tx    -= CP( y, tx    )->vbar_dx;
               endtx -= CP( y, endtx )->vbar_dx;
               tfhdr->fhposx = tx;
               tfhdr->fhmaxx = endtx - tx + 1;
            }
            FT_DIAG1( ERx( "fhposy   = %3d\n" ), tfhdr->fhposy );
            FT_DIAG1( ERx( "fhposx   = %3d\n" ), tfhdr->fhposx );
            FT_DIAG1( ERx( "fhmaxy   = %3d\n" ), tfhdr->fhmaxy );
            FT_DIAG1( ERx( "fhmaxx   = %3d\n" ), tfhdr->fhmaxx );
 
            /*
            ** Process each column of the table field.
            */
            y += 1;
            h -= 2;
            endx = vbarx[ 0 ];
            vbarxp = &vbarx[ 1 ];
            fcolp  = tf->tfflds;
            for ( j = 0; j < nfcolp; j++, vbarxp++, fcolp++ )
            {
               x = endx + 1;
               endx = *vbarxp;
 
               fcol = *fcolp;
               fhdr = &fcol->flhdr;
               ftyp = &fcol->fltype;
               FT_DIAG1( ERx( "processing column %s\n" ),
                         fhdr->fhdname );
               /*
               ** On first call to process_features,
               ** draw a character feature representing
               ** the title and the required part of the data window.
               */
               if ( draw )
               {
                  w = endx - x;
 
                  datax = ftyp->ftdatax;
                  dataw = ftyp->ftdataln;
                  if ( fhdr->fhd2flags & fdSCRLFD )
                  {
                     FT_DIAG0( ERx( "column is scrollable\n" ) );
                     if ( dataw > 8 ) dataw = 8;
                  }
                  titx = fhdr->fhtitx;
                  titw = STlength( fhdr->fhtitle );
                  if ( titw > 0 )
                  {
                     if ( titx < datax ) datax = titx;
                     if ( titw > dataw ) dataw = titw;
                  }
                  datax += tx;
 
                  if ( datax < x || datax + dataw > *vbarxp )
                  {
                     FTpanic( ERx( "FTstrch: data overflows col\n" ) );
                  }
                  clear_area(  frm_cp, frmaxy, y, x,     h, w     );
                  draw_chfeat( frm_cp, frmaxy, y, datax, h, dataw );
               }
 
               /*
               ** On second call to process_features,
               ** recompute the column's fhposx, fhmaxx, fhtitx,
               ** ftdatax, and ftdataln from the new postion
               ** of the vertical bars on either side.
               */
               else
               {
                  x -=        CP( y, x - 1 )->vbar_dx;
                  w  = endx - CP( y, endx  )->vbar_dx - x;
                  fhdr->fhposx = x - tx;
                  fhdr->fhtitx = x - tx + 1;
                  fhdr->fhmaxx = w;
                  ftyp->ftdatax = x - tx + 1;
                  dataw = w - 2;
                  if ( ftyp->ftdataln > dataw )
                  {
                     ftyp->ftdataln = dataw;
                     ftyp->ftwidth  = dataw;
                     ftyp->ftfmt->fmt_width = dataw;
		     /* 8 bytes is enough for a format string, e.g. c100. */
                     ftyp->ftfmtstr = ( char * ) FEreqmem(
                        ( u_i4 ) frm->frtag,
                        ( u_i4 ) 8,
                        ( bool ) FALSE,
                        ( STATUS * ) NULL );
                     if ( ftyp->ftfmtstr == NULL )
                     {
                        IIUGbmaBadMemoryAllocation( ERx( "IIFTsffa" ) );
                     }
                     STprintf( ftyp->ftfmtstr, ERx( "c%d" ), dataw );
                  }
               }
               FT_DIAG1( ERx( "fhposy   = %3d\n" ), fhdr->fhposy   );
               FT_DIAG1( ERx( "fhposx   = %3d\n" ), fhdr->fhposx   );
               FT_DIAG1( ERx( "fhmaxy   = %3d\n" ), fhdr->fhmaxy   );
               FT_DIAG1( ERx( "fhmaxx   = %3d\n" ), fhdr->fhmaxx   );
               FT_DIAG1( ERx( "fhtity   = %3d\n" ), fhdr->fhtity   );
               FT_DIAG1( ERx( "fhtitx   = %3d\n" ), fhdr->fhtitx   );
               FT_DIAG1( ERx( "ftdatax  = %3d\n" ), ftyp->ftdatax  );
               FT_DIAG1( ERx( "ftdataln = %3d\n" ), ftyp->ftdataln );
               FT_DIAG1( ERx( "ftwidth  = %3d\n" ), ftyp->ftwidth  );
               FT_DIAG1( ERx( "ftfmtstr = %s\n" ),  ftyp->ftfmtstr );
            }
         }
 
         /*
         ** Process a regular field.
         */
         else
         {
            rf = f->fld_var.flregfld;
            fhdr = &rf->flhdr;
            fval = &rf->flval;
            ftyp = &rf->fltype;
            FT_DIAG1( ERx( "processing field %s\n" ),
                      fhdr->fhdname );
 
            y = fhdr->fhposy;
            x = fhdr->fhposx;
            h = fhdr->fhmaxy;
            w = fhdr->fhmaxx;
 
            /*
            ** On first call to process_features,
            ** draw vertical lines on each side of the box, if any,
            ** and draw character feature(s) representing
            ** the title and the required part of the data window.
            */
            if ( draw )
            {
               datay = y + fval->fvdatay;
               datax = x + ftyp->ftdatax;
               dataw = ftyp->ftdataln;
               datah = 1 + ( ftyp->ftwidth - 1 ) / dataw;
 
               if ( fhdr->fhd2flags & fdSCRLFD && datah == 1 )
               {
                  FT_DIAG0( ERx( "field is scrollable\n" ) );
                  if ( dataw > 8 ) dataw = 8;
               }
               tity = y + fhdr->fhtity;
               titx = x + fhdr->fhtitx;
               titw = STlength( fhdr->fhtitle );
 
               if ( f->fldflags & fdBOXFLD )
               {
                  FT_DIAG0( ERx( "field is boxed\n" ) );
                  draw_vline( frm_cp, frmaxy, y, x,         h );
                  draw_vline( frm_cp, frmaxy, y, x + w - 1, h );
                  y += 1;
                  x += 1;
                  h -= 2;
                  w -= 2;
               }
 
               clear_area(  frm_cp, frmaxy, y, x, h, w );
 
               if ( titw > 0 )
               {
                  if ( tity >= datay && tity < datay + datah )
                  {
                     draw_chfeat( frm_cp, frmaxy, y, datax, h, dataw );
                     draw_chfeat( frm_cp, frmaxy, y, titx,  h, titw  );
                  }
                  else
                  {
                     featx = min( titx,        datax         );
                     featw = max( titx + titw, datax + dataw ) - featx;
 
                     draw_chfeat( frm_cp, frmaxy, y, featx, h, featw );
                  }
               }
               else
               {
                  featx = datax;
                  featw = dataw;
 
                  draw_chfeat( frm_cp, frmaxy, y, featx, h, featw );
               }
            }
 
            /*
            ** On second call to process_features,
            ** recompute the field's fhposx, fhmaxx, fhtitx,
            ** ftdatax, and ftdataln.
            */
            else
            {
               /*
               ** Get x coordinates of data window and title.
               ** Save old x coordinate of last data window position.
               */
               datax = x + ftyp->ftdatax;
               enddatax = datax + ftyp->ftdataln - 1;
               titx  = x + fhdr->fhtitx;
               titw  = STlength( fhdr->fhtitle );
               if ( titw == 0 ) titx = datax;
 
               /*
               ** Get new x coordinates of title and data window.
               */
               if ( f->fldflags & fdBOXFLD )
               {
                  y     += 1;
               }
               datax -= CP( y, datax )->dx;
               titx  -= CP( y, titx  )->dx;
 
               /*
               ** Recompute ftdataln if necessary:
               ** See if the last character of the data window
               ** is in an optional part of the feature.
               ** (This will occur if we found a 1-line horizontally
               ** scrollable data window longer than 8 characters
               ** for a field, on the same line as the title
               ** for the field or extending beyond the right end
               ** of the title).  If this is the case,
               ** we want to reclaim as much of the data window
               ** as we can without bumping into a feature to the right:
               ** The last call to position_features (which viewed
               ** the form upside-down and backwards) set the dx value
               ** for the blank cell at the end of the data window
               ** to cause the blank cell to be moved as far to the
               ** right as possible without overlaying (or going beyond)
               ** a feature to the right.  This will tell us how far
               ** to the right the data window can extend.  We'll need
               ** to allow for at least one position between the end of
               ** the data window and any feature to the right,
               ** and of course we don't want to make the data window
               ** bigger than it originally was.
               */
               cp = CP( y, enddatax );
               if ( ( cp->flags & CHFEAT ) == 0 )
               {
                  enddatax -= cp->dx;
                  dataw = enddatax - datax;
                  if ( ftyp->ftdataln > dataw )
                  {
                     ftyp->ftdataln = dataw;
                     ftyp->ftwidth  = dataw;
                     ftyp->ftfmt->fmt_width = dataw;
		     /* 8 bytes is enough for a format string, e.g. c100. */
                     ftyp->ftfmtstr = ( char * ) FEreqmem(
                        ( u_i4 ) frm->frtag,
                        ( u_i4 ) 8,
                        ( bool ) FALSE,
                        ( STATUS * ) NULL );
                     if ( ftyp->ftfmtstr == NULL )
                     {
                        IIUGbmaBadMemoryAllocation( ERx( "IIFTsffa" ) );
                     }
                     STprintf( ftyp->ftfmtstr, ERx( "c%d" ), dataw );
                  }
               }
 
               /*
               ** Determine new x coordinates of first and last
               ** horizontal positions of field.
               ** If the field is boxed, these will be the coordinates
               ** of the left and right edges of the box.
               ** If the field is not boxed, we'll pare the field down
               ** as far we can horizontally so long as the title and
               ** data window will still fit.
               **
               ** We'll also adjust y to point to the top line
               ** of the feature(s) we drew.
               */
               if ( f->fldflags & fdBOXFLD )
               {
                  y     -= 1;
                  endx   = x + w - 1;
                  endx  -= CP( y, endx  )->vbar_dx;
                  x     -= CP( y, x     )->vbar_dx;
                  y     += 1;
               }
               else
               {
                  dataw = ftyp->ftdataln;
 
                  x     = min( titx,        datax         );
                  endx  = max( titx + titw, datax + dataw ) - 1;
               }
 
               /*
               ** Recompute fhposx, fhmaxx, fhtitx, and ftdatax.
               */
               fhdr->fhposx  = x;
               fhdr->fhmaxx  = endx  - x + 1;
               fhdr->fhtitx  = titx  - x;
               ftyp->ftdatax = datax - x;
            }
            FT_DIAG1( ERx( "fhposy   = %3d\n" ), fhdr->fhposy   );
            FT_DIAG1( ERx( "fhposx   = %3d\n" ), fhdr->fhposx   );
            FT_DIAG1( ERx( "fhmaxy   = %3d\n" ), fhdr->fhmaxy   );
            FT_DIAG1( ERx( "fhmaxx   = %3d\n" ), fhdr->fhmaxx   );
            FT_DIAG1( ERx( "fhtity   = %3d\n" ), fhdr->fhtity   );
            FT_DIAG1( ERx( "fhtitx   = %3d\n" ), fhdr->fhtitx   );
            FT_DIAG1( ERx( "fvdatay  = %3d\n" ), fval->fvdatay  );
            FT_DIAG1( ERx( "ftdatax  = %3d\n" ), ftyp->ftdatax  );
            FT_DIAG1( ERx( "ftdataln = %3d\n" ), ftyp->ftdataln );
            FT_DIAG1( ERx( "ftwidth  = %3d\n" ), ftyp->ftwidth  );
            FT_DIAG1( ERx( "ftfmtstr = %s\n" ),  ftyp->ftfmtstr );
         }
      }
   }
   return;
#  undef   CP
}
 
/*{
**    Name: clear_area       -    Clear an area of a form
**
**    Description:
**       This routine sets flags in the associated POS_INFO structure
**       for each cell in a rectangular area, indicating it's
**       a required or optional part of a character feature of a form,
**       and that vertical bars will be obscured by it.
**       (What constitutes a "character feature" is discussed
**       in the description of IIFTsffaStretchFormForAttrs).
**
**       The rectangular area must not overlap another area that's
**       already been cleared by a call to clear_area; otherwise
**       FT3270 will panic ("overlapping features").
**
**    Inputs:
**       frm         ptr to FRAME structure for form to be processed
**       frm_cp      ptr to 2-dimensional array of POS_INFO structures
**                   for the cells of the form, with *columns* grouped
**                   together
**       frmaxy      the height of the form
**       y, x        0-rel row and column of rectangular area
**       h, w        height and width of rectangular area
**
**    Outputs:
**       see description
**
**    Returns:
**       VOID
**
**    History:
**       05/16/90 (esd) Created
**       09/03/90 (esd) Cleanup
*/
static
VOID
clear_area( frm_cp, frmaxy, y, x, h, w )
POS_INFO *frm_cp;
i4       frmaxy, y, x, h, w;
{
   POS_INFO *cp, *t_cp, *b_cp, *tl_cp, *bl_cp, *tr_cp, *br_cp;
 
   FT_DIAG4( ERx( "clear_area  at %3d, %3d; size %3d, %3d\n" ),
             y, x, h, w );
   h--;
   w--;
   w *= frmaxy;
   x *= frmaxy;
 
   tl_cp = frm_cp + x + y;
   bl_cp = tl_cp  + h;
   tr_cp = tl_cp  + w;
   br_cp = tr_cp  + h;
 
   for ( t_cp =  tl_cp,  b_cp =  bl_cp;
         t_cp <= tr_cp;
         t_cp += frmaxy, b_cp += frmaxy
       )
   {
      for ( cp = t_cp; cp <= b_cp; cp++ )
      {
         if ( ( cp->flags & VBARV ) == 0 )
         {
            FTpanic( ERx( "FTstrtch: overlapping features" ) );
         }
         cp->flags &= ~VBARV;
      }
   }
 
   for ( cp = tl_cp; cp <= bl_cp; cp++ )
   {
      cp->flags |= CHFEATLX;
   }
 
   for ( cp = tr_cp; cp <= br_cp; cp++ )
   {
      cp->flags |= CHFEATRX;
   }
   return;
}
 
/*{
**    Name: draw_chfeat      -    Draw a character feature
**
**    Description:
**       This routine sets flags in the associated POS_INFO structure
**       for each cell in a rectangular area, indicating it's
**       a required part of a character feature of a form.
**       (What constitutes a "character feature" is discussed
**       in the description of IIFTsffaStretchFormForAttrs).
**
**       The rectangular area must have previously been cleared
**       by a call to clear_area; otherwise FT3270 will panic
**       ("misplaced feature").
**
**    Inputs:
**       frm         ptr to FRAME structure for form to be processed
**       frm_cp      ptr to 2-dimensional array of POS_INFO structures
**                   for the cells of the form, with *columns* grouped
**                   together
**       frmaxy      the height of the form
**       y, x        0-rel row and column of rectangular area
**       h, w        height and width of rectangular area
**
**    Outputs:
**       see description
**
**    Returns:
**       VOID
**
**    History:
**       05/16/90 (esd) Created
**       09/03/90 (esd) Cleanup
*/
static
VOID
draw_chfeat( frm_cp, frmaxy, y, x, h, w )
POS_INFO *frm_cp;
i4       frmaxy, y, x, h, w;
{
   POS_INFO *cp, *t_cp, *b_cp, *tl_cp, *bl_cp, *tr_cp, *br_cp;
 
   FT_DIAG4( ERx( "draw_chfeat at %3d, %3d; size %3d, %3d\n" ),
             y, x, h, w );
   h--;
   w--;
   w *= frmaxy;
   x *= frmaxy;
 
   tl_cp = frm_cp + x + y;
   bl_cp = tl_cp  + h;
   tr_cp = tl_cp  + w;
   br_cp = tr_cp  + h;
 
   for ( t_cp =  tl_cp,  b_cp =  bl_cp;
         t_cp <= tr_cp;
         t_cp += frmaxy, b_cp += frmaxy
       )
   {
      for ( cp = t_cp; cp <= b_cp; cp++ )
      {
         if ( cp->flags & VBARV )
         {
            FTpanic( ERx( "FTstrtch: misplaced feature" ) );
         }
         cp->flags |= CHFEAT;
      }
   }
 
   for ( cp = tl_cp; cp <= bl_cp; cp++ )
   {
      cp->flags |= CHFEATL;
   }
 
   for ( cp = tr_cp; cp <= br_cp; cp++ )
   {
      cp->flags |= CHFEATR;
   }
 
   tl_cp->flags |= CHFEATT;
   tr_cp->flags |= CHFEATT;
   bl_cp->flags |= CHFEATB;
   br_cp->flags |= CHFEATB;
 
   return;
}
 
/*{
**    Name: draw_vline       -    Draw a vertical line
**
**    Description:
**       This routine sets flags in the associated POS_INFO structure
**       for each cell in a column, indicating it's part of
**       a vertical line.
**
**    Inputs:
**       frm         ptr to FRAME structure for form to be processed
**       frm_cp      ptr to 2-dimensional array of POS_INFO structures
**                   for the cells of the form, with *columns* grouped
**                   together
**       frmaxy      the height of the form
**       y, x        0-rel row and column of top of line
**       h           height of line ("1" indicates a point)
**
**    Outputs:
**       see description
**
**    Returns:
**       VOID
**
**    History:
**       05/16/90 (esd) Created
**       09/03/90 (esd) Cleanup
*/
static
VOID
draw_vline( frm_cp, frmaxy, y, x, h )
POS_INFO *frm_cp;
i4       frmaxy, y, x, h;
{
   POS_INFO *cp, *t_cp, *b_cp;
 
   FT_DIAG3( ERx( "draw_vline  at %3d, %3d; height %3d\n" ),
             y, x, h );
   h--;
   x *= frmaxy;
 
   t_cp = frm_cp + x + y;
 
   if ( h > 0 )
   {
      b_cp = t_cp + h;
 
      for ( cp = t_cp + 1; cp < b_cp; cp++ )
      {
         cp->flags |= VBAR | VBART | VBARB;
      }
 
      t_cp->flags  |= VBAR | VBARB;
      b_cp->flags  |= VBAR | VBART;
   }
   else
   {
      t_cp->flags  |= VBAR;
   }
 
   return;
}
