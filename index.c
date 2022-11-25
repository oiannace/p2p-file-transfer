#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

struct content_pdu {
	char type;
	char data[1459];
};

struct rec_pdu {
	char type;
	char data[100];
};

struct PDU_reg {
	char type;
	char peer_name[10];
	char content_name[10];
	char port_number[6];
	char ip_address[4];	
//	char ip_address[60];
};

struct PDU_D_S {
	char type;
	char peer_name[10];
	char content_name[10];
};


/*------------------------------------------------------------------------
 * main - Iterative UDP server for FILE DOWNLOAD service
 *------------------------------------------------------------------------
 */
int main(int argc, char *argv[])
{
	struct  sockaddr_in fsin;	/* the from address of a client	*/
	char	buf[100];		/* "input" buffer; any size > 0	*/
	char    *pts;
	int		sock;			/* server socket		*/
	time_t	now;			/* current time			*/
	int		alen;			/* from-address length		*/
	struct  sockaddr_in sin; /* an Internet endpoint address         */
	int     s,j, type;        /* socket descriptor and socket type    */
	int 	port=3002;
	struct rec_pdu recieve_pdu, listing_pdu;
	struct PDU_reg reg_recieve_pdu;
	struct PDU_reg content_list[10];
	int content_list_index = 0;
	int i;
	
	switch(argc){
		case 1:
			break;
		case 2:
			port = atoi(argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
                                                                                                 
    /* Allocate a socket */
	s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
		fprintf(stderr, "can't creat socket\n");
	}
	
    /* Bind the socket */
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "can't bind to %d port\n",port);
	}
	listen(s, 5);	
	alen = sizeof(fsin);


	struct 	PDU_D_S search_pdu, s_recieve_pdu, dereg_pdu;
	int 	n;
	char	socket_buf[101];
	char	file_name[100];
	int	    file_size;	
	struct PDU_reg match;
	
	//infinite loop
	while (1) {
		
		//memset(&socket_buf, 0, sizeof(socket_buf));
		if (recvfrom(s, &recieve_pdu, sizeof(recieve_pdu), 0, 
				(struct sockaddr *)&fsin, &alen) < 0) {
			printf("recvfrom error\n");
		}
		
		//memcpy(&spdu, socket_buf, sizeof(socket_buf));
		//printf("%d", content_list[content_list_index].port_number);
		if(recieve_pdu.type == 'R'){
			memset(&content_list[content_list_index], '\0', sizeof(content_list[content_list_index]));			
			//memset(&reg_recieve_pdu, '\0', sizeof(reg_recieve_pdu));
			//memcpy(&reg_recieve_pdu, socket_buf, sizeof(socket_buf));
			
			for(i=0; i<10; i++){
				content_list[content_list_index].peer_name[i] = recieve_pdu.data[i];
				content_list[content_list_index].content_name[i] = recieve_pdu.data[i+10];
				if(i < 6){
					content_list[content_list_index].port_number[i] = recieve_pdu.data[i+20];
					content_list[content_list_index].ip_address[i] = recieve_pdu.data[i+27];
				}
//
			}	
			printf("\nRegistered Content:\n");		
			write(1, &content_list[content_list_index], sizeof(content_list[content_list_index]));
			
			content_list_index++;
		}
		else if(recieve_pdu.type == 'S' || recieve_pdu.type == 'D')
		{
			memset(&search_pdu, 0, sizeof(search_pdu));	
			
			

			for(i=0; i<10; i++){
				search_pdu.peer_name[i] = recieve_pdu.data[i];
				search_pdu.content_name[i] = recieve_pdu.data[i+10];

			}
			for(i = 0; i<10; i++){
				if(strcmp(search_pdu.peer_name, content_list[i].peer_name) == 0 && strcmp(search_pdu.content_name, content_list[i].content_name) == 0){
					memset(&match, 0, sizeof(match));
					memcpy(&match, &content_list[i], sizeof(content_list[i]));
					match.type = 'A';
					
					sendto(s, &content_list[i], sizeof(content_list[i]), 0, (struct sockaddr *)&fsin, sizeof(fsin));
				}
				
			}
			
		}
		else if(recieve_pdu.type == 'O')
		{
			//content names
			memset(&listing_pdu, 0, sizeof(listing_pdu));
			listing_pdu.type = 'O';
			
			for(i = 0; i< content_list_index; i++){
				for(j = 0; j< 10; j++){
				listing_pdu.data[i*10 + j] = content_list[i].content_name[j];
				}
			}
			listing_pdu.data[i*10 + j + 1] = '\0';
			sendto(s, &listing_pdu, sizeof(listing_pdu), 0, (struct sockaddr *)&fsin, sizeof(fsin));

			//peer names
			memset(&listing_pdu, 0, sizeof(listing_pdu));
			listing_pdu.type = 'O';
			
			for(i = 0; i< content_list_index; i++){
				for(j = 0; j< 10; j++){
				listing_pdu.data[i*10 + j] = content_list[i].peer_name[j];
				}
			}
			listing_pdu.data[i*10 + j + 1] = '\0';
			sendto(s, &listing_pdu, sizeof(listing_pdu), 0, (struct sockaddr *)&fsin, sizeof(fsin));
		}
		else if(recieve_pdu.type == 'T')
		{
			memset(&dereg_pdu, 0, sizeof(dereg_pdu));
			
			for(i=0; i<10; i++){
				dereg_pdu.peer_name[i] = recieve_pdu.data[i];
				dereg_pdu.content_name[i] = recieve_pdu.data[i+10];

			}
			
			for(i = 0; i<10; i++){
				if(strcmp(dereg_pdu.peer_name, content_list[i].peer_name) == 0 && strcmp(dereg_pdu.content_name, content_list[i].content_name) == 0){					
					memset(&content_list[i], 0, sizeof(content_list[i]));
				}
			}
		}		
	}
}
