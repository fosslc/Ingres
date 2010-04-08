#   Name: ibmpc437.chs - IBM PC Code Page 437
#
#   Description:
#
#       Standard US/English character set
#       Character Set Classification Codes:
#        p - printable
#        c - control
#        o - operator
#        x - hex digit
#        d - digit
#        u - upper-case alpha
#        l - lower-case alpha
#        a - non-cased alpha
#        w - whitespace
#        s - name start
#        n - name
#        1 - first byte of double-byte character
#        2 - second byte of double-byte character
#
#   History:
#
#       2-nov 1992 (fraser)
#           Created.
#       15-may-2002 (peeje01)
#           Add 'a' property to E1 (referred to as 'Beta')
#           Changed comment to the Unicode Character Name
#           Added Character Set Classification Code table comments
#	21-May-2002 (hanje04)
#	    Change 'a' property to 's' for E1 to correct peeje01 change.
#	13-June-2002 (gupsh01)
#	    Added 'l' property to E1 to retain the alphabet property.
#	14-June-2002 (gupsh01)
#	    Modified 'l' property of E1 to 'a' in order to correct the
#	    previous change. 
#
0-8     c               # Control characters
9-D     cw
E-14    c
15      p               # section character
16-1F   c
20      pw              # space
21-22   po              # !"
23-24   pon             # #$
25-2F   po              # %&'()*+,-./
30-39   pndx            # decimal digits
3A-3F   po              # :;<=>?
40      pon             # "at" sign
41-46   psux    61      # A-F
47-5A   psu     67      # G-Z
5B-5E   po              # \]]^
5F      ps              # underscore
60      po              # apostrophe
61-66   pslx            # a-f
67-7A   psl             # g-z
7B-7E   po              # {|}~
7F      c

80      psu     87      # C cedilla
81      psl             # u umlaut
82      psl             # e acute
83      psa             # a circumflex
84      psl             # a umlaut
85      psa             # a grave
86      psl             # a ring
87      psl             # c cedilla
88      psa             # e circumflex
89      psa             # e umlaut
8A      psa             # e grave
8B      psa             # i umlaut
8C      psa             # i circumflex
8D      psa             # i grave
8E      psu     84      # A umlaut
8F      psu     86      # A ring
90      psu     82      # E acute
91      psl             # ae
92      psu     91      # AE
93      psa             # o circumflex
94      psl             # o umlaut
95      psa             # o grave
96      psa             # u circumflex
97      psa             # u grave
98      psa             # y umlaut
99      psu     94      # O umlaut
9A      psu     81      # U umlaut
9B-9F   po
A0      psa             # a acute
A1      psa             # i acute
A2      psa             # o acute
A3      psa             # u acute
A4      psl             # n tilde
A5      psu     A4      # N tilde
A6-AF   po

B0-DF   c               # graphics (line-drawing) characters

E0      po              # alpha
E1      posa            # LATIN SMALL LETTER SHARP S
E2      po              # gamma - uppercase
E3      po              # pi
E4      po              # sigma - uppercase
E5      po              # sigma
E6      po              # mu
E7      po              # tau
E8      po              # phi - uppercase
E9      po              # theta - upppercase
EA      po              # omega - uppercase
EB      po              # delta
EC      po
ED      po              # phi
EF      po              # epsilon
F0-FD   po
FE-FF   c
