/* Message Sender - for sending messages from user mode */

#include <stdio.h>
#include <fcntl.h>   
#include <sys/ioctl.h>     
#include <unistd.h>    
#include <string.h>
#include <stdlib.h> 
#include "message_slot.h"

int main(int argc, char* argv[]) {
    int fp, returnValue;
    fp = open(argv[1], O_RDWR);
    if (argc < 4) { /* missing arguements */
        printf("Missing arguements! \n");
        exit(-1);
    }
    if (fp < 0) { /* error opening file */
        printf("Error! opening file: %s\n", argv[1]);
        exit(-1);
    } 
    returnValue = ioctl(fp, MSG_SLOT_CHANNEL, atoi(argv[2])); /* sets the channel id to the id specified on the command line */
    if(returnValue < 0){ /* ioctl error */
        exit(returnValue);
    }
    returnValue =  write(fp, argv[3], strlen(argv[3])); /* writes the specified message to the message slot file */
    if(returnValue < 0){
        exit(returnValue);
    }
    printf("Writing finished successfully \n");
    close(fp); 
    return 0; 
}