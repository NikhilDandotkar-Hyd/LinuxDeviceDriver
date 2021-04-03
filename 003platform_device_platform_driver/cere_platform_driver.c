#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>
#include <linux/platform_device.h>
#include<linux/slab.h>
#include<linux/mod_devicetable.h>
#include "platform.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

struct dev_prv_data 
{
    struct ceredev_platform_data pdata;
    char *buffer;
    dev_t dev_num;
    struct cdev cdev;
};

struct driver_prv_data
{
    int total_devices;
    dev_t device_num_base;
    struct class* class_cere;
    struct device* device_cere;
};

struct driver_prv_data ceredrv_data;
int check_permission(void)
{
    return 0;
}
loff_t cere_lseek (struct file *filp, loff_t offset, int whence)
{
    return 0;
}
ssize_t cere_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
    return 0;
}
ssize_t cere_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    return 0;
}
int cere_open (struct inode *inode, struct file *filp)
{
    return 0;
}
int cere_release (struct inode *inode, struct file *filp)
{
    return 0;
}

struct file_operations cere_fops =
{
    .open = cere_open,
    .write = cere_write,
    .read = cere_read,
    .llseek = cere_lseek,
    .release = cere_release,
    .owner = THIS_MODULE
};

/* called when match platform driver is called */
int cere_probe (struct platform_device * cere_platform_device)
{
    int ret;
    struct dev_prv_data *dev_data;

    struct ceredev_platform_data *pdata;
    pr_info("Device detected\n");
    /*1. Get platform data*/
    pdata = (struct ceredev_platform_data *)dev_get_platdata(&cere_platform_device->dev);
    if(!pdata)
    {
        pr_info("No platform data available\n");
        ret = -EINVAL;
        goto out;
    }

    /*2. Dynamically allocate memory for the device private data*/
    dev_data = kzalloc(sizeof(*dev_data),GFP_KERNEL);
    if (!dev_data)
    {
        pr_info("cannot allocate memory \n");
        ret = -ENOMEM;
        goto out;
    }
    dev_data->pdata.size = pdata->size;
    dev_data->pdata.perm = pdata->perm;
    dev_data->pdata.serial_number = pdata->serial_number;

    pr_info("Device serial number = %s \n",dev_data->pdata.serial_number);
    pr_info("Device size = %d \n",dev_data->pdata.size);
    pr_info("Device permission = %d \n",dev_data->pdata.perm);
    
    /*3. Dynamically alocate memory for the device buff using size information
    from the platform data*/
    dev_data->buffer = kzalloc(sizeof(dev_data->pdata.size ),GFP_KERNEL);
    if (!dev_data->buffer)
    {
        pr_info("cannot allocate memory \n");
        ret = -ENOMEM;
        goto dev_data_free;
    }
    
    /*4. Get the device number*/
    dev_data->dev_num = ceredrv_data.device_num_base + cere_platform_device->id;
    
    /*5. Do cdev init and cdev add*/
    cdev_init(&dev_data->cdev,&cere_fops);
    dev_data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&dev_data->cdev,dev_data->dev_num,1);
    if (ret<0)
    {
        pr_info("cdev add failed\n");
        goto buffer_free;
    }
    
    /*6. Create device file for detected platform device*/
    ceredrv_data.device_cere = device_create(ceredrv_data.class_cere,NULL,dev_data->dev_num,NULL,"ceredev-%d",cere_platform_device->id);
    if (IS_ERR(ceredrv_data.device_cere))
    {
        pr_err("Device create failed\n");
        ret = PTR_ERR(ceredrv_data.device_cere);
        goto cdev_del;
    }
    
    /*7. Error handling*/

    pr_info("Probe successful\n");
    return 0;
    cdev_del:
        cdev_del(&dev_data->cdev);
    buffer_free:
        kfree(dev_data->buffer);
    dev_data_free:
        kfree(dev_data);
    out:
        pr_info("probe failed\n");
        return ret;
}

/* gets called when device is removed */
int cere_remove(struct platform_device *cere_platform_device)
{
    pr_info("Device is removed\n");
    return 0;
}

struct platform_driver cere_platform_driver = 
{
    .probe = cere_probe,
    .remove = cere_remove,
    .driver = {
        .name = "p-char-device"
    }
};

#define NO_OF_DEVICES 4
static int __init cere_platform_driver_init(void)
{
    int ret;
    /*Dynamically allocate a device number for MAX DEVICES*/
    ret = alloc_chrdev_region(&ceredrv_data.device_num_base, 0, NO_OF_DEVICES, "cere_devices");
    if (ret < 0)
    {
        pr_err("Alloc chrdev failed\n");
        return ret;
    }
    /* Create device class under /sys/class*/
    ceredrv_data.class_cere = class_create(THIS_MODULE, "class_cere");
    if (IS_ERR(ceredrv_data.class_cere))
    {
        pr_err("Class creation failed\n");
        ret = PTR_ERR(ceredrv_data.class_cere);
        unregister_chrdev_region(ceredrv_data.device_num_base, NO_OF_DEVICES);
    }
    /* Register platform driver */
    platform_driver_register(&cere_platform_driver);
    pr_info("platform driver registered\n");
    return 0;
}

static void __exit cere_platform_driver_cleanup(void)
{
    /* UnRegister platform driver */
    platform_driver_unregister(&cere_platform_driver);

    /* Destroy class*/
    class_destroy(ceredrv_data.class_cere);

    /* DeAllocate the device number*/
    unregister_chrdev_region(ceredrv_data.device_num_base, NO_OF_DEVICES);
    pr_info("platform driver unregistered\n");
}

module_init(cere_platform_driver_init);
module_exit(cere_platform_driver_cleanup);

//module_platform_driver(pcd_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nikhil");
MODULE_DESCRIPTION("A pseudo character platform driver which handles n platform pcdevs");