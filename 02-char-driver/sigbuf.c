// SPDX-License-Identifier: GPL-2.0
/*
 * sigbuf - a character device backed by a circular buffer.
 *
 * Step 02 of an embedded Linux driver project. Userspace writes bytes to
 * /dev/sigbuf and reads them back in FIFO order - the same shape as a sample
 * buffer sitting between a hardware producer and a userspace consumer.
 */
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "sigbuf_ioctl.h"

#define DEVICE_NAME "sigbuf"

static unsigned int capacity = 1024;
module_param(capacity, uint, 0444);
MODULE_PARM_DESC(capacity, "ring buffer size in bytes (default 1024)");

static dev_t sigbuf_devt;
static struct cdev sigbuf_cdev;
static struct class *sigbuf_class;

static u8 *ring;
static size_t head;    /* next write position */
static size_t tail;    /* next read position  */
static size_t stored;  /* bytes currently held */
static DEFINE_MUTEX(sigbuf_lock);

static int sigbuf_open(struct inode *inode, struct file *file)
{
	pr_info("sigbuf: opened by pid %d\n", current->pid);
	return 0;
}

static int sigbuf_release(struct inode *inode, struct file *file)
{
	pr_info("sigbuf: released by pid %d\n", current->pid);
	return 0;
}

static ssize_t sigbuf_read(struct file *file, char __user *buf,
			   size_t len, loff_t *off)
{
	size_t n, first;
	ssize_t ret;

	if (mutex_lock_interruptible(&sigbuf_lock))
		return -ERESTARTSYS;

	n = min(len, stored);
	if (n == 0) {
		mutex_unlock(&sigbuf_lock);
		return 0;  /* empty buffer looks like end of file */
	}

	/* stored data may wrap past the end, so copy it in up to two chunks */
	first = min(n, (size_t)capacity - tail);
	if (copy_to_user(buf, ring + tail, first)) {
		ret = -EFAULT;
		goto out;
	}
	if (n > first && copy_to_user(buf + first, ring, n - first)) {
		ret = -EFAULT;
		goto out;
	}

	tail = (tail + n) % capacity;
	stored -= n;
	ret = n;
	pr_info("sigbuf: read %zu bytes, %zu still stored\n", n, stored);
out:
	mutex_unlock(&sigbuf_lock);
	return ret;
}

static ssize_t sigbuf_write(struct file *file, const char __user *buf,
			    size_t len, loff_t *off)
{
	size_t space, n, first;
	ssize_t ret;

	if (mutex_lock_interruptible(&sigbuf_lock))
		return -ERESTARTSYS;

	space = capacity - stored;
	n = min(len, space);
	if (n == 0) {
		mutex_unlock(&sigbuf_lock);
		return -ENOSPC;
	}

	first = min(n, (size_t)capacity - head);
	if (copy_from_user(ring + head, buf, first)) {
		ret = -EFAULT;
		goto out;
	}
	if (n > first && copy_from_user(ring, buf + first, n - first)) {
		ret = -EFAULT;
		goto out;
	}

	head = (head + n) % capacity;
	stored += n;
	ret = n;
	pr_info("sigbuf: wrote %zu bytes, %zu now stored\n", n, stored);
out:
	mutex_unlock(&sigbuf_lock);
	return ret;
}

static long sigbuf_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned int val;
	long ret = 0;

	/* reject commands that were never meant for this driver */
	if (_IOC_TYPE(cmd) != SIGBUF_IOC_MAGIC || _IOC_NR(cmd) > SIGBUF_IOC_MAXNR)
		return -ENOTTY;

	if (mutex_lock_interruptible(&sigbuf_lock))
		return -ERESTARTSYS;

	switch (cmd) {
	case SIGBUF_RESET:
		head = 0;
		tail = 0;
		stored = 0;
		memset(ring, 0, capacity);
		pr_info("sigbuf: buffer reset\n");
		break;
	case SIGBUF_GET_STORED:
		val = stored;
		if (copy_to_user((unsigned int __user *)arg, &val, sizeof(val)))
			ret = -EFAULT;
		break;
	case SIGBUF_GET_CAPACITY:
		val = capacity;
		if (copy_to_user((unsigned int __user *)arg, &val, sizeof(val)))
			ret = -EFAULT;
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	mutex_unlock(&sigbuf_lock);
	return ret;
}

static const struct file_operations sigbuf_fops = {
	.owner          = THIS_MODULE,
	.open           = sigbuf_open,
	.release        = sigbuf_release,
	.read           = sigbuf_read,
	.write          = sigbuf_write,
	.unlocked_ioctl = sigbuf_ioctl,
};

static int __init sigbuf_init(void)
{
	struct device *dev;
	int ret;

	if (capacity == 0) {
		pr_err("sigbuf: capacity must be greater than zero\n");
		return -EINVAL;
	}

	ring = kzalloc(capacity, GFP_KERNEL);
	if (!ring)
		return -ENOMEM;

	ret = alloc_chrdev_region(&sigbuf_devt, 0, 1, DEVICE_NAME);
	if (ret) {
		pr_err("sigbuf: alloc_chrdev_region failed (%d)\n", ret);
		goto err_free;
	}

	cdev_init(&sigbuf_cdev, &sigbuf_fops);
	sigbuf_cdev.owner = THIS_MODULE;
	ret = cdev_add(&sigbuf_cdev, sigbuf_devt, 1);
	if (ret) {
		pr_err("sigbuf: cdev_add failed (%d)\n", ret);
		goto err_region;
	}

	sigbuf_class = class_create(DEVICE_NAME);
	if (IS_ERR(sigbuf_class)) {
		ret = PTR_ERR(sigbuf_class);
		pr_err("sigbuf: class_create failed (%d)\n", ret);
		goto err_cdev;
	}

	dev = device_create(sigbuf_class, NULL, sigbuf_devt, NULL, DEVICE_NAME);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		pr_err("sigbuf: device_create failed (%d)\n", ret);
		goto err_class;
	}

	pr_info("sigbuf: ready at /dev/%s (major %d, minor %d), capacity %u bytes\n",
		DEVICE_NAME, MAJOR(sigbuf_devt), MINOR(sigbuf_devt), capacity);
	return 0;

err_class:
	class_destroy(sigbuf_class);
err_cdev:
	cdev_del(&sigbuf_cdev);
err_region:
	unregister_chrdev_region(sigbuf_devt, 1);
err_free:
	kfree(ring);
	return ret;
}

static void __exit sigbuf_exit(void)
{
	device_destroy(sigbuf_class, sigbuf_devt);
	class_destroy(sigbuf_class);
	cdev_del(&sigbuf_cdev);
	unregister_chrdev_region(sigbuf_devt, 1);
	kfree(ring);
	pr_info("sigbuf: unloaded\n");
}

module_init(sigbuf_init);
module_exit(sigbuf_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yusuf Mehmed");
MODULE_DESCRIPTION("Character device backed by a circular buffer");
MODULE_VERSION("0.3");
