
/*
 * firewall-cmd --get-default-zone
 * firewall-cmd --get-zones
 * firewall-cmd --zone=public --list-all
 * 使用firewall-cmd --set-default-zone=home命令，该命令可用于从公共网络到家庭网络制定一个默认网络
 * 命令firewall-cmd --zone=public --add-service=ftp，在Linux防火墙的公共区域上添加FTP服务。
 *
 * 使用firewall-cmd --permanent --zone=dmz --add-port=22/tcp命令为特定网络指定特定端口，然后使用firewall-cmd --zone=dmz --list-all确认端口已成功添加
 *
 * firewall-cmd --direct --add-rule ipv4 filter INPUT 0 -p tcp --dport 80 -j ACCEPT
 *
 * firewall-cmd--permanent--zone=public--add-rich-rule="rule family="ipv4" source address="192.168.4.0/24" service name="tftp" log prefix="tftp"
 * firewall-cmd --permanent --zone=public --add-rich-rule="rule family="ipv4" \
 *   source address="192.168.0.4/24" service name="http" accept"
 *
 */
