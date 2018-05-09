/* ofd.c – Our First Driver code */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
 
static int __init main_init(void) /* Constructor */
{
    printk(KERN_INFO "CDNS: Hello world\n");
    return 0;
}
 
static void __exit main_exit(void) /* Destructor */
{
    printk(KERN_INFO "CDNS: Goodbye world\n");
}
 
module_init(main_init);
module_exit(main_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Basavaraja M S <basavam_at_cadence_dot_com>");
MODULE_DESCRIPTION("Chapter 1 Driver");
