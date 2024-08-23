#include <linux/ctype.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>  	
#include <linux/slab.h>
#include <linux/fs.h>       		
#include <linux/errno.h>  
#include <linux/types.h> 
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/string.h>

#include "encdec.h"

#define MODULE_NAME "encdec"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SHARBEL_OBAIDA");

int 	encdec_open(struct inode *inode, struct file *filp);
int 	encdec_release(struct inode *inode, struct file *filp);
int 	encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

int memory_size = 0;

MODULE_PARM(memory_size, "i");

int major = 0;
char* caesar_cipher_buff;
char* xor_buff;

struct file_operations fops_caesar = {
	.open 	 =	encdec_open,
	.release =	encdec_release,
	.read 	 =	encdec_read_caesar,
	.write 	 =	encdec_write_caesar,
	.llseek  =	NULL,
	.ioctl 	 =	encdec_ioctl,
	.owner 	 =	THIS_MODULE
};

struct file_operations fops_xor = {
	.open 	 =	encdec_open,
	.release =	encdec_release,
	.read 	 =	encdec_read_xor,
	.write 	 =	encdec_write_xor,
	.llseek  =	NULL,
	.ioctl 	 =	encdec_ioctl,
	.owner 	 =	THIS_MODULE
};

// Implemetation suggestion:
// -------------------------
// Use this structure as your file-object's private data structure
typedef struct {
	unsigned char key;
	int read_state;
} encdec_private_date;

void caesar_encrypt(char* s, size_t amount, unsigned int key)
{   
    int i;
    int amount2 = (int)amount; /* the amout of the chars to encrypt */
    for(i = 0; i < amount2; i++) /* encrypting the required chars */
    {
        s[i] = (s[i] + key) % 128; /* caesar encryption */
    }
}

void caesar_decrypt(char* s, size_t amount, unsigned int key)
{    
    int i;
    int amount2 = (int)amount; /* the amout of the chars to encrypt */
    for(i = 0; i < amount2; i++) /* decrypting the required chars */
    {
        s[i] = ((s[i] - key) + 128) % 128; /* caesar decryption */
    }
}

void xor_encrypt_decrypt(char* s, size_t amount, unsigned int key)
{
    int i;
    int amount2 = (int)amount; /* the amout of the chars to encrypt */
    for(i = 0; i < amount2; i++) /* encrypting/decrypting the required chars */
    {
        s[i] = s[i] ^ key; /* xor encryption/decryption */
    }
}
int init_module(void)
{
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);
	if(major < 0)
	{	
		return major;
	}

	// Implemetation suggestion:
	// -------------------------
	// 1. Allocate memory for the two device buffers using kmalloc (each of them should be of size 'memory_size')
	
    caesar_cipher_buff = kmalloc(memory_size,GFP_KERNEL); /* allocating memory for the caesar buffer in the kernel */
    if(caesar_cipher_buff == NULL)
    {
        return -ENOMEM; /* no memory */
    }
    xor_buff = kmalloc(memory_size, GFP_KERNEL);  /* allocating memory for the caesar buffer in the kernel */
    if(xor_buff == NULL)
    {
        kfree(caesar_cipher_buff); /* free "caesar_cipher_buff" that has allocated before */
        return -ENOMEM;
    }
    
    memset(caesar_cipher_buff, 0, memory_size); /* set all buffers cells to zero */
    memset(xor_buff, 0, memory_size);
    
	return 0;
}

void cleanup_module(void)
{
	// Implemetation suggestion:
	// -------------------------	
	// 1. Unregister the device-driver
	// 2. Free the allocated device buffers using kfree
	unregister_chrdev(major, MODULE_NAME);
	if(caesar_cipher_buff != NULL) /* if the buffer is not NULL */
	{
	    kfree(caesar_cipher_buff); /* freeing the allocated memory from the kernel for the caesar buffer */
	}
	
	if(xor_buff != NULL) /* if the buffer is not NULL */
	{
	    kfree(xor_buff); /* freeing the allocated memory from the kernel for the caesar buffer */
	}
}

int encdec_open(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);

	// Implemetation suggestion:
	// -------------------------
	// 1. Set 'filp->f_op' to the correct file-operations structure (use the minor value to determine which)
	// 2. Allocate memory for 'filp->private_data' as needed (using kmalloc)

    encdec_private_date *pd; /* for the private data */
    if(inode == NULL || filp == NULL)
    {
        return -EINVAL;
    }
    
    if(minor == 0) /* when minor is 0 we will give f_op field to the caesar functions */
    {
        filp->f_op = &fops_caesar; 
    }
    else if(minor == 1) /* when minor is 0 we will give f_op field to the xor functions */
    {
        filp->f_op = &fops_xor;
    }
    else
    {
        return -ENODEV; /* no such device */
    }
    
    /* allocating memory for the private data */
    pd = (encdec_private_date*)kmalloc(sizeof(encdec_private_date), GFP_KERNEL);
    if(pd == NULL) /* memory allocation failed */
    {
        return -ENOMEM;
    }
    
    filp->private_data = pd; /* private_data pointing to the new allocated memory */
    pd->key = 0; /* set key = 0*/
    pd->read_state = ENCDEC_READ_STATE_DECRYPT; /* set read_state  */
    
	return 0;
}

int encdec_release(struct inode *inode, struct file *filp)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Free the allocated memory for 'filp->private_data' (using kfree)

    if(filp->private_data != NULL)
    {
        kfree(filp->private_data); /* freeing the allocated memory */
        filp->private_data = NULL; /* private_data pointing to NULL */
    }
	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Update the relevant fields in 'filp->private_data' according to the values of 'cmd' and 'arg'
    
    encdec_private_date *pd;
    if(filp == NULL || inode == NULL)
    {
        return -EINVAL;
    }
    
    pd = (encdec_private_date*)filp->private_data;
    switch(cmd)
    {
        /* set key */
        case(ENCDEC_CMD_CHANGE_KEY):
            pd->key = arg;
        break;
        /* set read_state */
        case(ENCDEC_CMD_SET_READ_STATE):
            pd->read_state = arg;
        break;
        /* set all the required buffer cells to zero */
        case(ENCDEC_CMD_ZERO):
            if(MINOR(inode->i_rdev) == 0) /* for caesar buffer */
            {
                memset(caesar_cipher_buff, 0, memory_size);
            }
            else if(MINOR(inode->i_rdev) == 1) /* for xor buffer */
            {
                memset(xor_buff, 0, memory_size);
            }
            else
            {
                return -ENODEV; /* if minor was unexpected value */
            }
        break;
    }
    
	return 0;
}

// Add implementations for:
// ------------------------
// 1. ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
// 2. ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
// 3. ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
// 4. ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos )
{
    if(*f_pos >= memory_size) /* when f_ops value geater than the memory_size */
    {
        return -EINVAL;
    }
    
    if((filp != NULL) && (filp->private_data != NULL))
    {
        if (((*f_pos) + count) > memory_size) /* the number of the required chars to read exceeds the memory_size */
        {
            count = memory_size - (*f_pos); /* reading the amount till memory_size */
        }
        
        if(count > 0) /* there are chars to read */
        {
            if((((encdec_private_date*)(filp->private_data))->read_state) == ENCDEC_READ_STATE_RAW)
            {
                if(copy_to_user(buf, caesar_cipher_buff + (*f_pos), count)) /* copying raw data to user buffer */
                {
                    return -EFAULT;
                }
            }
            else if((((encdec_private_date*)(filp->private_data))->read_state) == ENCDEC_READ_STATE_DECRYPT)
            {
                /* copying raw data to user buffer (decrypting at first and encrypting after that) */
                caesar_decrypt(caesar_cipher_buff + (*f_pos), count, ((encdec_private_date*)(filp->private_data))->key);
                if(copy_to_user(buf, caesar_cipher_buff + (*f_pos), count)) 
                {
                    caesar_encrypt(caesar_cipher_buff + (*f_pos), count, ((encdec_private_date*)(filp->private_data))->key);
                    return -EFAULT;
                }
                caesar_encrypt(caesar_cipher_buff + (*f_pos), count, ((encdec_private_date*)(filp->private_data))->key);

            }
            else
            {
                return -EINVAL; /* unexpected read_state */
            }
            (*f_pos) = (*f_pos) + count; /* moving forward the f_ops after reading */
            return count; /* the chars that we read */
        }
        
    }
    return -EINVAL;
}

ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    if(*f_pos >= memory_size) /* when f_ops value geater than the memory_size */
    {
        return -ENOSPC;
    }
    
    if((filp == NULL) || (filp->private_data == NULL)) /* filp or it's private_data is NULL */
    {
        return -EINVAL;   
    }
    
    if (((*f_pos) + count) > memory_size) /* the number of the required chars to write exceeds the memory_size */
    {
        count = memory_size - (*f_pos); /* writing the amount till memory_size */
    }

    if(copy_from_user(caesar_cipher_buff + (*f_pos), buf, count) != 0) /* copying from user to caesar buffer */
    {
        return -EFAULT;
    }
    /* encrypting the data that we copied using caesar encryption */
    caesar_encrypt(caesar_cipher_buff + (*f_pos), count, ((encdec_private_date*)(filp->private_data))->key);
    (*f_pos) = (*f_pos) + count;/* moving forward the f_ops after reading */

    return count; /* the chars that we write */
}

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos )
{
    if(*f_pos >= memory_size) /* when f_ops value geater than the memory_size */
    {
        return -EINVAL;
    }
    
    if((filp != NULL) && (filp->private_data != NULL))
    {
        if (((*f_pos) + count) > memory_size) /* the number of the required chars to read exceeds the memory_size */
        {
            count = memory_size - (*f_pos); /* reading the amount till memory_size */
        }
        
        if(count > 0) /* there are chars to read */
        {
            if((((encdec_private_date*)(filp->private_data))->read_state) == ENCDEC_READ_STATE_RAW)
            {
                if(copy_to_user(buf, xor_buff + (*f_pos), count))/* copying raw data to user buffer */
                {
                    return -EFAULT;
                }
            }
            else if((((encdec_private_date*)(filp->private_data))->read_state) == ENCDEC_READ_STATE_DECRYPT)
            {
                /* copying raw data to user buffer (decrypting at first and encrypting after that by using xor encryption) */
                xor_encrypt_decrypt(xor_buff + (*f_pos), count, ((encdec_private_date*)(filp->private_data))->key);
                if(copy_to_user(buf, xor_buff + (*f_pos), count))
                {
                    xor_encrypt_decrypt(xor_buff + (*f_pos), count, ((encdec_private_date*)(filp->private_data))->key);
                    return -EFAULT;
                }
                xor_encrypt_decrypt(xor_buff + (*f_pos), count, ((encdec_private_date*)(filp->private_data))->key);

            }
            else
            {
                return -EINVAL;
            }
            (*f_pos) = (*f_pos) + count;/* moving forward the f_ops after reading */
            return count; /* the chars that we read */
        }
        
    }
    return -EINVAL;
}

ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    if(*f_pos >= memory_size) /* when f_ops value geater than the memory_size */
    {
        return -ENOSPC;
    }
    
    if((filp == NULL) || (filp->private_data == NULL)) /* filp or it's private_data is NULL */
    {
        return -EINVAL;   
    }
    
    if (((*f_pos) + count) > memory_size) /* the number of the required chars to write exceeds the memory_size */
    {
        count = memory_size - (*f_pos); /* writing the amount till memory_size */
    }

    if(copy_from_user(xor_buff + (*f_pos), buf, count) != 0) /* copying from user to xor buffer */
    {
        return -EFAULT;
    }
    /* encrypting the data that we copied using xor encryption */
    xor_encrypt_decrypt(xor_buff + (*f_pos), count, ((encdec_private_date*)(filp->private_data))->key);
    (*f_pos) = (*f_pos) + count;
    
    return count; /* the chars that we write */
}
