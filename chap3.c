/* chapter 2: creation of first charechter driver*/
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/pci.h>

static int  major;

struct class *cl;

#define DRV_NAME "cdns_chap2" 
#define DRV_CLS_NAME "cdns_cls_chap2" 
#define DRV_DEV_NAME "cdns_dev_name" 


static int cdns_open(struct inode *inode, struct file *filp);
static int cdns_close(struct inode *inode, struct file *filp);
static ssize_t cdns_read(struct file *filp, char __user *buff, size_t count, loff_t *offp);
static ssize_t cdns_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp);
static int cdns_ioctl(struct inode *inode, struct file *filp, unsigned int ioctl_num, unsigned long ioctl_param);

static const struct file_operations fileops = {
	.owner = THIS_MODULE,
	.open = cdns_open,
	.release = cdns_close,
	.unlocked_ioctl = cdns_ioctl,
	.read = cdns_read,
	.write = cdns_write,
};

#define PCI_VENDOR_ID_CDNS	0x17CD
#define PCI_DEVICE_ID_CDNS	0x0100

static const struct pci_device_id cdns_ids[ ] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_CDNS, PCI_DEVICE_ID_CDNS) },
};

static int cdns_pci_probe(struct pci_dev *dev, const struct pci_device_id *id);
static void cdns_pci_remove(struct pci_dev *dev);

static struct pci_driver pci_driver = {
	.name = "pci_cdns",
	.id_table = cdns_ids,
	.probe = cdns_pci_probe,
	.remove = cdns_pci_remove,
};

enum bars {bar0, bar1, bar2, bar3, bar4, bar5};

	void __iomem *bar0base;
static int cdns_pci_probe(struct pci_dev *dev, const struct pci_device_id *id){
	int retval, bars;
	u16 u16data;
	printk("CDNS ITs pobed\n");
	bars = pci_select_bars(dev, IORESOURCE_MEM);
	printk ("CDNS bars retval 0x%x\n", bars);

	if (pci_request_region(dev, bar0,"cdn_pci_driver")==0)
		printk("CDNS region requested properly\n");
	else{
		printk("CDNS coudnt allocate regions\n");
		return -1;
	}

	bar0base = pci_iomap(dev, bar0, 0); 

	pci_set_master(dev);
	
	if (!device_create(cl, NULL, MKDEV(major, 1), NULL, DRV_DEV_NAME)){
		printk("CDNS: device create failed\n");
		return -1;
	}
	retval = pci_enable_device(dev);
	if (retval < 0){
		printk("CDNS Enable pci device failed\n");
		return -1;
	}
	u16data = readw(bar0base);
	printk("CDNS u16data 0x0 - 0x%x\n", u16data);
	return 0;
}

static void cdns_pci_remove(struct pci_dev *dev){
	printk("CDNS calling pci remove\n");
	pci_iounmap(dev, bar0base);
	pci_disable_device(dev);
	pci_release_region(dev, bar0);
}
	
static int cdns_open(struct inode *inode, struct file *filp) {
	printk ("CDNS cdns_open open\n");
	return 0;

}

static int cdns_close(struct inode *inode, struct file *filp){
	printk ("CDNS cdns_close released\n");
	return 0;
}


static ssize_t cdns_read(struct file *filp, char __user *buff, size_t count, loff_t *offp) {
	printk ("CDNS Request for read\n");
	return count;
}

static ssize_t cdns_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp) {
	printk ("CDNS Request for write\n");
	return count;
}

static int cdns_ioctl(struct inode *inode, struct file *filp, unsigned int ioctl_num, unsigned long ioctl_param) {
	printk ("CDNS ioctl\n");
	return 0;
}


static int __init main_init(void) /* Constructor */
{
	printk(KERN_INFO "CDNS: Hello world\n");
	major = register_chrdev(0, DRV_NAME, &fileops);
	printk("MajorNumber 0x%x\n", major);
	if (major < 0){
		printk("CDNS: Kernel not able to allocate major number\n");
		return -1;
	}

	cl = class_create(THIS_MODULE, DRV_CLS_NAME);
	if (cl == NULL){
		printk("CDNS: Couldnt make class\n");
		return -1;
	}
	
	if (pci_register_driver(&pci_driver) < 0){
		printk("CDNS: Error in pci_rigister\n");
		return -1;
	}
	printk("CDNS PCIE registered successfully\n");



	return 0;
}
 
static void __exit main_exit(void) /* Destructor */
{
	device_destroy(cl, MKDEV(major, 1));
	pci_unregister_driver(&pci_driver);
	class_destroy(cl);
	unregister_chrdev(major, DRV_NAME);
	printk(KERN_INFO "CDNS: Goodbye world\n");
}
 
module_init(main_init);
module_exit(main_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Basavaraja M S <basavam_at_cadence_dot_com>");
MODULE_DESCRIPTION("Chapter 1 Driver");
