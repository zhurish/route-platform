/*
 * ip_iptables.c
 *
 *  Created on: Oct 16, 2016
 *      Author: zhurish
 */

/*
即时生效，重启后复原
开启： service iptables start
关闭： service iptables stop
需要说明的是对于Linux下的其它服务都可以用以上命令执行开启和关闭操作。
也可以做简单的修改。在开启了防火墙时，做如下设置，开启相关端口，
修改/etc/sysconfig/iptables 文件，添加以下内容：
-A RH-Firewall-1-INPUT -m state –state NEW -m tcp -p tcp –dport 80 -j ACCEPT
-A RH-Firewall-1-INPUT -m state –state NEW -m tcp -p tcp –dport 21 -j ACCEPT
-A RH-Firewall-1-INPUT -m state --state NEW -m tcp -p tcp --dport 22 -j ACCEPT
-A RH-Firewall-1-INPUT -m state --state NEW -m tcp -p tcp --dport 23 -j ACCEPT
80端口是http服务，21端口是ftp服务，22端口是ssh服务，23端口是telnet服务且均是采用TCP协议
 */
