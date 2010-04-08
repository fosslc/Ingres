/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>

/**
**
**  Name: ULB.C - Functions for bit/byte manipulation.
**
**  Description:
**      This file contains functions for bit/byte manipulation.
**
**          ulb_rev_bits - reverse bits in a u_i4
**
**  History: 
**      26-Oct-93 (andre)   
**          created
**/

/*{
** Name: ulb_rev_bits -	reverse bits in a u_i4
**
** Description:
**	This function will reverse bits in a u_i4 (i.e. 0-th bit will become 
**	31-bit, 1-st will become 30-th, etc..)  At least originally, it will be
**	used to convert an object id into a "randomized" id which will be used 
**	as a key into IITREE and IIQRYTEXT and will allegedly reduce contention
**	for IIQRYTEXT pages when several users simultaneously create DDL objects
**
** Inputs:
**	val				u_i4 whose bits are to be reversed
**
** Outputs:
**	none
**
** Returns:
**	u_i4 derived by reversing bits in val
**
** Exceptions:
** 	none
**
** Side Effects:
** 	none
**
** History:
**	26-oct-93 (andre)
**          written (by jhahn)
**	08-Aug-2009 (drivi01)
**		The compiler is throwing warning about the precedence
**		in return statement of ulb_rev_bits function.
**		Added pragma to remove the warning b/c it was decided
**		not to fix the precedence error due to the fact that
**		algorithm changes may cause compatibility problems
**		with the old versions of Ingres b/c this algorithm 
**		is used as a hash function.
**		The correct algorithm would be:
**		((((((((u_i4) flip_byte[val &0xff]) << 8) +
**			((u_i4) flip_byte[(val >> 8) &0xff])) << 8) +
**			((u_i4) flip_byte[(val >> 16) & 0xff])) << 8) +
**			((u_i4) flip_byte[val >> 24]));
**		However, as it stands right now, changing it should
**		be avoided!
**		   
*/
#ifdef NT_GENERIC	
#pragma warning(disable: 4554)
#endif
/*
** array which will make flipping those bits so much faster
** flip_byte[n] is equal to the number obtained by reversing bits in a byte 
** containing value n
*/
static u_char flip_byte[] =
    {
        0, 128, 64, 192, 32, 160, 96, 224,
        16, 144, 80, 208, 48, 176, 112, 240,
        8, 136, 72, 200, 40, 168, 104, 232,
        24, 152, 88, 216, 56, 184, 120, 248,
        4, 132, 68, 196, 36, 164, 100, 228,
        20, 148, 84, 212, 52, 180, 116, 244,
        12, 140, 76, 204, 44, 172, 108, 236,
        28, 156, 92, 220, 60, 188, 124, 252,
        2, 130, 66, 194, 34, 162, 98, 226,
        18, 146, 82, 210, 50, 178, 114, 242,
        10, 138, 74, 202, 42, 170, 106, 234,
        26, 154, 90, 218, 58, 186, 122, 250,
        6, 134, 70, 198, 38, 166, 102, 230,
        22, 150, 86, 214, 54, 182, 118, 246,
        14, 142, 78, 206, 46, 174, 110, 238,
        30, 158, 94, 222, 62, 190, 126, 254,
        1, 129, 65, 193, 33, 161, 97, 225,
        17, 145, 81, 209, 49, 177, 113, 241,
        9, 137, 73, 201, 41, 169, 105, 233,
        25, 153, 89, 217, 57, 185, 121, 249,
        5, 133, 69, 197, 37, 165, 101, 229,
        21, 149, 85, 213, 53, 181, 117, 245,
        13, 141, 77, 205, 45, 173, 109, 237,
        29, 157, 93, 221, 61, 189, 125, 253,
        3, 131, 67, 195, 35, 163, 99, 227,
        19, 147, 83, 211, 51, 179, 115, 243,
        11, 139, 75, 203, 43, 171, 107, 235,
        27, 155, 91, 219, 59, 187, 123, 251,
        7, 135, 71, 199, 39, 167, 103, 231,
        23, 151, 87, 215, 55, 183, 119, 247,
        15, 143, 79, 207, 47, 175, 111, 239,
        31, 159, 95, 223, 63, 191, 127, 255
        };

u_i4
ulb_rev_bits(
	u_i4		val)
{
    return (((((((u_i4) flip_byte[val &0xff]) << 8) +
              ((u_i4) flip_byte[(val >> 8) &0xff])) << 8) +
             (((u_i4) flip_byte[(val >> 16) & 0xff])) << 8) +
            ((u_i4) flip_byte[val >> 24]));
}
