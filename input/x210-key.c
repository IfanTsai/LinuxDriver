/*
 * 中断方式实现按键驱动
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

#define LEFT_GPIO  S5PV210_GPH0(2)
#define LEFT_IRQ   IRQ_EINT2

static struct input_dev *key_dev;

//#define USE_TASKLET  /* 中断下半部使用task */
#define USE_WORKQUEUE  /* 中断下半部使用workqueue */

/* 中断下半部 */
#ifdef USE_TASKLET
void func(unsigned long arg)
#else
void func(struct work_struct *work)
#endif
{
    int val;

    printk("\n%s: this is bottom half\n", __func__);

    s3c_gpio_cfgpin(LEFT_GPIO, S3C_GPIO_SFN(0));
    val = gpio_get_value(LEFT_GPIO);
    s3c_gpio_cfgpin(LEFT_GPIO, S3C_GPIO_SFN(0xff));

    input_report_key(key_dev, KEY_LEFT, !val);

    input_sync(key_dev);

    printk("\nleft key down or up\n");
}

#ifdef USE_TASKLET
/* 定义一个tasklet来处理中断下半部 */
DECLARE_TASKLET(key_tasklet, func, 0);
#else
/* 定义一个work来处理中断下半部 */
DECLARE_WORK(key_work, func);
#endif

/* 中断上半部 */
static irqreturn_t button_interrupt(int irq, void *arg)
{
    printk("\n%s: this is top half\n", __func__);

    #ifdef USE_TASKLET
    tasklet_schedule(&key_tasklet);
    printk("\n%s: schedule tasklet\n", __func__);
    #else
    schedule_work(&key_work);
    printk("\n%s: schedule workqueue\n", __func__);
    #endif

    return IRQ_HANDLED;
}

static int __init button_init(void)
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
        return -EBUSY;
    }

    /* input */

    key_dev = input_allocate_device();
    if (!key_dev) {
        printk(KERN_ERR "\nbutton.c: Not enough memory\n");
        error = -ENOMEM;
        goto err_input_allocate_device;
    }

    key_dev->name = "x210-key";

    set_bit(EV_KEY, key_dev->evbit);
    set_bit(KEY_LEFT, key_dev->keybit);

    error = input_register_device(key_dev);
    if (error) {
        printk(KERN_ERR "\nbutton.c: Failed to register device\n");
        goto err_register_device;
    }
    return 0;

err_register_device:
    input_free_device(key_dev);
err_input_allocate_device:
    free_irq(LEFT_IRQ, NULL);
    gpio_free(LEFT_GPIO);
    return error;
}

static void __exit button_exit(void)
{
    input_unregister_device(key_dev);
    input_free_device(key_dev);
    free_irq(LEFT_IRQ, NULL);
    gpio_free(LEFT_GPIO);
}

module_init(button_init);
module_exit(button_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ifan Tsai");
