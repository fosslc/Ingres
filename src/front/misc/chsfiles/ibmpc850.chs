# ibmpc850.chs
#
#   Character set IBMPC850
#
#   IBM PC Code Page 850
#
#   History:
#
#       11-11-92 (fraser)
#           Picked up file which I think was originally created by
#           philiph.
#
0-8     c
9-D     cw              # CR's, LF's, FF's, etc.
20      pw              # space
21-22   po              # !"
23-24   pon             # #$
25-2F   po              # various printable operators
30-39   pndx            # digits
3A-3F   po              # various printable operators
40      pon             # "at" sign

41-46   psux    61      # A-F
47-5A   psu     67      # G-Z

5B-5E   po              # various printable operators
5F      ps              # underscore
60      po              # grave
61-66   pslx            # a-f
67-7A   psl             # g-z
7B-7E   po              # various printable operators
7F      c

80      psu     87      # C cedilla
81      psl             # u umlaut
82      psl             # e acute
83      psl             # a circumflex
84      psl             # a umlaut
85      psl             # a grave
86      psl             # a ring
87      psl             # c cedilla
88      psl             # e circumflex
89      psl             # e umlaut
8A      psl             # e grave
8B      psl             # i dieresis
8C      psl             # i circumflex
8D      psl             # i grave
8E      psu     84      # A umlaut
8F      psu     86      # A ring

90      psu     82      # E acute
91      psl             # ae
92      psu     91      # AE
93      psl             # o circumflex
94      psl             # o umlaut
95      psl             # o grave
96      psl             # u circumflex
97      psl             # u grave
98      po              # y dieresis
99      psu     94      # O umlaut
9A      psu     81      # U umlaut
9B      psl             # o crossbar
9C      po              # sterling
9D      psu     9B      # O crossbar
9E      po              # printable operator
9F      po              # dutch guilder


A0      psl             # a acute
A1      psl             # i acute
A2      psl             # o acute
A3      psl             # u acute
A4      psl             # n tilde
A5      psu     A4      # N tilde
A6-AF   po

B0-B4   c
B5      psu     A0      # A acute
B6      psu     83      # A circumflex
B7      psu     85      # A grave
B8      po              # copyright (C in circle)
B9-BC   c
BD      po              # cent sign
BE      po              # yen
BF      c

C0-C5   c
C6      psl             # a tilde
C7      psu     C6      # A tilde
C8-CE   c
CF      po

D0      psl             # lower
D1      psu     D0      # upper
D2      psu     88      # E circumflex
D3      psu     89      # E umlaut
D4      psu     8A      # E grave
D5      po
D6      psu     A1      # I acute
D7      psu     8C      # I circumflex
D8      psu     8B      # I dieresis
D9-DC   c
DD      po
DE      psu     8D      # I grave
DF      c

E0      psu     A2      # O acute
E1      psa             # German sharp S
E2      psu     93      # O circumflex
E3      psu     95      # O grave
E4      psl             # o tilde
E5      psu     E4      # O tilde
E6      po              # mu
E7      psl             # Thorn lowercase
E8      psu     E7      # Thorn uppercase
E9      psu     A3      # U acute
EA      psu     96      # U circumflex
EB      psu     97      # U grave
EC      psl             # y acute
ED      psu     EC      # Y acute

EE-FD   po

FE-FF   c
