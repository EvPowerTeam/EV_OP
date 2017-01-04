#include "blescan.h"

volatile int signal_received = 0;
size_t uuid_len = 16;

uint8_t MACARON_UUID[] = {0X4d,0X61,0X63,0X61,0X72,0X6f,0X6e,0X05,0X12,0X50,0X00,0X20,0X03,0X02,0X0A,0X00};
uint8_t temp_uuid[16];
char temp_str[18];
int is_running = 1;
char command_str[100];

struct MACARON_BLE_INFO *p1,*p2;

//int getAdvertisingDevices(int sock)
int getAdvertisingDevices(void *arg)
{
	unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;
	struct hci_filter of;
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
			//printf("----------------------skip----------------------\n");
			continue;
			//goto done;
		}
		/* Ignoring multiple reports */
		info = (le_advertising_info *) (meta->data + 1);
	
		/*int i=0;
		printf("receive data = ");
		for(i=0; i<info->length; i++) {
			printf("%x ", *(info->data + i));
		}
		printf("\n");*/

		uint8_t evt_type = info->evt_type;
		//printf("evt_type = %d\n", evt_type);
		if(evt_type == 4) {
			if(parseData(info) == TRUE) {
				/*if(head != Null) {
					struct MACARON_BLE_INFO *p;
					p = head;
					int index = 0;
					while(index < counter) {
						printf("\n");
						printf("---------------------------------------------------------\n" );
						printf("uuid = ");
						i = 0;
						for(;i < uuid_len; i++) {
							printf("%x ", p->uuid[i]);
						}
						printf("\n");
						printf("major = %x %x\n", p->major[0], p->major[1]);
						printf("minor = %x %x\n", p->minor[0], p->minor[1]);
						printf("address: %s (RSSI: %d)\n", p->addr, p->rssi);
						printf("---------------------------------------------------------\n" );
						p = p->next;
						index++;
					}
				}
				printf("\n" );*/
			}
		}else if(evt_type == 0) {
			if(parseApplication(info) == TRUE) {
				/*if(head != Null) {
					struct MACARON_BLE_INFO *p;
					p = head;
					int index = 0;
					while(index < counter) {
						printf("\n");
						printf("---------------------------------------------------------\n" );
						printf("uuid = ");
						i = 0;
						for(;i < uuid_len; i++) {
							printf("%x ", p->uuid[i]);
						}
						printf("\n");
						printf("major = %x %x\n", p->major[0], p->major[1]);
						printf("minor = %x %x\n", p->minor[0], p->minor[1]);
						printf("extra = %x %x\n", p->extra[0], p->extra[1]);
						printf("address: %s (RSSI: %d)\n", p->addr, p->rssi);
						printf("---------------------------------------------------------\n" );
						p = p->next;
						index++;
					}
				}
				printf("\n" );*/
			}
		}
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

int parseData(le_advertising_info *info) 
{
	uint8_t *data = info->data;
	switch(data[1]) {
		case EIR_NAME_SHORT:
		case EIR_NAME_COMPLETE:
			if(isMacaron(data) == TRUE) {
				if(isNewMacaron(&info->bdaddr, getRssi(info)) == TRUE) {
					//sleepBeacon(&info->bdaddr);
					counter++;
					p1 = (struct MACARON_BLE_INFO *) malloc(MACARON_BLE_LEN);
					// set uuid
					memcpy(p1->uuid, &data[2], uuid_len);
					// set major
					//memcpy(p1->major, &data[2 + uuid_len], 2);
					// set minor
					//memcpy(p1->minor, &data[2 + uuid_len + 2], 2);
					// set mac address
					ba2str(&info->bdaddr, p1->addr);
					// set rssi
					p1->rssi = getRssi(info);
					p1->extra[0] = 0x00;
					p1->extra[1] = 0x00;
					if(counter == 1) {
						head = p1;
					} else {
						p2 -> next = p1;
					}
					p2 = p1;
					return TRUE;
				}
			}
		break;
		default:
			//printf("Wrong format\n");
		break;
	}
	return FALSE;
}

int parseApplication(le_advertising_info *info)
{
	uint8_t *data = info->data;
	switch(data[1]) {
		case FLAGS_LIMITED_MODE_BIT:
			if(isNewMacaron(&info->bdaddr , 0) == TRUE) {
				counter++;
				p1 =  (struct MACARON_BLE_INFO *) malloc(MACARON_BLE_LEN);
				// set mac address
				ba2str(&info->bdaddr, p1->addr);
				// set major
				memcpy(p1->major, &data[9], 2);
				// set minor
				memcpy(p1->minor, &data[9 + 2], 2);
				// set extra
				memcpy(p1->extra, &data[9 + 2 + 2], 2);
				if(counter == 1) {
						head = p1;
				} else {
					p2 -> next = p1;
				}
				p2 = p1;
				return TRUE;
			}
		break;
		default:
			//printf("Wrong format\n");
		break;
	}
	return FALSE;
}

// check this a Macaron device
int isMacaron(uint8_t *data) {
	memcpy(temp_str, &data[2], uuid_len);
	int i = 0;
	for(; i<uuid_len; i++) {
		if(temp_str[i] != MACARON_UUID[i]) {
			//printf("It is not macaron device.\n\n");
			return FALSE;
		}
	}
	return TRUE;
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

// check this a new Macaron
int isNewMacaron(const bdaddr_t *ba, int rssi) {
	struct MACARON_BLE_INFO *p;
	p = head;
	
	ba2str(ba, temp_str);
	//printf("mac = %s\n", temp_str);
	if(head != Null) {
		int i = 0;
		while(i < counter) {
			if(strcmp(temp_str, p->addr) == 0) {
				//printf("The macaron device exists.\n\n");
				if(rssi != 0) {
					p->rssi = rssi;
				}
				return FALSE;
			}
			p = p->next;
			i++;
		}
	}


	return TRUE;
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