#include <linux/module.h>
#include <linux/fs.h> 
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEV_MEM_SIZE 512

// pesudo device memory
char device_buffer[DEV_MEM_SIZE];

// holds major & minior number of device
dev_t device_num;

// 
struct cdev cere_cdev;

// class
struct class* class_cere;

// device
struct device* device_cere;

loff_t cere_lseek (struct file *filp, loff_t offset, int whence)
{
    loff_t temp;
	pr_info("lseek requested\n");
    pr_info("current file position =%lld",filp->f_pos);
    switch (whence)
    {
    case SEEK_SET:
        if ((offset > DEV_MEM_SIZE)||(offset < 0)){
            return -EINVAL;
        }
        filp->f_pos = offset;
        break;
    case SEEK_CUR:
        temp = filp->f_pos + offset;
        if ((temp > DEV_MEM_SIZE)||(temp < 0))
        {
            return -EINVAL;
        }
        filp->f_pos = temp;
        break;
    case SEEK_END:
        temp = DEV_MEM_SIZE + offset;
        if ((temp > DEV_MEM_SIZE)||(temp < 0))
            return -EINVAL;
        filp->f_pos = temp;
        break;
    default:
        return -EINVAL;
    }
    pr_info("end file position =%lld",filp->f_pos);
    return filp->f_pos;
}

ssize_t cere_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
    pr_info("Read requested for %zu byies\n",count);
    // Adjust the count
    if((*f_pos + count) > DEV_MEM_SIZE)
    {
        count = DEV_MEM_SIZE + *f_pos;
    }

    /*copy to user*/
    if(copy_to_user(buff,&device_buffer[*f_pos],count)){
        return -EFAULT;
    }

    //update the current file postion
    *f_pos += count;

	return count;
}

ssize_t cere_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	pr_info("write requested for %zu byies\n",count);
    pr_info("Current file position = %lld\n",*f_pos);
    // Adjust the count
    if((*f_pos + count) > DEV_MEM_SIZE)
    {
        count = DEV_MEM_SIZE - *f_pos;
    }

    if(!count){
        return ENOMEM;
    }
    /*copy from user*/
    if(copy_from_user(&device_buffer[*f_pos],buff,count)){
        return -EFAULT;
    }

    //update the current file postion
    *f_pos += count;

	return count;
}

int cere_open (struct inode *inode, struct file *filp)
{
    
	pr_info("open requested \n");
	return 0;
}

int cere_release (struct inode *inode, struct file *filp)
{
	pr_info("release requested \n");
	return 0;
}

// file operation struct
struct file_operations cere_fops = 
{
    .open = cere_open,
    .write = cere_write,
    .read = cere_read,
    //.llseek = cere_lseek,
    .release = cere_release,
    .owner = THIS_MODULE
};



// Module Entry point
static int __init cere_init(void)
{
    int ret;
    int check_err;
    pr_info("Hello world\n");

    // STEP1: Create device number
    ret = alloc_chrdev_region(&device_num, 0, 1, "cere_devices");
    if(ret < 0){
		pr_err("Alloc chrdev failed\n");
		goto out;
	}

    pr_info("Device number <major>:<minor> = %d:%d\n",MAJOR(device_num),MINOR(device_num));

    // STEP2-PART1: Initializes cdev, remembering fops, 
    //              making it ready to add to the system with cdev_add.
    cdev_init(&cere_cdev, &cere_fops);

    // STEP2-PART2: cdev_add adds the device represented by p to the system,
    //              making it live immediately
    cere_cdev.owner = THIS_MODULE;
    cdev_add(&cere_cdev, device_num, 1);
    if(ret < 0){
		pr_err("Cdev add failed\n");
		goto unreg_chrdev;
	}
    pr_info("fops registered\n");

    // STEP3-PART1: creates the device class under /sys/class/
    class_cere = class_create(THIS_MODULE,"class_cere");
    if(IS_ERR(class_cere)){
		pr_err("Class creation failed\n");
		ret = PTR_ERR(class_cere);
		goto cdev_del;
	}
    pr_info("sys class created\n");

    // STEP3-PART2: populates the sysfs with device information
    device_cere = device_create(class_cere,NULL,device_num,NULL,"cer");
    check_err = PTR_ERR(device_cere);
    pr_info("check err = %x\n",check_err);
    pr_info("is err = %x\n",IS_ERR(device_cere));
    if(IS_ERR(device_cere)){
		pr_err("Device create failed\n");
		ret = PTR_ERR(device_cere);
		goto class_del;
	}

    pr_info("Cere module init success ...\n");

    return 0;

class_del:
	class_destroy(class_cere);
cdev_del:
	cdev_del(&cere_cdev);	
unreg_chrdev:
	unregister_chrdev_region(device_num,1);
out:
	pr_info("Module insertion failed\n");

	return ret;
}

// Module Exit point
static void __exit cere_exit(void){

    //
    device_destroy(class_cere,device_num);

    //
    class_destroy(class_cere);

    //
    cdev_del(&cere_cdev);

    //Releasing the device number
    unregister_chrdev_region(device_num,1);
    
    pr_info("cere module unloaded\n");
}
module_init(cere_init);
module_exit(cere_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NIKHIL");
MODULE_DESCRIPTION("This is a parctice char driver");
