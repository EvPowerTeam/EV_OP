//#include "main.h"
#ifndef BLESCAN_H
#define BLESCAN_H

#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#define Null 0
#define TRUE 1
#define FALSE 0

#define FLAGS_AD_TYPE 0x01
#define FLAGS_LIMITED_MODE_BIT 0x01
#define FLAGS_GENERAL_MODE_BIT 0x02
#define EIR_NAME_SHORT 0x08  /* shortened local name */
#define EIR_NAME_COMPLETE 0x09  /* complete local name */

struct MACARON_BLE_INFO
{
	uint8_t uuid[30];
	uint8_t major[2];
	uint8_t minor[2];
	uint8_t extra[2];
	char addr[18];
	int  rssi;
	struct MACARON_BLE_INFO *next;
};
#define MACARON_BLE_LEN sizeof(struct MACARON_BLE_INFO)

struct MACARON_BLE_INFO *head;
int counter;

//int getAdvertisingDevices(int sock);

int getAdvertisingDevices(void *args);

void stopScan();

int getRssi(le_advertising_info *info);

int parseData(le_advertising_info *info);

int parseApplication(le_advertising_info *info);

int isMacaron(uint8_t *data);

void sleepBeacon(const bdaddr_t *ba);

int isNewMacaron(const bdaddr_t *ba, int rssi);

int hciConfigure(int sock, struct hci_filter of);

void resetData();

#endif