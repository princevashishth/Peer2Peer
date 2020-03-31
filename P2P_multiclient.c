

#include <bits/stdc++.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h> // STDIN_FILENO is defined in this
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h> //macros like FD_SET, FD_ISSET, FD_ZERO are defined in this
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

using namespace std;

int n_peers;
int *client_fds;

#define BUFSIZE 1024
#define PEER_TIMEOUT_DURATION 120       // in seconds


#define KMAG  "\x1B[31m"
#define CYAN "\x1B[1;36m"
#define RESET "\x1B[0m"



void error(const char *msg) {
  perror(msg); //perror for printing error message
  exit(1); // exit(1) to terminate with error
}

int get_max_fd(vector<int> all_fds){  //finding maximum FD
  int max_val=INT_MIN;                 //INT_MIN is macro
  for (vector<int>::iterator itr=all_fds.begin();itr!=all_fds.end();itr++){
    if(*itr>max_val){
      max_val=*itr;
    }
  }
  return max_val;
}

// overloaded functions for searching the index of the peer based on 'to_search' parameter
int search_peer(int n_peers,char **peer_details,char *to_search){
  for (int i=0;i<n_peers;i++){
    if(strcmp(peer_details[i],to_search)==0)  //returns index of that peer
      return i;
  }
  return -1;
}


void sig_handler(int signo){     //Signal handler for closing all client file descriptors 
  for(int i=0;i<n_peers;i++){
    close(client_fds[i]);
  }
  exit(0);
}


int main(int argc, char **argv) {
  int server_fd; // parent socket 
  int portno; // port to listen on 
  socklen_t clientlen; // byte size of client's address 
  struct sockaddr_in serveraddr; // server's addr 
  struct hostent *hostp; // client host info 
  char buf[BUFSIZE]; // message buffer 
  char *hostaddrp; // dotted decimal host addr string 
  int optval; // flag value for setsockopt 
  int n; // message byte size 
	long int filesize; /*file size*/
	char filename[200];//filename
  char  str[1000];
	FILE *filept; /*file pointer to record input*/  
  
  portno=8234; //change


  // first of all, creating the TCP SERVER 

  server_fd = socket(AF_INET, SOCK_STREAM, 0);                    //from here
  if (server_fd < 0) 
    error("ERROR opening socket");

  // ensuring socket reuse for ease of debugging
  optval = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int)); //changing socket option

  //creating the datastruct for server
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  // binding to a well-known port
  if (bind(server_fd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");                                   

  //putting it in LISTEN MODE

  
  printf("P2P Server Running at port: %d\n",portno);
  if (listen(server_fd, 5) < 0) // allowing max of 5 peer requests as backlog 
    error("ERROR on listen");                                               // to here server is created and listening
  

  //accepting the user_info data structure and storing it.
  cout<<"Enter the details of the peers and respective IP addresses: "<<endl;
  cout<<"number of peers: ";
  int n_peers;
  cin>>n_peers;
  client_fds=(int*)malloc(n_peers*sizeof(int));
  float last_interaction[n_peers];             // the time of last interaction - used to set time-out used for further operations
  struct sockaddr_in peeraddr[n_peers];
  char **peer_ips=(char**)malloc(n_peers*sizeof(char*));
  char **peer_names=(char**)malloc(n_peers*sizeof(char*));
  for(int j=0;j<n_peers;j++){
    peer_ips[j]=(char*)malloc(30*sizeof(char));
    peer_names[j]=(char*)malloc(30*sizeof(char));
  }
  unsigned short peer_ports[n_peers],x;
  // default server port
  x=8111;                        
  
  
  for(int i=0;i<n_peers;i++){
    client_fds[i]=last_interaction[i]=-1;
    char name[20],ip[30];
    cout<<"name of peer "<<i<<" : ";
    scanf(" %s",name);
    strcpy(peer_names[i],name);
    cout<<"ip of peer "<<i<<" : ";
    scanf(" %s",ip);
    strcpy(peer_ips[i],ip);
    
    peer_ports[i]=htons(x);          //here x is 8111
  }

  signal(SIGINT,sig_handler);
  cout<<KMAG<<"Chat session set up complete..."<<RESET<<endl;//uptil now server is running and listening and we have taken information about all clients

  //initializing the select() and adding the known file descriptors-stdin and server_fd to select()
  fd_set readfds;
  fd_set writefds;
  fd_set exceptfds;
  struct timeval timeout;
  int maxfd, result=-1;
  vector<int> all_fds;
  vector<int> input_fds;
  
  input_fds.push_back(STDIN_FILENO);
  input_fds.push_back(server_fd);
  all_fds.push_back(STDIN_FILENO);
  all_fds.push_back(server_fd);

  clock_t session_begin_time=clock();       // setting reference time for all future clock-based operations
  while(1){                                 // running in loop till user exits
    result=-1;

    while(result==-1){
      // checking last usage times of each of the peers, to TIME OUT...
      //... those that have not interacted for more than time out duaration
      for(int j=0;j<n_peers;j++){
        float time_gap=(float)(clock()-last_interaction[j])/CLOCKS_PER_SEC;
        if(time_gap>=PEER_TIMEOUT_DURATION){
          last_interaction[j]=-1;
          client_fds[j]=-1;
        }
      }         //ending for loop
      FD_ZERO(&readfds);
      FD_ZERO(&writefds);
      // at the time of initialization, there are no connections. The select should just keep track of stdin and server_fd...
      for(vector<int>::iterator it=input_fds.begin();it!=input_fds.end();it++){
        FD_SET(*it,&readfds);
      }
      timeout.tv_sec=120;                     // setting at 2 min 
      result=select(get_max_fd(all_fds)+1,&readfds,&writefds,&exceptfds,&timeout);
    }              //ending result=-1 while loop

    //got some connection request               //something happening with server_fd
    if(FD_ISSET(server_fd,&readfds)){        
      int childfd; /* child socket */
      struct sockaddr_in clientaddr; /* client addr */
      clientlen = sizeof(clientaddr);

      childfd = accept(server_fd, (struct sockaddr *) &clientaddr, &clientlen);
      hostaddrp = inet_ntoa(clientaddr.sin_addr);
      if (hostaddrp == NULL)
        error("ERROR on inet_ntoa\n");
      int client_idx= search_peer(n_peers,peer_ips,hostaddrp);//ntohs
      if(client_idx==-1){
        cout<<"Request received from unknown host"<<endl;
      }
      else{
        client_fds[client_idx]=childfd;
        last_interaction[client_idx]=(float)(clock()-session_begin_time)/CLOCKS_PER_SEC;
      }
      input_fds.push_back(childfd);
      all_fds.push_back(childfd);
    }


    if(FD_ISSET(STDIN_FILENO,&readfds)){  //if user has input something through keyboard
        // reading the message and separating the peer_name and message parts 
        bzero(str,1000);
        read(STDIN_FILENO,str,1000);
        char *friend_name=(char*)malloc(100*sizeof(char));
        char *msg=(char*)malloc(900*sizeof(char));  //pointer becauses return value from strtok() function is a pointer to message being sent
        friend_name=strtok(str,"/");
        msg=strtok(NULL,"/");
        int peer_idx=search_peer(n_peers,peer_names,friend_name);
        if(peer_idx==-1){
          cout<<"No such peer found in the list: "<<friend_name<<endl;
        }

        if(last_interaction[peer_idx]==-1){          // that is, the session with this peer has either timed-out or has not been established yet
          int newfd=socket(AF_INET, SOCK_STREAM, 0);
          if (newfd < 0) 
            error("ERROR opening socket");
          
          struct hostent *server=gethostbyname(peer_ips[peer_idx]);
          // build the peer's Internet address 
          bzero((char *) &peeraddr[peer_idx], sizeof(peeraddr[peer_idx])); //bzero ends here
          peeraddr[peer_idx].sin_family = AF_INET;
          bcopy((char *)server->h_addr, 
          (char *)&peeraddr[peer_idx].sin_addr.s_addr, server->h_length);  //bcopy function copies from arg1 to arg2 with arg3 specifying size
          peeraddr[peer_idx].sin_port = (peer_ports[peer_idx]);//htons

          //connect: create a connection with the peer
          if (connect(newfd, (struct sockaddr*)&peeraddr[peer_idx], sizeof(peeraddr[peer_idx])) < 0) 
            error("ERROR connecting peer");

          client_fds[peer_idx]=newfd;
          input_fds.push_back(newfd);
          all_fds.push_back(newfd);
        }

        // the connection with this peer now exists
        // sending the message
        n = write(client_fds[peer_idx], msg, strlen(msg));
        if (n < 0) 
          error("ERROR sending message to peer");
        bzero(msg,strlen(msg));
        last_interaction[peer_idx]=(float)(clock()-session_begin_time)/CLOCKS_PER_SEC;
    } //closing for input from keyboard

    // if the select has returned the fd of a peer sending a message
    for(int j=0;j<n_peers;j++){
      if(FD_ISSET(client_fds[j],&readfds)){ //check which client sent the message
        bzero(buf,BUFSIZE);
        n = read(client_fds[j], buf, BUFSIZE);
        last_interaction[j]=(float)(clock()-session_begin_time)/CLOCKS_PER_SEC;
        if (n < 0) 
          error("ERROR reading from peer");
        if(n==0)
          close(client_fds[j]);
        else{
          // printing the message...
          cout<<CYAN<<peer_names[j]<<" : "<<RESET<<buf<<endl;
        }                   //closing else

      }                     //closing if statement
    } 				// closing for loop

  } 				//closing while loop
}				 //closing main
