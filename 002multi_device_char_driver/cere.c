#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define NO_OF_DEVICES 4

#define DEV1_MEM_SIZE 512
#define DEV2_MEM_SIZE 1024
#define DEV3_MEM_SIZE 512
#define DEV4_MEM_SIZE 256

#define RDONLY 0X01
#define WRONLY 0X10
#define RDWR 0X11

// pesudo device memory
char device1_buffer[DEV1_MEM_SIZE];
char device2_buffer[DEV2_MEM_SIZE];
char device3_buffer[DEV3_MEM_SIZE];
char device4_buffer[DEV4_MEM_SIZE];

/* This struct represent device private data */
struct device_private_data
{
    char *buffer;
    unsigned size;
    const char *serial_number;
    int perm;
    struct cdev cere_cdev;
};

/* This struct represent driver private data*/
struct driver_private_data
{
    int total_devices;
    // holds major & minior number of device
    dev_t device_num;
    // class
    struct class *class_cere;
    // device
    struct device *device_cere;
    struct device_private_data device_data[NO_OF_DEVICES];
};

struct driver_private_data drv_data =
    {
        .total_devices = NO_OF_DEVICES,
        .device_data =
            {
                [0] = {
                    .buffer = device1_buffer,
                    .size = DEV1_MEM_SIZE,
                    .serial_number = "ABCXYZ_1",
                    .perm = RDONLY /*RDONLY*/

                },
                [1] = {
                    .buffer = device2_buffer,
                    .size = DEV2_MEM_SIZE,
                    .serial_number = "ABCXYZ_2",
                    .perm = WRONLY /*WRONLY*/

                },
                [2] = {
                    .buffer = device3_buffer, 
                    .size = DEV3_MEM_SIZE, 
                    .serial_number = "ABCXYZ_3", 
                    .perm = RDWR /*RDWR*/

                },
                [3] = {
                    .buffer = device4_buffer, 
                    .size = DEV4_MEM_SIZE, 
                    .serial_number = "ABCXYZ_4", 
                    .perm = RDWR /*RDWR*/

                }}};


loff_t cere_lseek (struct file *filp, loff_t offset, int whence)
{
    struct device_private_data* devprv_data = (struct device_private_data*)filp->private_data;
    loff_t temp;
	pr_info("lseek requested\n");
    pr_info("current file position =%lld",filp->f_pos);
    switch (whence)
    {
    case SEEK_SET:
        if ((offset > devprv_data->size)||(offset < 0)){
            return -EINVAL;
        }
        filp->f_pos = offset;
        break;
    case SEEK_CUR:
        temp = filp->f_pos + offset;
        if ((temp > devprv_data->size)||(temp < 0))
        {
            return -EINVAL;
        }
        filp->f_pos = temp;
        break;
    case SEEK_END:
        temp = devprv_data->size + offset;
        if ((temp > devprv_data->size)||(temp < 0))
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
    struct device_private_data* devprv_data = (struct device_private_data*)filp->private_data;
    pr_info("Read requested for %zu byies\n",count);
    // Adjust the count
    if((*f_pos + count) > devprv_data->size)
    {
        count = devprv_data->size + *f_pos;
    }

    /*copy to user*/
    if(copy_to_user(buff,&devprv_data->buffer[*f_pos],count)){
        return -EFAULT;
    }

    //update the current file postion
    *f_pos += count;

	return count;
}

ssize_t cere_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    struct device_private_data* devprv_data = (struct device_private_data*)filp->private_data;
	pr_info("write requested for %zu byies\n",count);
    pr_info("Current file position = %lld\n",*f_pos);
    // Adjust the count
    if((*f_pos + count) > devprv_data->size)
    {
        count = devprv_data->size - *f_pos;
    }

    if(!count){
        return ENOMEM;
    }
    /*copy from user*/
    if(copy_from_user(&devprv_data->buffer[*f_pos],buff,count)){
        return -EFAULT;
    }

    //update the current file postion
    *f_pos += count;

	return count;
}

int check_permission(int dev_perm, int acc_mode)
{
    if(dev_perm == RDWR)
        return 0;
    /* RDONLY */
    if((dev_perm == RDONLY)&& ((acc_mode == FMODE_READ)&&!(acc_mode == FMODE_WRITE)))
        return 0;
    /* WRONLY */
    if((dev_perm == WRONLY)&& (!(acc_mode == FMODE_READ)&&(acc_mode == FMODE_WRITE)))
        return 0;
    
    return -EPERM;
    
}
int cere_open (struct inode *inode, struct file *filp)
{
    int minor_num;
    int ret;
    struct device_private_data* devprv_data;
    /*find out  which device file open was attemped by the user*/
    minor_num = MINOR(inode->i_rdev);
    pr_info("minor access = %d\n",minor_num);

    /*Get device's private data structure*/
    devprv_data = container_of(inode->i_cdev,struct device_private_data,cere_cdev);

    /*to supply device private data to other methods of the driver*/
    filp->private_data = devprv_data;

    /*check permission*/
    ret = check_permission(devprv_data->perm, filp->f_mode);
	(!ret)?pr_info("open was successful \n"):pr_info("open was unsuccessful");
	return ret;
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
        .llseek = cere_lseek,
        .release = cere_release,
        .owner = THIS_MODULE
};

// Module Entry point
static int __init cere_init(void)
{

    int ret;
    int i;
    pr_info("Hello world\n");

    // STEP1: Create device number
    ret = alloc_chrdev_region(&drv_data.device_num, 0, NO_OF_DEVICES, "cere_devices");
    if (ret < 0)
    {
        pr_err("Alloc chrdev failed\n");
        goto out;
    }

    drv_data.class_cere = class_create(THIS_MODULE, "class_cere");
    if (IS_ERR(drv_data.class_cere))
    {
        pr_err("Class creation failed\n");
        ret = PTR_ERR(drv_data.class_cere);
        goto unreg_chrdev;
    }

    for (i = 0; i < NO_OF_DEVICES; i++)
    {
        pr_info("Device number <major>:<minor> = %d:%d\n", MAJOR(drv_data.device_num + i), MINOR(drv_data.device_num + i));

        // STEP2-PART1: Initializes cdev, remembering fops,
        //              making it ready to add to the system with cdev_add.
        cdev_init(&drv_data.device_data[i].cere_cdev, &cere_fops);

        // STEP2-PART2: cdev_add adds the device represented by p to the system,
        //              making it live immediately
        drv_data.device_data[i].cere_cdev.owner = THIS_MODULE;
        ret = cdev_add(&drv_data.device_data[i].cere_cdev, drv_data.device_num + i, 1);
        if (ret < 0)
        {
            pr_err("Cdev add failed\n");
            goto cdev_del;
        }

        // STEP3-PART1: creates the device class under /sys/class/

        pr_info("sys class created\n");

        // STEP3-PART2: populates the sysfs with device information
        drv_data.device_cere = device_create(drv_data.class_cere, NULL, drv_data.device_num + i, NULL, "ceredev-%d", i + 1);

        if (IS_ERR(drv_data.device_cere))
        {
            pr_err("Device create failed\n");
            ret = PTR_ERR(drv_data.device_cere);
            goto class_del;
        }
    }
    pr_info("Cere module init success ...\n");
    return 0;
cdev_del:
class_del:
    for(;i>=0;i--){
        device_destroy(drv_data.class_cere,drv_data.device_num + i);
        cdev_del(&drv_data.device_data[i].cere_cdev);
    }
    class_destroy(drv_data.class_cere);
unreg_chrdev:
    unregister_chrdev_region(drv_data.device_num, NO_OF_DEVICES);
out:
    pr_info("Module insertion failed\n");

    return ret;
}

// Module Exit point
static void __exit cere_exit(void)
{
    int i;
    for(i=0;i<NO_OF_DEVICES;i++){
        device_destroy(drv_data.class_cere,drv_data.device_num + i);
        cdev_del(&drv_data.device_data[i].cere_cdev);
    }
    class_destroy(drv_data.class_cere);
    unregister_chrdev_region(drv_data.device_num, NO_OF_DEVICES);
    pr_info("cere module unloaded\n");
}
module_init(cere_init);
module_exit(cere_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NIKHIL");
MODULE_DESCRIPTION("This is a parctice char driver for multiple devices");
