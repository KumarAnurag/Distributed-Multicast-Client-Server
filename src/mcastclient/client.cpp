#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#define LISTENING_PORT 6000
#define PEER_PORT  9000
#define EXAMPLE_GROUP "239.0.0.1"


int g_sock;

int find_max(void)
{

   return 1000; //hardcoded

}

void *process_data_and_send_response(void *data)
{
   char *buff=(char*) data;
   int ans;
   //decode data
   //let data be integrs 1000, 2000 etc


   //process data - apply algorithm to calculate task
   //let task be to calculate MAX of number
   
  int task_type = 1; // max of n numbers
  switch(task_type)
  {
	case 1:
               ans = find_max(/*array_of_integers*/);
               break;
        default:
               break;
  }


  //encode ans to send the response
  //....
  



  //send the response
  
   char rsp_message[1024]={0};
   struct sockaddr_in addr;
   int addrlen = sizeof(addr);
   bzero((char *)&addr, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons(9000);
   sprintf(rsp_message, "%d", htonl(1000)); // sample message , hardcoded 
   int cnt = sendto(g_sock, rsp_message, 1024, 0,
		      (struct sockaddr *) &addr, addrlen);
   if (cnt < 0) 
   {
 	    perror("sendto");
	    exit(1);
   }
   free(data);
}

void receive_req_from_server(int sock)
{
   struct sockaddr_in addr;
   int addrlen;
   char message[64001];
   g_sock = sock;
   while (1) 
   {
 	 int cnt = recvfrom(sock, message, sizeof(message), 0, 
			(struct sockaddr *) &addr, (socklen_t*)&addrlen);
	 if (cnt < 0)
         {
	    perror("recvfrom");
	    exit(1);
	 } else if (cnt == 0)
         { 
           continue;
         }
        char *p=(char*)malloc(64001);
        memcpy(p, message, sizeof(message));
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, process_data_and_send_response, p);
   }

}




int join_the_multicast_group(int *sck)
{
  struct sockaddr_in addr;
  int addrlen, sock;
  struct ip_mreq mreq;

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if(sock < 0)
  {
    perror("socket");
    exit(1); 
  }

  int reuse = 1;
 
  if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
  {   
      perror("Setting SO_REUSEADDR error");
      close(sock);
      exit(1);
  }   

   bzero((char *)&addr, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons(6000);
   addrlen = sizeof(addr);

   if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) 
   {
       perror("bind");
       exit(1);
   }
   mreq.imr_multiaddr.s_addr = inet_addr(EXAMPLE_GROUP);
   mreq.imr_interface.s_addr = htonl(INADDR_ANY);
   if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) 
   {
        perror("setsockopt mreq");
        exit(1);
   }
   *sck=sock;
   return 0;
}
    

int main()
{

    int ret, sock;
    ret = join_the_multicast_group(&sock);
    assert(ret == 0);
    receive_req_from_server(sock);

    return 0;
}
