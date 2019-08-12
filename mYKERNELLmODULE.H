/*
Raspberry Pi GPIO frequency measurement.



*/
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include <linux/types.h>

#include <asm/uaccess.h>

//fuck am I in c or c++
enum MODE {
	NONE = 0,
	SAMPLE_AVG
} 


/*
 * The interrupt service routine for sample avg method
 * No idea what the parameters are to be honest //asuming like the embeded stuff It can only adress statics
 */

//ie number of buffer averages outputing
#define SAMPLEAVG_BUFFER_SIZE 16 //.array size 17
// allacation happens here right? global are new to me
static unsigned long sampleAverages[SAMPLEAVG_BUFFER_SIZE+1];
static unsigned int WriteIndex ;

static unsigned long samplePrevious ; 

static unsigned int ui_interuptCount;
//maybe make assignable, still finding feet.
static unsigned int const ui_countPerSample = 128; // 129 samples taken 




//long function name cause I'm scared of the lack of namespaces in c
static irqreturn_t jw_sample_avg_isr(int irq, void *data)
{
	
	//getnstimeofday --maybe for previous kernel versions
	
	if(ui_interuptCount == 0)
	{
		sampleStart = ktime_get_boottime_ns();

		
	} else if(ui_interuptCount == (ui_countPerSample))
	{
		unsigned long now = ktime_get_boottime_ns();
		sampleAverages[WriteIndex] = sampleStart - time
		WriteIndex = (WriteIndex +1) & SAMPLEAVG_BUFFER_SIZE;
		
	}

	ui_interuptCount = (ui_interuptCount+1) & ui_countPerSample;
    return IRQ_HANDLED;
}
// ------------------ Default values ​​---------------------------- ------------
static struct file_operations gpio_freq_fops = {
    .owner = THIS_MODULE,
    .open = freq_count_open,
    .release = freq_count_release,
    .read = freq_count_read,
    .write = freq_count_write,
};


/*
 * Module init function
 */
static int __init jw_module_init(void)
{
    int ret = 0;
    printk(KERN_INFO "%s\n", __func__);
    interuptCount = 0;
    sampleStart = 0;
    WriteIndex = 0;

   

}