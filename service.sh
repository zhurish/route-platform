#!/bin/sh

cd /app/rt-platform/sbin
case $2 in
	"stop")
	pkill -9 $1
	;;
	"start")
	./$1 -d
	;;
	"restart")
	pkill -9 $1
	./$1 -d
	;;
	*)
	;;
esac
exit 0	
