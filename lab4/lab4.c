#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

#define MAJOR_NUM 100
#define DEVICE_NAME "mydev"
#define MAX_QUE_LEN  500

ushort seg_for_c[27] = {
    0b1111001100010001, // A
    0b0000011100000101, // b
    0b1100111100000000, // C
    0b0000011001000101, // d
    0b1000011100000001, // E
    0b1000001100000001, // F
    0b1001111100010000, // G
    0b0011001100010001, // H
    0b1100110001000100, // I
    0b1100010001000100, // J
    0b0000000001101100, // K
    0b0000111100000000, // L
    0b0011001110100000, // M
    0b0011001110001000, // N
    0b1111111100000000, // O
    0b1000001101000001, // P
    0b0111000001010000, //q
    0b1110001100011001, //R
    0b1101110100010001, //S
    0b1100000001000100, //T
    0b0011111100000000, //U
    0b0000001100100010, //V
    0b0011001100001010, //W
    0b0000000010101010, //X
    0b0000000010100100, //Y
    0b1100110000100010, //Z
    0b0000000000000000
};

uint8_t que[16]={0};

static ssize_t my_read( struct file *fp,char *buf, size_t count, loff_t *fpos) {
    printk("call read\n");
    // printk("<debug> : fpos = %u\n", *fpos);
    // printk("<debug> : count = %u\n", count);
    
    if(copy_to_user(buf, que, count) != 0 ) return -EFAULT;

    return count;
}

static ssize_t my_write(struct file *fp ,const char *buf , size_t count, loff_t *fpos){
    printk("call write\n");
    // printk("<debug> : write (%u, %u)\n", *fpos, count);
    int i;

    if(count == 1){
        // printk("<debug> : i = %u\n", i);
        ushort tmp = 0;
        if(copy_from_user(&tmp, buf, 1) != 0) return -EFAULT;

        if(122 >= tmp && tmp >= 97){
            // printk("<write> : (%u)\n", tmp-'a');
            tmp = seg_for_c[tmp-'a'];
        }
        else if(90 >= tmp && tmp >= 65){
            // printk("<write> : (%u)\n", tmp-'A');
            tmp = seg_for_c[tmp-'A'];
        }
        else{
            // printk("<write> : else\n");
            tmp = 0;
        }
        for(i=15;i>=0;--i){
            que[15-i] = (tmp>>i & 1);
        }
    }

    return count;
}

static int my_open(struct inode *inode , struct file *fp){
    printk("call open\n");
    return 0;
}

static int my_close(struct inode *inode , struct file *fp){
    printk("call close\n");
    return 0;
}

struct file_operations my_fops = {
    owner : THIS_MODULE, 
    read : my_read ,
    write : my_write,
    open : my_open,
    release : my_close
};

static int my_init(void){
    printk("call init\n");

    if(register_chrdev(MAJOR_NUM, DEVICE_NAME, &my_fops) < 0) {
        printk ("Can not get major%d\n", MAJOR_NUM ) ;
        return (-EBUSY);
    }
    printk("My device is started and the major is %d \n", MAJOR_NUM);
    
    return 0;
}


static void my_exit(void){
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    printk("call exit\n");
}

module_init(my_init);
module_exit(my_exit);
