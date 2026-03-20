/*
 * MIT License
 *
 * Copyright (c) 2024 Newracom, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

//#include "cli_cmd.h"
//#include "cli_util.h"
//#include "cli_xfer.h"
#include "cli_netlink.h"
#include "cli_api.h"


int cli_api_apf_set_enable (bool enable)
{
	int ret;
	int req = 0;
	char resp[NL_MSG_MAX_RESPONSE_SIZE];

	req = enable;

	ret = netlink_send_data(NL_APF_SET_ENABLE, (char *)&req, resp);

	if (ret == -1) {
		return -1;
	}

	ret = *(int *)resp;

	return ret;
}

int cli_api_apf_get_enable (bool *enable)
{
	int ret;
	char resp[NL_MSG_MAX_RESPONSE_SIZE];

	ret = netlink_send_data(NL_APF_GET_ENABLE, NULL, resp);

	if (ret == -1) {
		return -1;
	}

	*enable = *(int *)resp;

	return ret;
}

int cli_api_apf_get_cap (struct cli_api_apf_cap *cap)
{
	int ret;
	char resp[NL_MSG_MAX_RESPONSE_SIZE];

	ret = netlink_send_data(NL_APF_GET_CAPABILITIES, NULL, resp);

	if (ret == -1) {
		return -1;
	}

	memcpy(cap, resp, sizeof(struct cli_api_apf_cap));

	return 0;
}

int cli_api_apf_set_filter (uint8_t *buf, int len)
{
	int ret;
	struct cli_api_apf_filter req_filter, resp_filter;
	char resp[NL_MSG_MAX_RESPONSE_SIZE];

	req_filter.len = len;
	req_filter.offset = 0;
	memcpy(req_filter.data, buf, len);

	ret = netlink_send_data(NL_APF_SET_PACKET_FILTER, (char *)&req_filter, resp);

	if (ret == -1) {
		return -1;
	}

	memcpy(&resp_filter, resp, sizeof(struct cli_api_apf_filter));

	return resp_filter.len;
}

int cli_api_apf_get_filter (uint8_t *buf, int offset, int len)
{
	int ret;
	struct cli_api_apf_filter req_filter, resp_filter;
	char resp[NL_MSG_MAX_RESPONSE_SIZE];

	req_filter.len = len;
	req_filter.offset = offset;

	ret = netlink_send_data(NL_APF_GET_PACKET_FILTER, (char *)&req_filter, resp);

	if (ret == -1) {
		return -1;
	}

	memcpy(&resp_filter, resp, sizeof(struct cli_api_apf_filter));

	if (resp_filter.len > 0)
		memcpy(buf, resp_filter.data, resp_filter.len); 

	return resp_filter.len;
}


