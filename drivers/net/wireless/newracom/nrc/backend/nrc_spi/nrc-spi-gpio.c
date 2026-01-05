/*
 * Copyright (c) 2016-2024 Newracom, Inc.
 *
 * NRC SPI GPIO Helper Functions Implementation
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

/* Linux kernel headers */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
#include <linux/gpio/consumer.h>
#include <linux/device.h>
#endif

/* Common directory headers */
#include "nrc-debug-common.h"

/* Local headers */
#include "nrc-spi-gpio.h"

/* GPIO descriptor storage for kernel 6.6+ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
#define MAX_GPIO_DESCRIPTORS 20
static struct {
	unsigned gpio_num;
	struct gpio_desc *desc;
	bool in_use;
} gpio_desc_table[MAX_GPIO_DESCRIPTORS];

static struct gpio_desc *find_gpio_desc(unsigned gpio)
{
	int i;
	for (i = 0; i < MAX_GPIO_DESCRIPTORS; i++) {
		if (gpio_desc_table[i].in_use &&
		    gpio_desc_table[i].gpio_num == gpio) {
			return gpio_desc_table[i].desc;
		}
	}
	return NULL;
}

static int store_gpio_desc(unsigned gpio, struct gpio_desc *desc)
{
	int i;
	for (i = 0; i < MAX_GPIO_DESCRIPTORS; i++) {
		if (!gpio_desc_table[i].in_use) {
			gpio_desc_table[i].gpio_num = gpio;
			gpio_desc_table[i].desc = desc;
			gpio_desc_table[i].in_use = true;
			return 0;
		}
	}
	return -ENOMEM;
}

static void remove_gpio_desc(unsigned gpio)
{
	int i;
	for (i = 0; i < MAX_GPIO_DESCRIPTORS; i++) {
		if (gpio_desc_table[i].in_use &&
		    gpio_desc_table[i].gpio_num == gpio) {
			gpio_desc_table[i].in_use = false;
			gpio_desc_table[i].desc = NULL;
			gpio_desc_table[i].gpio_num = 0;
			break;
		}
	}
}
#endif

/**
 * nrc_gpio_request - Request a GPIO pin with kernel version compatibility
 * @gpio: GPIO number (for legacy API)
 * @label: Label for the GPIO request
 *
 * Kernel 6.6-6.11: Uses gpio_request with gpio_to_desc conversion
 * Kernel 6.12+: Uses gpio_request_one for proper allocation
 * Older kernels: Uses legacy gpio_request
 *
 * Returns: GPIO descriptor pointer for 6.6-6.11, NULL for success on other kernels, negative error code on failure
 */
struct gpio_desc *nrc_gpio_request(unsigned gpio, const char *label)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	/* Kernel 6.6+: Use descriptor API with gpio_to_desc() */
	struct gpio_desc *desc;
	int ret;

	/* First try to request using legacy GPIO API, then convert to descriptor */
	ret = gpio_request(gpio, label);
	if (ret < 0) {
		ERR_HIF("GPIO: Failed to request GPIO %d with label '%s': %d",
			gpio, label, ret);
		return ERR_PTR(ret);
	}

	/* Get GPIO descriptor using gpio number */
	desc = gpio_to_desc(gpio);
	if (!desc) {
		ERR_HIF("GPIO: Invalid GPIO %d", gpio);
		gpio_free(gpio);
		return ERR_PTR(-EINVAL);
	}

	/* Store the descriptor for later use */
	ret = store_gpio_desc(gpio, desc);
	if (ret < 0) {
		ERR_HIF("GPIO: Failed to store descriptor for GPIO %d", gpio);
		gpio_free(gpio);
		return ERR_PTR(ret);
	}

	DBG_HIF("GPIO: Successfully requested GPIO %d with label '%s'", gpio,
		label);

	return desc;
#else
	/* Older kernels: Use legacy GPIO API */
	int ret = gpio_request(gpio, label);
	if (ret < 0) {
		ERR_HIF("GPIO: Failed to request GPIO %d with label '%s': %d",
			gpio, label, ret);
		return ERR_PTR(ret);
	}
	DBG_HIF("GPIO: Successfully requested GPIO %d with label '%s' (legacy API)",
		gpio, label);
	return NULL; /* Success with legacy API */
#endif
}

/**
 * nrc_gpio_free - Free a GPIO pin with kernel version compatibility
 * @gpio: GPIO number
 */
void nrc_gpio_free(unsigned gpio)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	/* Kernel 6.6+: Clean up descriptor table */
	struct gpio_desc *desc = find_gpio_desc(gpio);
	if (desc && !IS_ERR(desc)) {
		/* Remove from our descriptor table first */
		remove_gpio_desc(gpio);
	}
#endif
	/* All kernels: Use legacy gpio_free */
	gpio_free(gpio);
	DBG_HIF("GPIO: Successfully freed GPIO %d", gpio);
}

/**
 * nrc_gpio_direction_output - Set GPIO as output with kernel version compatibility
 * @gpio: GPIO number
 * @value: Initial output value
 *
 * Returns: 0 on success, negative error code on failure
 */
int nrc_gpio_direction_output(unsigned gpio, int value)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	/* Kernel 6.6+: Use descriptor API */
	struct gpio_desc *desc = find_gpio_desc(gpio);
	if (!desc) {
		ERR_HIF("GPIO: GPIO %d descriptor not found", gpio);
		return -EINVAL;
	}
	return gpiod_direction_output(desc, value);
#else
	/* Kernel 6.12+ or older: Use legacy API */
	return gpio_direction_output(gpio, value);
#endif
}

/**
 * nrc_gpio_set_value - Set GPIO output value with kernel version compatibility
 * @gpio: GPIO number
 * @value: Output value (0 or 1)
 */
void nrc_gpio_set_value(unsigned gpio, int value)
{
	DBG_HIF("Set GPIO %d to %s", gpio, value ? "HIGH" : "LOW");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	/* Kernel 6.6+: Use descriptor API */
	struct gpio_desc *desc = find_gpio_desc(gpio);
	if (!desc) {
		ERR_HIF("GPIO: GPIO %d descriptor not found", gpio);
		return;
	}
	gpiod_set_value(desc, value);
#else
	/* Kernel 6.12+ or older: Use legacy API */
	gpio_set_value(gpio, value);
#endif
}

/**
 * nrc_gpio_direction_input - Set GPIO as input with kernel version compatibility
 * @gpio: GPIO number
 *
 * Returns: 0 on success, negative error code on failure
 */
int nrc_gpio_direction_input(unsigned gpio)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	/* Kernel 6.6+: Use descriptor API */
	struct gpio_desc *desc = find_gpio_desc(gpio);
	if (!desc) {
		ERR_HIF("GPIO: GPIO %d descriptor not found", gpio);
		return -EINVAL;
	}
	return gpiod_direction_input(desc);
#else
	/* Kernel 6.12+ or older: Use legacy API */
	return gpio_direction_input(gpio);
#endif
}
