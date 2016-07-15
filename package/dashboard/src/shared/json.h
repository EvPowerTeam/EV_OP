
void json_array_open(struct checkin_state *checkin_state, const char *name);
void json_array_close(struct checkin_state *checkin_state, int comma);
void json_array(struct checkin_state *checkin_state, const char *name,
		const char *value, int comma);
void json_object_open_unnamed(struct checkin_state *checkin_state);
void json_object_open_named(struct checkin_state *checkin_state, char *name);
void json_object_close(struct checkin_state *checkin_state, int comma);
void json_var_value_print_string(struct checkin_state *checkin_state, char *var,
				 char *value, int comma);
void json_var_value_print_string_nokey(struct checkin_state *checkin_state,
				       char *value, int comma);
void json_var_value_print_int_stringified(struct checkin_state *checkin_state, char *var,
					  char *value, int comma);
void json_var_value_print_int64(struct checkin_state *checkin_state, char *var,
				int64_t value, int comma);
void json_var_value_print_uint64(struct checkin_state *checkin_state, char *var,
				 uint64_t value, int comma);
void json_var_value_print_uint64_nokey(struct checkin_state *checkin_state,
				       uint64_t value, int comma);
void json_var_value_print_float(struct checkin_state *checkin_state, char *var,
				float value, int comma);
