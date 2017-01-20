#include "blescan.h"

volatile int signal_received = 0;
size_t uuid_len = 16;

uint8_t temp_uuid[16];
char temp_str[18];
int is_running = 1;
char command_str[100];

struct MACARON_BLE_INFO *p1,*p2;


static void eir_parse_name(uint8_t *eir, size_t eir_len,
						char *buf, size_t buf_len)
{
	size_t offset;

	offset = 0;
	while (offset < eir_len) {
		uint8_t field_len = eir[0];
		size_t name_len;

		/* Check for the end of EIR */
		if (field_len == 0)
			break;

		if (offset + field_len > eir_len)
			goto failed;

		switch (eir[1]) {
		case EIR_NAME_SHORT:
		case EIR_NAME_COMPLETE:
			name_len = field_len - 1;
			if (name_len > buf_len)
				goto failed;

			memcpy(buf, &eir[2], name_len);
			return;
		}

		offset += field_len + 1;
		eir += field_len + 1;
	}

failed:
	snprintf(buf, buf_len, "(unknown)");
}

//int getAdvertisingDevices(int sock)
int getAdvertisingDevices(void *arg)
{
	unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;
	struct hci_filter of;
	char name[12];
	int len = 0;
	int sock = *(int *)arg;

	if (hciConfigure(sock, of) == -1)
		return -1;

	counter = 0;
	head = Null;
	while (is_running) {
		evt_le_meta_event *meta;
		le_advertising_info *info;
		while ((len = read(sock, buf, sizeof(buf))) < 0) {
			if (errno == EINTR && signal_received == SIGINT) {
				len = 0;
				goto done;
			}

			if (errno == EAGAIN || errno == EINTR)
				continue;
			goto done;
		}
		ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
		len -= (1 + HCI_EVENT_HDR_SIZE);

		meta = (void *) ptr;

		if (meta->subevent != 0x02)
		{
			printf("----------------------skip----------------------\n");
			continue;
			//goto done;
		}
		/* Ignoring multiple reports */
		info = (le_advertising_info *) (meta->data + 1);
		uint8_t evt_type = info->evt_type;
		/**/
		int i=0;
		ba2str(&info->bdaddr, temp_str);
		printf("receive data = ");
		for(i=0; i<info->length; i++) {
			printf("%x ", *(info->data + i));
		}
		printf("\tevt_type = %d\n", evt_type);
		printf("\taddress = %s\n", temp_str);
		if (evt_type == 0) {
			eir_parse_name(info->data, info->length,
				       name, sizeof(name) - 1);
			printf("\tname = %s\n", name);
		}
		printf("*********************************\n");
		
	}
	p2->next = Null;
done:
	p2->next = Null;
	setsockopt(sock, SOL_HCI, HCI_FILTER, &of, sizeof(of));

	if (len < 0)
		return -1;

	return 0;
}

void stopScan(int sig)
{
	signal_received = sig;
	is_running = 0;
}



int hciConfigure(int sock, struct hci_filter of)
{
	struct hci_filter nf;
	socklen_t olen;

	olen = sizeof(of);
	if (getsockopt(sock, SOL_HCI, HCI_FILTER, &of, &olen) < 0) {
		printf("Could not get socket options\n");
		return -1;
	}

	hci_filter_clear(&nf);
	hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
	hci_filter_set_event(EVT_LE_META_EVENT, &nf);

	if (setsockopt(sock, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
		printf("Could not set socket options\n");
		return -1;
	}

	return 0;
}

void sleepBeacon(const bdaddr_t *ba)
{
	memset(command_str,0,strlen(command_str));
	strcat(command_str, "gatttool -b ");
	ba2str(ba, temp_str);
	strcat(command_str, temp_str);
	strcat(command_str, " --char-write-req --handle=0x001f --value=0a000000 &");
	printf("\n%s\n", command_str);
	system(command_str);
}

int getRssi(le_advertising_info *info) 
{
  if(!info)
		return 127;  /* "RSSI not available" */
	return *(((int8_t *) info->data) + info->length);
}

void resetData()
{
	//printf("clear data\n");
	counter = 0;
	head = Null;
}
