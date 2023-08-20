#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "bmp280"
#define DEVICE_DRIVER DEVICE_NAME "_driver"
#define DEVICE_CLASS DEVICE_NAME

#define I2C_BUS_AVAILABLE 1
#define BMP280_ADDRESS 0x76

#define BUF_SIZE 1024
#define TIMER_INTERVAL 5000

struct bmp_info {
	int id;
	int temp;
	int press;
};

struct bmp_kobj_item {
	struct attribute attr;
	char value[20];
};

static dev_t dev;
static struct class *cls;
static struct cdev cdev;
static struct i2c_adapter *bmp_adapter;
static struct i2c_client *bmp_client;
static struct bmp_info bmp_info;
static struct hrtimer bmp_timer;
static struct kobject *kobj;
static struct workqueue_struct *wq;

static char kbuf[BUF_SIZE];

static s32 dig_t1, dig_t2;
static s32 dig_p1, dig_p3, dig_p4, dig_p5, dig_p6, dig_p7, dig_p9;
static s64 dig_t3, dig_p2, dig_p8;
static int fine;

size_t get_bmp_info_str(char *buf, size_t len);

static struct i2c_driver bmp_driver = {
	.driver = {
		.name = DEVICE_DRIVER,
		.owner = THIS_MODULE
	}
};

static struct i2c_board_info bmp_board_info = {
	I2C_BOARD_INFO(DEVICE_NAME, BMP280_ADDRESS)
};

static struct bmp_kobj_item notify = {
	.attr = {
		.name = "notify",
		.mode = 0644
	},
	.value = "",
};

static struct bmp_kobj_item trigger = {
	.attr = {
		.name = "trigger",
		.mode = 0644
	},
	.value = "",
};

int open_device(struct inode *ip, struct file *fp)
{
	pr_info("[%s] open device\n", DEVICE_NAME);
	return 0;
}

int close_device(struct inode *ip, struct file* fp)
{
	pr_info("[%s] close device\n", DEVICE_NAME);
	return 0;
}

ssize_t read_device(struct file *fp, char __user *buf, size_t len, loff_t *offset)
{
	char info[20];
	size_t _len = get_bmp_info_str(info, 20);
	pr_info("[%s] read device\n", DEVICE_NAME);
	copy_to_user(buf, info, _len);
	return _len;
}

ssize_t write_device(struct file *fp, const char __user *buf, size_t len, loff_t *offset)
{
	pr_info("[%s] cannot write to device\n", DEVICE_NAME);
	return len;
}

ssize_t show_kobj(struct kobject *ko, struct attribute *at, char *buf)
{
	struct bmp_kobj_item *item = container_of(at, struct bmp_kobj_item, attr);
	pr_info("[%s] %s: %s\n", DEVICE_NAME, item->attr.name, item->value);
	return scnprintf(buf, strlen(item->value) + 1, "%s", item->value);
}

ssize_t store_kobj(struct kobject *ko, struct attribute *at, const char *buf, size_t count)
{
	struct bmp_kobj_item *item = container_of(at, struct bmp_kobj_item, attr);
	char *attr_name = item->attr.name;

	sscanf(buf, "%s", &item->value);
	pr_info("[%s] %s: %d\n", DEVICE_NAME, attr_name, item->value);

	if (!strcmp(attr_name, "notify")) {
		strncpy(notify.value, item->value, sizeof(item->value));
		sysfs_notify(kobj, NULL, "notify");
	} else if (!strcmp(attr_name, "trigger")) {
		strncpy(trigger.value, item->value, sizeof(item->value));
		sysfs_notify(kobj, NULL, "trigger");
	}

	return count;
}

int read_temperature(void)
{
	s32 d1, d2, d3, raw_temp;
	long var1, var2;

	d1 = i2c_smbus_read_byte_data(bmp_client, 0xfa);
	d2 = i2c_smbus_read_byte_data(bmp_client, 0xfb);
	d3 = i2c_smbus_read_byte_data(bmp_client, 0xfc); // msb 4bit

	raw_temp = ((d1 << 16) | (d2 << 8) | (d3 << 0)) >> 4;

	var1 = (((raw_temp >> 3) - (dig_t1 << 1)) * dig_t2) >> 11;
	var2 = (((((raw_temp >> 4) - dig_t1) * ((raw_temp >> 4) - dig_t1)) >> 12) * dig_t3) >> 14;
	fine = var1 + var2;

	return (fine * 5 + 128) >> 8; // °C
}

long read_pressure(void)
{
	s32 d1, d2, d3, raw_press;
	long var1, var2, p;

	d1 = i2c_smbus_read_byte_data(bmp_client, 0xf7);
	d2 = i2c_smbus_read_byte_data(bmp_client, 0xf8);
	d3 = i2c_smbus_read_byte_data(bmp_client, 0xf9); // msb 4bit

	raw_press = ((d1 << 16) | (d2 << 8) | (d3 << 0)) >> 4;

	var1 = fine - 128000;
	var2 = var1 * var1 * dig_p6;
	var2 = var2 + (var1 * ((s64)dig_p5 << 17));
	var2 = var2 + ((s64)dig_p4 << 35);
	var1 = ((var1 * var1 * dig_p3) >> 8) + ((var1 * dig_p2) << 12);
	var1 = (((1 << 47) + var1) * dig_p1) >> 33;

	if (var1 == 0) return 0; // avoid exception caused by division by zero

	p = 1048576 - raw_press;
	p = ((p << 31) - var2) * 3125 / var1;
	var1 = (dig_p9 * (p >> 13) * (p >> 13)) >> 25;
	var2 = (dig_p8 * p) >> 19;
	p = ((p + var1 + var2) >> 8) + (dig_p7 << 4);

	return (u32)p / 256; // hPa
}

size_t get_bmp_info_str(char *buf, size_t len)
{
	scnprintf(buf, len, "%d.%d %d.%d", bmp_info.temp / 100, bmp_info.temp % 100, bmp_info.press / 100, bmp_info.press % 100);
	pr_info("temperature: %d.%d°C, pressure: %d.%dhPa", bmp_info.temp / 100, bmp_info.temp % 100, bmp_info.press / 100, bmp_info.press % 100);
	return strlen(buf) + 1;
}

void wq_func(struct work_struct *w)
{
	char info[20];

	bmp_info.temp = read_temperature();
	bmp_info.press = read_pressure();

	get_bmp_info_str(info, sizeof(info));
	strncpy(notify.value, info, sizeof(info));
	sysfs_notify(kobj, NULL, "notify");
}

DECLARE_WORK(work, wq_func);

enum hrtimer_restart restart_bmp_timer(struct hrtimer *timer)
{
	queue_work(wq, &work);
	hrtimer_forward_now(timer, ms_to_ktime(TIMER_INTERVAL));
	return HRTIMER_RESTART;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = open_device,
	.read = read_device,
	.write = write_device,
	.release = close_device,
};

struct attribute *bmp_attrs[] = {
	&notify.attr,
	&trigger.attr,
	NULL,
};

ATTRIBUTE_GROUPS(bmp);

struct sysfs_ops sfops = {
	.show = show_kobj,
	.store = store_kobj,
};

struct kobj_type ktype = {
	.sysfs_ops = &sfops,
	.default_groups = bmp_groups,
};

static int __init bmp_module_init(void)
{
	u8 id;
	u16 tmp;

	pr_info("[%s] init module\n", DEVICE_NAME);

	if (alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0) {
		pr_err("[%s]fail to allocate character device number\n", DEVICE_NAME);
		goto region_error;
	}

	pr_info("[%s] major: %d, minor: %d\n", DEVICE_NAME, MAJOR(dev), MINOR(dev));

	if ((cls = class_create(THIS_MODULE, DEVICE_CLASS)) == NULL) {
		pr_err("[%s] fail to create device class\n", DEVICE_NAME);
		goto class_error;
	}

	if (device_create(cls, NULL, dev, NULL, DEVICE_DRIVER) == NULL) {
		pr_err("[%s] fail to create device driver\n", DEVICE_NAME);
		goto class_error;
	}

	cdev_init(&cdev, &fops);

	if (cdev_add(&cdev, dev, 1) < 0) {
		pr_err("[%s] fail to add character device driver\n", DEVICE_NAME);
		goto cdev_error;
	}

	bmp_adapter = i2c_get_adapter(1);
	if (bmp_adapter) {
		bmp_client = i2c_new_client_device(bmp_adapter, &bmp_board_info);
		if (bmp_client) {
			if (i2c_add_driver(&bmp_driver)) {
				pr_info("[%s] fail to add i2c driver\n");
				goto i2c_error;
			}
		}
		i2c_put_adapter(bmp_adapter);
	}

	id = i2c_smbus_read_byte_data(bmp_client, 0xd0);
	bmp_info.id = id;
	pr_info("[%s] ID: %x\n", DEVICE_NAME, id);

	dig_t1 = i2c_smbus_read_word_data(bmp_client, 0x88); // 2byte
	dig_t2 = i2c_smbus_read_word_data(bmp_client, 0x8a);
	tmp = i2c_smbus_read_word_data(bmp_client, 0x8c);
	dig_t3 = (s32)((int16_t)tmp);
	pr_info("dig_t: %d, %d, %d\n", dig_t1, dig_t2, dig_t3);

	dig_p1 = i2c_smbus_read_word_data(bmp_client, 0x8e);
	tmp = i2c_smbus_read_word_data(bmp_client, 0x90);
	dig_p2 = (s32)((int16_t)tmp);
	dig_p3 = i2c_smbus_read_word_data(bmp_client, 0x92);
	dig_p4 = i2c_smbus_read_word_data(bmp_client, 0x94);
	dig_p5 = i2c_smbus_read_word_data(bmp_client, 0x96);
	dig_p6 = i2c_smbus_read_word_data(bmp_client, 0x98);
	tmp = i2c_smbus_read_word_data(bmp_client, 0x9a);
	dig_p7 = (s32)((int16_t)tmp);
	tmp = i2c_smbus_read_word_data(bmp_client, 0x9c);
	dig_p8 = (s32)((int16_t)tmp);
	dig_p9 = i2c_smbus_read_word_data(bmp_client, 0x9e);
	pr_info("dig_p: %d, %d, %d, %d, %d, %d, %d, %d, %d\n", dig_p1, dig_p2, dig_p3, dig_p4, dig_p5, dig_p6, dig_p7, dig_p8, dig_p9);

	i2c_smbus_write_byte_data(bmp_client, 0xf5, 5 << 5); // inactive rate
	i2c_smbus_write_byte_data(bmp_client, 0xf4, ((5 << 5) | (5 << 2) | (3 << 0))); // measurement mode

	kobj = kzalloc(sizeof(struct kobject), GFP_KERNEL);
	if (kobj) {
		kobject_init(kobj, &ktype);
		if (kobject_add(kobj, kernel_kobj, DEVICE_NAME) < 0) {
			pr_err("[%s] fail to add kobject to sysfs\n", DEVICE_NAME);
			goto kobj_error;
		}
	}

	wq = create_workqueue("bmp_workqueue");

	hrtimer_init(&bmp_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	bmp_timer.function = &restart_bmp_timer;
	hrtimer_start(&bmp_timer, ms_to_ktime(TIMER_INTERVAL), HRTIMER_MODE_REL);

	return 0;

kobj_error:
	i2c_unregister_device(bmp_client);
	i2c_del_driver(&bmp_driver);
	kobject_put(kobj);

i2c_error:
	cdev_del(&cdev);

cdev_error:
	device_destroy(cls, dev);

driver_error:
	class_destroy(cls);

class_error:
	unregister_chrdev_region(dev, 1);

region_error:
	return -1;
}

static void __exit bmp_module_exit(void)
{
	hrtimer_cancel(&bmp_timer);
	destroy_workqueue(wq);

	if (kobj) {
		kobject_put(kobj);
		kfree(kobj);
	}

	i2c_unregister_device(bmp_client);
	i2c_del_driver(&bmp_driver);

	cdev_del(&cdev);
	device_destroy(cls, dev);
	class_destroy(cls);
	unregister_chrdev_region(dev, 1);

	pr_info("[%s] exit module\n", DEVICE_NAME);
}

module_init(bmp_module_init);
module_exit(bmp_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("nk <seven3126@gmail.com>");
MODULE_DESCRIPTION("BMP280 sensor driver");
MODULE_VERSION("1.0.0");