#!/bin/sh

case $1 in
        "stop")
        killall -9 watchquagga
        killall -9 ospfd
        killall -9 zebra
        killall -9 imish
        ;;
        "start")
        cd /app/rt-platform/sbin
        ./zebra -d
        ./ospfd -d
        ./watchquagga -daz -r './service.sh %s restart' -s './service.sh %s start' -k './service.sh %s stop' zebra ospfd

        cd /app/rt-platform/bin
        ./imish -d
        ;;
        *)
        echo "nothing to do"
        echo "you may ./rt-service.sh start/stop"
        exit 0
        ;;
esac  
