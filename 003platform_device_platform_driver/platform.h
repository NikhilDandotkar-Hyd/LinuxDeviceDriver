#define RDONLY 0X01
#define WRONLY 0X10
#define RDWR 0X11

struct ceredev_platform_data
{
    int size;
    int perm;
    const char *serial_number;
};