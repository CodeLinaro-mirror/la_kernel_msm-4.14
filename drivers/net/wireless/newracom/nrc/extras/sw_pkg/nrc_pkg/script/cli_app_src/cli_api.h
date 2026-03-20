#ifndef __CLI_API_H__
#define __CLI_API_H__

/* APF API */

struct cli_api_apf_cap {
	int version;
	int maxlen;
};

#define MAX_APF_LEN				2048
#define MAX_BIN_BUFFER_SIZE			MAX_APF_LEN
#define MAX_HEX_BUFFER_SIZE			(MAX_BIN_BUFFER_SIZE * 2 + 10) /* \n or \0 ..etc */

struct cli_api_apf_filter {
	int len;
	int offset;
	uint8_t data[MAX_APF_LEN];
};

int cli_api_apf_set_enable (bool enable);
int cli_api_apf_get_enable (bool *enable);
int cli_api_apf_get_cap (struct cli_api_apf_cap *cap);
int cli_api_apf_set_filter (uint8_t *buf, int len);
int cli_api_apf_get_filter (uint8_t *buf, int offset, int len);

#endif
