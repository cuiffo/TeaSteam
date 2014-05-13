#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <signal.h>

// This is the launcher for Tea Steam. The Tea Steam launcher enables a player to pick one of the two
// games offered, set a username, and see other users currently using TeaSteam.

// Truncated SA struct def
typedef struct sockaddr SA;
#define	MAXLINE	 8192
#define MAXCONN  1024

// prototypes:
void u_error(char *msg);
int Socket(int domain, int type, int protocol);
void Bind(int sockfd, struct sockaddr *my_addr, int addrlen);
ssize_t Recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t Sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
int getresp(void); // replaces getchar to be instant!

int main() {
// First check if the user has set a name yet.
	FILE *fp;
	struct sockaddr_in saddr;
	char name[256];
	int namesize = 0;
	int valid = 1;
	int sockfd;
	// The following operations check for a username and ensures it is a safe username (no overflows)
	fp = fopen("username", "r");
	if (fp) {
		fseek(fp, 0L, SEEK_END);
		namesize = ftell(fp);
		rewind(fp);
		fgets(name, 255, fp);
		fclose(fp);
	}
	// If the file for username has an error/is empty/or does not yet exist, create it:
	if (namesize>255 || namesize<=0) {
		pid_t pid=fork();
		if (pid==0) {
		// in case the file is corrupted, we'll have to delete it!
		// 	here is a SAFE execl deletion line, that executes as a fork
			execl("/bin/rm", "rm", "-f", "username", NULL);
		} else 
		{ 
			waitpid(pid,0,0);
		}
		fp = fopen("username", "w");
		while (valid == 1) {
		fprintf(stdout, "Username not found! Please type your username and hit <Enter>: ");
			fgets (name, 255, stdin);
		if ( name[0]=='\n' || name[0]=='/' )
			fprintf(stdout, "Not a valid name!");
		else
			valid = 0;	
		}
		fputs (name, fp);
		fclose(fp);
	}
	// fgets puts a newline, this very quietly deletes it!
	// We can now welcome our user!
	// We've welcomed the user, now we can start the launcher!
	while(1) {
        char *pos;
	// take out a newline if it was accidentally appended
        if ((pos=strchr(name, '\n')) != NULL)
                *pos = '\0';
	// All communications utilize UDP to keep data organized.
	sockfd = Socket(AF_INET, SOCK_DGRAM,0);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	// Tea Steam Launcher communicates with engsoft.rutgers.edu:
	saddr.sin_addr.s_addr=inet_addr("162.248.161.124");
	saddr.sin_port=htons(9001);
	Sendto(sockfd, name, strlen(name), 0, (struct sockaddr *) &saddr, sizeof(saddr));
	// Flush the terminal so everything is display nicely:	
	printf("\033[2J\033[1;1H");
	rewind(stdout);
	ftruncate(1,0);
	// Next line is the banner, it'll look weird here though!
	fprintf(stdout, "____         __              \n /    _  _  (   _/  _  _  _ _ \n(    (- (/ __)  /  (- (/ ////");
	fprintf(stdout, "\n\nWelcome %s!\n\n", name);  
	fprintf(stdout, "Tea Steam launcher Version 1.00, by Eric Cuiffo, Jeff Rabinowitz, and Val Red. \n Last update: 12 May 2014. http://himede.re/TeaSteam\n Developed by the Scarlet Shield team. Bugs, feedback, and grievances can be addressed to scarlet.shield@rutgers.edu\n\n Please type one of the following numbers for the respective option or game:\n\n 1: Connect Four\n 2: Speed Typer\n 3: Change Name \n 4: List Players Online\n\n 9: Quit\n");
	// The above is a welcome message. Everything below is the main user control interface:
	int control = 0;
	control = getresp();
	// We use switch cases for the console control UI for the launcher:
	switch (control) {
	case 49:
		/* forks into a Connect 4 client */
		fprintf(stdout, "Loading Connect 4...\n\n");
                pid_t pid1=fork();
                if (pid1==0) {
                // in case the file is corrupted, we'll have to delete it!
                //      here is a SAFE execl deletion line, that executes as a fork
                        execl("connectFour", "connectFour", NULL);
                } else
                {
                        waitpid(pid1,0,0);
			fprintf(stdout, "Completed! Press any key to continue.\n\n");
			continue;
                }
	case 50:
		/* forks into a Speed Typer client */
		fprintf(stdout, "Loading Speed Typer...\n\n");
                pid_t pid2=fork();
                if (pid2==0) {
                // in case the file is corrupted, we'll have to delete it!
                //      here is a SAFE execl deletion line, that executes as a fork
                        execl("speedTyper", "speedTyper", NULL);
                } else
                {
                        waitpid(pid2,0,0);
			fprintf(stdout, "Completed! Press any key to continue.\n\n");
                        continue;
                }
	case 51:
		/* allows users to change their name */
		valid = 1;
		// technically log-out the user due to name change.
                if ((pos=strchr(name, '\0')) != NULL) {
                // server will know user intends to log out via newline!
                        *pos = '\n';
                        pos++;
                        *pos = '\0';
                }
		Sendto(sockfd, name, strlen(name), 0, (struct sockaddr *) &saddr, sizeof(saddr));
		fprintf(stdout,"WARNING: This will associate leaderboard rankings to another player name as well.\n ");
                fp = fopen("username", "w");
		while (valid == 1) {
                fprintf(stdout, "Please type your username and hit <Enter>: ");
                        fgets (name, 255, stdin);
                if ( name[0]=='\n' || name[0]=='/' )
                        fprintf(stdout, "Not a valid name!");
                else
                        valid = 0;
                }
                fputs (name, fp);
                fclose(fp);
		continue;
	case 52:
		/* Tis option sees who is online from the server's perspective! */
       	 	printf("\033[2J\033[1;1H");
        	rewind(stdout);
        	ftruncate(1,0);
	        Sendto(sockfd, "/list\0", 6, 0, (struct sockaddr *) &saddr, sizeof(saddr));
		fprintf(stdout, "Player list : \n\n");
		char listbuf[1024] = {0};
		while((strncmp(listbuf,"\n\n",2))) {
			int y = 0;
			// first we need to ensure the buffer is cleared.
			for(y = 0; listbuf[y]!='\0'; y++) 
				listbuf[y]='\0';
			recvfrom(sockfd,listbuf,255,0,NULL,NULL);
			char clean[255] = {0};
			strncpy(clean,listbuf,strlen(listbuf));
			fprintf(stdout, " %s \n", clean);
		}
		fprintf(stdout, "\nPress any key to continue... \n");
		getresp();	
		continue;

	case 57:
		/* quits the launcher */
		fprintf(stdout, "Quitting! Thanks for playing!\n");
       		if ((pos=strchr(name, '\0')) != NULL) {
		// server will know user intends to log out via newline!
                	*pos = '\n';
			pos++;
			*pos = '\0';
		}
		Sendto(sockfd, name, strlen(name), 0, (struct sockaddr *) &saddr, sizeof(saddr));
		break;
	default:
		fprintf(stdout, "Hey! That's not a legal command!\n\n");
		continue;
	}
	return 0;
	}
}

void u_error(char *msg) /* unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(1);
}
//

// Functions for sockets with error checking

int Socket(int domain, int type, int protocol) 
{
    int rc;

    if ((rc = socket(domain, type, protocol)) < 0)
		u_error("[ERROR] Socket error");
    return rc;
}

void Bind(int sockfd, struct sockaddr *my_addr, int addrlen) 
{
    int rc;

    if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
	u_error("[ERROR] Bind error");
}

ssize_t Recvfrom(int sockfd, void *buf, size_t len, int flags,
				struct sockaddr *src_addr, socklen_t *addrlen) {
	ssize_t rc;
	if ((rc = recvfrom(sockfd, buf, len, flags, src_addr, addrlen)) < 0)
		u_error("[ERROR] Recvfrom error");
	return rc;

}
ssize_t Sendto(int sockfd, const void *buf, size_t len, int flags, 
				const struct sockaddr *dest_addr, socklen_t addrlen) {
	ssize_t rc;
	if ((rc = sendto(sockfd, buf, len, flags, dest_addr, addrlen)) < 0)
		u_error("[ERROR] Sendto error");
	return rc;
}


int getresp(void) {
        int v;
        struct termios vold;
        struct termios vnew;
        tcgetattr(STDIN_FILENO, &vold); /*old term*/
        vnew = vold;
        vnew.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO,TCSANOW, &vnew);
        v = getchar(); // This is our replaced getchar with the above settings
        tcsetattr(STDIN_FILENO, TCSANOW, &vold); // return to old getchar;
        return v;
}
