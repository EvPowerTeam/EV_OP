
#include "include/apps.h"
#include "include/dashboard.h"
#include "apps/dashboard_controller.h"
#include "apps/dashboard_checkin.h"
#include "apps/dashboard_udpserver.h"
#include "apps/mq.h"


#include <stdlib.h>

DASH_APP(checkin, dshbrd_controller_checkin, NULL, NULL);
DASH_APP(checkin_now, dashboard_checkin_now, NULL, NULL);
DASH_APP(url_post, dashboard_url_post, NULL, NULL);
DASH_APP(dashboard_controller, NULL, dshbrd_controller_start, dshbrd_controller_stop);
DASH_APP(post_file, dashboard_post_file, NULL, NULL);
DASH_APP(udpserver, dashboard_udpserver, NULL, NULL);
DASH_APP(update_fast, dashboard_update_fastcharger, dashboard_update_fastcharger, NULL);
//DASH_APP(charger, dashboard_post_file, startcharging, stopcharging);
//DASH_APP(parse_config, parse_config, parse_config, NULL);

#if defined(DASH_DEBUG)
DASH_APP(mq, mqcreate_debug, mqsend_debug, mqreceive_debug);
#endif

const struct dash_app *dash_apps[] = {
	&dash_app_checkin,
	&dash_app_checkin_now,
	&dash_app_url_post,
	&dash_app_dashboard_controller,
	&dash_app_post_file,
	&dash_app_udpserver,
	&dash_app_update_fast,
	//&dash_app_parse_config,
	//&dash_app_reload,
#if defined(DASH_DEBUG)
	&dash_app_mq,
#endif
	NULL,
};
