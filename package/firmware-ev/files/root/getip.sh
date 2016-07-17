/sbin/ifconfig $1 | /bin/grep "P-t-P" | awk -F: '{print $3}' | awk '{print $1}'
