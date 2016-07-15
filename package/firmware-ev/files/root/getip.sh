/sbin/ifconfig $1 | grep "P-t-P" | awk -F: '{print $3}' | awk '{print $1}'
