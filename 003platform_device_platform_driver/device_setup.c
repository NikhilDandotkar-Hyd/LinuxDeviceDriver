#include<linux/module.h>
#include<linux/platform_device.h>

#include "platform.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__
void ceredev_release(void){
    pr_info("Device released\n");
}
/* Create 2 platform data*/
struct ceredev_platform_data ceredev_data[2] = {
    [0] = {.size =512, .perm = RDWR, .serial_number= "ABC111"},
    [1] = {.size =1024, .perm = RDWR, .serial_number= "XYZ222"}
};

/* Create 2 platform device */
struct platform_device platform_dev_1 = {
    .name = "p-char-device",
    .id = 0,
    .dev = { 
        .platform_data = &ceredev_data[0]
    }
};

struct platform_device platform_dev_2 = {
    .name = "p-char-device",
    .id = 1,
    .dev = { 
        .platform_data = &ceredev_data[1]
    }
};

static int __init cere_platform_init(void)
{
    platform_device_register(&platform_dev_1);
    platform_device_register(&platform_dev_2);
    pr_info("Device setup module inserted\n");
    return 0;
}

static void __exit cere_platform_exit(void)
{
    platform_device_unregister(&platform_dev_1);
    platform_device_unregister(&platform_dev_2);
    pr_info("Device setup module removed\n");
}

module_init(cere_platform_init);
module_exit(cere_platform_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module which registers n platform devices ");
MODULE_AUTHOR("Nikhil");