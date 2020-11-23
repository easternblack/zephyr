#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <usb_descriptor.h>
#include <sys/byteorder.h>
#include <usb/class/usb_eb.h>

#define LOG_LEVEL CONFIG_USB_DEVICE_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_easternblack);

#define EASTERNBLACK_OUT_EP_ADDR		0x01
#define EASTERNBLACK_IN_EP_ADDR		0x81

#define EASTERNBLACK_OUT_EP_IDX		0
#define EASTERNBLACKB_IN_EP_IDX		1

#define READ_PACKET_MAX_SIZE	CONFIG_EB_BULK_EP_MPS
#define WRITE_PACKET_MAX_SIZE	4200


struct usb_easternblack_config {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
} __packed;

struct usb_easternblack_common easternblack_dev;

USBD_CLASS_DESCR_DEFINE(primary, 0) struct usb_easternblack_config easternblack_cfg = {
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = 0,
		.bInterfaceSubClass = EASTERNBLACK_SUBCLASS,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},

	.if0_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = EASTERNBLACK_OUT_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(CONFIG_EB_BULK_EP_MPS),
		.bInterval = 0x00,
	},

	.if0_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = EASTERNBLACK_IN_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(CONFIG_EB_BULK_EP_MPS),
		.bInterval = 0x00,
	},
};

uint8_t read_buf[READ_PACKET_MAX_SIZE] = {0, };
uint8_t write_buf[WRITE_PACKET_MAX_SIZE] = {0, };
static void easternblack_in_cb(uint8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	uint32_t write_buf_len = 0;
	uint32_t chunk;
	int ret;

	if (ep_status != USB_DC_EP_SETUP && ep_status != USB_DC_EP_DATA_IN) {
		return;
	}

	switch (ep_status) {
	case USB_DC_EP_SETUP:
		/* Zero length packet */
		usb_write(ep, NULL, 0, NULL);
		break;
	case USB_DC_EP_DATA_IN:
		if (easternblack_dev.data_buf_residue > 0) {
			goto write_buf_flash;
		} else {
			ret = easternblack_dev.in_callback(write_buf, &write_buf_len);
			if (!ret) {
				easternblack_dev.data_buf = write_buf;
				easternblack_dev.data_buf_len = easternblack_dev.data_buf_residue
						     = write_buf_len;
				goto write_buf_flash;
			} else {
				usb_write(ep, NULL, 0, NULL);
				break;
			}
		}
		break;
	default:
		break;
	}

	return;

write_buf_flash:
	usb_write(ep, easternblack_dev.data_buf, easternblack_dev.data_buf_residue, &chunk);
	easternblack_dev.data_buf += chunk;
	easternblack_dev.data_buf_residue -= chunk;
}

static void easternblack_out_cb(uint8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	int ret;
	uint32_t write_buf_len = 0, read_buf_len;

	if (ep_status != USB_DC_EP_DATA_OUT) {
		return;
	}

	memset(read_buf, 0, sizeof(read_buf));
	memset(write_buf, 0, sizeof(write_buf));

	usb_read(ep, NULL, 0, &read_buf_len);
	usb_read(ep, read_buf, read_buf_len, NULL);

	ret = easternblack_dev.out_callback(read_buf, &read_buf_len,
					write_buf, &write_buf_len);
	if (ret) {
		return;
	}

	if (write_buf_len > 0) {
		easternblack_dev.data_buf = write_buf;
		easternblack_dev.data_buf_len = easternblack_dev.data_buf_residue
				     = write_buf_len;
	} else {
		easternblack_dev.data_buf_len = easternblack_dev.data_buf_residue = 0;
	}

	easternblack_in_cb(EASTERNBLACK_IN_EP_ADDR, USB_DC_EP_SETUP);
}

static struct usb_ep_cfg_data ep_cfg[] = {
	{
		.ep_cb = easternblack_out_cb,
		.ep_addr = EASTERNBLACK_OUT_EP_ADDR,
	},
	{
		.ep_cb = easternblack_in_cb,
		.ep_addr = EASTERNBLACK_IN_EP_ADDR,
	},
};

static void easternblack_status_cb(struct usb_cfg_data *cfg,
			       enum usb_dc_status_code status,
			       const uint8_t *param)
{
	ARG_UNUSED(cfg);
	ARG_UNUSED(status);
	ARG_UNUSED(param);
}

static int easternblack_vendor_handler(struct usb_setup_packet *setup,
				   int32_t *len, uint8_t **data)
{
	LOG_DBG("Class request: bRequest 0x%x bmRequestType 0x%x len %d",
		setup->bRequest, setup->bmRequestType, *len);

	if (REQTYPE_GET_RECIP(setup->bmRequestType) != REQTYPE_RECIP_DEVICE) {
		return -ENOTSUP;
	}

	if (REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_DEVICE &&
	    setup->bRequest == 0x5b) {
		LOG_DBG("Host-to-Device, data %p", *data);
		return 0;
	}

	if ((REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_HOST) &&
	    (setup->bRequest == 0x5c)) {
		LOG_DBG("Device-to-Host, wLength %d, data %p",
			setup->wLength, *data);
		return 0;
	}

	return -ENOTSUP;
}

static void easternblack_interface_config(struct usb_desc_header *head,
				      uint8_t bInterfaceNumber)
{
	ARG_UNUSED(head);

	easternblack_cfg.if0.bInterfaceNumber = bInterfaceNumber;
}

USBD_CFG_DATA_DEFINE(primary, easternblack) struct usb_cfg_data easternblack_config = {
	.usb_device_description = NULL,
	.interface_config = easternblack_interface_config,
	.interface_descriptor = &easternblack_cfg.if0,
	.cb_usb_status = easternblack_status_cb,
	.interface = {
		.class_handler = NULL,
		.custom_handler = NULL,
		.vendor_handler = easternblack_vendor_handler,
	},
	.num_endpoints = ARRAY_SIZE(ep_cfg),
	.endpoint = ep_cfg,
};
