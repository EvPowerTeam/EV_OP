        echo 1 > /proc/sys/net/ipv4/ip_forward
        for each in /proc/sys/net/ipv4/conf/*
        do
                echo 0 > $each/accept_redirects
                echo 0 > $each/send_redirects
        done
        sysctl -p
        sleep 3
        echo "d 9qu" > ../var/run/xl2tpd/l2tp-control
        sleep 3
        ipsec down 9qu
        sleep 3
        ADDR="vpn.1-chong.com"
        vpnip=`ping ${ADDR} -c 1 | sed '1{s/[^(]*(//;s/).*//;q}'`
        sed -i '57d' /etc/ipsec.conf
        sed -i " 56a \        rightid       = ${vpnip} " /etc/ipsec.conf
        ipsec restart
        sleep 5
        ./etc/init.d/xl2tpd restart
        sleep 5
        ipsec up 9qu
        sleep 2
        echo "c 9qu" > ../var/run/xl2tpd/l2tp-control
        sleep 6
        PPP_INT=`/bin/sh /root/getint.sh`
        echo $PPP_INT
        ifplugd -I -f -i $PPP_INT -r /root/if.sh
        sleep 1
        PPP_GW_ADD=`/bin/sh /root/getip.sh $PPP_INT`
        echo $PPP_GW_ADD
        sleep 2
        route add -net 10.168.0.0 gateway $PPP_GW_ADD netmask 255.255.0.0 dev $PPP_INT
        route add -net 10.168.0.0 gateway 10.168.1.1 netmask 255.255.0.0 dev ppp1
        route add -net 10.168.0.0 gateway 10.168.1.1 netmask 255.255.0.0 dev ppp2
        route add -net 10.168.0.0 gateway 10.168.1.1 netmask 255.255.0.0 dev ppp3
        sleep 6
        /bin/dashboard udpserver
        /bin/dashboard checkin 4

