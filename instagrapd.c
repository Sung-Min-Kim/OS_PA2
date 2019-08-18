// Partly taken from https://www.geeksforgeeks.org/socket-programming-cc

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/stat.h>

typedef struct{
  char id[32];
  char pw[32];
}USER;

void
child_proc(int conn, char *ip_address_worker, int port_worker, char *testcase_path, USER *user_list, int* user_count, int port_submitter)
{
    char buf[1024];
    char *data = 0x0, * orig = 0x0;
    int len = 0;
    int s;

    printf("Connection Num: %d\n", conn);

    while((s = recv(conn, buf, 1023, 0)) > 0){
        buf[s] = 0x0;
        if (data == 0x0){ // receiving data is the first time
            data = strdup(buf); // copy the data
            len = s;
  	    }
      	else{ // Data is not empty
      	    data = realloc(data, len + s + 1);
      	    strncpy(data + len, buf, s);
      	    data[len + s] = 0x0;
      	    len += s;
      	}
    }
    printf("Given Source File(Instagrapd):\n>%s\n", data);//data is a string containing the source code from the submitter

    /* Getting USER INFO From Named Pipe  */
    int pd, info;
    char buf_pipe[64];
    char user_id[64];
    char user_pw[64];

    if((pd = open("./USER_INFO", O_RDONLY)) == -1){
      	perror("open");
      	exit(1);
    }

    while((info = read(pd, buf_pipe, sizeof(buf_pipe))) > 0)
	       ;

    if(info == -1){
      	perror("read");
      	exit(1);
    }

    //Parsing User INFO
    char *ptr2;
    ptr2 = strtok(buf_pipe, ":");
    strcpy(user_id, ptr2);
    while (ptr2 != NULL)
    {
        //printf("%s\n", ptr);
        ptr2 = strtok(NULL, ":");
        if(ptr2 == NULL)
            break;
        strcpy(user_pw, ptr2);
    }

    close(pd);
    /* End Reading from Pipe */

    /* Save User Information in the User Array */
    strcpy(user_list[*user_count].id, user_id);
    strcpy(user_list[*user_count].pw, user_pw);
    *user_count++;




    //This part should deliver the source code string to worker process
    //by using pipe or socket
    //I think worker should be multi threaded program which concurrently runs 10 test cases on the same code

    /* Make a Socket to Instagrapd */
    printf("Instagrapd is about to connect to Worker!\n");

    struct sockaddr_in serv_addr;
    int sock_fd;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd <= 0){
        perror("socket failed: ");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_worker);
    if(inet_pton(AF_INET, ip_address_worker, &serv_addr.sin_addr) <= 0){
      	perror("inet_pton failed: ");
      	exit(EXIT_FAILURE);
    }

    if(connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
      	perror("connect failed!!!: ");
      	exit(EXIT_FAILURE);
    }

    //Sending a source code to worker
    orig = data;
    while(len > 0 && (s = send(sock_fd, data, len, 0)) > 0){
	     data += s;
	     len -= s;
    }
    shutdown(sock_fd, SHUT_WR);
    // if(orig != 0x0)
	  //    free(orig);


    /* Sending Path of Testcase Directory Using Named Pipe(FIFO) */

    int fd;
    if(mkfifo("./TESTCASE", 0666) == -1){
	     perror("mkfifo");
	     exit(1);
    }

    if((fd = open("./TESTCASE", O_WRONLY)) == -1){
	     perror("open");
	     exit(1);
    }

    printf("Testcase path : %s\n", testcase_path);

    if(write(fd, testcase_path, strlen(testcase_path)+1) == -1){
      	perror("write");
      	exit(1);
    }
    close(fd);


    //Getting message from the worker
    char buf_worker[1024];
    data = 0x0;
    len = 0;
    while((s = recv(sock_fd, buf_worker, 1023, 0)) > 0) {//getting a message from instagrap
      	buf_worker[s] = 0x0;
      	if(data == 0x0){
      	    data = strdup(buf_worker);
      	    len = s;
      	}
      	else{
      	    data = realloc(data, len + s + 1);
      	    strncpy(data + len, buf_worker, s);
      	    data[len + s] = 0x0;
      	    len += s;
      	}

    }
    printf(">%s\n", data);



    // MSG send back to the submitter
    //This part should send back the message given from the worker process
    orig = data;
    while(len > 0 && (s = send(conn, data, len, 0)) > 0){
      	data += s;
      	len -= s;
    }
    shutdown(conn, SHUT_WR);
    // if (orig != 0x0)
	  //    free(orig);
}



int
main(int argc, char const *argv[])
{
    int listen_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt;
    char *ptr;
    int port_sub, port_worker;
    char ip_address_worker[64];
    char testcase_path[64];

    USER user_list[10];
    int user_count = 0;

    /* Parsing the argument of the file */
    while((opt = getopt(argc, argv, "p:w:")) != -1){
      	switch(opt){
      	    case 'w': //IP and Port number of Worker
            		ptr = strtok(optarg, ":");
            		strcpy(ip_address_worker, ptr);
            		while (ptr != NULL)
            		{
            		    //printf("%s\n", ptr);
            		    ptr = strtok(NULL, ":");
            		    if(ptr == NULL)
            			      break;
            		    port_worker = atoi(ptr);
            		}
            		//printf("IP and Port num parsed!\n");
            		break;

      	    case 'p':
            		port_sub = atoi(optarg);
            		break;
      	}
    }

    if(!strcpy(testcase_path, argv[optind]))
	     printf("File Name COPY ERROR!\n");

    /* End Parsing */




    /* Socket */
    char buffer[1024] = {0};

    listen_fd = socket(AF_INET /*IPv4*/, SOCK_STREAM /*TCP*/, 0 /*IP*/);
    if(listen_fd == 0){
      	perror("socket failed: ");
      	exit(EXIT_FAILURE);
    }

    memset(&address, '0', sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY /* the localhost */;
    address.sin_port = htons(port_sub);
    if(bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
      	perror("bind failed: ");
      	exit(EXIT_FAILURE);
    }

    while(1){
      	if(listen(listen_fd, 16 /* the size of waiting queue */) < 0){
      	    perror("listen failed: ");
      	    exit(EXIT_FAILURE);
      	}

      	new_socket = accept(listen_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen);
      	if(new_socket < 0){
      	    perror("accept");
      	    exit(EXIT_FAILURE);
      	}

      	if(fork() > 0){
      	    child_proc(new_socket, ip_address_worker, port_worker, testcase_path, user_list, &user_count, port_sub);
      	}
      	else{
      	    close(new_socket);
      	}
    }
}

