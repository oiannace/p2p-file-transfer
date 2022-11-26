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

//function to ge the length of an int
int get_int_len (int value){
  int l=1;
  while(value>9){ l++; value/=10; }
  return l;
}

int main(int argc, char **argv)
{
	char buffer[5000];
	struct hostent *hp, *hp2;
	int port_length, ip_length;
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
	peer1_tcp.sin_family = AF_INET;  //IPV4
	peer1_tcp.sin_port = htons(0);   //argument 0 indicates that we want a randomized port number
	//peer1_tcp.sin_addr.s_addr = htonl(INADDR_ANY);

	//following section was used in the echo file provided in class
	//converts the host name to a format that the sockets can use
	if (hp2 = gethostbyname(host)) 
	  bcopy(hp2->h_addr, (char *)&peer1_tcp.sin_addr, hp2->h_length);
	else if ( inet_aton(host, (struct in_addr *) &peer1_tcp.sin_addr) ){
	  fprintf(stderr, "Can't get server's address\n");
	  exit(1);
	}	

	//bind the created socket to the structure with the port number, ip address, etc
	if (bind(sd, (struct sockaddr *)&peer1_tcp, sizeof(peer1_tcp)) == -1){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	
	listen(sd, 10); //listen on the tcp socket

	alen_tcp = sizeof(struct sockaddr_in);
	
	getsockname(sd, (struct sockaddr *)&peer1_tcp, &alen_tcp);	//retrieves the socket information, need to do this since we randomized port number


	//udp socket setup
	memset(&sin, 0, sizeof(sin)); //empty struct
	sin.sin_family = AF_INET;     //IPV4                                                            
	sin.sin_port = htons(port);	  //hardcoded port for udp connection to server
                                                                                        
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
                                                                    
    /* Connect the socket to the server via udp */
	//not a connection like tcp, just saves the info so instead of sendto and recvfrom can use read and write
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "Can't connect to %s\n", host);
	}

	//prompt the user to enter the name of this peer
	printf("Enter the name of this host:\n");
	m = read(0, host_name, 10);
	host_name[m-1] = '\0';  //conver the \n to a \0 which indicates end of string

	//tcp select setup
	fd_set rfds, afds;
	FD_ZERO(&afds); //initialize struct to empty
	FD_SET(sd, &afds); //set up tcp port as an option for select
	FD_SET(0, &afds); //set up standard in (command line) as an option for select
	
	
	while(1){
		
		// if the last iteration of the loop was tcp file transfer, user_flag = 0 and so dont reprint the following options
		if(user_flag == 1){
			printf("What function would you like to complete?:\n");
			printf("---------------------------------------------\n");
			printf("R : Content registration\n");
			printf("D : Content download request\n");
			printf("S : Search for content and associated server\n");
			printf("T : Content deregistration\n");
			printf("O : List registered content\n");
			printf("Q : Quit and deregister all content\n");
		}
		
		memcpy(&rfds, &afds, sizeof(rfds));
		select(FD_SETSIZE, &rfds, NULL, NULL, NULL); //wait until either connection through tcp port or standard in

		//if connection through standard in		
		if(FD_ISSET(0, &rfds)){
			
			memset(&register_pdu, 0, sizeof(register_pdu)); //initialize as empty
			
			alen = sizeof(sin); 
	
			m = read(0, function, 2); //read the input from the "What function would you like to complete?" question
	
			//registration
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
		
				register_pdu.content_name[m-1] = '\0'; //sets \n to \0
				FILE* fp = fopen(register_pdu.content_name, "r");
			
				if (fp == NULL) {
					printf("Error: No File\n");
				}
				else{
					register_pdu.type = 'R';

					sprintf(register_pdu.peer_name, "%s", host_name); //copy hostname into the pdu field peer_name
	
					port_length = get_int_len(peer1_tcp.sin_port); //get the length of the port number in digits
					//ip_length = get_int_len(peer1_tcp.sin_addr.s_addr)

					sprintf(register_pdu.port_number, "%d", peer1_tcp.sin_port); //copy the port number into the pdu field port_number
					sprintf(register_pdu.ip_address, "%d", peer1_tcp.sin_addr.s_addr); //copy the ip address into the pdu field ip_address

					register_pdu.port_number[port_length] = '\0'; //since port number could be diff lengths, set the char after the last digit to \0
					//register_pdu.ip_address[ip_length] = '\0';
			
					printf("Port number: %s\n", register_pdu.port_number);

					//send the register pdu to the index server
					sendto(s, &register_pdu, sizeof(register_pdu), 0, (struct sockaddr *)&sin, sizeof(sin)); 

					//copy the content name into a list of registered content, so when peer quits it knows all the content to deregister
					sprintf(registered_content_list[registered_content_index], "%s", register_pdu.content_name);
					registered_content_index++;
				}
			}
			//Search server for content
			else if(function[0] == 'S')
			{
				//init all to empty
				memset(&search_pdu, 0, sizeof(search_pdu));
				memset(&other_host_name, 0, 10);
				memset(&content_name, 0, 10);

				search_pdu.type = 'S'; //set type to S so server knows what to do
		
				printf("Enter the name of the host to search the server for:\n");
				m = read(0, other_host_name, 10);
				other_host_name[m-1] = '\0';
				
				sprintf(search_pdu.peer_name, "%s", other_host_name); //copy host name into search pdu field peer_name
		
				printf("Enter the name of the content to search the server for:\n");
				m = read(0, content_name, 10);
				content_name[m-1] = '\0';
		
				sprintf(search_pdu.content_name, "%s", content_name); //copy content name into search pdu field content_name

				sendto(s, &search_pdu, sizeof(search_pdu), 0, (struct sockaddr *)&sin, sizeof(sin)); //send search pdu to index server
		
				memset(&acknowledge, 0, sizeof(acknowledge)); //init recieve pdu to empty
				recvfrom(s, &acknowledge, sizeof(&acknowledge), 0, (struct sockaddr *)&sin, &alen); //recieve back info from server 
			
				//if the receive pdu has type E then content doesnt exist
				if(acknowledge.type == 'E'){
					write(1, "Content does not exist\n", 24);
				}	
				//if the recieve pdu has type A then the content does exist
				else if(acknowledge.type == 'A'){
					write(1, "Content exists at this peer\n", 31);
				}
		
			}
			//Download content from another peer
			else if(function[0] == 'D')
			{
				//init all to empty
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

				//send search pdu to server
				sendto(s, &search_pdu, sizeof(search_pdu), 0, (struct sockaddr *)&sin, sizeof(sin));
		
				memset(&acknowledge, 0, sizeof(acknowledge));

				//recieve pdu from server which will contain information of the peer you want to download content from
				recvfrom(s, &acknowledge, sizeof(acknowledge), 0, (struct sockaddr *)&sin, &alen);
		
				//determine if content exists before proceeding
				if(acknowledge.type == 'E'){
					write(1, "Content does not exist", 22);
					exit(1);
				}
				else if(acknowledge.type == 'A'){
					write(1, "Content exists at this peer", 27);
				}
		
				//since the recieve pdu has only 1 field of 100 byte size, need to iterate over each char and assign it to the appropriate fields
				for(i=0; i<10; i++){
					download_pdu.peer_name[i] = acknowledge.data[i];
					download_pdu.content_name[i] = acknowledge.data[i+10];
					if(i < 6){
						download_pdu.port_number[i] = acknowledge.data[i+20];
						download_pdu.ip_address[i] = acknowledge.data[i+27];
					}
				}
				

				//create a new tcp socket to connect with the other peer in order to download content
				if ((sd_read = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
					fprintf(stderr, "Can't creat a socket\n");
					exit(1);
				}
				
				bzero((char *)&new_peer, sizeof(struct sockaddr_in));
				new_peer.sin_family = AF_INET; //IPV4
				new_peer.sin_port = atoi(download_pdu.port_number); //atoi converts a string into an integer, assign port number recieved from server

				//ip_addr.s_addr = atoi(download_pdu.ip_address);
				//new_peer.sin_addr.s_addr = inet_addr(inet_ntoa(ip_addr));
				
				//conver host name to a format the socket can use
				if(hp = gethostbyname(host)){
					bcopy(hp->h_addr, (char *)&new_peer.sin_addr, hp->h_length);
				}
				else if(inet_aton(host, (struct in_addr *) &new_peer.sin_addr)){
					printf("error");
				}

				//connect new tcp port created to the other peer you want to download content from
				if (connect(sd_read, (struct sockaddr *)&new_peer, sizeof(new_peer)) == -1){
				  	fprintf(stderr, "Can't connect \n");
				  	exit(1);
				}

				struct content_pdu content; //content struct with 1 byte for type and 1459 bytes for data
				memset(&content, 0, sizeof(content)); //init pdu to empty
				FILE *fp; //init file pointer

				write(sd_read, content_name, sizeof(content_name)); //send name of content you want to other peer 
				

				n = read(sd_read, &content, 1460);    //read the first 1459 bytes of the file from server
				
				
				
				if(content.type == 'E'){		//compare message from server to see if theres a file not found error
					write(1,"file not found", 14);
				}
				else
				{
					fp = fopen(content_name, "a"); 	//open the file in append mode
					fputs(content.data, fp);			//write contents to file
					fclose(fp); 						//close the file pointer

					while(n == 1460){					//if n == 1460 then the max bytes were read, so read from server again					
						n = read(sd_read, &content, 1460);    //read the next 1459 bytes of the file from server

						fp = fopen(content_name, "a"); 	//open file in append mode
						fputs(content.data, fp);			//write contents to file
						fclose(fp); 					//close file pointer
					}
					
					close(sd_read); //when file transfer is complete, close the tcp socket
				
					//immediately after file download, this peer needs to register the content with the server

					//init register pdu to empty
					memset(&register_pdu, 0, sizeof(register_pdu));
					register_pdu.type = 'R';

					sprintf(register_pdu.peer_name, "%s", host_name);  		//copy host name to register pdu field peer_name
					sprintf(register_pdu.content_name, "%s", content_name); //copy content name to register pdu field content_name
						
					port_length = get_int_len(peer1_tcp.sin_port); //get length in digits of port number
					//ip_length = get_int_len(peer1_tcp.sin_addr.s_addr)

					sprintf(register_pdu.port_number, "%d", peer1_tcp.sin_port);   //copy port number into reg pdu field port_number
					sprintf(register_pdu.ip_address, "%d", peer1_tcp.sin_addr.s_addr); //copy ip addr into reg pdu field ip_address
					
					register_pdu.port_number[port_length] = '\0'; //since port number could be diff lengths, set the char after the last digit to \0
					//register_pdu.ip_address[ip_length] = '\0';

					//send register pdu to server via udp to register this new content under this peer
					sendto(s, &register_pdu, sizeof(register_pdu), 0, (struct sockaddr *)&sin, sizeof(sin)); 
				}
			}
			//List content registered on the server
			else if(function[0] == 'O')
			{
				memset(&listing, 0, sizeof(listing));
				listing.type = 'O';

				//send a pdu with the only data being the type O, which lets the server know we want to see all registered content
				sendto(s, &listing, sizeof(listing), 0, (struct sockaddr *)&sin, sizeof(sin)); 
				
				//recieving a general pdu from the server with 100 bytes of data to list the 10 possible content names
				memset(&listing, 0, sizeof(listing));
				recvfrom(s, &listing, sizeof(listing), 0, (struct sockaddr *)&sin, &alen);
				if(listing.type != 'O'){
					printf("Data returned from index server is not the correct type");
					exit(1);
				}

				//recieving a general pdu from server with 100 bytes of data to list the corresponding 10 possible peers that are hosting the content
				memset(&listing_peers, 0, sizeof(listing_peers));
				recvfrom(s, &listing_peers, sizeof(listing_peers), 0, (struct sockaddr *)&sin, &alen);
				if(listing_peers.type != 'O'){
					printf("Data returned from index server is not the correct type");
					exit(1);
				}

				//print all 10 content names with their corresponding peers
				for(i = 0; i<(sizeof(listing.data)/10); i++){
				
					printf("Name of content/host %d: %.*s, %.*s\n",i+1, (i+1)*10, (i*10) + listing.data, (i+1)*10, (i*10) + listing_peers.data);
				}		
				
				
			}
			//Deregister
			else if(function[0] == 'T')
			{		

				memset(&dereg_pdu, 0, sizeof(dereg_pdu));
				dereg_pdu.type = 'T';
		
				sprintf(dereg_pdu.peer_name, "%s", host_name);
		
				printf("Enter the name of the content to deregister:\n");
				m = read(0, content_name, 10);
				content_name[m-1] = '\0';
		
				sprintf(dereg_pdu.content_name, "%s", content_name);
				
				//send pdu to server with type T and content and host information for server to deregister
				sendto(s, &dereg_pdu, sizeof(dereg_pdu), 0, (struct sockaddr *)&sin, sizeof(sin)); 
			}
			//Quit: when a peer quits, all content they have registered must be deregistered before they can exit
			else if(function[0] == 'Q')
			{
				//so for each content in the list of content registered_content_list, send a T type pdu so the server can deregister

				memset(&dereg_pdu, 0, sizeof(dereg_pdu));
				dereg_pdu.type = 'T';
		
				sprintf(dereg_pdu.peer_name, "%s", host_name); //copy peer name into pdu because that will be the same for each deregistration pdu

				//iterate of each piece of content in the registered_content_list array
				for(i = 0; i < registered_content_index; i++){
					sprintf(dereg_pdu.content_name, "%s", registered_content_list[i]); //copy the content name into the deregister pdu field contetn_name
					sendto(s, &dereg_pdu, sizeof(dereg_pdu), 0, (struct sockaddr *)&sin, sizeof(sin)); //send a T type pdu to server to deregister the content
				}
				exit(0); //exit the application
			}
			
			user_flag = 1; //when this if statement is finished, we want the options at the top to re-print, so user_flag = 1
		
		}

		//if connection is recieved via the tcp port as opposed to standard in
		if(FD_ISSET(sd, &rfds)){
			
			alen_client = sizeof(client);
			sd_new = accept(sd, (struct sockaddr *)&client, &alen_client);	//create a new socket using the accept function		
			if(sd_new < 0){ //if sd_new < 0 then the socket wasnt created
			    printf("accept error: %s\n", strerror(errno));
			    fprintf(stderr, "Can't accept client \n");
			    exit(1);
			  }
			  switch (fork()){  //create a child process which is identical to the parent
			  case 0:		/* child */
			  	//deal with the new socket in the child process
				(void) close(sd);  //close the original socket in the child
				char filename[30];
				char buffer[1459];
				int n;
				struct content_pdu content; //pdu that this peer will send to the other with file data - 1 byte for type, 1459 bytes for data
				FILE *fp;
				
				n = read(sd_new, filename, 30); //read file name from other peer
				filename[strcspn(filename, "\n")] = 0; //set \n to 0
				fp = fopen(filename, "r");  //open file in read mode
				
				char * p = buffer; //pointer to the buffer that will read from the file
				int j = 0;
				 
				fgets(buffer, 5000, fp);  //read 5000 bytes from the file
				
				content.type = 'C';  //set type to C, meaning data pdu

				sprintf(content.data, p+(j*1459), 1459); //copy first 1459 bytes of file to content.data
				j++;
				n = write(sd_new, &content, 1460); //send first 1459 bytes to other peer
				
				while(n == 1460){  //if n == 1460 then max bytes were sent, so need to send another pdu with more data from the file
					memset(&content, 0, sizeof(content));  //reset content pdu to empty
					content.type = 'C';
					sprintf(content.data, p+(j*1459), 1459); //copy next 1459 bytes from file buffer to content.data
					n = write(sd_new, &content, 1460); //send pdu with next 1459 bytes to other peer
					j++;
				}
				fclose(fp); //close file pointer
				close(sd_new); //close this new socket
				
				exit(0); //exit child process
			  default:		/* parent */
				(void) close(sd_new); //in original process close the new socket that was created via accept() function
				break;
			  case -1:
				fprintf(stderr, "fork: error\n");
				
			  }
			
			user_flag = 0; //if this block of code has run then we dont want the options at the top to reprint since they have just been printed, so user_flag = 0
			
		}
	}

	close(s); //close udp socket
	close(sd); //close tcp socket
	exit(0); //exit process
}
