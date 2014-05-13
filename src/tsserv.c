#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <netinet/in.h>
#include <sys/socket.h>

// This is the Tea Steam Launcher home server, keeping track of everyone who is online! 
// 	hosted on engsoft.rutgers.edu, 128.6.238.3
int main()
{
   int sockfd,n;
   int c = 0;
   struct sockaddr_in saddr,cliaddr;
   socklen_t len;
   char mesg[256]; // messages are limited to 256 bytes.
   char users[1024][257] = {{0}}; // supporting about 1024 simultaneous users. 
   sockfd=socket(AF_INET,SOCK_DGRAM,0);
   bzero(&saddr,sizeof(saddr));
   saddr.sin_family = AF_INET;
   saddr.sin_addr.s_addr=htonl(INADDR_ANY);
   saddr.sin_port=htons(9001);
   bind(sockfd,(struct sockaddr *)&saddr,sizeof(saddr));

for(;;)
   {
      len = sizeof(cliaddr);
      // Will always check for incoming connections.
      n = recvfrom(sockfd,mesg,255,0,(struct sockaddr *)&cliaddr,&len);
      mesg[n] = 0;
      /// Prints out server information for administrative purposes.
      printf("Server Message: Received: ");
      char *cptr;
      // This checks if a user logged in wants to see the user list:
      if((cptr=strstr(mesg, "/list"))!=NULL) {
	printf("Player list request.\n");
      // Polls through the users 2D array and returns all logged in users. 
	for(c=0;c<1024;c++) {
		if(users[c][0]!='\0') 
			sendto(sockfd,users[c],sizeof(users[c]),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
	}
	// A double line break is the signal that we are done sending the player list to clients.
	sendto(sockfd,"\n\n",2,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));	
	continue;
	}
	// A single line-break from a client following their username means the user is logging out:
      if((cptr=strchr(mesg, '\n')) != NULL) {
	*cptr = '\0';
	printf("%s",mesg);
	printf(" [User logging out]\n");
	// looks for user so they can be removed from the online list.
	for(c=0;c<1024;c++) {
		if(!(strncmp(users[c],mesg,strlen(mesg)))) 
				users[c][0] = 0;		
	}
      }
	// if they're not logging out, they're logging in:
      else {
      	printf("%s",mesg);
	printf(" [User communication]\n");
	int recorded = 0;
	// We need to check if they're already logged in so they aren't recorded twice.
	for(c=0;c<1024;c++) {
		if(!(strncmp(users[c],mesg,strlen(mesg)))) {
			printf("User already recorded!\n");
			recorded = 1;
			break;
			}
		}
	if (recorded!=1) {
		// if the user isn't recorded yet, add them to the online list on a free slot
		for(c=0;c<1024;c++) {
			// slot is free if the first element of a string array isn't the null operator:
			if(users[c][0]=='\0') {
				strncpy(users[c],mesg,255);
				printf("[LOGIN] %s recorded at block %d\n",mesg,c);
				break;
				}
			}
		}
	}
   }
}
