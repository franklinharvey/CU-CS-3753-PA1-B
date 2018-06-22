/* 
 * I'd say half of this comes from the code posted in PIAZZA and half comes from 
 * http://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device/
 * which was good because combining the two allowed me to better understand
 */

#include<linux/init.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/slab.h>
#include<linux/uaccess.h>
#include<linux/device.h>

#define BUFFER_SIZE 1024
#define D_NAME "frank device"

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("Franklin Harvey"); 
MODULE_DESCRIPTION("Char Driver Simple Device"); 
MODULE_VERSION("1");

int printk(const char *, ...);

static char *device_buffer;
static int major_num; //registerd with major number 240
static int open_count;
static int close_count;
static unsigned long bytes_not_copied;
static unsigned long bytes_not_written;
static int size_of_kbuff = 0;

static char   message[256] = {0};
static short  size_of_message;

static ssize_t simple_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
static ssize_t simple_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);
static int simple_char_driver_open (struct inode *pinode, struct file *pfile);
static int simple_char_driver_close (struct inode *pinode, struct file *pfile);
static loff_t simple_char_driver_seek (struct file *pfile, loff_t offset, int whence);

static struct file_operations simple_char_driver_file_operations = {
	.owner = THIS_MODULE, 
	.read = simple_char_driver_read, 
	.write = simple_char_driver_write, 
	.open = simple_char_driver_open, 
	.release = simple_char_driver_close, 
	.llseek = simple_char_driver_seek, 
};

/*	READ	*/
ssize_t simple_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	printk(KERN_INFO "DEVICE-BUFFER: %s", device_buffer);
	// (to_user_space,from_kernel_space,length)
	bytes_not_copied = copy_to_user(buffer, device_buffer, size_of_kbuff);
	if (bytes_not_copied==0){
		printk(KERN_INFO "READ: Sent %d characters to the user\n", size_of_kbuff);
		size_of_kbuff = 0;
		return (size_of_message=0);
	}
	else {
		printk(KERN_INFO "READ FAILED: Did not copy %lu bytes\n", bytes_not_copied);
		size_of_kbuff = bytes_not_copied;
		return -EFAULT;
	}
}

/*	WRITE	*/
ssize_t simple_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	// (to_kernel_space, from_user_space, length)
	bytes_not_written = copy_from_user(device_buffer, buffer, length);
	printk(KERN_INFO "Received %s as write message\n", device_buffer);
	if (bytes_not_written==0){
		printk(KERN_INFO "WRITE: Sent %d characters to the kernel\n", length);
		size_of_kbuff=length;
		return(length=0);
	}
	else {
		printk(KERN_INFO "WRITE FAILED: Did not write %lu bytes\n", bytes_not_written);
		return -EFBIG;
	}
}

/*	SEEK	*/
static loff_t simple_char_driver_seek (struct file *pfile, loff_t offset, int whence)
{
	/*	
	 *	Inspired by
	 *	https://www.safaribooksonline.com/library/view/linux-device-drivers/0596000081/ch05s05.html 
	 *
	 *
	 *	This won't work with f_pos not in place (in relation to pfile).
	 *
	*/

	loff_t newpos;	
	if (offset > BUFFER_SIZE)
	{
		//INVALID OFFSET
		return -EINVAL;
	}

	if (whence == SEEK_SET)
	{
		//SEEK_SET
		newpos = offset;
	}
	else if (whence == SEEK_CUR)
	{
		//SEEK_CUR
		newpos = pfile->f_pos + offset;
	}
	else if (whence == SEEK_END)
	{
		//SEEK_END
		newpos = BUFFER_SIZE + offset;
	}
	else
	{
		//INVALID WHENCE
		return -EINVAL;
	}
	pfile->f_pos = newpos;
	return newpos;
}

/*	OPEN	*/
static int simple_char_driver_open(struct inode *inodep, struct file *filep){
	open_count++;
	printk(KERN_INFO "OPEN: %d time(s)\n", open_count);
	return 0;
}

/*	CLOSE	*/
static int simple_char_driver_close(struct inode *inodep, struct file *filep){
	printk(KERN_INFO "CLOSED\n");
	return 0;
}

/*	INIT	*/
static int simple_char_driver_init(void)
{	
	/*Didn't bother changing this*/

	printk(KERN_EMERG "simple char driver init function is called!\n");
	major_num = register_chrdev(240, D_NAME, &simple_char_driver_file_operations);
	if (major_num < 0) {
		printk(KERN_EMERG "register_chrdev failed\n");
		return -1;
	}
	device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	open_count = close_count = 0;
	return 0;
}

//exit module
static void simple_char_driver_exit(void)
{
	/*Didn't bother changing this*/

	printk(KERN_EMERG "simple char driver exiting!\n");
	unregister_chrdev(240, D_NAME);
	printk(KERN_EMERG "unregister successfull\n");
	kfree(device_buffer);
	printk(KERN_EMERG "kernel buffer freed\n");
} 

module_init(simple_char_driver_init);
module_exit(simple_char_driver_exit);
