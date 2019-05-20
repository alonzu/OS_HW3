/* we will define here two Linked Lists - the first one will be used as a list of channels inside a specified minor number/
   the other one will be used as a Linked List of the minors currently being used in the message slot.
   we also define here the two different nodes that will be used by those lists */ 

#define BUFFERSIZE 128
#define MAJOR_NUM 111 
#define MSG_SLOT_CHANNEL _IOWR(MAJOR_NUM, 0, unsigned long)
#define DEVICE_FILE_NAME "message_slot" 
#define DEVICE_RANGE_NAME "message_dev"
#include <linux/string.h>

/* this structute will be used as a channel node in the channels linked list */
typedef struct ChannelNode { 
	int channelID;
	int messageLength;
	char txtmessage[BUFFERSIZE];
	struct ChannelNode* next;
} ChannelNode;

/* this structure will be used as a list for the channels */
typedef struct Channel_List { 
	ChannelNode* ChannelHead;
	ChannelNode* ChannelCurrent;
	int size;
} Channel_List;


/* this structute will be used as a Minor node in the Minors linked list */
typedef struct MinorsNode {
	Channel_List* slot; //will hold the channels list
	int minorNumber;
	struct MinorsNode* next;
} MinorsNode;

/* this structure will be used as a list for the Minors currently being used */
typedef struct Minors_List {
	MinorsNode* MinorsHead;
	MinorsNode* MinorsCurrent;
} Minors_List;
