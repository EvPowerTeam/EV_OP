
//#define GATEWAY_SPEED "0-KBit"
#define TAB_POS	"chargerinfo.TABS.tables"

/* path used when calling the checkin API */
#define API_UPDATE_FMT "/ChargerState/updateState"
#define API_START_CHARGING_FMT "/Charging/canStartCharging"
#define API_CHARGING_RECORD_FMT "/ChargingRecord/uploadRecord"
#define JSON_MAX 2048

#define UCI_DATABASE_NAME "chargerinfo"
#define UCI_DATABASE_CMD_NAME "serverinfo"
#define UIC_TABLE_CHARGER "charger%d"
#define MAX_LEN 255

int dashboard_checkin_now(int argc, char **argv);

int dashboard_checkin(void *arg);

int dashboard_url_post(int argc, char **argv, char *extra_arg);

int dashboard_post_file(int argc, char **argv, char *extra_arg);

int dashboard_update_fastcharger(void *arg);
