#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int
main(int argc, char const *argv[])
{
    struct sockaddr_in serv_addr;
    int sock_fd;
    int s, len;
    char buffer[1024] = {0};
    char *data;

    //Parsing the parameters that are given at the execution of program
    char ip_address[64];
    int port_num = 0;
    char user_id[64];
    char user_pw[64];
    char file_name[64];
    int opt;
    char *ptr;

    /* Parsing the argument of the file */
    while((opt = getopt(argc, argv, "n:u:k:")) != -1){
      	switch(opt){
      	    case 'n': //IP and Port number
            		ptr = strtok(optarg, ":");
            		strcpy(ip_address, ptr);
            		while (ptr != NULL)
            		{
            		    // printf("%s\n", ptr);
            		    ptr = strtok(NULL, ":");
            		    if(ptr == NULL)
            			break;
            		    port_num = atoi(ptr);
            		}
            		//printf("IP and Port num parsed!\n");
            		break;

      	    case 'u':
            		if(!strcpy(user_id, optarg))
            		    printf("User ID Failed!\n");
            		break;

      	    case 'k':
            		if(!strcpy(user_pw, optarg))
            		    printf("User PW Failed!\n");
            		break;
      	}
    }

    if(!strcpy(file_name, argv[optind]))
	     printf("File Name COPY ERROR!\n");


    /* End Parsing */


    //get a C source code as a string and store the whole text in one character pointer
    //in order to use it in send function
    FILE* fp = fopen(file_name, "r");
    char tmp_buf[1024];
    char *result = 0x0;
    int total_len = 0, now_len = 0;

    while(1){
      	fgets(tmp_buf, sizeof(tmp_buf), fp);
            if(feof(fp) != 0)
                break;

        //printf("Check if it read well\n");
        //printf("buf: %s\n", tmp_buf);

        now_len = strlen(tmp_buf);

        if(result == 0x0){
            result = strdup(tmp_buf);
            total_len = now_len;
        }else{
            result = realloc(result, total_len + now_len + 1);
            strncpy(result + total_len, tmp_buf, now_len);
            result[total_len + now_len] = 0x0;
            total_len += now_len;
        }
    }
    fclose(fp);




    /* Make a Socket to Instagrapd */
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd <= 0){
        perror("socket failed: ");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_num);
    if(inet_pton(AF_INET, ip_address, &serv_addr.sin_addr) <= 0){
      	perror("inet_pton failed: ");
      	exit(EXIT_FAILURE);
    }

    if(connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
      	perror("connect failed: ");
      	exit(EXIT_FAILURE);
    }

    //Sending a source code as a string to instagrap
    data = result;
    len = strlen(result);
    s = 0;
    while(len > 0 && (s = send(sock_fd, data, len, 0)) > 0) {
      	data += s;
      	len -= s;
    }

    shutdown(sock_fd, SHUT_WR);

    //free(data);

    /* Sending User Information Using Named Pipe(FIFO) */
    int pd;
    char user_info[128];

    // strcpy(user_info_file_name, "./");
    // strcat(user_info_file_name, port_num);
    // printf("file name: %s\n", user_info_file_name);

    if(mkfifo("./USER_INFO", 0666) == -1){//"./USER_INFO", 0666) == -1){
      	perror("mkfifo");
      	exit(1);
    }

    if((pd = open("./USER_INFO", O_WRONLY)) == -1){
        perror("open");
        exit(1);
    }

    strcpy(user_info, user_id);
    strcat(user_info, ":");
    strcat(user_info, user_pw);

    //Send USER INFO to Instagrapd
    if(write(pd, user_info, strlen(user_info)+1) == -1){
      	perror("write");
      	exit(1);
    }
    close(pd);

    /* Named Pipe END */




    //Getting message from the server
    char buf[1024];
    data = 0x0;
    len = 0;
    while((s = recv(sock_fd, buf, 1023, 0)) > 0) {//getting a message from instagrap
      	buf[s] = 0x0;
      	if(data == 0x0){
      	    data = strdup(buf);
      	    len = s;
      	}
      	else{
      	    data = realloc(data, len + s + 1);
      	    strncpy(data + len, buf, s);
      	    data[len + s] = 0x0;
      	    len += s;
      	}

    }
    printf(">%s\n", data);

}

