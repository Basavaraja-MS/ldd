/* chapter 2: creation of first charechter driver*/
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>

#include "local_managment.h"

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


#include "local_managment.h"

LIST_HEAD(metrics_dev_ll);
/*
 * find device from the device linked list. Returns pointer to the
 * device if found otherwise returns NULL.
 */
static struct metrics_device_list *find_device(struct inode *inode) {
  int dev_found = 1;
  struct metrics_device_list *pmetrics_device;
	printk("CDNS invoked find device\n");
	if (is_bad_inode(inode)){
		printk("CDNS Inode is bad\n");
		return NULL;
	}else
		printk("CDNS Inode is ok!");
  /* Loop through the devices available in the metrics list */
  list_for_each_entry(pmetrics_device, &metrics_dev_ll, metrics_device_hd) {
    if (iminor(inode) == pmetrics_device->metrics_device->private_dev.minor_no) {
      return pmetrics_device;
    } else {
      dev_found = 0;
    }
  }

  /* The specified device could not be found in the list */
  if (dev_found == 0) {
    printk("CDNS Cannot find the device with minor no. %d", iminor(inode));
    return NULL;
  }
  return NULL;
}

/*
 * lock_device function will call find_device and if found device locks my
 * taking the mutex. This function returns a pointer to successfully found
 * device.
 */
struct metrics_device_list *lock_device(struct inode *inode) {
  struct  metrics_device_list *pmetrics_device;

  pmetrics_device = find_device(inode);
  if (pmetrics_device == NULL) {
    printk("CDNS Cannot find the device with minor no. %d", iminor(inode));
    return NULL;
  }

  /* Grab the Mutex for this device in the linked list */
  mutex_lock(&pmetrics_device->metrics_mtx);
  return pmetrics_device;
}

/*
 * unlock_device is called to release the associated mutex.
 */
void unlock_device(struct  metrics_device_list *pmetrics_device) {
  if (mutex_is_locked(&pmetrics_device->metrics_mtx)) {
    mutex_unlock(&pmetrics_device->metrics_mtx);
  }
}

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

static int cdns_pci_probe(struct pci_dev *dev, const struct pci_device_id *id){
	int retval, bars;
	int dev_minor = 0;
	u16 u16data;
	void __iomem *bar0base;
	struct metrics_device_list *pmetrics_device = NULL;

	printk("CDNS pobed\n");
	bars = pci_select_bars(dev, IORESOURCE_MEM);
	printk ("CDNS bars retval 0x%x\n", bars);

	if (pci_request_region(dev, bar0,"cdn_pci_driver")==0)
		printk("CDNS region requested properly\n");
	else{
		printk("CDNS coudnt allocate regions\n");
		return -1;
	}

	bar0base = pci_iomap(dev, bar0, 0); 

	pmetrics_device = kzalloc(sizeof(struct metrics_device_list),GFP_KERNEL);
	if (pmetrics_device == NULL){
		printk("CDNS Uable to allocate memory for pmetrics_device\n");
		goto cleanup0;
	}
	pmetrics_device->metrics_device = kmalloc(sizeof(struct _device), GFP_KERNEL);
	if (pmetrics_device->metrics_device == NULL){
		printk("CDNS uanble to allocate memory for pmetrics_device->metrics_device\n");
		goto cleanup1;
	}

	pmetrics_device->metrics_device->private_dev.pdev = dev;
	pmetrics_device->metrics_device->private_dev.bar0 = bar0base;
	
	mutex_init(&pmetrics_device->metrics_mtx);
 	pmetrics_device->metrics_device->private_dev.open_flag = 0;
	pmetrics_device->metrics_device->private_dev.minor_no = dev_minor;

	pci_set_master(dev);
	
	if ((pmetrics_device->metrics_device->private_dev.spcl_dev 
		= device_create(cl, NULL, MKDEV(major, dev_minor), NULL, DRV_DEV_NAME)) < 0){
		printk("CDNS: device create failed\n");
		goto cleanup1;
	}
	retval = pci_enable_device(dev);
	if (retval < 0){
		printk("CDNS Enable pci device failed\n");
		goto devicedel;
	}

	u16data = readw(bar0base);
	printk("CDNS u16data 0x0 - 0x%x\n", u16data);
	list_add_tail(&pmetrics_device->metrics_device_hd, &metrics_dev_ll);
	dev_minor++;
	return 0;

devicedel:
	device_del(pmetrics_device->metrics_device->private_dev.spcl_dev);

cleanup1:
	kfree(pmetrics_device->metrics_device);
cleanup0:
	kfree(pmetrics_device);
	return -1;
}

static void cdns_pci_remove(struct pci_dev *dev){
	struct pci_dev *pdev;
	struct metrics_device_list *pmetrics_device;
	printk("CDNS calling pci remove\n");
  /* Loop through the devices available in the metrics list */
	list_for_each_entry(pmetrics_device, &metrics_dev_ll, metrics_device_hd) {
		pdev = pmetrics_device->metrics_device->private_dev.pdev;
		if (pdev == dev) {
      			mutex_lock(&pmetrics_device->metrics_mtx);
      			pci_disable_device(pdev);
      			/* Release the selected memory regions that were reserved */
      			if (pmetrics_device->metrics_device->private_dev.bar0 != NULL) {
        		//destroy_dma_pool(pmetrics_device->metrics_device);
        			pci_iounmap(dev, pmetrics_device->metrics_device->private_dev.bar0);
        			pci_release_regions(dev);
      			}

      			/* Unlock, then destroy all mutexes */
      			mutex_unlock(&pmetrics_device->metrics_mtx);
     			mutex_destroy(&pmetrics_device->metrics_mtx);
      			//mutex_destroy(&pmetrics_device->irq_process->irq_track_mtx);

      			device_del(pmetrics_device->metrics_device->private_dev.spcl_dev);
    		}	
  	}

  /* Free up the device linked list if there are no items left */
  	if (list_empty(&metrics_dev_ll)) {
    		list_del(&metrics_dev_ll);
  	}
}
	
static int cdns_open(struct inode *inode, struct file *filp) {
	struct metrics_device_list *pmetrics_device;
	printk ("CDNS cdns_open open\n");

	pmetrics_device = lock_device(inode);
	if (pmetrics_device == NULL) {
		printk("CDNS cannot lock the device\n");
		unlock_device(pmetrics_device);
		return -ENODEV;
	}
	filp->private_data = pmetrics_device;
	if (pmetrics_device->metrics_device->private_dev.open_flag == 0) 
   		pmetrics_device->metrics_device->private_dev.open_flag = 1;
	else{
		printk("CDNS attempt to open the device multiple time \n");
		return -EPERM;
	}
	return 0;
}

static int cdns_close(struct inode *inode, struct file *filp){
	struct metrics_device_list *pmetrics_device;
	printk ("CDNS cdns_close released\n");
	pmetrics_device = lock_device(inode);
	if (pmetrics_device == NULL) {
		printk("CDNS cannot lock the device\n");
		unlock_device(pmetrics_device);
		return -ENODEV;
	}
	pmetrics_device->metrics_device->private_dev.open_flag = 0;
	return 0;
}


static ssize_t cdns_read(struct file *filp, char __user *buff, size_t count, loff_t *offp) {
	printk ("CDNS Request for read\n");
	struct metrics_device_list *pmetrics_device = filp->private_data;
	struct pci_dev *pdev = pmetrics_device->metrics_device->private_dev.pdev;
	struct _device *dev = pmetrics_device->metrics_device;
	u16 u16data;
	int indx;
	char *local_buff;
	int retval;
	u8 __iomem *bar0 = dev->private_dev.bar0;
	*bar0 += *offp;
	
	if ((count % 2) || (*offp % 2)){
		printk("CDNS Error offset and cont must be word len\n");
		return -EINVAL;
	}
	local_buff = kmalloc((count + *offp) * sizeof (u16), GFP_KERNEL | __GFP_ZERO);

	if (retval = copy_from_user(local_buff, buff, count * sizeof(u16))){
		printk("CDNS Error in copy from 0x%x\n", retval);
	}
	
	for (indx = 0; indx < count; indx++){
		u16data = readw(bar0);
		memcpy((u8 *)&local_buff[indx], &u16data, sizeof(u16));
		bar0 += 4;
		indx += 8;
	}

	if (retval = copy_to_user(local_buff, buff, count * sizeof(u16))){
		printk("CDNS Error in copy to 0x%x\n", retval);
	}
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


