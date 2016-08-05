
#define APP_NAME_MAX 25

struct dash_app {
	char name[APP_NAME_MAX];
	int (*dflt)(int argc, char **argv);
	int (*start)(int argc, char **argv);
	int (*stop)(int argc, char **argv);
};

#define DASH_APP(_name, _dflt, _start, _stop) 		\
const struct dash_app dash_app_##_name = {		\
	.name = #_name,					\
	.dflt = _dflt,					\
	.start = _start,				\
	.stop = _stop,					\
}

extern const struct dash_app *dash_apps[];
