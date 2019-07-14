/*
 *  默认阻塞读取的按键驱动
*/
#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <mach/irqs.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>

#define DEVICE_NAME  "x210-button"
#define LEFT_GPIO    S5PV210_GPH0(2)
#define LEFT_IRQ     IRQ_EINT2

static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

static volatile int ev_press = 0;    /* wait's condition */
static int key_value;                /* key value, to user */

static irqreturn_t button_interrupt(int irq, void *arg)
{
    s3c_gpio_cfgpin(LEFT_GPIO, S3C_GPIO_SFN(0));
    key_value = gpio_get_value(LEFT_GPIO);  /* read key value */
    s3c_gpio_cfgpin(LEFT_GPIO, S3C_GPIO_SFN(0xff));

    ev_press = 1;                           /* set wait's condition */
    wake_up_interruptible(&button_waitq);   /* wake up */

    printk("\nleft key down or up\n");

    return IRQ_HANDLED;
}

static int
x210_button_read(struct file *filp, char __user *buf, size_t count, loff_t *offp)
{
    unsigned long err;

    if (!ev_press) {
        if (filp->f_flags & O_NONBLOCK)   /* no block */
            return -EAGAIN;
        else                              /* block */
            wait_event_interruptible(button_waitq, ev_press);  /* wait */
    }

    ev_press = 0;    /* clear wait's condition */

    err = copy_to_user((void *)buf, (const void *)(&key_value),
                       min(sizeof(key_value), count));

    return err ? -EFAULT : min(sizeof(key_value), count);
}

static unsigned int
x210_button_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;

    poll_wait(filp, &button_waitq, wait);
    if (ev_press)
        mask |= POLLIN | POLLRDNORM;

    return mask;
}

static int x210_button_open(struct inode *inode, struct file *file)
{
    int error;

    /* gpio */
    error = gpio_request(LEFT_GPIO, "left key");
    if (error)
        printk("\nbutton-x210: request gpio %d fail\n", LEFT_GPIO);
    s3c_gpio_setpull(LEFT_GPIO, S3C_GPIO_PULL_UP);
    s3c_gpio_cfgpin(LEFT_GPIO, S3C_GPIO_SFN(0xff));

    /* irq */
    if (request_irq(LEFT_IRQ, button_interrupt, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "left key", NULL)) {
        printk(KERN_ERR "\nbutton.c: Can't allocate irq %d\n", LEFT_IRQ);
        goto err_request_irq;
    }

    return 0;

err_request_irq:
    gpio_free(LEFT_GPIO);
    return -EBUSY;
}

static int x210_button_close(struct inode *inode, struct file *file)
{
    free_irq(LEFT_IRQ, NULL);
    gpio_free(LEFT_GPIO);

    return 0;
}

static struct file_operations dev_fops = {
    .owner   =   THIS_MODULE,
    .open    =   x210_button_open,
    .release =   x210_button_close,
    .read    =   x210_button_read,
    .poll    =   x210_button_poll,
};

static struct miscdevice misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEVICE_NAME,
    .fops  = &dev_fops,
};

static int __init button_init(void)
{
    int ret;

    ret = misc_register(&misc);
    printk(DEVICE_NAME" init\n");
    return ret;
}

static void __exit button_exit(void)
{
    misc_deregister(&misc);
}

module_init(button_init);
module_exit(button_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ifan Tsai");
