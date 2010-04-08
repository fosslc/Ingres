For more detail on the Application design, please read the file vnodemgr.doc
****************************************************************************


PROBLEM NOTES:
**************

When the application tries to query the list of Servers / Gateways for 
a Virtual Node:
     If the machine of that node is down then the cga_request hangs.
     (it should return an error due to the network problen or others)


