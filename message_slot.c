/* we will define here the characteristics of our message slot device */
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include "message_slot.h"
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>   
#include <linux/fs.h>
#include <linux/module.h> 
#include <linux/uaccess.h> /* get_user & put_user */
#include <linux/string.h>

MODULE_LICENSE("GPL");

static Channel_List* ChannelList;
static Minors_List* MinorsList;

/* updates the pointer to the relevant minor */
static int minorsearcher(int minor) {
	MinorsList->MinorsCurrent = MinorsList->MinorsHead;
	/* start searching relevant minor */
	while (MinorsList->MinorsCurrent->next != NULL) {
		if (MinorsList->MinorsCurrent->minorNumber == minor) {
			break;
		} else {
			MinorsList->MinorsCurrent = MinorsList->MinorsCurrent->next;
		}
	}
	/* not in the Minors List */
	if (MinorsList->MinorsCurrent->minorNumber != minor) {
		return -EINVAL;
	}
	/* empty list */
	if (MinorsList->MinorsCurrent->slot == NULL) {
		return -EINVAL;
	}
	/* search the channel */
	ChannelList = MinorsList->MinorsCurrent->slot;
	return 0;
}
/* updates the pointer to the relevant channel */
static int channelsearcher (int channel) {
	if (ChannelList == NULL) {
		return -EINVAL;
	}
	if (ChannelList->ChannelHead == NULL) {
		return -EINVAL;
	}
	ChannelList->ChannelCurrent = ChannelList->ChannelHead;
	while (ChannelList->ChannelCurrent->next != NULL) {
		if (ChannelList->ChannelCurrent->channelID == channel) {
			break;
		} else {
			ChannelList->ChannelCurrent = ChannelList->ChannelCurrent->next;
		}
	}
	/* not in the Channels List */
	if (ChannelList->ChannelCurrent->channelID != channel) {
		return -EINVAL;
	}
	return 0;
}
	
/* called when a process tries to open the device file */
static int device_open (struct inode *inode, struct file *file) {
	int minor;
	MinorsNode* node;
	MinorsNode* newNode;
	minor = iminor(inode);
	MinorsList->MinorsCurrent = MinorsList->MinorsHead;
	node = MinorsList->MinorsHead;
	/* we will search for the current minor in the minor's list */
	while (MinorsList->MinorsCurrent->next != NULL) { 
		if (MinorsList->MinorsCurrent->minorNumber == minor) {
			break;
		} else {
			MinorsList->MinorsCurrent = MinorsList->MinorsCurrent->next;
		}
	}
	/* If we didnt find the minor in the minor's list, 
	we will create a new node for that minor and add it to the list */
	if (MinorsList->MinorsCurrent->minorNumber != minor) {
		newNode = (MinorsNode*) kmalloc(sizeof(MinorsNode), GFP_KERNEL);
		if (newNode == NULL) {
			return -EINVAL;
		}
		MinorsList->MinorsCurrent->next = newNode; // we add the new node (connecting it to the list)
		MinorsList->MinorsCurrent = newNode; // and move the current pointer forward
		/* we will now "restart" the new node */
		MinorsList->MinorsCurrent->slot = (Channel_List*) kmalloc(sizeof(Channel_List),GFP_KERNEL);
		if (MinorsList->MinorsCurrent->slot == NULL) {
			return -EINVAL;
		}
		MinorsList->MinorsCurrent->slot->ChannelHead = (ChannelNode* ) kmalloc(sizeof(ChannelNode),GFP_KERNEL );
        if (MinorsList->MinorsCurrent->slot->ChannelHead == NULL) {
            return -EINVAL;
		}
		MinorsList->MinorsCurrent->slot->ChannelCurrent = MinorsList->MinorsCurrent->slot->ChannelHead;
		MinorsList->MinorsCurrent->slot->size = 0;
		MinorsList->MinorsCurrent->minorNumber = minor;
		MinorsList->MinorsCurrent->next = NULL;
		}
		/* so we found the relrvant node or created it. we will put it in channelList */
		ChannelList = MinorsList->MinorsCurrent->slot;
		return 0;
	}

/* Called when a process closes the devive file */
static int device_release (struct inode *inode, struct file *file) {
	return 0;
}

/* Called when a process writes to device file */
static ssize_t device_write (struct file* file, const char __user* buffer, size_t length, loff_t* offset) {
    int i;
    int channel, minor;
    channel = (int)file->private_data;
    minor = iminor(file_inode(file));
    if (minorsearcher(minor) < 0) {
    	return -EINVAL;
    }
    if (channelsearcher(channel) < 0) {
    	return -EINVAL;
    }
    if (MinorsList->MinorsCurrent->slot->size == 0) {
    	return -EINVAL;
    }
    /* channel list is empty = no channel has been set */
    if (ChannelList -> size == 0){
        return -EINVAL;
    }
    /* inappropirate length value */
    if (length == 0 || length > BUFFERSIZE){
        return -EMSGSIZE;
    }
    strcpy(ChannelList->ChannelCurrent->txtmessage,""); /* we overwrite the current message */
    for(i = 0;i < length; i++) {
        get_user(ChannelList->ChannelCurrent->txtmessage[i], &buffer[i]); /*we copy from buffer in user space to our message*/
    }
    ChannelList->ChannelCurrent->messageLength = i; /* update the length of the message */
	return i;
}

/* Called when a process, which has been opened by the device file. attempts to read from it */
static ssize_t device_read(struct file* file, char __user* buffer,size_t length, loff_t* offset ) {
    int i, messageLength;
    char* message; 
    int channel, minor;
    channel = (int)file->private_data;
    minor = iminor(file_inode(file));
    if (minorsearcher(minor) < 0) {
    	return -EINVAL;
    }
    if (channelsearcher(channel) < 0) {
    	return -EINVAL;
    }
    if (MinorsList->MinorsCurrent->slot->size == 0) {
    	return -EINVAL;
    }
    /* channel list is empty = no channel has been set */
    if (ChannelList -> size == 0){
        return -EINVAL;
    }
    messageLength = ChannelList->ChannelCurrent->messageLength;
    message = ChannelList->ChannelCurrent->txtmessage; 
    /* the message is empty */
    if (messageLength == 0){
        return -EWOULDBLOCK;
    }
    /* buffer length is too small */
    if (length < ChannelList->ChannelCurrent->messageLength){ 
        return -ENOSPC;
    }
    for(i = 0; i < messageLength;i++) {
       put_user(ChannelList->ChannelCurrent->txtmessage[i], &buffer[i]); 
    }
    return i;
}

/* Called when a process tries to do an ioctl on our device file */
static long device_ioctl(struct file* file, unsigned int ioctlCommandId, unsigned long channelId) {
    ChannelNode* node;
    ChannelNode* newNode;
    int minor;
    minor = iminor(file_inode(file));
    if (minorsearcher(minor) < 0) {
    	return -EINVAL;
    }
    if (channelId == 0 || (MSG_SLOT_CHANNEL != ioctlCommandId)) { /* the passed command is not MSG_SLOT_CHANNEL or wrong number */
        return -EINVAL;
    }
    /* taking care of the first channel in minors list */
    if (ChannelList->size == 0){
        ChannelList->ChannelHead->channelID = (int) channelId;
        ChannelList->ChannelHead->next = NULL;
        ChannelList->ChannelHead->messageLength = 0;
        ChannelList->size++;
        file->private_data = (void * )channelId;
        return 0;
    }
    /* we are not taking care of the first channel */
    /* we stay on the same channel = no need to do anything */
    if (ChannelList->ChannelCurrent->channelID == channelId){
    	file->private_data = (void * )channelId;
        return 0;
    }
    node = ChannelList->ChannelHead;
    /* searching for channelId */
    while (node->next != NULL){
        if (node->channelID == channelId) {
            break; /* we found the channel node */
        } else {
        	node = node->next;
      	}
    }
    /* we found the channel in the channels list so we update the pointer */
    if (node->channelID == channelId) {
        ChannelList->ChannelCurrent = node;
        file->private_data = (void * )channelId;
        return 0;
    }
    /* we didnt find the channel in the channels list so we need to create it */
    if(node->channelID != channelId) {
        newNode = (ChannelNode *) kmalloc(sizeof(ChannelNode), GFP_KERNEL);
        if (newNode == NULL){
            return -EINVAL;
        }
        newNode->channelID = channelId;
        newNode->messageLength = 0;
        newNode->next = NULL;
        node->next = newNode;
        ChannelList->ChannelCurrent = newNode;
        ChannelList->size++;
    }
    file->private_data = (void * )channelId;
    return 0;
}


//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
        {
                .read           = device_read,
                .write          = device_write,
                .open           = device_open,
                .unlocked_ioctl = device_ioctl,
                .release        = device_release,
        };

//---------------------------------------------------------------

static int __init simple_init(void) {
	int status;
	/* we will register our device with major_num as its major number */
    status = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);
    if (status < 0) {
        printk(KERN_ERR "%s registraion failed for  %d\n", DEVICE_FILE_NAME, MAJOR_NUM );
        return status;
    }
    MinorsList = (Minors_List*) kmalloc(sizeof(Minors_List),GFP_KERNEL);
    if(MinorsList == NULL) {
        return -EINVAL;
    }
    MinorsList->MinorsHead = (MinorsNode*) kmalloc (sizeof(MinorsNode),GFP_KERNEL);
	if(MinorsList->MinorsHead == NULL) {
        return -EINVAL;
    }
	MinorsList->MinorsHead->next = NULL;
	MinorsList->MinorsHead->slot = NULL;
	MinorsList->MinorsHead->minorNumber = -1;
	MinorsList->MinorsCurrent = MinorsList->MinorsHead;

    printk(KERN_INFO "message_slot: registered major number %d\n", MAJOR_NUM);
    return 0;
}

/* frees the channel list */
static int freeChannelList (ChannelNode* channel){
    if(channel == NULL){
        return 0;
    }
    freeChannelList(channel->next);
    kfree(channel);
    return 0;
}

/* frees the minors list */
static int freeMinorsList (MinorsNode* MinorsNode){
    if(MinorsNode == NULL){
        return 0;
    }
    freeMinorsList(MinorsNode->next);
    if (MinorsNode->slot != NULL) {
        freeChannelList(MinorsNode->slot->ChannelHead);
    }
    kfree(MinorsNode);
    return 0;
}


static void __exit simple_cleanup(void) {
	if (MinorsList != NULL) {
		if (MinorsList->MinorsHead != NULL) {
			freeMinorsList(MinorsList->MinorsHead);
		}
	}
	kfree(MinorsList);
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
    printk(KERN_INFO "message_slot: free finished successfully %d\n", MAJOR_NUM);

}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

