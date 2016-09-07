        echo "`date`"+":Sonic VPN reconnected" >> /mnt/umemory/routerlog/vpn.log
        sleep 3
        ipsec down sonicwall
        sleep 1
        echo "d sonicwall" > ../var/run/xl2tpd/l2tp-control
        sleep 1
        ipsec up sonicwall
        sleep 2
        echo "c sonicwall" > ../var/run/xl2tpd/l2tp-control
        sleep 6
        PPP_INT=`./root/getint.sh`
        echo $PPP_INT
        sleep 1
        PPP_GW_ADD=`./root/getip.sh $PPP_INT`
        echo $PPP_GW_ADD
	route add -net 192.168.168.0 gateway 192.168.168.168 netmask 255.255.255.0 dev ppp1
	route add -net 192.168.168.0 gateway 192.168.168.168 netmask 255.255.255.0 dev ppp2

