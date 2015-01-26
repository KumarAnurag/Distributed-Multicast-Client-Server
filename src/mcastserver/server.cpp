#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <queue>


#define LISTENING_PORT 9000
#define MCAST_PORT 6000
#define EXAMPLE_GROUP "239.0.0.1"

using namespace std;

queue<int>q;

time_t start_time;
time_t end_time;

struct clients_info{
	int addr;
	short port;
	char free;
};

int total_req=0;
int total_rsp=0;
int done = 0;


int send_packet_to_client(char *filename)
{
   struct sockaddr_in addr;
   int addrlen, sock; 
   done = 0;
  
   sock = socket(AF_INET, SOCK_DGRAM, 0); 
   if (sock < 0) {
     perror("socket");
     exit(1);
   }   
   bzero((char *)&addr, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(6000);
   addrlen = sizeof(addr);
   addr.sin_addr.s_addr = inet_addr(EXAMPLE_GROUP);
   

  FILE *fp = fopen(filename,"r");
  if(fp == NULL)
  {
    printf("Error: file %s cannot be opened for reading\n",filename);
    return -1;
  }

  int tot_num,data_size,ret;
  if((ret=fscanf(fp,"%d%d",&tot_num,&data_size)) != 2)
  {
    printf("File not in correct format, number of arguments read=%d\n",ret);
    return -1;
  }

  int tot_data_bytes = tot_num*data_size;
  int rem_data = tot_data_bytes;

  //let there be 2 clients, fill info statically for now
                                  //127.0.0.1  6000                     6000
  struct clients_info clients[2]={{0x7f000001, 0x1770,1},{0x7f000001, 0x1770,1}};

  int max_data=0;
  while(rem_data > 0)
  {
    max_data=rem_data;
    if(rem_data > 60000)
    {
      max_data = 60000;
    }

	
    //find the N, number of free clients in the group
    //for now let there be two clients
    //int N=group[id].free_clients;
    int N=2; //clients have ip address ip1/port1 ip2/port2
    int each_client_data = max_data/N;
    while(each_client_data % 4 != 0) each_client_data--;

    int i=0,offset1=0,count_bytes=0,num;
    char buff[60001];
    while(1)
    {
      fscanf(fp,"%d",&num);
      /* call encode routine for encoding actual data */
      int tmp=htonl(num);
      memcpy(buff+offset1,&tmp,sizeof(tmp));
      offset1+=4;
      count_bytes+=4;
      printf("%x\n",tmp);
      if(count_bytes == max_data) break;
    }

    //iterate through the client list for particular group to find which all clients are free
    char buff_client_info[255*4*4+1];
    int offset2=0;
    int remaining = max_data;
    int start_offset=0, end_offset=0;
    for(i=0;i<N;i++) //255 clients iteration
    {
      if(clients[i].free)
      {
        if(remaining <= 0) break;
        if(remaining < each_client_data)
          each_client_data = remaining;

        remaining = remaining - each_client_data;
        if(start_offset==0)
          end_offset = each_client_data;
        else
        {
          start_offset = end_offset;
          end_offset += each_client_data;
        }

        /*call incode routine  for meta data like ip address, port, offset info*/
        //fill client ip address
        int tmp=htonl(clients[i].addr);
        memcpy(buff_client_info+offset2, &tmp, sizeof(tmp));
        offset2+=4;
        //fill client port address
        short tmp2=htonl(clients[i].port);
        memcpy(buff_client_info+offset2, &tmp2, sizeof(tmp2));
        offset2+=2;
        //fill each_client_data start_offset end_offset
        tmp=htonl(start_offset);
        memcpy(buff_client_info+offset2,&tmp, sizeof(tmp));
        offset2+=4;
        tmp=htonl(end_offset);
        memcpy(buff_client_info+offset2, &tmp, sizeof(tmp));
        offset2+=4;
        /* call encode routine */
        total_req++; 
      }
    }


    char final_buffer[65535] = {0};
    memcpy(final_buffer, buff, offset1);
    memcpy(final_buffer+offset1, buff_client_info, offset2);

    //send final_buffer
    int cnt = sendto(sock, final_buffer, offset1+offset2, 0,
                  (struct sockaddr *) &addr, addrlen);

    if(cnt != offset1+offset2)
    {
	printf("all data not sent\n");
	return -1;
    }
    else
    {
	rem_data -= max_data;
        printf("%d bytes sent on wire\n",cnt);
    }
 }
        start_time = time(NULL);
        done = 1;
   return 0;
}


void * receive_rsp_from_clients(void *)
{
 struct sockaddr_in addr;
 int sock, addrlen;
 char message[1024];
 sock = socket(AF_INET, SOCK_DGRAM, 0);
 if (sock < 0) {
     perror("socket");
     exit(1);
 }
 bzero((char *)&addr, sizeof(addr));
 addr.sin_family = AF_INET;
 addr.sin_addr.s_addr = htonl(INADDR_ANY);
 addr.sin_port = htons(9000);
 if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {    
         perror("bind");
         exit(1);
 }   

   while (1) {
     int cnt = recvfrom(sock, message, sizeof(message), 0,  
                        (struct sockaddr *) &addr, (socklen_t*)&addrlen);
         if (cnt < 0) {
            perror("recvfrom");
            exit(1);
         } else if (cnt == 0) {
            break;
         }
         q.push(ntohl(atoi(message)));
	 total_rsp++;
  }

}



void * test_process_receive_rsp_from_clients(void *)
{
   int maxx=INT_MIN;
    while(1)
    {
        if((done) &&(total_req != 0) && (total_req == total_rsp)) break;
        if(q.empty()) continue;
        end_time=time(NULL);
        double timeout = (double)(end_time-start_time); //seconds
	if(done && timeout >= 120 ) break; //break in case some rsp are not received within given time
		
        int item=q.front();
	q.pop();
        #define MAX 1
        int task_type = MAX;
        
	switch(task_type)
	{
		case MAX:
				if(item>maxx) maxx=item;
				break;
		default:
				break;
	}
      total_rsp++;
    }

   printf("Ans is %d\n",maxx);

}

void test_send_packet_to_client(char *filename)
{

  int ret=send_packet_to_client(filename);
  assert(ret == 0);

}

int main(int argc, char **argv)
{
  pthread_t thread1,thread2;

  pthread_create(&thread1, 0, receive_rsp_from_clients, NULL);
  pthread_create(&thread2, 0, test_process_receive_rsp_from_clients, NULL);

  test_send_packet_to_client((char*)"data-file.txt");
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  printf("Ok, all test passed\n");

  return 0;
}
