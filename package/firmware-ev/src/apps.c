
#include "ev.h"
#include "apps.h"
#include <ev.h>
#include <apps/init_config.h>
#include <apps/init_boot.h>
#include <apps/init_vpn.h>
#include <apps/ev_upgrade.h>

#include <stdlib.h>

EV_APP(init_config, init_config, NULL, NULL, NULL, NULL);
EV_APP(init_boot, init_boot_boot, NULL, NULL, NULL, NULL);
EV_APP(upgrade, ev_upgrade, NULL, NULL, NULL, NULL);
EV_APP(download, ev_download, NULL, NULL, NULL, NULL);
EV_APP(install, ev_install, NULL, NULL, NULL, NULL);
EV_APP(init_vpn, init_vpn_check, NULL, init_vpn_up, init_vpn_down, NULL);

#if defined(EV_DEBUG)
//EV_APP(new_config, new_config_wrapper, new_config_wrapper, NULL);
#endif

const struct ev_app *ev_apps[] = {
	&app_init_boot,
	&app_init_config,
	&app_init_vpn,
	&app_upgrade,
	&app_download,
	&app_install,
#if defined(EV_DEBUG)
#endif
	NULL,
};
