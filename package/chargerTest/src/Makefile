

DST = chargerTest

$(DST):main.o AES.o CRC.o uci.o net_info.o file.o
	$(CC)  $^  -lpthread -luci -lcrypto -L/lib/x86_64-linux-gnu/ -o $@

net_info.o:net_info.c
	$(CC) -c $<
main.o:main.c
	$(CC) -c   $<
AES.o:AES.c
	$(CC) -c  $<
CRC.o:CRC.c
	$(CC) -c $<
uci.o:uci.c
	$(CC) -c $<

clean:
	rm -rf *.o  $(DST)

