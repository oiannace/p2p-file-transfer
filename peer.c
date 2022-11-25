#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>                                                                            
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <math.h>
struct content_pdu {
	char type;
	char data[1459];
};

struct PDU_reg {
	char type;
	char peer_name[10];
	char content_name[10];
	char port_number[6];
	char ip_address[4];
	//char ip_address[60];
};
struct PDU_D_S {
	char type;
	char peer_name[10];
	char content_name[10];
};

struct gen_pdu {
	char type;
	char data[100];
};

/*------------------------------------------------------------------------
 * main - UDP client for FILE DOWNLOAD service that prints the resulting time
 *------------------------------------------------------------------------
 */
int get_int_len (int value){
  int l=1;
  while(value>9){ l++; value/=10; }
  return l;
}

int main(int argc, char **argv)
{
	char buffer[5000];
	struct hostent *hp, *hp2;
	int port_length;
	char *serverHost = "0";
	char	*host = "localhost";
	int		port = 3002;
	int peer1_port = 3005;
	char	now[100];		/* 32-bit integer to hold time	*/ 
	struct 	hostent	*phe;	/* pointer to host information entry	udp socket*/
	struct 	sockaddr_in sin, client;	/* an Internet endpoint address		udp socket*/

	//struct	hostent		*hp;   //tcp socket
	struct	sockaddr_in peer1_tcp, new_peer;  //tcp socket
	struct in_addr ip_addr;
	int		s, sd, sd_read, sd_new, type, alen_tcp, alen, alen_client;	/* socket descriptor and socket type	*/
	char registered_content_list[10][10];
	int registered_content_index = 0;
	char host_name[10];
	char function[2];
	struct  PDU_D_S         search_pdu, dereg_pdu, download;
	struct gen_pdu acknowledge, listing, listing_peers;
	struct PDU_reg download_pdu;
	int	n, m, i;
	char	socket_read_buf[101];
	char 	console_read_buf[100];
	FILE*	fp;
	char	file_name[100];
			
	char other_host_name[10];
	char content_name[10];
	int user_flag = 1;
	struct 	PDU_reg 	register_pdu;


	switch (argc) {
		case 1:
			break;
		case 2:
			host = argv[1];
		case 3:
			host = argv[1];
			port = atoi(argv[2]);
			break;
		default:
			fprintf(stderr, "usage: UDP File Download [host [port]]\n");
			exit(1);
	}

	//tcp socket setup
	/* Create a stream socket*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	/* Bind an address to the socket	*/
	bzero((char *)&peer1_tcp, sizeof(struct sockaddr_in));
	peer1_tcp.sin_family = AF_INET;
	peer1_tcp.sin_port = htons(0);
	//peer1_tcp.sin_addr.s_addr = htonl(INADDR_ANY);
	if (hp2 = gethostbyname(host)) 
	  bcopy(hp2->h_addr, (char *)&peer1_tcp.sin_addr, hp2->h_length);
	else if ( inet_aton(host, (struct in_addr *) &peer1_tcp.sin_addr) ){
	  fprintf(stderr, "Can't get server's address\n");
	  exit(1);
	}	
	if (bind(sd, (struct sockaddr *)&peer1_tcp, sizeof(peer1_tcp)) == -1){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	listen(sd, 10);
	alen_tcp = sizeof(struct sockaddr_in);
	getsockname(sd, (struct sockaddr *)&peer1_tcp, &alen_tcp);	//need to send peer1_tcp.sin_addr.s_addr to index via udp


	//udp socket setup
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;                                                                
	sin.sin_port = htons(port);
                                                                                        
    /* Map host name to IP address, allowing for dotted decimal */
	if ( phe = gethostbyname(host) ){
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	}
	else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
		fprintf(stderr, "Can't get host entry \n");
                                                                                
    /* Allocate a socket */
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		fprintf(stderr, "Can't create socket \n");
                                                                    
    /* Connect the socket */
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "Can't connect to %s\n", host);
	}

	printf("Enter the name of this host:\n");
	m = read(0, host_name, 10);
	host_name[m-1] = '\0';

	//tcp select setup
	fd_set rfds, afds;
	FD_ZERO(&afds);
	FD_SET(sd, &afds);
	FD_SET(0, &afds);
	while(1){
		
		// if the last iteration of the loop was tcp file transfer, user_flag = 0 and so dont reprint the following options
		if(user_flag == 1){
			printf("What function would you like to complete?:\n");
			printf("---------------------------------------------\n");
			printf("R : Content registration\n");
			printf("D : Content download request\n");
			//printf("S : Search for content and associated server\n");
			printf("T : Content deregistration\n");
			printf("O : List registered content\n");
			printf("Q : Quit and deregister all content\n");
		}
		memcpy(&rfds, &afds, sizeof(rfds));
		select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
		
		if(FD_ISSET(0, &rfds)){
			
			memset(&register_pdu, 0, sizeof(register_pdu));
			
			alen = sizeof(sin);
	
			m = read(0, function, 2);
	

			if(function[0] == 'R')
			{
				
				printf("Enter the content name to be registered:\n");
				m = read(0, register_pdu.content_name, 10);
				if (m <= 0) {
					printf("Error. Content file name not read.\n");
					exit(1);
				}
				else if( m > 11){
					printf("Error. Content file name must be 10 bytes or less.\n");
					exit(1);
				}
		
				register_pdu.content_name[m-1] = '\0';
				FILE* fp = fopen(register_pdu.content_name, "r");
			
				if (fp == NULL) {
					//spdu.type = 'E';
					printf("Error: No File\n");
					//sendto(s, &spdu, 101, 0, (struct sockaddr *)&fsin, sizeof(fsin));
				}
				else{
					register_pdu.type = 'R';
					sprintf(register_pdu.peer_name, "%s", host_name);  //, 6);
	
					port_length = get_int_len(peer1_tcp.sin_port);
					
					sprintf(register_pdu.port_number, "%d", peer1_tcp.sin_port);
					sprintf(register_pdu.ip_address, "%d", peer1_tcp.sin_addr.s_addr);
					register_pdu.port_number[port_length] = '\0';

			
					printf("Port number: %s\n", register_pdu.port_number);
					sendto(s, &register_pdu, sizeof(register_pdu), 0, (struct sockaddr *)&sin, sizeof(sin)); 

					sprintf(registered_content_list[registered_content_index], "%s", register_pdu.content_name);
					registered_content_index++;
				}
			}
			else if(function[0] == 'S')
			{
				memset(&search_pdu, 0, sizeof(search_pdu));
				memset(&other_host_name, 0, 10);
				memset(&content_name, 0, 10);

				search_pdu.type = 'S';
		
				printf("Enter the name of the host to search the server for:\n");
				m = read(0, other_host_name, 10);
				other_host_name[m-1] = '\0';
				
				sprintf(search_pdu.peer_name, "%s", other_host_name);
		
				printf("Enter the name of the content to search the server for:\n");
				m = read(0, content_name, 10);
				content_name[m-1] = '\0';
		
				sprintf(search_pdu.content_name, "%s", content_name);

				sendto(s, &search_pdu, sizeof(search_pdu), 0, (struct sockaddr *)&sin, sizeof(sin));
		
				memset(&acknowledge, 0, sizeof(acknowledge));
				recvfrom(s, &acknowledge, sizeof(&acknowledge), 0, (struct sockaddr *)&sin, &alen);
			
				if(acknowledge.type == 'E'){
					write(1, "Content does not exist", 22);
					exit(1);
				}	
				else if(acknowledge.type == 'A'){
					write(1, "Content exists at this peer", 27);
				}
		
			}
			else if(function[0] == 'D')
			{
				memset(&search_pdu, 0, sizeof(search_pdu));
				memset(&other_host_name, 0, 10);
				memset(&content_name, 0, 10);
		
				search_pdu.type = 'D';
		
				printf("Enter the name of the host to download content from:\n");
				m = read(0, other_host_name, 10);
				other_host_name[m-1] = '\0';
				
				sprintf(search_pdu.peer_name, "%s", other_host_name);
		
				printf("Enter the name of the content to download:\n");
				m = read(0, content_name, 10);
				content_name[m-1] = '\0';
		
				sprintf(search_pdu.content_name, "%s", content_name);

				sendto(s, &search_pdu, sizeof(search_pdu), 0, (struct sockaddr *)&sin, sizeof(sin));
		
				memset(&acknowledge, 0, sizeof(acknowledge));
				recvfrom(s, &acknowledge, sizeof(acknowledge), 0, (struct sockaddr *)&sin, &alen);
		
				for(i=0; i<10; i++){
					download_pdu.peer_name[i] = acknowledge.data[i];
					download_pdu.content_name[i] = acknowledge.data[i+10];
					if(i < 6){
						download_pdu.port_number[i] = acknowledge.data[i+20];
						download_pdu.ip_address[i] = acknowledge.data[i+27];
					}
		//
				}
		
				if(acknowledge.type == 'E'){
					write(1, "Content does not exist", 22);
					exit(1);
				}
				else if(acknowledge.type == 'A'){
					write(1, "Content exists at this peer", 27);
				}
		
				//recieve from other peer via tcp


				if ((sd_read = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
					fprintf(stderr, "Can't creat a socket\n");
					exit(1);
				}
				
				bzero((char *)&new_peer, sizeof(struct sockaddr_in));
				new_peer.sin_family = AF_INET;
				new_peer.sin_port = atoi(download_pdu.port_number);

				//ip_addr.s_addr = atoi(download_pdu.ip_address);
				//new_peer.sin_addr.s_addr = inet_addr(inet_ntoa(ip_addr));
				
				if(hp = gethostbyname(host)){
					bcopy(hp->h_addr, (char *)&new_peer.sin_addr, hp->h_length);
				}
				else if(inet_aton(host, (struct in_addr *) &new_peer.sin_addr)){
					printf("error");
				}


				printf("%d\n", new_peer.sin_port );
				//printf("%d", new_peer.sin_addr.s_addr );
				
				

				if (connect(sd_read, (struct sockaddr *)&new_peer, sizeof(new_peer)) == -1){
				  	fprintf(stderr, "Can't connect \n");
				  	exit(1);
				}

				struct content_pdu content;
				memset(&content, 0, sizeof(content));
				FILE *fp;
				write(sd_read, content_name, sizeof(content_name));
				fp = fopen(content_name, "a");

				n = read(sd_read, &content, 1460);    //read the first 1459 bytes of the file from server
				fputs(content.data, fp);			//write contents to file

				
				fclose(fp);
				
				
				if(content.type == 'E'){		//compare message from server to see if theres a file not found error
					write(1,"file not found", 14);
				}
				else
				{
					while(n == 1460){			//if n == 100 then the max bytes were read, so read from server again					
						fp = fopen(content_name, "a");

						n = read(sd_read, &content, 1460);    //read the first 1459 bytes of the file from server
						fputs(content.data, fp);			//write contents to file

						fclose(fp);
						
					}
					close(sd_read);
				
				
					memset(&register_pdu, 0, sizeof(register_pdu));
					register_pdu.type = 'R';
					sprintf(register_pdu.peer_name, "%s", host_name);  //, 6);
					sprintf(register_pdu.content_name, "%s", content_name);
						
					port_length = get_int_len(peer1_tcp.sin_port);
					
					sprintf(register_pdu.port_number, "%d", peer1_tcp.sin_port);
					sprintf(register_pdu.ip_address, "%d", peer1_tcp.sin_addr.s_addr);
					register_pdu.port_number[port_length] = '\0';
						
					sendto(s, &register_pdu, sizeof(register_pdu), 0, (struct sockaddr *)&sin, sizeof(sin)); 
				}
			}
			else if(function[0] == 'O')
			{
				memset(&listing, 0, sizeof(listing));
				listing.type = 'O';
				sendto(s, &listing, sizeof(listing), 0, (struct sockaddr *)&sin, sizeof(sin)); 
				
				memset(&listing, 0, sizeof(listing));
				recvfrom(s, &listing, sizeof(listing), 0, (struct sockaddr *)&sin, &alen);
				if(listing.type != 'O'){
					printf("Data returned from index server is not the correct type");
					exit(1);
				}

				memset(&listing_peers, 0, sizeof(listing_peers));
				recvfrom(s, &listing_peers, sizeof(listing_peers), 0, (struct sockaddr *)&sin, &alen);
				if(listing_peers.type != 'O'){
					printf("Data returned from index server is not the correct type");
					exit(1);
				}

				for(i = 0; i<(sizeof(listing.data)/10); i++){
				
					printf("Name of content/host %d: %.*s, %.*s\n",i+1, (i+1)*10, (i*10) + listing.data, (i+1)*10, (i*10) + listing_peers.data);
				}		
				
				
			}
			else if(function[0] == 'T')
			{		

				memset(&dereg_pdu, 0, sizeof(dereg_pdu));
				dereg_pdu.type = 'T';
		
				sprintf(dereg_pdu.peer_name, "%s", host_name);
		
				printf("Enter the name of the content to deregister:\n");
				m = read(0, content_name, 10);
				content_name[m-1] = '\0';
		
				sprintf(dereg_pdu.content_name, "%s", content_name);

				write(1, &dereg_pdu, sizeof(dereg_pdu));
				
				sendto(s, &dereg_pdu, sizeof(dereg_pdu), 0, (struct sockaddr *)&sin, sizeof(sin)); 
			}
			else if(function[0] == 'Q')
			{
				memset(&dereg_pdu, 0, sizeof(dereg_pdu));
				dereg_pdu.type = 'T';
		
				sprintf(dereg_pdu.peer_name, "%s", host_name);

				for(i = 0; i < registered_content_index; i++){
					sprintf(dereg_pdu.content_name, "%s", registered_content_list[i]);
					sendto(s, &dereg_pdu, sizeof(dereg_pdu), 0, (struct sockaddr *)&sin, sizeof(sin)); 
				}
				exit(0);			
			}
			
			user_flag = 1;
		
		}
	
		if(FD_ISSET(sd, &rfds)){
			
			alen_client = sizeof(client);
			sd_new = accept(sd, (struct sockaddr *)&client, &alen_client);			
			if(sd_new < 0){
			    printf("accept error: %s\n", strerror(errno));
			    fprintf(stderr, "Can't accept client \n");
			    exit(1);
			  }
			  switch (fork()){
			  case 0:		/* child */
				(void) close(sd);
				char filename[30];
				char buffer[1459];
				int n;
				struct content_pdu content;
				FILE *fp;
				n = read(sd_new, filename, 30);
				filename[strcspn(filename, "\n")] = 0;
				fp = fopen(filename, "r");
				char * p = buffer;
				int j = 0;
				fgets(buffer, 5000, fp);
				content.type = 'C';
				sprintf(content.data, p+(j*1459), 1459);
				j++;
				n = write(sd_new, &content, 1460);
				
				while(n == 1460){
					sprintf(content.data, p+(j*1459), 1459);
					n = write(sd_new, &content, 1460);

				}
				fclose(fp);
				close(sd_new);
				
				exit(0);
			  default:		/* parent */
				(void) close(sd_new);
				break;
			  case -1:
				fprintf(stderr, "fork: error\n");
				
			  }
			
			user_flag = 0;
			
		}
	}
	/*while(n = read(s, socket_read_buf, 101)) {
		
		memcpy(&spdu, socket_read_buf, 101);
		
		if (spdu.type == 'C') {
			memcpy(file_name, spdu.data, n - 4);
			file_name[n - 1] = '\0';
			fp = fopen(file_name, "w");
		}
		else if (spdu.type == 'D') {
			fwrite(spdu.data, 1, n - 1, fp);
		}
		else if (spdu.type == 'F') {
			int i;
			int data_size = 0;
			for (i = 0; i < n - 1; i++) {
				if (spdu.data[i] == '\0') {
					data_size = i + 1;
					break;
				}
			}
			fwrite(spdu.data, 1, data_size - 1, fp);
			fclose(fp);
			break;
		}
		else if (spdu.type == 'E') {
			printf("Got an error\n");
			printf("Error. File Didnt Send.\n");
			exit(1);
			
		}
	}*/

	
	close(s);
	close(sd);
	exit(0);
}
