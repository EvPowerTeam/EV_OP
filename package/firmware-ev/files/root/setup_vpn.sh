        echo 1 > /proc/sys/net/ipv4/ip_forward
        for each in /proc/sys/net/ipv4/conf/*
        do
                echo 0 > $each/accept_redirects
                echo 0 > $each/send_redirects
        done
        sysctl -p
        sleep 3
        ipsec restart
        /etc/init.d/xl2tpd restart
        ipsec down evpowergroup
        echo "d evpowergroup" > ../var/run/xl2tpd/l2tp-control
        ipsec up evpowergroup
        sleep 3
        echo "c evpowergroup" > ../var/run/xl2tpd/l2tp-control
        sleep 4
        PPP_INT=`./root/getint.sh`
        echo $PPP_INT
        PPP_GW_ADD=`./root/getip.sh $PPP_INT`
        echo $PPP_GW_ADD
        route add -net 10.9.8.0 gateway $PPP_GW_ADD netmask 255.255.248.0 dev $PPP_INT
	route add -net 10.9.8.0 gateway 10.9.8.100 netmask 255.255.248.0 dev ppp1
	route add -net 10.9.8.0 gateway 10.9.8.100 netmask 255.255.248.0 dev ppp2

