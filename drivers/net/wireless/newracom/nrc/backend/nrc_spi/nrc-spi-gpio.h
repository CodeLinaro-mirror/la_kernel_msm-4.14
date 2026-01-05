/*
 * Copyright (c) 2016-2024 Newracom, Inc.
 *
 * NRC SPI GPIO Helper Functions Header
 * Kernel version compatibility for GPIO operations
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _NRC_SPI_GPIO_H_
#define _NRC_SPI_GPIO_H_

#include <linux/gpio/consumer.h>

/**
 * nrc_gpio_request - Request a GPIO pin
 * @gpio: GPIO number (for legacy API)
 * @label: Label for the GPIO request
 *
 * Returns: GPIO descriptor pointer for 6.6+, 0 for success on older kernels, negative error code on failure
 */
struct gpio_desc *nrc_gpio_request(unsigned gpio, const char *label);

/**
 * nrc_gpio_free - Free a GPIO pin
 * @gpio: GPIO number
 */
void nrc_gpio_free(unsigned gpio);

/**
 * nrc_gpio_direction_output - Set GPIO as output
 * @gpio: GPIO number
 * @value: Initial output value
 *
 * Returns: 0 on success, negative error code on failure
 */
int nrc_gpio_direction_output(unsigned gpio, int value);

/**
 * nrc_gpio_set_value - Set GPIO output value
 * @gpio: GPIO number
 * @value: Output value (0 or 1)
 */
void nrc_gpio_set_value(unsigned gpio, int value);

/**
 * nrc_gpio_direction_input - Set GPIO as input
 * @gpio: GPIO number
 *
 * Returns: 0 on success, negative error code on failure
 */
int nrc_gpio_direction_input(unsigned gpio);

#endif /* _NRC_SPI_GPIO_H_ */