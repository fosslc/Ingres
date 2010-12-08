/*{ Prototype Cleanup (Bug 124685) (gupsh01)
** Add the following lines between begin 
** prototypes and end prototypes to adudtab.h 
** if this file is regerated by running 
** gperf on adudtab.txt. 
*/

/* Begin Prototypes */

static unsigned int
adu_dttemplate_hash (
     register const char *str,
     register unsigned int len);

const struct _ANSIDATETMPLT *
adu_dttemplate_lookup (
     register const char *str,
     register unsigned int len);

/* End Prototypes }*/

/* ANSI-C code produced by gperf version 3.0.1 */
/* Command-line: gperf -m 50 adudtab.txt  */
/* Computed positions: -k'1-4,7,9,12,14' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

/* maximum key range = 156, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
adu_dttemplate_hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164,  19, 164,   5,   3,  76,  35, 164,
      164, 164, 164, 164, 164, 164, 164, 164,   3, 113,
      164, 164, 164, 164, 164,   7, 164, 164,  67, 164,
       66,  62, 164,  55, 164, 164, 164,  51,  50, 164,
      164, 164, 164, 164, 164,  39, 164, 164,   8,   3,
      164, 164, 164, 164, 164, 164,  11, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164,  68,
        3,   6, 164, 164,  53, 164, 164, 164, 164, 164,
      164, 164,  35, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164, 164, 164, 164,
      164, 164, 164, 164, 164, 164, 164
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[13]];
      /*FALLTHROUGH*/
      case 13:
      case 12:
        hval += asso_values[(unsigned char)str[11]];
      /*FALLTHROUGH*/
      case 11:
      case 10:
      case 9:
        hval += asso_values[(unsigned char)str[8]];
      /*FALLTHROUGH*/
      case 8:
      case 7:
        hval += asso_values[(unsigned char)str[6]];
      /*FALLTHROUGH*/
      case 6:
      case 5:
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
        hval += asso_values[(unsigned char)str[2]+1];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

#ifdef __GNUC__
__inline
#endif
const struct _ANSIDATETMPLT *
adu_dttemplate_lookup (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 113,
      MIN_WORD_LENGTH = 2,
      MAX_WORD_LENGTH = 17,
      MIN_HASH_VALUE = 8,
      MAX_HASH_VALUE = 163
    };

  static const struct _ANSIDATETMPLT wordlist[] =
    {
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 253 "adudtab.txt"
      {"Yn",		"Y", DB_DTE_TYPE, DT_TYP},
      {""}, {""}, {""},
#line 156 "adudtab.txt"
      {"An",			"y",		DB_INYM_TYPE,	DT_I },
      {""}, {""}, {""},
#line 252 "adudtab.txt"
      {"Yn-n",		"md", DB_DTE_TYPE, DT_TYP},
      {""},
#line 249 "adudtab.txt"
      {"Yn-n-n",		"ymd", DB_DTE_TYPE, DT_CHK},
#line 162 "adudtab.txt"
      {"Ann",			"dh",		DB_INDS_TYPE,	DT_I|DT_T},
#line 157 "adudtab.txt"
      {"An-n",			"yq",		DB_INYM_TYPE,	DT_I },
#line 160 "adudtab.txt"
      {"A-n",			"x",		DB_INYM_TYPE,	DT_I },
#line 143 "adudtab.txt"
      {"An-n-n",			"ymd",		DB_ADTE_TYPE,	DT_0 },
#line 288 "adudtab.txt"
      {"Xn-n-n",		"ymd", DB_DTE_TYPE, DT_0},
#line 163 "adudtab.txt"
      {"Ann:n",			"dhi",		DB_INDS_TYPE,	DT_I|DT_T},
#line 170 "adudtab.txt"
      {"A-nn",			"eh",		DB_INDS_TYPE,	DT_I|DT_T},
#line 246 "adudtab.txt"
      {"Yn_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 171 "adudtab.txt"
      {"A-nn:n",			"ehi",		DB_INDS_TYPE,	DT_I|DT_T},
#line 161 "adudtab.txt"
      {"A-n-n",			"xq",		DB_INYM_TYPE,	DT_I },
#line 164 "adudtab.txt"
      {"Ann:n:n",		"dhis",		DB_INDS_TYPE,	DT_I|DT_T},
      {""},
#line 287 "adudtab.txt"
      {"Xn_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 172 "adudtab.txt"
      {"A-nn:n:n",		"ehis",		DB_INDS_TYPE,	DT_I|DT_T},
#line 144 "adudtab.txt"
      {"An-n-nn:n:n",		"ymdhis",	DB_TSWO_TYPE,	DT_T},
#line 165 "adudtab.txt"
      {"Ann:n:n.n",		"dhisl",	DB_INDS_TYPE,	DT_I|DT_T},
#line 158 "adudtab.txt"
      {"A+n",			"y",		DB_INYM_TYPE,	DT_I },
      {""},
#line 173 "adudtab.txt"
      {"A-nn:n:n.n",		"ehisl",	DB_INDS_TYPE,	DT_I|DT_T},
#line 145 "adudtab.txt"
      {"An-n-nn:n:n.n",		"ymdhisl",	DB_TSWO_TYPE,	DT_T},
#line 166 "adudtab.txt"
      {"A+nn",			"dh",		DB_INDS_TYPE,	DT_I|DT_T},
#line 254 "adudtab.txt"
      {"Yz",		"f", DB_DTE_TYPE, DT_0},
#line 167 "adudtab.txt"
      {"A+nn:n",			"dhi",		DB_INDS_TYPE,	DT_I|DT_T},
#line 159 "adudtab.txt"
      {"A+n-n",			"yq",		DB_INYM_TYPE,	DT_I },
      {""},
#line 185 "adudtab.txt"
      {"Un",		"K", DB_DTE_TYPE, DT_TYP},
#line 146 "adudtab.txt"
      {"An-n-nn:n:n-n:n",	"ymdhiscv",	DB_TSW_TYPE,	DT_T },
#line 168 "adudtab.txt"
      {"A+nn:n:n",		"dhis",		DB_INDS_TYPE,	DT_I|DT_T},
#line 147 "adudtab.txt"
      {"An-n-nn:n:n.n-n:n",	"ymdhislcv",	DB_TSW_TYPE,	DT_T },
#line 251 "adudtab.txt"
      {"Yn/n",		"md", DB_DTE_TYPE, DT_0},
      {""},
#line 247 "adudtab.txt"
      {"Yn/n/n",		"ymd", DB_DTE_TYPE, DT_0},
#line 169 "adudtab.txt"
      {"A+nn:n:n.n",		"dhisl",	DB_INDS_TYPE,	DT_I|DT_T},
#line 184 "adudtab.txt"
      {"Un-n",		"md", DB_DTE_TYPE, DT_TYP},
      {""},
#line 181 "adudtab.txt"
      {"Un-n-n",		"mdy", DB_DTE_TYPE, DT_CHK},
#line 198 "adudtab.txt"
      {"Nn",		"K", DB_DTE_TYPE, DT_TYP},
#line 267 "adudtab.txt"
      {"Mn",		"M", DB_DTE_TYPE, DT_TYP},
      {""},
#line 255 "adudtab.txt"
      {"Yr",		"r", DB_DTE_TYPE, DT_0},
#line 149 "adudtab.txt"
      {"An-n-nn:n:n+n:n",	"ymdhisbv",	DB_TSW_TYPE,	DT_T },
#line 225 "adudtab.txt"
      {"In",		"J", DB_DTE_TYPE, DT_TYP},
#line 148 "adudtab.txt"
      {"An-n-nn:n:n.n+n:n",	"ymdhislbv",	DB_TSW_TYPE,	DT_T },
#line 178 "adudtab.txt"
      {"Un_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 197 "adudtab.txt"
      {"Nn-n",		"md", DB_DTE_TYPE, DT_TYP},
#line 266 "adudtab.txt"
      {"Mn-n",		"md", DB_DTE_TYPE, DT_TYP},
#line 194 "adudtab.txt"
      {"Nn-n-n",		"mdy", DB_DTE_TYPE, DT_CHK},
#line 263 "adudtab.txt"
      {"Mn-n-n",		"mdy", DB_DTE_TYPE, DT_CHK},
#line 239 "adudtab.txt"
      {"Gn",		"G", DB_DTE_TYPE, DT_TYP},
#line 224 "adudtab.txt"
      {"In-n",		"md", DB_DTE_TYPE, DT_TYP},
      {""},
#line 221 "adudtab.txt"
      {"In-n-n",		"mdy", DB_DTE_TYPE, DT_CHK},
#line 211 "adudtab.txt"
      {"Fn",		"K", DB_DTE_TYPE, DT_TYP},
#line 281 "adudtab.txt"
      {"Dn",		"D", DB_DTE_TYPE, DT_TYP},
#line 191 "adudtab.txt"
      {"Nn_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 260 "adudtab.txt"
      {"Mn_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 238 "adudtab.txt"
      {"Gn-n",		"md", DB_DTE_TYPE, DT_TYP},
#line 186 "adudtab.txt"
      {"Uz",		"f", DB_DTE_TYPE, DT_0},
#line 235 "adudtab.txt"
      {"Gn-n-n",		"mdy", DB_DTE_TYPE, DT_CHK},
#line 218 "adudtab.txt"
      {"In_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 210 "adudtab.txt"
      {"Fn-n",		"md", DB_DTE_TYPE, DT_TYP},
#line 280 "adudtab.txt"
      {"Dn-n",		"dm", DB_DTE_TYPE, DT_TYP},
#line 207 "adudtab.txt"
      {"Fn-n-n",		"ymd", DB_DTE_TYPE, DT_CHK},
#line 277 "adudtab.txt"
      {"Dn-n-n",		"dmy", DB_DTE_TYPE, DT_CHK},
#line 248 "adudtab.txt"
      {"Yn-m-n",		"ymd", DB_DTE_TYPE, DT_0},
#line 183 "adudtab.txt"
      {"Un/n",		"md", DB_DTE_TYPE, DT_0},
#line 232 "adudtab.txt"
      {"Gn_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 179 "adudtab.txt"
      {"Un/n/n",		"mdy", DB_DTE_TYPE, DT_0},
#line 199 "adudtab.txt"
      {"Nz",		"f", DB_DTE_TYPE, DT_0},
#line 268 "adudtab.txt"
      {"Mz",		"f", DB_DTE_TYPE, DT_0},
#line 204 "adudtab.txt"
      {"Fn_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 274 "adudtab.txt"
      {"Dn_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 250 "adudtab.txt"
      {"Yn.n.n",		"ymd", DB_DTE_TYPE, DT_0},
#line 226 "adudtab.txt"
      {"Iz",		"f", DB_DTE_TYPE, DT_0},
      {""},
#line 187 "adudtab.txt"
      {"Ur",		"r", DB_DTE_TYPE, DT_0},
#line 196 "adudtab.txt"
      {"Nn/n",		"md", DB_DTE_TYPE, DT_0},
#line 265 "adudtab.txt"
      {"Mn/n",		"md", DB_DTE_TYPE, DT_0},
#line 192 "adudtab.txt"
      {"Nn/n/n",		"dmy", DB_DTE_TYPE, DT_0},
#line 261 "adudtab.txt"
      {"Mn/n/n",		"mdy", DB_DTE_TYPE, DT_0},
#line 240 "adudtab.txt"
      {"Gz",		"f", DB_DTE_TYPE, DT_0},
#line 223 "adudtab.txt"
      {"In/n",		"md", DB_DTE_TYPE, DT_0},
      {""},
#line 219 "adudtab.txt"
      {"In/n/n",		"mdy", DB_DTE_TYPE, DT_0},
#line 212 "adudtab.txt"
      {"Fz",		"f", DB_DTE_TYPE, DT_0},
#line 282 "adudtab.txt"
      {"Dz",		"f", DB_DTE_TYPE, DT_0},
#line 200 "adudtab.txt"
      {"Nr",		"r", DB_DTE_TYPE, DT_0},
#line 269 "adudtab.txt"
      {"Mr",		"r", DB_DTE_TYPE, DT_0},
#line 237 "adudtab.txt"
      {"Gn/n",		"md", DB_DTE_TYPE, DT_0},
      {""},
#line 233 "adudtab.txt"
      {"Gn/n/n",		"mdy", DB_DTE_TYPE, DT_0},
#line 227 "adudtab.txt"
      {"Ir",		"r", DB_DTE_TYPE, DT_0},
#line 209 "adudtab.txt"
      {"Fn/n",		"md", DB_DTE_TYPE, DT_0},
#line 279 "adudtab.txt"
      {"Dn/n",		"dm", DB_DTE_TYPE, DT_0},
#line 205 "adudtab.txt"
      {"Fn/n/n",		"mdy", DB_DTE_TYPE, DT_0},
#line 275 "adudtab.txt"
      {"Dn/n/n",		"dmy", DB_DTE_TYPE, DT_0},
      {""}, {""},
#line 241 "adudtab.txt"
      {"Gr",		"r", DB_DTE_TYPE, DT_0},
      {""},
#line 180 "adudtab.txt"
      {"Un-m-n",		"dmy", DB_DTE_TYPE, DT_0},
      {""},
#line 213 "adudtab.txt"
      {"Fr",		"r", DB_DTE_TYPE, DT_0},
#line 283 "adudtab.txt"
      {"Dr",		"r", DB_DTE_TYPE, DT_0},
      {""}, {""}, {""}, {""},
#line 182 "adudtab.txt"
      {"Un.n.n",		"ymd", DB_DTE_TYPE, DT_0},
      {""}, {""},
#line 193 "adudtab.txt"
      {"Nn-m-n",		"dmy", DB_DTE_TYPE, DT_0},
#line 262 "adudtab.txt"
      {"Mm-n-n",		"mdy", DB_DTE_TYPE, DT_0},
#line 150 "adudtab.txt"
      {"An:n:n",			"his",		DB_TMWO_TYPE,	DT_T },
      {""}, {""},
#line 220 "adudtab.txt"
      {"In-m-n",		"dmy", DB_DTE_TYPE, DT_0},
      {""},
#line 151 "adudtab.txt"
      {"An:n:n.n",		"hisl",		DB_TMWO_TYPE,	DT_T },
#line 195 "adudtab.txt"
      {"Nn.n.n",		"ymd", DB_DTE_TYPE, DT_0},
#line 264 "adudtab.txt"
      {"Mn.n.n",		"mdy", DB_DTE_TYPE, DT_0},
      {""}, {""},
#line 234 "adudtab.txt"
      {"Gn-m-n",		"dmy", DB_DTE_TYPE, DT_0},
#line 222 "adudtab.txt"
      {"In.n.n",		"ymd", DB_DTE_TYPE, DT_0},
#line 154 "adudtab.txt"
      {"An:n:n-n:n",		"hiscv",	DB_TMW_TYPE,	DT_T },
      {""},
#line 206 "adudtab.txt"
      {"Fn-m-n",		"dmy", DB_DTE_TYPE, DT_0},
#line 276 "adudtab.txt"
      {"Dn-m-n",		"dmy", DB_DTE_TYPE, DT_0},
      {""},
#line 155 "adudtab.txt"
      {"An:n:n.n-n:n",		"hislcv",	DB_TMW_TYPE,	DT_T },
#line 236 "adudtab.txt"
      {"Gn.n.n",		"dmy", DB_DTE_TYPE, DT_0},
      {""}, {""}, {""},
#line 208 "adudtab.txt"
      {"Fn.n.n",		"ymd", DB_DTE_TYPE, DT_0},
#line 278 "adudtab.txt"
      {"Dn.n.n",		"dmy", DB_DTE_TYPE, DT_0},
      {""}, {""},
#line 152 "adudtab.txt"
      {"An:n:n+n:n",		"hisbv",	DB_TMW_TYPE,	DT_T },
      {""}, {""}, {""}, {""},
#line 153 "adudtab.txt"
      {"An:n:n.n+n:n",		"hislbv",	DB_TMW_TYPE,	DT_T }
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = adu_dttemplate_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].dt_tmplt;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
#line 289 "adudtab.txt"

