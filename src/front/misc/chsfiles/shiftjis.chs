0-8     c       # shiftjis
9-D     cw      # CR's, LF's, FF's, etc.
E-1F    c
20      pw      # space
21-22   po      # !"
23-24   pon     # #$
25-2F   po      # various printable operators (%&'()*+,~./)
30-39   pndx    # digits (0-9)
3A-3F   po      # various printable operators (:;<=>?)
40      p2on     # "at" sign

41-46   p2sux   61      # A-F
47-5A   p2su    67      # G-Z

5B-5E   p2o     # various printable operators ([\]^)
5F      p2s     # underscore
60      p2o     # grave
61-66   p2slx   # a-f
67-7A   p2sl    # g-z
7B-7E   p2o     # various printable operators

7F      c       #

80      p2      #
81      p12sa    #
82-9F   p12sa    #

A0      p2      #
A1-DF   p2as    #support for Kana set here
E0-EA   p12sa    #
EB-EF   p12sa    #
F0-F9   p12a     #

FA-FC   pa12    #

FD-FF   c       #
