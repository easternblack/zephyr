#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_EB_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_EB_H_

/** Communications Class Subclass Codes */
#define EASTERNBLACK_SUBCLASS		0x02

struct usb_easternblack_common {
	uint8_t *data_buf;
	uint32_t data_buf_len;
	int32_t data_buf_residue;
	int (*in_callback)(uint8_t *write_buf, uint32_t *write_len);
	int (*out_callback)(uint8_t *read_buf, uint32_t *read_len,
			    uint8_t *write_buf, uint32_t *write_len);
};

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_EB_H_ */
