#include <linux/module.h>
#include <linux/kernel.h>

int init_module(void)
{
    printk(KERN_INFO "Maryam Jafarabadi Ashtiani, Sana Sari Navaei, Bita Nasiri\n");
    return 0;
}

void cleanup_module(void)
{
    ;
}
