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
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>



#define _PIN  14 // 0




#define DEV_NAME "count"
#define THREAD_PRIORITY 45
#define THREAD_NAME "toggler"
struct task_struct *task;

static bool toggleValue = true;
static struct gpio signals[] = {
        { _PIN, GPIOF_IN, "RX Signal" },  // Rx signal
};

// int toggle_thread(void *data){

//    struct task_struct *TSK;
//    struct sched_param schedParam = { .sched_priority = MAX_RT_PRIO  };
//   //struct sched_param PARAM = { .sched_priority = DEFAULT_PRIO };
//   TSK = current;

//   schedParam.sched_priority = 99;
//   sched_setscheduler(TSK, SCHED_FIFO, &schedParam);

//   while(1) {
//       toggleValue = !toggleValue;
//       gpio_set_value(TOGGLE_PIN,toggleValue);

//       //usleep_range(1,2);
      
        
//     if (kthread_should_stop()) break;
//   }
//   return 0;
// }


static unsigned long freqCountDuration;
static struct timespec sampleStart;
static struct timespec sampleEnd;
static unsigned int ui_interuptCount;
static const unsigned long ui_countPerSample = 100;

static int IrqNumber = -1;
spinlock_t spinlock;

static irqreturn_t jw_sample_avg_isr(int irq, void *data)
{
	
	//ktime_get_boottime_ns -- for newer kernels
	
	if(ui_interuptCount == 0)
	{
		 getnstimeofday(&sampleStart);
		
   
	} else if(ui_interuptCount >= (ui_countPerSample))
	{
		getnstimeofday(&sampleEnd);
        spin_lock (&spinlock);
            freqCountDuration = timespec_to_ns(&sampleStart) - timespec_to_ns(&sampleEnd);
        spin_unlock(&spinlock);
		ui_interuptCount = 0;
		
		
	}else
   ++ui_interuptCount;

	ui_interuptCount = (ui_interuptCount+1) & ui_countPerSample;
    return IRQ_HANDLED;
}

static int freq_count_open (struct inode * ind, struct file * filp)
{
     printk (KERN_INFO ": freq count open \n" );
     int err;

    err = gpio_request(_PIN,"COUNT");
    if(err)
        printk (KERN_ERR ": GPIO request failed \n");
    err = gpio_direction_input(_PIN);
    if(err)
        printk (KERN_ERR ": GPIO input failed \n");


    IrqNumber = gpio_to_irq(signals[0].gpio);
     printk (KERN_INFO ": irq number %d \n",IrqNumber );
    err = request_irq (IrqNumber, jw_sample_avg_isr,
                       IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_DISABLED,
                      THIS_MODULE->name, NULL);
    printk (KERN_INFO ": request irq err %d \n",err );                      
    if (err != 0) {
        printk (KERN_ERR ": unable to handle GPIO IRQ \n");
        gpio_free (_PIN);
        //kfree (data);
        return err;
    }
    return 0;
}
//no clue about PRIORITY the thing I read shows minus is higher hmmm

static int freq_count_release (struct inode * ind, struct file * filp)
{
    printk(KERN_INFO ": frequency  release \n" );

    free_irq (IrqNumber,filp);
    printk(KERN_INFO ":  IRQ released \n" );

    gpio_free(_PIN);
    printk(KERN_INFO ":  release gpio done \n" );
    return 0;
}

static int freq_count_read (struct file * filp, char * buffer, size_t length, loff_t * offset)
{
    printk(KERN_INFO ": freq count read function \n" );
    printk (KERN_ERR ": GPIO input failed %d",freqCountDuration);

    char * kbuffer;
    int lg;
    kbuffer = kmalloc (128, GFP_KERNEL);
    if (kbuffer == NULL)
        return -ENOMEM;

     spin_lock(&spinlock); //shit should propbably use something like spin_lock_irqsave and pass the spinlock in on data but internet down can't google
    snprintf (kbuffer, 128, "%d \n", freqCountDuration);
    spin_unlock (&spinlock);
   
    lg = strlen (kbuffer);
    if (lg> length)
        lg = length;
    
    int err = copy_to_user (buffer, kbuffer, lg);

    kfree (kbuffer);

    if (err != 0)
        return -EFAULT;
    return lg;

  
   

    return 8;
}


static struct file_operations freq_count_file_operations = {
    .owner = THIS_MODULE,
    .open = freq_count_open,
    .release = freq_count_release,
    .read = freq_count_read
};








static struct class * pin_toggle_class = NULL;
 static dev_t pin_toggler_dev;
 static struct cdev pin_toggler_cdev;

 static int Major;

static int __init freq_count_init (void)
{
    int err;
    int i;
    spin_lock_init (&spinlock);
    Major = register_chrdev(0, DEV_NAME, &freq_count_file_operations);
	if(Major < 0){
		printk(KERN_ALERT "Reg. char dev fail %d\n",Major);
		return Major;
	}
	printk(KERN_INFO "Major number %d.\n", Major);
	printk(KERN_INFO "created a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEV_NAME, Major);


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

    // cdev_init (& pin_toggler_cdev, &freq_count_file_operations);

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

void __exit freq_count_exit (void)
{
    int i;

    unregister_chrdev(Major, DEV_NAME);
}

module_init (freq_count_init);
module_exit (freq_count_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR ( "jacques.erasmus@fakeemail.com");