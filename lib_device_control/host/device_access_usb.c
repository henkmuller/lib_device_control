// Copyright (c) 2016, XMOS Ltd, All rights reserved
#if USE_USB

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#include "usb.h"
#else
#include <unistd.h>
#include "libusb.h"
#endif
#include "control_host.h"
#include "control_host_support.h"
#include "util.h"

//#define DBG(x) x
#define DBG(x)

static unsigned num_commands = 0;

#ifdef _WIN32
static usb_dev_handle *devh = NULL;
#else
static libusb_device_handle *devh = NULL;
#endif

static const int sync_timeout_ms = 100;

control_ret_t control_query_version(control_version_t *version)
{
  uint16_t windex, wvalue, wlength;
  uint8_t request_data[64];

  control_usb_fill_header(&windex, &wvalue, &wlength,
    CONTROL_SPECIAL_RESID, CONTROL_GET_VERSION, sizeof(control_version_t));

  DBG(printf("%u: send version command: 0x%04x 0x%04x 0x%04x\n",
    num_commands, windex, wvalue, wlength));

#ifdef _WIN32
  int ret = usb_control_msg(devh,
    USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
    0, wvalue, windex, (char*)request_data, wlength, sync_timeout_ms);
#else
  int ret = libusb_control_transfer(devh,
    LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
    0, wvalue, windex, request_data, wlength, sync_timeout_ms);
#endif

  num_commands++;

  if (ret != sizeof(control_version_t)) {
    printf("libusb_control_transfer returned %d\n", ret);
    return CONTROL_ERROR;
  }

  memcpy(version, request_data, sizeof(control_version_t));
  DBG(printf("version returned: 0x%X\n", *version));

  return CONTROL_SUCCESS;
}

control_ret_t
control_write_command(control_resid_t resid, control_cmd_t cmd,
                      const uint8_t payload[], size_t payload_len)
{
  uint16_t windex, wvalue, wlength;

  control_usb_fill_header(&windex, &wvalue, &wlength,
    resid, CONTROL_CMD_SET_WRITE(cmd), payload_len);

  DBG(printf("%u: send write command: 0x%04x 0x%04x 0x%04x ",
    num_commands, windex, wvalue, wlength));
  DBG(print_bytes(payload, payload_len));

#ifdef _WIN32
  int ret = usb_control_msg(devh,
    USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
    0, wvalue, windex, (char*)payload, wlength, sync_timeout_ms);
#else
  int ret = libusb_control_transfer(devh,
    LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
    0, wvalue, windex, (unsigned char*)payload, wlength, sync_timeout_ms);
#endif

  num_commands++;

  if (ret != (int)payload_len) {
    printf("libusb_control_transfer returned %d\n", ret);
    return CONTROL_ERROR;
  }

  return CONTROL_SUCCESS;
}

control_ret_t
control_read_command(control_resid_t resid, control_cmd_t cmd,
                     uint8_t payload[], size_t payload_len)
{
  uint16_t windex, wvalue, wlength;

  control_usb_fill_header(&windex, &wvalue, &wlength,
    resid, CONTROL_CMD_SET_READ(cmd), payload_len);

  DBG(printf("%u: send read command: 0x%04x 0x%04x 0x%04x\n",
    num_commands, windex, wvalue, wlength));

#ifdef _WIN32
  int ret = usb_control_msg(devh,
    USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
    0, wvalue, windex, (char*)payload, wlength, sync_timeout_ms);
#else
  int ret = libusb_control_transfer(devh,
    LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
    0, wvalue, windex, payload, wlength, sync_timeout_ms);
#endif

  num_commands++;

  if (ret != (int)payload_len) {
    printf("libusb_control_transfer returned %d\n", ret);
    return CONTROL_ERROR;
  }

  DBG(printf("read data returned: "));
  DBG(print_bytes(payload, payload_len));

  return CONTROL_SUCCESS;
}

#ifdef _WIN32

static void find_xmos_device(int vendor_id, int product_id)
{
  for (struct usb_bus *bus = usb_get_busses(); bus && !devh; bus = bus->next) {
    for (struct usb_device *dev = bus->devices; dev; dev = dev->next) {
      if ((dev->descriptor.idVendor == vendor_id) &&
              (dev->descriptor.idProduct == product_id)) {
        devh = usb_open(dev);
        if (!devh) {
          fprintf(stderr, "failed to open device\n");
          exit(1);
        }
        break;
      }
    }
  }

  if (!devh) {
    fprintf(stderr, "could not find device\n");
    exit(1);
  }
}

control_ret_t control_init_usb(int vendor_id, int product_id, int interface)
{
  usb_init();
  usb_find_busses(); /* find all busses */
  usb_find_devices(); /* find all connected devices */

  find_xmos_device(vendor_id, product_id);

  int r = usb_set_configuration(devh, 1);
  if (r < 0) {
    fprintf(stderr, "Error setting config 1\n");
    usb_close(devh);
    return CONTROL_ERROR;
  }

  r = usb_claim_interface(devh, interface);
  if (r < 0) {
    fprintf(stderr, "Error claiming interface %d %d\n", 0, r);
    return CONTROL_ERROR;
  }

  return CONTROL_SUCCESS;
}

control_ret_t control_cleanup_usb(void)
{
  usb_release_interface(devh, 0);
  usb_close(devh);
  return CONTROL_SUCCESS;
}

#else

control_ret_t control_init_usb(int vendor_id, int product_id, int interface)
{
  int ret = libusb_init(NULL);
  if (ret < 0) {
    fprintf(stderr, "failed to initialise libusb\n");
    return CONTROL_ERROR;
  }

  libusb_device **devs = NULL;
  libusb_get_device_list(NULL, &devs);

  libusb_device *dev = NULL;
  for (int i = 0; devs[i] != NULL; i++) {
    struct libusb_device_descriptor desc;
    libusb_get_device_descriptor(devs[i], &desc);
    if (desc.idVendor == vendor_id && desc.idProduct == product_id) {
      dev = devs[i];
      break;
    }
  }

  if (dev == NULL) {
    fprintf(stderr, "could not find device\n");
    return CONTROL_ERROR;
  }

  if (libusb_open(dev, &devh) < 0) {
    fprintf(stderr, "failed to open device\n");
    return CONTROL_ERROR;
  }

  libusb_free_device_list(devs, 1);

  return CONTROL_SUCCESS;
}

control_ret_t control_cleanup_usb(void)
{
  libusb_close(devh);
  libusb_exit(NULL);

  return CONTROL_SUCCESS;
}

#endif // _WIN32

#endif // USE_USB