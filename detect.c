#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/kallsyms.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stewart Sentanoe & Tamas K Lengyel");
MODULE_DESCRIPTION("A simple Linux driver for detect DRAKVUF presence");
MODULE_VERSION("0.1");

int g_time_interval = 100;
struct timer_list g_timer;

void test_syscall_entry(void)
{
    uint8_t test = *(uint8_t*)kallsyms_lookup_name("do_syscall_64");

    if ( test == 0xCC )
    {
        unsigned int tmp;

        asm volatile ("cpuid"
                  : "=a" (tmp)
                  : "a" (0xdeadbeef)
                  : "bx", "cx", "dx");

        printk(KERN_INFO "DRAKVUF system call trap detected: %x\n", test);
    }
}

void test_shadow_page(void)
{
	//declare 2 pointers
	unsigned char *my_ptr1;
	unsigned char *my_ptr2;

	//need to use this function since DRAKVUF will add another memory
	//beyond the physical
	request_mem_region((unsigned long)0xff006930, 2, "BP1");

	//point the first pointer to the shadow copy address (RPA)
	my_ptr1 = (unsigned char *) ioremap((unsigned long)0xff006930,2);

	//write 'A' to it
	writeb('A', my_ptr1);

	//check if 'A' is there
	printk(KERN_INFO "BP1: %02X\n", *my_ptr1);

	//unmap
	iounmap(my_ptr1);

	//point to the second address where we expect 'A' to be there
	request_mem_region((unsigned long)0xff007930, 2, "BP2");
	my_ptr2 = (unsigned char *) ioremap((unsigned long)0xff007930, 2);

	//print the content of BP2
	printk(KERN_INFO "BP2: %02X\n", *my_ptr2);

	if(*my_ptr2 == 'A')
    {
        unsigned int tmp;

        asm volatile ("cpuid"
                  : "=a" (tmp)
                  : "a" (0xdeadbeef)
                  : "bx", "cx", "dx");

        printk(KERN_INFO "DRAKVUF shadow page detected");
    }
}

void _TimerHandler(struct timer_list *timer)
{
    test_syscall_entry();
    test_shadow_page();

    /*Restarting the timer...*/
    mod_timer( timer, jiffies + msecs_to_jiffies(g_time_interval));
}

static int __init detect_init(void){
    printk(KERN_INFO "DRAKVUF test module inserted. do_syscall_64 @ %lx\n", kallsyms_lookup_name("do_syscall_64"));

    /*Starting the timer.*/
    timer_setup(&g_timer, _TimerHandler, 0);
    mod_timer( &g_timer, jiffies + msecs_to_jiffies(g_time_interval));
    return 0;
}


static void __exit detect_exit(void){
}

module_init(detect_init);
module_exit(detect_exit);
