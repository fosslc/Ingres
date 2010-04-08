/* These defines determine the meaning of the fFlags variable.  The low byte
** is used for the various types of "boxes" to draw.  The high byte is
** available for special commands.
**
**	History:
**	25-Oct-93	(marc_b)
**			Added to support teselect.c
**/

#define SL_BOX    1             /* Draw a solid border around the rectangle  */
#define SL_BLOCK  2             /* Draw a solid rectangle                    */

#define SL_EXTEND 256           /* Extend the current pattern                */

#define SL_TYPE    0x00FF       /* Mask out everything but the type flags    */
#define SL_SPECIAL 0xFF00       /* Mask out everything but the special flags */

void FAR PASCAL StartSelection(HWND, POINT, LPRECT, short);
void FAR PASCAL UpdateSelection(HWND, POINT, LPRECT, short);
void FAR PASCAL EndSelection(POINT, LPRECT);
void FAR PASCAL ClearSelection(HWND, LPRECT, short);
