        echo 1 > /proc/sys/net/ipv4/ip_forward
        for each in /proc/sys/net/ipv4/conf/*
        do
                echo 0 > $each/accept_redirects
                echo 0 > $each/send_redirects
        done
        sysctl -p
	echo "`date` "+":Windcloud VPN reconnected" >> /mnt/umemory/routerlog/vpn.log 
        sleep 3
        echo "d evpowergroup" > ../var/run/xl2tpd/l2tp-control
        sleep 3
        ipsec down evpowergroup
	sleep 2
        ipsec up evpowergroup
        sleep 2
        echo "c evpowergroup" > ../var/run/xl2tpd/l2tp-control
        sleep 6
        PPP_INT=`./root/getint.sh`
        echo $PPP_INT
        sleep 1
        PPP_GW_ADD=`./root/getip.sh $PPP_INT`
        echo $PPP_GW_ADD
	sleep 2
        route add -net 10.9.8.0 gateway $PPP_GW_ADD netmask 255.255.248.0 dev $PPP_INT
        route add -net 10.9.8.0 gateway 10.9.8.100 netmask 255.255.248.0 dev ppp3
	route add -net 10.9.8.0 gateway 10.9.8.100 netmask 255.255.248.0 dev ppp1
	route add -net 10.9.8.0 gateway 10.9.8.100 netmask 255.255.248.0 dev ppp2
	sleep 6 
	/bin/dashboard udpserver
