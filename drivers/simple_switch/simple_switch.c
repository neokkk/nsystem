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

/* left led group */
#define GPIO_OUTPUT_LEFT_1 17
#define GPIO_OUTPUT_LEFT_2 27
#define GPIO_OUTPUT_LEFT_3 22
#define GPIO_OUTPUT_LEFT_4 5

/* right led group */
#define GPIO_OUTPUT_RIGHT_1 6
#define GPIO_OUTPUT_RIGHT_2 26
#define GPIO_OUTPUT_RIGHT_3 23
#define GPIO_OUTPUT_RIGHT_4 24

#define GPIO_INPUT_1 16

#define BUF_SIZE 1024

#define WAIT_QUEUE_WAIT 0
#define WAIT_QUEUE_KEY 1
#define WAIT_QUEUE_NEXT 2
#define WAIT_QUEUE_EXIT 3

#define DEVICE_DRIVER "simple_switch_driver"
#define DEVICE_CLASS "simple_switch_class"

static unsigned int gpio_outputs[] = {
	GPIO_OUTPUT_LEFT_1,
	GPIO_OUTPUT_LEFT_2,
	GPIO_OUTPUT_LEFT_3,
	GPIO_OUTPUT_LEFT_4,
	GPIO_OUTPUT_RIGHT_1,
	GPIO_OUTPUT_RIGHT_2,
	GPIO_OUTPUT_RIGHT_3,
	GPIO_OUTPUT_RIGHT_4,
};
static unsigned int gpio_inputs[] = {
	GPIO_INPUT_1,
};

static unsigned gpio_outputs_len = sizeof(gpio_outputs) / sizeof(int);
static unsigned gpio_inputs_len = sizeof(gpio_inputs) / sizeof(int);

static char kernel_write_buffer[BUF_SIZE];
static dev_t dev;
static struct class *cls;
static struct cdev cdev;
static struct task_struct *wait_thread;
static unsigned int wait_queue_flag = WAIT_QUEUE_WAIT;
static unsigned long pre_jiffies;
static int cur_led = 0x00;

static struct state_machine {
	int next_state;
	int action;
};

static enum input {
	OFF,
	ON,
	TIMEOUT,
	INPUT_SIZE,
};

static enum state {
	ST_INIT,
	ST_LIT,
	ST_SHORT,
	ST_LONG,
	ST_SHORTSHORT,
	ST_SHORTLONG,
	ST_LONGSHORT,
	ST_LONGLONG,
	STATE_SIZE,
};

static enum action {
	AT_NOP,
	AT_INIT,
	AT_LED_ON_STRAIGHT,
	AT_LED_ON_LEFT_4,
	AT_LED_ON_REVERSE,
	AT_LED_ON_LEFT_6,
	AT_LED_ON_ALL,
	AT_LED_OFF_ALL,
	ACTION_SIZE,
};

typedef void (*action_t)(void);

/**
 * 상태 변경의 기준이 되는 입력 값은
 * OFF, ON, TIMEOUT이다.
*/
struct state_machine transitions[STATE_SIZE][INPUT_SIZE] = {
	{ // INIT
		{ ST_INIT, AT_NOP },
		{ ST_LIT, AT_INIT },
		{ ST_INIT, AT_NOP },
	},
	{ // LIT
		{ ST_SHORT, AT_INIT },
		{ ST_LIT, AT_NOP },
		{ ST_LONG, AT_INIT },
	},
	{ // SHORT
		{ ST_SHORT, AT_NOP }, // OFF가 되었다고 바로 상태를 전환하는 것이 아니라, 추가 상태(SHORT-) 판단을 위해 대기한다.
		{ ST_SHORTSHORT, AT_INIT },
		{ ST_INIT, AT_LED_ON_STRAIGHT },
	},
	{ // LONG
		{ ST_LONG, AT_NOP },
		{ ST_LONGSHORT, AT_INIT },
		{ ST_INIT, AT_LED_ON_LEFT_4 },
	},
	{ // SHORTSHORT
		{ ST_INIT, AT_LED_ON_REVERSE },
		{ ST_SHORTSHORT, AT_NOP },
		{ ST_SHORTLONG, AT_NOP },
	},
	{ // SHORTLONG
		{ ST_INIT, AT_LED_ON_LEFT_6 },
		{ ST_SHORTLONG, AT_NOP },
		{ ST_SHORTLONG, AT_NOP },
	},
	{ // LONGSHORT
		{ ST_INIT, AT_LED_ON_ALL },
		{ ST_LONGSHORT, AT_NOP },
		{ ST_LONGLONG, AT_NOP },
	},
	{ // LONGLONG
		{ ST_INIT, AT_LED_OFF_ALL },
		{ ST_LONGLONG, AT_NOP },
		{ ST_LONGLONG, AT_NOP },
	},
};

DECLARE_WAIT_QUEUE_HEAD(wait_queue);
DECLARE_WAIT_QUEUE_HEAD(input_wait_queue);

int simple_switch_driver_open(struct inode *in, struct file *fp)
{
	pr_info("simple_switch_driver opened\n");
	return 0;
}

int simple_switch_driver_close(struct inode *in, struct file *fp)
{
	pr_info("simple_switch_driver closed\n");
	return 0;
}

ssize_t simple_switch_driver_read(struct file *fp, char __user *buf, size_t count, loff_t *offset)
{
	return 0;
}

ssize_t simple_switch_driver_write(struct file *fp, const char __user *buf, size_t count, loff_t *offset)
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
	.open = simple_switch_driver_open,
	.release = simple_switch_driver_close,
	.read = simple_switch_driver_read,
	.write = simple_switch_driver_write,
};

int is_short_push(unsigned long start, unsigned int ms)
{
	return jiffies_to_msecs(jiffies - start) < ms;
}

void set_bit_led(unsigned char led)
{
	int i;

	for (i = 0; i < gpio_outputs_len; i++) {
		gpio_set_value(gpio_outputs[i], (led >> i) & 1);
	}
}

void init_led(void)
{
	set_bit_led(0x00);
}

void act_nop(void) {
}

void act_init(void)
{
	pre_jiffies = jiffies;
}

void act_short(void)
{
	cur_led = (cur_led << 1) + 1;
	set_bit_led(cur_led);
	pr_info("<short>\n");
}

void act_long(void)
{
	cur_led = 0x0F;
	set_bit_led(cur_led);
	pr_info("<long>\n");
}

void act_shortshort(void)
{
	cur_led = (cur_led >> 1);
	set_bit_led(cur_led);
	pr_info("<short><short>\n");
}

void act_shortlong(void)
{
	cur_led = 0x3F;
	set_bit_led(cur_led);
	pr_info("<short><long>\n");
}

void act_longshort(void)
{
	cur_led = 0xFF;
	set_bit_led(cur_led);
	pr_info("<long><short>\n");
}

void act_longlong(void)
{
	cur_led = 0x00;
	set_bit_led(cur_led);
	pr_info("<long><long>\n");
}

action_t actions[ACTION_SIZE] = {
	act_nop,
	act_init,
	act_short,
	act_long,
	act_shortshort,
	act_shortlong,
	act_longshort,
	act_longlong,
};

int wait_thread_fn(void *data)
{
	enum state state = ST_INIT;
	enum input input;

	pr_info("wait thread created\n");

	while (1) {
		input = gpio_get_value(GPIO_INPUT_1);

		if ((state == ST_LONG || state == ST_SHORT) && is_short_push(pre_jiffies, 500)) input = TIMEOUT;
		else if ((state == ST_LIT || state == ST_SHORTSHORT || state == ST_LONGSHORT) && !is_short_push(pre_jiffies, 1000)) input = TIMEOUT;

		actions[transitions[state][input].action]();
		state = transitions[state][input].next_state;
		usleep_range(1000, 1001);

		if (wait_queue_flag == WAIT_QUEUE_EXIT || wait_queue_flag == WAIT_QUEUE_NEXT) {
			init_led();
			return wait_queue_flag;
		}
	}
	
	pr_info("wait thread destroyed\n");

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
	snprintf(name, sizeof(name), "simple-io-%d", gpio);

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

static int __init simple_switch_module_init(void)
{
	int i;
	pr_info("simple_switch module init\n");

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

	for (i = 0; i < gpio_outputs_len; i++)
		register_gpio(gpio_outputs[i], 0);

	for (i = 0; i < gpio_inputs_len; i++)
		register_gpio(gpio_inputs[i], 1);

	wait_thread = kthread_create(wait_thread_fn, NULL, "wait_thread");
	if (wait_thread) {
		wake_up_process(wait_thread);
	} else {
		pr_err("fail to create wait thread\n");
		goto gpio_error;
	}

	return 0;

gpio_error:
	for (i = 0; i < gpio_inputs_len; i++)
		gpio_free(gpio_inputs[i]);
	for (i = 0; i < gpio_outputs_len; i++)
		gpio_free(gpio_outputs[i]);
device_error:
	class_destroy(cls);
class_error:
	unregister_chrdev_region(dev, 1);
region_error:
	return -1;
}

static void __exit simple_switch_module_exit(void)
{
	int i;

	wait_queue_flag = WAIT_QUEUE_EXIT;
	wake_up_interruptible(&wait_queue);

	gpio_set_value(GPIO_INPUT_1, 0);

	for (i = 0; i < gpio_outputs_len; i++)
		gpio_free(gpio_outputs[i]);
	for (i = 0; i < gpio_inputs_len; i++)
		gpio_free(gpio_inputs[i]);

	cdev_del(&cdev);
	device_destroy(cls, dev);
	class_destroy(cls);
	unregister_chrdev_region(dev, 1);

	pr_info("simple_switch module exit\n");
}

module_init(simple_switch_module_init);
module_exit(simple_switch_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("nk <seven3126@gmail.com>");
MODULE_DESCRIPTION("GPIO LED control with switch");
MODULE_VERSION("1.0.0");