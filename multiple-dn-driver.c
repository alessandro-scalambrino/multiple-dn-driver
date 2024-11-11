#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h> /* macros minor - major */
#include <linux/uaccess.h> /* copy_to_user  - copy_from_user */

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt, __func__ /* modified pr_info formatting stuff */

#define NO_OF_DEVICES 4
#define MAX_MEM_SIZE_PCDEV1 1024
#define MAX_MEM_SIZE_PCDEV2 1024
#define MAX_MEM_SIZE_PCDEV3 1024
#define MAX_MEM_SIZE_PCDEV4 1024

/* Buffer for the devices */
char device_buffer_pcdev1[MAX_MEM_SIZE_PCDEV1];
char device_buffer_pcdev2[MAX_MEM_SIZE_PCDEV2];
char device_buffer_pcdev3[MAX_MEM_SIZE_PCDEV3];
char device_buffer_pcdev4[MAX_MEM_SIZE_PCDEV4];

/* Private data device structure */
struct pcdev_private_data {
    char *buffer;
    unsigned size;
    const char *serial_number;
    int perm;
    struct cdev cdev;
};

/* Private data driver structure */
struct pcdrv_private_data {
    int total_devices;
    dev_t device_number;
    struct class *class_pcd; /* Device class */
    struct device *device_pcd; /* Device structure */
    struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
};

#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR 0x11

/* Global private data */
struct pcdrv_private_data pcdrv_data = 
{
    .total_devices = NO_OF_DEVICES,
    .pcdev_data = {
        [0] = {
            .buffer = device_buffer_pcdev1,
            .size = MAX_MEM_SIZE_PCDEV1,
            .serial_number = "PCDEV1XYZ123",
            .perm = RDONLY /* readonly */
        },
        [1] = {
            .buffer = device_buffer_pcdev2,
            .size = MAX_MEM_SIZE_PCDEV2,
            .serial_number = "PCDEV2XYZ123",
            .perm = WRONLY /* write-only */
        },
        [2] = {
            .buffer = device_buffer_pcdev3,
            .size = MAX_MEM_SIZE_PCDEV3,
            .serial_number = "PCDEV3XYZ123",
            .perm = RDWR /* read-write */
        },
        [3] = {
            .buffer = device_buffer_pcdev4,
            .size = MAX_MEM_SIZE_PCDEV4,
            .serial_number = "PCDEV4XYZ123",
            .perm = RDWR /* read-write */
        }
    }
};

/* Function prototypes */
loff_t pcd_llseek(struct file *filp, loff_t offset, int whence);
ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);
int check_permission(int dev_perm, int acc_mode);
int pcd_open(struct inode *inode, struct file *filp);
int pcd_release(struct inode *inode, struct file *filp);

/* Implementation of file operations */
loff_t pcd_llseek(struct file *filp, loff_t offset, int whence) {
    struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
    int max_size = pcdev_data->size; 
    loff_t temp;
    
    pr_info("pcd_llseek requested\n");
    pr_info("Current file position: %lld\n", filp->f_pos);

    switch (whence) {
        case SEEK_SET:
            if ((offset > max_size) || (offset < 0)) return -EINVAL;
            filp->f_pos = offset;
            break;
        case SEEK_CUR:
            temp = filp->f_pos + offset;
            if ((temp > max_size) || (temp < 0)) return -EINVAL;
            filp->f_pos = temp;
            break;
        case SEEK_END:
            temp = max_size + offset;
            if ((temp > max_size) || (temp < 0)) return -EINVAL;
            filp->f_pos = temp;
            break;
        default:
            return -EINVAL;
    }

    pr_info("pcd_llseek completed\n");
    pr_info("New file position: %lld\n", filp->f_pos);
    return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos) {
    struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
    int max_size = pcdev_data->size;

    pr_info("pcd_read requested read of: %zd bytes\n", count);
    pr_info("Initial file position before read: %lld bytes\n", *f_pos);

    if (*f_pos + count > max_size) {
        count = max_size - *f_pos;  
    }

    if (copy_to_user(buff, pcdev_data->buffer + (*f_pos), count)) {
        pr_info("Error copying data to user space");
        return -EFAULT;
    }

    *f_pos += count; 
    pr_info("pcd_read: %zd bytes\n", count);
    pr_info("Updated file position after read: %lld bytes\n", *f_pos);

    return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos) {
    struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
    int max_size = pcdev_data->size;

    pr_info("pcd_write requested write of: %zd bytes\n", count);
    pr_info("Initial file position before write: %lld\n", *f_pos);

    if (*f_pos + count > max_size) {
        count = max_size - *f_pos;
    }

    if (!count) return -ENOMEM;

    if (copy_from_user(pcdev_data->buffer + (*f_pos), buff, count)) {
        pr_info("Error copying data from user space");
        return -EFAULT;
    }

    *f_pos += count;
    pr_info("pcd_write: %zd bytes\n", count);
    pr_info("Updated file position after write: %lld\n", *f_pos);

    return count;
}

int check_permission(int dev_perm, int acc_mode) {
    if (dev_perm == RDWR) return 0;
    if ((dev_perm == RDONLY) && ((acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE))) return 0;
    if ((dev_perm == WRONLY) && ((acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ))) return 0;
    return -EPERM;
}

int pcd_open(struct inode *inode, struct file *filp) {
    int ret;
    int minor_n = MINOR(inode->i_rdev);
    struct pcdev_private_data *pcdev_data;

    pr_info("Minor access: %d", minor_n);
    pr_info("pcd_open successful\n");

    pcdev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);
    filp->private_data = pcdev_data;

    ret = check_permission(pcdev_data->perm, filp->f_mode);
    if (!ret)
        pr_info("File successfully opened\n");
    else
        pr_info("File open failed\n");

    return ret;
}

int pcd_release(struct inode *inode, struct file *filp) {
    pr_info("pcd_release successful\n");
    return 0;
}

/* File operations of the driver */
struct file_operations pcd_fops = {
    .llseek = pcd_llseek,
    .read = pcd_read,
    .write = pcd_write,
    .open = pcd_open,
    .release = pcd_release,
    .owner = THIS_MODULE
};

static int __init pcd_driver_init(void) {
    int ret = 0, i = 0;

    ret = alloc_chrdev_region(&pcdrv_data.device_number, 0, NO_OF_DEVICES, "pcd_devices1");
    if (ret < 0) {
        pr_err("Cdev allocation failed\n");
        goto out;
    }

    pcdrv_data.class_pcd = class_create("pcd_class"); // Removed THIS_MODULE
    if (IS_ERR(pcdrv_data.class_pcd)) {
        pr_err("Class creation failed\n");
        ret = PTR_ERR(pcdrv_data.class_pcd);
        goto unreg_chrdev;
    }

    for (i = 0; i < NO_OF_DEVICES; i++) {
        pr_info("Device number <major>:<minor> = %d:%d\n", MAJOR(pcdrv_data.device_number + i), MINOR(pcdrv_data.device_number + i));

        cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);
        pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
        
        ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev, pcdrv_data.device_number + i, 1);
        if (ret < 0) {
            pr_err("Cdev add failed for device %d\n", i);
            goto cdev_del;
        }

        pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, pcdrv_data.device_number + i, NULL, "pcdev-%d", i);
        if (IS_ERR(pcdrv_data.device_pcd)) {
            pr_err("Device create failed\n");
            ret = PTR_ERR(pcdrv_data.device_pcd);
            goto class_del;
        }
    }

    pr_info("Module init success\n");
    return 0;

cdev_del:
    for (; i >= 0; i--) {
        device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number + i);
        cdev_del(&pcdrv_data.pcdev_data[i].cdev);
    }

class_del:
    class_destroy(pcdrv_data.class_pcd);

unreg_chrdev:
    unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);

out:
    return ret;
}

static void __exit pcd_driver_cleanup(void) {
    int i;

    for (i = 0; i < NO_OF_DEVICES; i++) {
        device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number + i);
        cdev_del(&pcdrv_data.pcdev_data[i].cdev);
    }

    class_destroy(pcdrv_data.class_pcd);
    unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);

    pr_info("Module unloaded\n");
}

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alessandro Scalambrino");
MODULE_DESCRIPTION("A Pseudo Character Driver Module for Multiple Devices");
