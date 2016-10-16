# route-platform
route protocol platform base on quagga

# 
# --- Please add this to your /etc/services ---
#
#
# GNU Zebra services
#

zebrasrv	2600/tcp
zebra		2601/tcp
ripd		2602/tcp
ripng		2603/tcp
ospfd		2604/tcp
bgpd		2605/tcp
ospf6d		2606/tcp
ospfapi		2607/tcp
isisd		2608/tcp
babeld		2609/tcp
olsrd		2610/tcp
pimd		2611/tcp

vrrp		2615/tcp
ldpd		2616/tcp
lldpd		2617/tcp
mplsd		2618/tcp
rsvpd		2619/tcp
vpnd		2620/tcp
utils		2621/tcp

adovd		2612/tcp
frpd		2613/tcp
icrpd		2614/tcp


