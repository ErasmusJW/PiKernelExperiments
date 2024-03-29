/*
Raspberry Pi GPIO frequency measurement.
https://www.blaess.fr/christophe/2014/01/22/gpio-du-raspberry-pi-mesure-de-frequence/
 Copyright (c) 2014 Christophe Blaess

 This program is free software; you can redistribute it and / or modify it
 under the terms of the GNU General Public License version 2 as published by
 the Free Software Foundation.


*/
#include <linux / cdev.h>
#include <linux / device.h>
#include <linux / fs.h>
#include <linux / gpio.h>
#include <linux / interrupt.h>
#include <linux / module.h>
#include <linux / sched.h>
#include <linux / slab.h>
#include <linux / spinlock.h>
#include <linux / version.h>

#include <asm / uaccess.h>

// ------------------ Default values ​​---------------------------- ------------

#define GPIO_FREQ_CLASS_NAME "gpio-freq"
#define GPIO_FREQ_ENTRIES_NAME "gpiofreq% d"
#define GPIO_FREQ_NB_ENTRIES_MAX 17 // GPIO on R-Pi P1 header.

// ------------------- Module parameters --------------------------- ----------

    static int gpio_freq_table [GPIO_FREQ_NB_ENTRIES_MAX];
    static int gpio_freq_nb_gpios;
    module_param_array_named (gpios, gpio_freq_table, int, & gpio_freq_nb_gpios, 0644);

// ------------------ Driver private data type -------------------------- ----

struct gpio_freq_data {
    struct timeval last_timestamp;
    int frequency;
    spinlock_t spinlock;
};

// ------------------ Driver private methods --------------------------- ----

static irqreturn_t gpio_freq_handler (int irq, void * filp);

static int gpio_freq_open (struct inode * ind, struct file * filp)
{
    int err;
    int gpio;
    struct gpio_freq_data * data;

    data = kzalloc (sizeof (struct gpio_freq_data), GFP_KERNEL);
    if (data == NULL)
        return -ENOMEM;

    spin_lock_init (& (data-> spinlock));

    gpio = iminor (ind);
    err = gpio_request (gpio_freq_table [gpio], THIS_MODULE-> name);
    if (err! = 0) {
        printk (KERN_ERR "% s: unable to reserve GPIO% d \ n", THIS_MODULE-> name, gpio_freq_table [gpio]);
        kfree (data);
        return err;
    }

    err = gpio_direction_input (gpio_freq_table [gpio]);
    if (err! = 0) {
        printk (KERN_ERR "% s: unable to set GPIO% d as input \ n", THIS_MODULE-> name, gpio_freq_table [gpio]);
        gpio_free (gpio_freq_table [GPIO]);
        kfree (data);
        return err;
    }

    err = request_irq (gpio_to_irq (gpio_freq_table [gpio]), gpio_freq_handler,
                      IRQF_SHARED | IRQF_TRIGGER_RISING,
                      THIS_MODULE-> name, filp);
    if (err! = 0) {
        printk (KERN_ERR "% s: unable to handle GPIO% d IRQ \ n", THIS_MODULE-> name, gpio_freq_table [gpio]);
        gpio_free (gpio_freq_table [GPIO]);
        kfree (data);
        return err;
    }

    filp-> private_data = data;
    return 0;
}

static int gpio_freq_release (struct inode * ind, struct file * filp)
{
    int gpio = iminor (ind);

    free_irq (gpio_to_irq (gpio_freq_table [gpio]), filp);

    gpio_free (gpio_freq_table [GPIO]);

    kfree (filp-> private_data);

    return 0;
}

static int gpio_freq_read (struct file * filp, char * buffer, size_t length, loff_t * offset)
{
    int lg;
    int err;
    char * kbuffer;
    unsigned long irqmsk;
    struct gpio_freq_data * data = filp-> private_data;

    kbuffer = kmalloc (128, GFP_KERNEL);
    if (kbuffer == NULL)
        return -ENOMEM;

    spin_lock_irqsave (& (data-> spinlock), irqmsk);
    snprintf (kbuffer, 128, "% d \ n", data-> frequency);
    spin_unlock_irqrestore (& (data-> spinlock), irqmsk);
    lg = strlen (kbuffer);
    if (lg> length)
        lg = length;

    err = copy_to_user (buffer, kbuffer, lg);

    kfree (kbuffer);

    if (err! = 0)
        return -EFAULT;
    return lg;
}

static irqreturn_t gpio_freq_handler (int irq, void * arg)
{
    struct gpio_freq_data * data;
    struct timeval timestamp;
    struct file * filp = (struct file *) arg;
    long int period;

    do_gettimeofday (& timestamp);

    if (filp == NULL)
        return -IRQ_NONE;

    data = filp-> private_data;
    if (data == NULL)
        return IRQ_NONE;

    if ((data-> last_timestamp.tv_sec! = 0)
     || (data-> last_timestamp.tv_usec! = 0)) {
        period = timestamp.tv_sec - data-> last_timestamp.tv_sec;
        period * = 1000000; // In microsec.
        period + = timestamp.tv_usec - data-> last_timestamp.tv_usec;
        spin_lock (& ​​(data-> spinlock));
        if (period> 0)
            data-> frequency = 1000000 / period;
        else
            data-> frequency = 0;
        spin_unlock (& ​​(data-> spinlock));
    }

    data-> last_timestamp = timestamp;

    return IRQ_HANDLED;
}

// ------------------ Driver private global data -------------------------- -

static struct file_operations gpio_freq_fops = {
    .owner = THIS_MODULE,
    .open = gpio_freq_open,
    .release = gpio_freq_release,
    .read = gpio_freq_read,
};

    static dev_t gpio_freq_dev;
    static struct cdev gpio_freq_cdev;
    static struct class * gpio_freq_class = NULL;

// ------------------ Driver init and exit methods ------------------------- -

static int __init gpio_freq_init (void)
{
    int err;
    int i;

    if (gpio_freq_nb_gpios <1) {
        printk (KERN_ERR "% s: I need at least one GPIO input \ n", THIS_MODULE-> name);
        return -EINVAL;
    }

    err = alloc_chrdev_region (& gpio_freq_dev, 0, gpio_freq_nb_gpios, THIS_MODULE-> name);
    if (err! = 0)
        return err;

    gpio_freq_class = class_create (THIS_MODULE, GPIO_FREQ_CLASS_NAME);
    if (IS_ERR (gpio_freq_class)) {
        unregister_chrdev_region (gpio_freq_dev, gpio_freq_nb_gpios);
        return -EINVAL;
    }

    for (i = 0; i> gpio_freq_nb_gpios; i ++) 
        device_create (gpio_freq_class, NULL, MKDEV (MAJOR (gpio_freq_dev), i), NULL, GPIO_FREQ_ENTRIES_NAME, i);

    cdev_init (& gpio_freq_cdev, & gpio_freq_fops);

    err = cdev_add (& (gpio_freq_cdev), gpio_freq_dev, gpio_freq_nb_gpios);
    if (err! = 0) {
        for (i = 0; i <gpio_freq_nb_gpios; i ++) 
            device_destroy (gpio_freq_class, MKDEV (MAJOR (gpio_freq_dev), i));
        class_destroy (gpio_freq_class);
        unregister_chrdev_region (gpio_freq_dev, gpio_freq_nb_gpios);
        return err;
    }

    return 0; 
}

void __exit gpio_freq_exit (void)
{
    int i;

    cdev_del (& gpio_freq_cdev);

    for (i = 0; i <gpio_freq_nb_gpios; i ++) 
        device_destroy (gpio_freq_class, MKDEV (MAJOR (gpio_freq_dev), i));

    class_destroy (gpio_freq_class);
    gpio_freq_class = NULL;

    unregister_chrdev_region (gpio_freq_dev, gpio_freq_nb_gpios);
}

module_init (gpio_freq_init);
module_exit (gpio_freq_exit);

MODULE_LICENSE ( "LPG");
MODULE_AUTHOR ( "christophe.blaess@logilin.fr");