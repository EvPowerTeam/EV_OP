#!/bin/bash

HOME_PATH=/root
ETHNAME=eth0

SafeExec()
{
    $*
    if [ $? -ne 0 ];
    then
    echo "[ERROR]: Failed to exec $*"
    return 1
    fi
    echo "[info]: succeed to execute $*."
    return 0
}

gen_username()
{
    echo "start gen username"
    USERNAME=`ifconfig ${ETHNAME}|grep HWaddr|awk '{print $5}'|md5sum |cut -c 1-32`
    echo "USERNAME IS", ${USERNAME}
    sed -i "s/^username=.*/username=${USERNAME}/" ${HOME_PATH}/.svpn/l3vpn/vpn.ini &
    if [ $? -eq 0 ];then
        return 0
    fi
    return 1
}

start_vpn()
{
    SafeExec ln -f ${HOME_PATH}/.svpn/l3vpn/l3vpnd.sh /etc/rc.d/S91svpnd
    if [ $? -ne 0 ];then
        return 1
    fi
    chmod +x ${HOME_PATH}/.svpn/l3vpn/l3vpnd
    chmod +x ${HOME_PATH}/.svpn/l3vpn/l3vpn_check.sh
    if [ $? -ne 0 ];then
        return 1
    fi
    ls /lib/libstdc++.so.6
    if [ $? -eq 0 ]
    then
        echo "/lib/libstdc++.so.6 already exit"
    else
        echo "can't find  /lib/libstdc++.so.6"
        SafeExec ln -f -s ${HOME_PATH}/.svpn/l3vpn/libs/libstdc++.so.6 /lib/libstdc++.so.6
    fi

    kill -9 `pgrep l3vpnd` > /dev/null  2>&1 &
    ${HOME_PATH}/.svpn/l3vpn/l3vpn_check.sh >/dev/null 2>&1 &
    #./l3vpnd > /dev/null & 2>&1
    SafeExec kill -6 `pidof l3vpnd`
    if [ 0 -ne $? ]
    then
        echo "start l3vpnd failed"
        return 1
    else
        echo "start l3vpnd success"
        return 0
    fi
}

if [ $# -lt 1 ]
then
    echo "usage: sh l3vpn.sh install/uninstall"
    exit 1
fi

echo $*
if [ "start" = $* ]
then
    start_vpn
    if [ 0 -eq $? ]
    then
        echo "---------install l3vpn success--------"
	kill -6 `pidof l3vpnd`
        exit 0
    fi
        echo "---------install l3vpn failed--------"
else
    echo "usage: sh l3vpn.sh start"
fi
