#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#define BUF_SIZE 1024

#define WAIT_QUEUE_WAIT 0
#define WAIT_QUEUE_KEY 1
#define WAIT_QUEUE_NEXT 2
#define WAIT_QUEUE_EXIT 3

/* left led group */
#define N_GPIO_OUTPUT_LEFT_1 17
#define N_GPIO_OUTPUT_LEFT_2 27
#define N_GPIO_OUTPUT_LEFT_3 22
#define N_GPIO_OUTPUT_LEFT_4 5

/* right led group */
#define N_GPIO_OUTPUT_RIGHT_1 6
#define N_GPIO_OUTPUT_RIGHT_2 26
#define N_GPIO_OUTPUT_RIGHT_3 23
#define N_GPIO_OUTPUT_RIGHT_4 24

#define N_GPIO_INPUT_1 16

#define ON 1
#define OFF 0

#define LEFT_LED_GROUP_ON() \
do { \
	gpio_set_value(N_GPIO_OUTPUT_LEFT_1, ON); \
	gpio_set_value(N_GPIO_OUTPUT_LEFT_2, ON); \
	gpio_set_value(N_GPIO_OUTPUT_LEFT_3, ON); \
	gpio_set_value(N_GPIO_OUTPUT_LEFT_4, ON); \
} while (0)

#define LEFT_LED_GROUP_OFF() \
do { \
	gpio_set_value(N_GPIO_OUTPUT_LEFT_1, OFF); \
	gpio_set_value(N_GPIO_OUTPUT_LEFT_2, OFF); \
	gpio_set_value(N_GPIO_OUTPUT_LEFT_3, OFF); \
	gpio_set_value(N_GPIO_OUTPUT_LEFT_4, OFF); \
} while (0)

#define RIGHT_LED_GROUP_ON() \
do { \
	gpio_set_value(N_GPIO_OUTPUT_RIGHT_1, ON); \
	gpio_set_value(N_GPIO_OUTPUT_RIGHT_2, ON); \
	gpio_set_value(N_GPIO_OUTPUT_RIGHT_3, ON); \
	gpio_set_value(N_GPIO_OUTPUT_RIGHT_4, ON); \
} while (0)

#define RIGHT_LED_GROUP_OFF() \
do { \
	gpio_set_value(N_GPIO_OUTPUT_RIGHT_1, OFF); \
	gpio_set_value(N_GPIO_OUTPUT_RIGHT_2, OFF); \
	gpio_set_value(N_GPIO_OUTPUT_RIGHT_3, OFF); \
	gpio_set_value(N_GPIO_OUTPUT_RIGHT_4, OFF); \
} while (0)

#define DEVICE_DRIVER "n_gpio_driver"
#define DEVICE_CLASS "n_gpio_class"

unsigned int gpio_outputs[] = {
	N_GPIO_OUTPUT_LEFT_1,
	N_GPIO_OUTPUT_LEFT_2,
	N_GPIO_OUTPUT_LEFT_3,
	N_GPIO_OUTPUT_LEFT_4,
	N_GPIO_OUTPUT_RIGHT_1,
	N_GPIO_OUTPUT_RIGHT_2,
	N_GPIO_OUTPUT_RIGHT_3,
	N_GPIO_OUTPUT_RIGHT_4,
};
unsigned int gpio_inputs[] = {
	N_GPIO_INPUT_1,
};
unsigned gpio_outputs_len = sizeof(gpio_outputs) / sizeof(int);
unsigned gpio_inputs_len = sizeof(gpio_inputs) / sizeof(int);

static char kernel_write_buffer[BUF_SIZE];
static dev_t dev;
static struct class *cls;
static struct cdev cdev;
static struct task_struct *wait_thread;
unsigned int wait_queue_flag;
unsigned int input_irq;

DECLARE_WAIT_QUEUE_HEAD(wait_queue);

int n_gpio_driver_open(struct inode *in, struct file *fp)
{
	pr_info("n_gpio_driver opened\n");
	return 0;
}

int n_gpio_driver_close(struct inode *in, struct file *fp)
{
	pr_info("n_gpio_driver closed\n");
	return 0;
}

ssize_t n_gpio_driver_read(struct file *fp, char __user *buf, size_t count, loff_t *offset)
{
	return 0;
}

ssize_t n_gpio_driver_write(struct file *fp, const char __user *buf, size_t count, loff_t *offset)
{
	if (copy_from_user(kernel_write_buffer, buf, count)) {
		pr_err("fail to copy buffer from user\n");
		do_exit(1);
	}

	switch (kernel_write_buffer[0]) {
		case 0:
			wait_queue_flag = WAIT_QUEUE_WAIT;
			pr_info("set wait queue flag wait\n");
			break;
		case 1:
			wait_queue_flag = WAIT_QUEUE_KEY;
			wake_up_interruptible(&wait_queue);
			pr_info("set wait queue flag key\n");
			break;
		case 2:
			wait_queue_flag = WAIT_QUEUE_NEXT;
			wake_up_interruptible(&wait_queue);
			pr_info("set wait queue flag next\n");
			break;
		default:
			wait_queue_flag = WAIT_QUEUE_EXIT;
			pr_info("invalid input");
	}

	return count;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = n_gpio_driver_open,
	.read = n_gpio_driver_read,
	.write = n_gpio_driver_write,
	.release = n_gpio_driver_close,
};

/*
** left LED group ON/OFF 
*/
int pattern_1(void)
{
	int led = gpio_get_value(N_GPIO_OUTPUT_LEFT_1);
	pr_info("LED pattern 1\n");

	while (1) {
		if (led == ON) {
			led = OFF;
			pr_info("left LED group off\n");
			LEFT_LED_GROUP_OFF();
		} else {
			led = ON;
			pr_info("left LED group on\n");
			LEFT_LED_GROUP_ON();
		}

		wait_queue_flag = WAIT_QUEUE_WAIT;
		wait_event_interruptible(wait_queue, wait_queue_flag != WAIT_QUEUE_WAIT);

		if (wait_queue_flag == WAIT_QUEUE_NEXT || wait_queue_flag == WAIT_QUEUE_EXIT) {
			LEFT_LED_GROUP_OFF();
			break;
		}
	}

	return wait_queue_flag;
}

/*
** right LED group on/off 
*/
int pattern_2(void)
{
	int led = gpio_get_value(N_GPIO_OUTPUT_RIGHT_1);
	pr_info("LED pattern 2\n");

	while (1) {
		if (led == ON) {
			led = OFF;
			pr_info("right LED group off\n");
			RIGHT_LED_GROUP_OFF();
		} else {
			led = ON;
			pr_info("right LED group on\n");
			RIGHT_LED_GROUP_ON();
		}

		wait_queue_flag = WAIT_QUEUE_WAIT;
		wait_event_interruptible(wait_queue, wait_queue_flag != WAIT_QUEUE_WAIT);

		if (wait_queue_flag == WAIT_QUEUE_NEXT || wait_queue_flag == WAIT_QUEUE_EXIT) {
			RIGHT_LED_GROUP_OFF();
			break;
		}
	}

	return wait_queue_flag;
}

/*
** left and right LED group switching
*/
int pattern_3(void)
{
	int left = gpio_get_value(N_GPIO_OUTPUT_LEFT_1);
	pr_info("LED pattern 3\n");

	while (1) {
		if (left == ON) {
			left = OFF;
			pr_info("left LED group off and right LED group on\n");
			LEFT_LED_GROUP_OFF();
			RIGHT_LED_GROUP_ON();
		} else {
			left = ON;
			pr_info("left LED group on and right LED group off\n");
			LEFT_LED_GROUP_ON();
			RIGHT_LED_GROUP_OFF();
		}

		wait_queue_flag = WAIT_QUEUE_WAIT;
		wait_event_interruptible(wait_queue, wait_queue_flag != WAIT_QUEUE_WAIT);

		if (wait_queue_flag == WAIT_QUEUE_NEXT || wait_queue_flag == WAIT_QUEUE_EXIT) {
			LEFT_LED_GROUP_OFF();
			RIGHT_LED_GROUP_OFF();
			break;
		}
	}

	return wait_queue_flag;
}

/*
** left and right LED group switching automatically
*/
int pattern_4(void)
{
	int left = gpio_get_value(N_GPIO_OUTPUT_LEFT_1);
	pr_info("LED pattern 4\n");

	wait_queue_flag = WAIT_QUEUE_WAIT;

	while (1) {
		if (left == ON) {
			left = OFF;
			pr_info("left LED group off and right LED group on\n");
			LEFT_LED_GROUP_OFF();
			RIGHT_LED_GROUP_ON();
		} else {
			left = ON;
			pr_info("left LED group on and right LED group off\n");
			LEFT_LED_GROUP_ON();
			RIGHT_LED_GROUP_OFF();
		}

		usleep_range(500000, 500001); // 0.5s

		if (wait_queue_flag == WAIT_QUEUE_NEXT || wait_queue_flag == WAIT_QUEUE_EXIT) {
			LEFT_LED_GROUP_OFF();
			RIGHT_LED_GROUP_OFF();
			break;
		}
	}

	return wait_queue_flag;
}

/*
** input optionally
*/
int pattern_5(void)
{
	int input;

	pr_info("LED pattern 5\n");
	wait_queue_flag = WAIT_QUEUE_WAIT;

	while (1) {
		input = gpio_get_value(N_GPIO_INPUT_1);
		pr_info("input: %d\n", input);

		if (input == ON) {
			pr_info("left LED group on and right LED group off\n");
			LEFT_LED_GROUP_ON();
			RIGHT_LED_GROUP_OFF();
		} else {
			pr_info("left LED group off and right LED group on\n");
			LEFT_LED_GROUP_OFF();
			RIGHT_LED_GROUP_ON();
		}

		usleep_range(1000, 1001);

		if (wait_queue_flag == WAIT_QUEUE_NEXT || wait_queue_flag == WAIT_QUEUE_EXIT) {
			LEFT_LED_GROUP_OFF();
			RIGHT_LED_GROUP_OFF();
			break;
		}
	}

	return wait_queue_flag;
}

/*
** input optionally (triggered when low)
*/
int pattern_6(void)
{
	int old_input, new_input, left;

	pr_info("LED pattern 6\n");
	wait_queue_flag = WAIT_QUEUE_WAIT;

	gpio_set_debounce(N_GPIO_INPUT_1, 1000);

	while (1) {
		do {
			old_input = gpio_get_value(N_GPIO_INPUT_1);
			usleep_range(1000, 1001);
			new_input = gpio_get_value(N_GPIO_INPUT_1);
		} while (old_input == new_input);

		if (new_input == ON) continue;

		if (left == ON) {
			pr_info("left LED group off and right LED group on\n");
			left = OFF;
			LEFT_LED_GROUP_OFF();
			RIGHT_LED_GROUP_ON();
		} else {
			pr_info("left LED group on and right LED group off\n");
			left = ON;
			LEFT_LED_GROUP_ON();
			RIGHT_LED_GROUP_OFF();
		}

		if (wait_queue_flag == WAIT_QUEUE_NEXT || wait_queue_flag == WAIT_QUEUE_EXIT) {
			LEFT_LED_GROUP_OFF();
			RIGHT_LED_GROUP_OFF();
			break;
		}
	}

	return wait_queue_flag;
}

int wait_thread_fn(void *data)
{
	pr_info("wait thread created\n");
	while (1) {
		if (pattern_1() == WAIT_QUEUE_EXIT) break;
		if (pattern_2() == WAIT_QUEUE_EXIT) break;
		if (pattern_3() == WAIT_QUEUE_EXIT) break;
		if (pattern_4() == WAIT_QUEUE_EXIT) break;
		if (pattern_5() == WAIT_QUEUE_EXIT) break;
		if (pattern_6() == WAIT_QUEUE_EXIT) break;
	}
	pr_info("wait thread destroyed\n");

	LEFT_LED_GROUP_OFF();
	RIGHT_LED_GROUP_OFF();

	do_exit(0);
	return 0;
}

/*
** direction 0: output
** direction 1: input
*/
int register_gpio(unsigned gpio, unsigned direction)
{
	char name[60];
	snprintf(name, sizeof(name), "n-gpio-%d", gpio);

	if (gpio_request(gpio, name)) {
		pr_err("fail to use GPIO %d pin\n", gpio);
		return -1;
	}

	if (direction == 0) {
		if (gpio_direction_output(gpio, 0)) {
			pr_err("fail to change GPIO %d pin direction to out\n", gpio);
			return -1;
		}
	}
	else if (direction == 1) {
		if (gpio_direction_input(gpio)) {
			pr_err("fail to change GPIO %d pin direction to in\n", gpio);
			return -1;
		}
	}

	return 0;
}

static int __init n_gpio_module_init(void)
{
	int i;

	pr_info("n_gpio_module init\n");

	if (alloc_chrdev_region(&dev, 0, 1, DEVICE_DRIVER) < 0) {
		pr_err("fail to allocate char device region\n");
		goto region_error;
	}

	pr_info("device number: %d.%d\n", MAJOR(dev), MINOR(dev));

	if ((cls = class_create(THIS_MODULE, DEVICE_CLASS)) == NULL) {
		pr_err("fail to create device class\n");
		goto class_error;
	}

	if (device_create(cls, NULL, dev, NULL, DEVICE_DRIVER) == NULL) {
		pr_err("fail to create device file\n");
		goto device_error;
	}

	cdev_init(&cdev, &fops);

	if (cdev_add(&cdev, dev, 1) < 0) {
		pr_err("fail to add char device to kernel\n");
		goto device_error;
	}

	pr_info("gpio_outputs_len: %d\n", gpio_outputs_len);
	for (i = 0; i < gpio_outputs_len; i++)
		register_gpio(gpio_outputs[i], 0);

	pr_info("gpio_inputs_len: %d\n", gpio_inputs_len);
	for (i = 0; i < gpio_inputs_len; i++)
		register_gpio(gpio_inputs[i], 1);

	wait_thread = kthread_create(wait_thread_fn, NULL, "wait_thread");
	if (wait_thread) {
		pr_info("wait thread created\n");
		wake_up_process(wait_thread);
	} else {
		pr_err("fail to create wait thread\n");
		goto gpio_error;
	}

	return 0;

gpio_error:
	for (i = 0; i < gpio_outputs_len; i++)
		gpio_free(gpio_outputs[i]);
	for (i = 0; i < gpio_inputs_len; i++)
		gpio_free(gpio_inputs[i]);
device_error:
	device_destroy(cls, dev);
class_error:
	class_destroy(cls);
	unregister_chrdev_region(dev, 1);
region_error:
	return -1;
}

static void __exit n_gpio_module_exit(void)
{
	int i = 0;

	wait_queue_flag = WAIT_QUEUE_EXIT;
	wake_up_interruptible(&wait_queue);

	gpio_set_value(N_GPIO_INPUT_1, 0);

	for (i = 0; i < gpio_outputs_len; i++)
		gpio_free(gpio_outputs[i]);
	for (i = 0; i < gpio_inputs_len; i++)
		gpio_free(gpio_inputs[i]);

	cdev_del(&cdev);
	device_destroy(cls, dev);
	class_destroy(cls);
	unregister_chrdev_region(dev, 1);

	pr_info("n_gpio_module exit\n");
}

module_init(n_gpio_module_init);
module_exit(n_gpio_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("nk <seven3126@gmail.com>");
MODULE_DESCRIPTION("GPIO LED control");
MODULE_VERSION("1.0.0");