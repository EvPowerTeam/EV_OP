
#include "include/apps.h"
#include "include/dashboard.h"
#include "include/dashboard_controller.h"
#include "apps/dashboard_checkin.h"

#include <stdlib.h>

DASH_APP(checkin_now, dashboard_checkin_now, NULL, NULL);
DASH_APP(dashboard_controller, NULL, dshbrd_controller_start, dshbrd_controller_stop);
DASH_APP(checkin, dshbrd_controller_checkin, NULL, NULL);
//DASH_APP(parse_config, parse_config, parse_config, NULL);

#if defined(DASH_DEBUG)
#endif

const struct dash_app *dash_apps[] = {
	&dash_app_checkin,
	&dash_app_checkin_now,
	&dash_app_dashboard_controller,
	//&dash_app_parse_config,
	//&dash_app_reload,
#if defined(DASH_DEBUG)
#endif
	NULL,
};
