#!/bin/sh
echo "if1.sh"
echo "run" >> /tmp/record
if [ $# -eq 2 ];then
   if [ $2 = "up" ];then
   echo "`date` "+":9qu VPN connected ifplugd" >> /mnt/umemory/routerlog/vpn.log
   elif [ $2 = "down" ];then
   echo "`date` "+":9qu VPN disconnected ifplugd" >> /mnt/umemory/routerlog/vpn.log
   /bin/sh /root/setup_9qu.sh >/tmp/vpn.log 2>&1
   elif [ $2 = "disable" ];then
   echo "`date` "+":9qu VPN disabled ifplugd" >> /mnt/umemory/routerlog/vpn.log
   elif [ $2 = "error" ];then
   echo "`date` "+":error ifplugd" >> /mnt/umemory/routerlog/vpn.log
   fi
fi

