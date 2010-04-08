/* C code produced by gperf version 3.0.2 */
/* Command-line: gperf -m 50 adumtab.txt  */
/* Computed positions: -k'2-3' */

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
error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

/* maximum key range = 25, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
adu_monthname_hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static const unsigned char asso_values[] =
    {
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28,  0, 15,  8,
      16,  1, 28, 18, 28, 28, 28, 28, 16, 28,
       1,  6,  6, 28,  0, 28,  3,  2,  6, 16,
      28, 10, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
      28, 28, 28, 28, 28, 28
    };
  return len + asso_values[(unsigned char)str[2]] + asso_values[(unsigned char)str[1]];
}

#ifdef __GNUC__
__inline
#endif
const struct _ADU_DATENAME *
adu_monthname_lookup (str, len)
     register const char *str;
     register unsigned int len;
{
  enum
    {
      TOTAL_KEYWORDS = 25,
      MIN_WORD_LENGTH = 3,
      MAX_WORD_LENGTH = 9,
      MIN_HASH_VALUE = 3,
      MAX_HASH_VALUE = 27
    };

  static const struct _ADU_DATENAME wordlist[] =
    {
      {""}, {""}, {""},
#line 81 "adumtab.txt"
      {"mar", 'm', 3},
#line 77 "adumtab.txt"
      {"jan", 'm', 1},
#line 82 "adumtab.txt"
      {"march", 'm', 3},
#line 86 "adumtab.txt"
      {"jun", 'm', 6},
#line 87 "adumtab.txt"
      {"june", 'm', 6},
#line 78 "adumtab.txt"
      {"january", 'm', 1},
#line 83 "adumtab.txt"
      {"apr", 'm', 4},
#line 92 "adumtab.txt"
      {"sep", 'm', 9},
#line 84 "adumtab.txt"
      {"april", 'm', 4},
#line 98 "adumtab.txt"
      {"dec", 'm', 12},
#line 85 "adumtab.txt"
      {"may", 'm', 5},
#line 94 "adumtab.txt"
      {"oct", 'm', 10},
#line 96 "adumtab.txt"
      {"nov", 'm', 11},
#line 93 "adumtab.txt"
      {"september", 'm', 9},
#line 99 "adumtab.txt"
      {"december", 'm', 12},
#line 95 "adumtab.txt"
      {"october", 'm', 10},
#line 79 "adumtab.txt"
      {"feb", 'm', 2},
#line 97 "adumtab.txt"
      {"november", 'm', 11},
#line 88 "adumtab.txt"
      {"jul", 'm', 7},
#line 89 "adumtab.txt"
      {"july", 'm', 7},
#line 90 "adumtab.txt"
      {"aug", 'm', 8},
#line 80 "adumtab.txt"
      {"february", 'm', 2},
#line 101 "adumtab.txt"
      {"now", 'R', 0},
#line 91 "adumtab.txt"
      {"august", 'm', 8},
#line 100 "adumtab.txt"
      {"today", 'Z', 0}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = adu_monthname_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].d_name;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
