


#define APP_NAME_MAX 20

struct ev_app {
	char name[APP_NAME_MAX];
	int (*dflt)(int argc, char **argv, char *extra_arg);
	int (*boot)(int argc, char **argv);
	int (*start)(int argc, char **argv);
	int (*stop)(int argc, char **argv);
	int (*restart)(int argc, char **argv);
};

#define EV_APP(_name, _dflt, _boot, _start, _stop, _restart) 	\
const struct ev_app app_##_name = {				\
	.name = #_name,						\
	.dflt = _dflt,						\
	.boot = _boot,						\
	.start = _start,					\
	.stop = _stop,						\
	.restart = _restart,					\
}

extern const struct ev_app *ev_apps[];
