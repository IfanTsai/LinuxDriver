#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define E2PROM_MAJOR 100

struct e2prom_device {
    struct i2c_client *at24c02_client;   /* I2C client(从设备) */
    /* class和device用来自动创建设备节点 */
    struct class      *at24c02_class;
    struct device     *at24c02_device;
};

struct e2prom_device *e2prom_dev;

static int i2c_read_byte(char *buf, int count)
{
    int ret = 0;
    struct i2c_msg msg;

    msg.addr   = e2prom_dev->at24c02_client->addr; /* I2C从设备地址 */
    msg.flags |= I2C_M_RD;                         /* read flag */
    msg.len    = count;                            /* 数据长度 */
    msg.buf    = buf;                              /* 读取数据放置的buf */

    /* 调用I2C核心层提供的传输函数，其本质还是调用的I2C总线驱动(主机控制器驱动)层下实现的algo->master_xfe方法 */
    ret = i2c_transfer(e2prom_dev->at24c02_client->adapter, &msg, 1);
    if (ret < 0) {
        printk(KERN_ERR "i2c transfer failed!\n");
        return -EINVAL;
    }
    return ret;
}

static int i2c_write_byte(char *buf, int count)
{
    int ret = 0;
    struct i2c_msg msg;

    /* 封装I2C数据包 */
    msg.addr   = e2prom_dev->at24c02_client->addr; /* I2C从设备地址 */
    msg.flags  = 0;                                /* write flag */
    msg.len    = count;                            /* 数据长度 */
    msg.buf    = buf;                              /* 写入的数据 */

    /* 调用I2C核心层提供的传输函数，其本质还是调用的I2C总线驱动(主机控制器驱动)层下实现的algo->master_xfe方法 */
    ret = i2c_transfer(e2prom_dev->at24c02_client->adapter, &msg, 1);
    if (ret < 0) {
        printk(KERN_ERR "i2c transfer failed!\n");
        return -EINVAL;
    }
    return ret;
}

static int e2prom_open(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t e2prom_read(struct file *file, char __user *buf, size_t size,
        loff_t *offset)
{
    int ret = 0;
    char *tmp = kmalloc(size, GFP_KERNEL);

    if (tmp == NULL) {
        printk(KERN_ERR "mallo failed!\n");
        return -ENOMEM;
    }

    /* I2C read */
    ret = i2c_read_byte(tmp, size);
    if (ret < 0) {
        printk(KERN_ERR "wrtie byte failed!\n");
        ret = -EINVAL;
        goto err0;
    }

    /* 将内核空间数据拷贝到用户空间 */
    ret = copy_to_user(buf, tmp, size);
    if (ret) {
        printk("copy data faile!\n");
        ret = -EFAULT;
        goto err0;
    }
    kfree(tmp);
    return size;

err0:
    kfree(tmp);
    return ret;
}

static ssize_t e2prom_write(struct file *file, const char __user *buf,
        size_t size, loff_t *offset)
{
    int ret = 0;
    char *tmp;
    tmp = kmalloc(size, GFP_KERNEL);
    if (tmp == NULL) {
        printk(KERN_ERR "mallo failed!\n");
        return -ENOMEM;
    }

    /* 将用户空间数据拷贝到内核空间 */
    ret = copy_from_user(tmp, buf, size);
    if (ret) {
        printk("copy data faile!\n");
        goto err0;
    }

    /* I2C write */
    ret = i2c_write_byte(tmp, size);
    if (ret) {
        printk(KERN_ERR "wrtie byte failed!\n");
        goto err0;
    }

    kfree(tmp);
    return size;

err0:
    kfree(tmp);
    return -EINVAL;
}

struct file_operations e2prom_fops = {
    .owner = THIS_MODULE,
    .open  = e2prom_open,
    .write = e2prom_write,
    .read =  e2prom_read,
};

static int e2prom_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;

    printk(KERN_INFO "e2prom probe!\n");
    e2prom_dev = kmalloc(sizeof(struct e2prom_device), GFP_KERNEL);
    if (!e2prom_dev) {
        printk(KERN_ERR "malloc failed!\n");
        return -ENOMEM;
    }

    e2prom_dev->at24c02_client = client;

    /* 注册为字符设备驱动 */
    ret = register_chrdev(E2PROM_MAJOR, "e2prom_module", &e2prom_fops);
    if (ret < 0) {
        printk(KERN_ERR "malloc failed\n");
        ret = -ENOMEM;
        goto err0;
    }

    /* 创建类  */
    e2prom_dev->at24c02_class = class_create(THIS_MODULE, "e2prom_class");
    if (IS_ERR(e2prom_dev->at24c02_class)) {
        printk(KERN_ERR "class create failed!\n");
        ret = PTR_ERR(e2prom_dev->at24c02_class);
        goto err1;
    }

    /* 在类下创建设备 */
    e2prom_dev->at24c02_device = device_create(e2prom_dev->at24c02_class, NULL, MKDEV(E2PROM_MAJOR, 0), NULL, "at24c08");
    if (IS_ERR(e2prom_dev->at24c02_device)) {
        printk(KERN_ERR "class create failed!\n");
        ret = PTR_ERR(e2prom_dev->at24c02_device);
        goto err1;
    }

    return 0;
err1:
    unregister_chrdev(E2PROM_MAJOR, "e2prom_module");
err0:
    kfree(e2prom_dev);
    return ret;
}

static int e2prom_remove(struct i2c_client *client)
{
    unregister_chrdev(E2PROM_MAJOR, "e2prom_module");
    device_destroy(e2prom_dev->at24c02_class, MKDEV(E2PROM_MAJOR, 0));
    class_destroy(e2prom_dev->at24c02_class);
    kfree(e2prom_dev);
    return 0;
}

struct i2c_device_id e2prom_table[] = {
    [0] = {
        .name         = "24c02",
        .driver_data  = 0,
    },
    [1] = {
        .name         = "24c08",
        .driver_data  = 0,
    },
};

/* I2C设备驱动 */
struct i2c_driver e2prom_driver = {
    .probe     =  e2prom_probe,
    .remove    =  e2prom_remove,
    .id_table  =  e2prom_table,
    .driver    = {
        .name = "e2prom",
    },
};

static int __init e2prom_init(void)
{
    return i2c_add_driver(&e2prom_driver);   /* 注册I2C设备驱动 */
}

static void __exit e2prom_exit(void)
{
    i2c_del_driver(&e2prom_driver);
}

module_init(e2prom_init);
module_exit(e2prom_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ifan Tsai <i@caiyifan.cn>");
MODULE_DESCRIPTION("at24c02 driver for s5pv210");