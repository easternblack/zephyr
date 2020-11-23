#include <zephyr.h>
#include <logging/log.h>
#include <usb/usb_device.h>
#include <usb/class/usb_eb.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(usb_in_callback);

#define STACK_SIZE 256
static K_THREAD_STACK_DEFINE(test_thread_stack, STACK_SIZE);
static struct k_thread test_thread_data;
static K_THREAD_STACK_DEFINE(test_thread_stack_2, STACK_SIZE);
static struct k_thread test_thread_data_2;
static K_THREAD_STACK_DEFINE(test_thread_stack_3, STACK_SIZE);
static struct k_thread test_thread_data_3;

/* For the definition of out_callback */
extern struct usb_easternblack_common easternblack_dev;
int status;
int count;

int usb_in_callback(uint8_t *w_buf, uint32_t *w_buf_len)
{
	if (status == 1) {
		for(int i=0; i<100; i++) {
			w_buf[i] = i;
		}
		*w_buf_len = 100;
		status = 0;
	} else {
		return -1;
	}

	return 0;
}

int usb_out_callback(uint8_t *r_buf, uint32_t *r_buf_len,
			uint8_t *w_buf, uint32_t *w_buf_len)
{
	if ((r_buf[0] == 'a') && (r_buf[1] == 'b') && (r_buf[2] == 'c')) {
		status = 1;
		printk("%d ", count++);
	}

	return 0;
}

void test_thread()
{
	int do_something_in_thread = 0;

	while (1) {
		do_something_in_thread++;
		do_something_in_thread--;
		k_msleep(1);
	}
}

void test_thread_2()
{
	while (1) {
		k_msleep(1);
	}
}

void test_thread_3()
{
	while (1) {
		k_msleep(1);
	}
}

int main(void)
{
	int ret;
	int do_something = 0;

	count = 0;

	easternblack_dev.in_callback = usb_in_callback;
	easternblack_dev.out_callback = usb_out_callback;

	ret = usb_enable(NULL);
	if (ret) {
		LOG_ERR("Failed to enable USB");
		return -1;
	}

	/* Following threads are test for if k_sleep run in thread affect on USB */

/*	k_thread_create(&test_thread_data, test_thread_stack, STACK_SIZE,
                (k_thread_entry_t)test_thread, NULL, NULL, NULL,
                0, 0, K_MSEC(1));*/
	
/*	k_thread_create(&test_thread_data_2, test_thread_stack_2, STACK_SIZE,
                (k_thread_entry_t)test_thread_2, NULL, NULL, NULL,
                0, 0, K_MSEC(1));*/


/*	k_thread_create(&test_thread_data_3, test_thread_stack_3, STACK_SIZE,
                (k_thread_entry_t)test_thread_3, NULL, NULL, NULL,
                0, 0, K_MSEC(1));*/

	while(1) {
		k_yield();
		do_something++;
		do_something++;
		do_something--;
		do_something--;
	}

	return 0;
}
