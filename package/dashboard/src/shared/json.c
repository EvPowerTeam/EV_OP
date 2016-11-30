
#include "include/dashboard.h"
#include "json.h"


static void json_tab_depth_print(struct checkin_state *checkin_state)
{
	int i;

	for (i = 0; i < checkin_state->tab_depth; i++)
		fprintf(checkin_state->fd, "\t");
}

void json_array_open(struct checkin_state *checkin_state, const char *name)
{
	json_tab_depth_print(checkin_state);
	fprintf(checkin_state->fd, "\"%s\" : [\n", name);
	checkin_state->tab_depth++;
}

void json_array(struct checkin_state *checkin_state, const char *name,
		const char *value, int comma)
{
	json_tab_depth_print(checkin_state);
	fprintf(checkin_state->fd, "\"%s\" :\n", name);
	checkin_state->tab_depth++;
	json_tab_depth_print(checkin_state);
	fprintf(checkin_state->fd, "%s%s\n", value, comma ? "," : "");
	checkin_state->tab_depth--;
}

void json_array_close(struct checkin_state *checkin_state, int comma)
{
	checkin_state->tab_depth--;
	json_tab_depth_print(checkin_state);
	fprintf(checkin_state->fd, "]%s\n", comma ? "," : "");
}

void json_object_open_unnamed(struct checkin_state *checkin_state)
{
	json_tab_depth_print(checkin_state);
	fprintf(checkin_state->fd, "{\n");
	checkin_state->tab_depth++;
}

void json_object_open_named(struct checkin_state *checkin_state, char *name)
{
	json_tab_depth_print(checkin_state);
	fprintf(checkin_state->fd, "\"%s\" : {\n", name);
	checkin_state->tab_depth++;
}

void json_object_close(struct checkin_state *checkin_state, int comma)
{
	checkin_state->tab_depth--;
	json_tab_depth_print(checkin_state);
	fprintf(checkin_state->fd, "}%s\n", comma ? "," : "");
}

void json_var_value_print_string(struct checkin_state *checkin_state, char *var,
				 char *value, int comma)
{
	json_tab_depth_print(checkin_state);
	fprintf(checkin_state->fd, "\"%s\" : \"%s\"%s\n", var, value, comma ? "," : "");
}

void json_var_value_print_string_nokey(struct checkin_state *checkin_state,
				       char *value, int comma)
{
	json_tab_depth_print(checkin_state);
	fprintf(checkin_state->fd, "\"%s\"%s\n", value, comma ? "," : "");
}

void json_var_value_print_int_stringified(struct checkin_state *checkin_state, char *var,
					  char *value, int comma)
{
	json_tab_depth_print(checkin_state);
	fprintf(checkin_state->fd, "\"%s\" : %s%s\n", var, value, comma ? "," : "");
}

void json_var_value_print_int64(struct checkin_state *checkin_state, char *var,
				int64_t value, int comma)
{
	json_tab_depth_print(checkin_state);
	fprintf(checkin_state->fd, "\"%s\" : %lld%s\n", var, value, comma ? "," : "");
}

void json_var_value_print_uint64(struct checkin_state *checkin_state, char *var,
				 uint64_t value, int comma)
{
	json_tab_depth_print(checkin_state);
	fprintf(checkin_state->fd, "\"%s\" : %llu%s\n", var, value, comma ? "," : "");
}

void json_var_value_print_uint64_nokey(struct checkin_state *checkin_state,
				       uint64_t value, int comma)
{
	json_tab_depth_print(checkin_state);
	fprintf(checkin_state->fd, "%llu%s\n", value, comma ? "," : "");
}

void json_var_value_print_float(struct checkin_state *checkin_state, char *var,
				float value, int comma)
{
	json_tab_depth_print(checkin_state);
	fprintf(checkin_state->fd, "\"%s\" : %.2f%s\n", var, value,
		comma ? "," : "");
}
