#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <uapi/linux/sched/types.h>
#include <linux/interrupt.h>

#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/gpio.h>




#define TOGGLE_PIN  14 // 0

#include <asm/uaccess.h>

#define PIN_TOGGLER_CLASS_NAME "toggler-class"
#define GPIO_FREQ_ENTRIES_NAME "toggler"
#define THREAD_PRIORITY 45
#define THREAD_NAME "toggler"
struct task_struct *task;

static bool toggleValue = true;


int toggle_thread(void *data){

   struct task_struct *TSK;
   struct sched_param schedParam = { .sched_priority = MAX_RT_PRIO  };
  //struct sched_param PARAM = { .sched_priority = DEFAULT_PRIO };
  TSK = current;

  schedParam.sched_priority = 99;
  sched_setscheduler(TSK, SCHED_FIFO, &schedParam);

  while(1) {
      toggleValue = !toggleValue;
      gpio_set_value(TOGGLE_PIN,toggleValue);

      //usleep_range(1,2);
      
        
    if (kthread_should_stop()) break;
  }
  return 0;
}


static int pin_toggle_open (struct inode * ind, struct file * filp)
{
     printk (KERN_INFO ": ping toggle open \n" );
    int gpiReuqestStat = gpio_request(TOGGLE_PIN,"TOGGLE");
    if(gpiReuqestStat != 0 )
    {
         printk (KERN_ERR ": unable to reserve gpio \n" );
    }
    gpiReuqestStat = gpio_direction_output(TOGGLE_PIN, 1);
        if(gpiReuqestStat != 0 )
    {
         printk (KERN_ERR ": unable to set gpio direction  \n" );
    }

      printk(KERN_INFO ": ping toggle starting thread \n" );
        task = kthread_run(toggle_thread, NULL, THREAD_NAME);
      printk(KERN_INFO ": ping toggle starting thread DONE \n" );
    return 0;
}
//no clue about PRIORITY the thing I read shows minus is higher hmmm

static int pin_toggle_release (struct inode * ind, struct file * filp)
{
    printk(KERN_INFO ": ping toggle release \n" );
    printk(KERN_INFO ": stopping thread \n" );
    kthread_stop(task);
    printk(KERN_INFO ":  thread stopping done \n" );
    printk(KERN_INFO ":  release start \n" );
    gpio_free(TOGGLE_PIN);
    printk(KERN_INFO ":  release gpio done \n" );
    return 0;
}

static int pin_toggle_read (struct file * filp, char * buffer, size_t length, loff_t * offset)
{
    printk(KERN_INFO ": ping toggle read function \n" );
    return 0;
}


static struct file_operations pin_toggle_file_operations = {
    .owner = THIS_MODULE,
    .open = pin_toggle_open,
    .release = pin_toggle_release,
    .read = pin_toggle_read
};








static struct class * pin_toggle_class = NULL;
 static dev_t pin_toggler_dev;
 static struct cdev pin_toggler_cdev;

 static int Major;

static int __init pin_toggle_init (void)
{
    int err;
    int i;

    Major = register_chrdev(0, GPIO_FREQ_ENTRIES_NAME, &pin_toggle_file_operations);
	if(Major < 0){
		printk(KERN_ALERT "Reg. char dev fail %d\n",Major);
		return Major;
	}
	printk(KERN_INFO "Major number %d.\n", Major);
	printk(KERN_INFO "created a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", GPIO_FREQ_ENTRIES_NAME, Major);


    // err = alloc_chrdev_region (& pin_toggler_dev, 0, 1, THIS_MODULE-> name);
    // if (err != 0)
    //     return err;

    // pin_toggle_class = class_create (THIS_MODULE, PIN_TOGGLER_CLASS_NAME);
    // if (IS_ERR (pin_toggle_class)) {
    //     unregister_chrdev_region (pin_toggler_dev, 1);
    //     return -EINVAL;
    // }

    // for (i = 0; i> 1; i ++) 
    //     device_create (pin_toggle_class, NULL, MKDEV (MAJOR (pin_toggler_dev), i), NULL, GPIO_FREQ_ENTRIES_NAME, i);

    // cdev_init (& pin_toggler_cdev, &pin_toggle_file_operations);

    // err = cdev_add (& (pin_toggler_cdev), pin_toggler_dev, 1);
    // if (err  != 0) {
    //     for (i = 0; i <1; i ++) 
    //         device_destroy (pin_toggle_class, MKDEV (MAJOR (pin_toggler_dev), i));
    //     class_destroy (pin_toggle_class);
    //     unregister_chrdev_region (pin_toggler_dev, 1);
    //     return err;
    // }

    return 0; 
}

void __exit pin_toggle_exit (void)
{
    int i;

    unregister_chrdev(Major, GPIO_FREQ_ENTRIES_NAME);
}

module_init (pin_toggle_init);
module_exit (pin_toggle_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR ( "jacques.erasmus@fakeemail.com");