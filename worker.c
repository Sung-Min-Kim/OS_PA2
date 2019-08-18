#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sched.h>

void
build_file(char *code)
{
    //printf("Enter Build_file Function\n");
    FILE *fp = fopen("result.c", "w");

    if(fprintf(fp, "%s", code))
        //printf("The file has been created succesfully\n");// 파일이 생성됨

    fclose(fp);
}


int right_cnt = 0, wrong_cnt = 0;

void
exe_file(char *testcase_path, int num)
{

        char input_path[32];
        char output_path[32];
        strcpy(input_path, testcase_path);
        strcpy(output_path, testcase_path);

        strcat(input_path, ".in");
        strcat(output_path, ".out");
        /* Making file descriptor for input files */
        int fd_input = open(input_path, O_RDONLY, 0755);
        dup2(fd_input, 0); // switch the standard input to the input file

        char file_name[32];
        sprintf(file_name, "%d", num+1);
        strcat(file_name, ".output");

        int fd_output = open(file_name, O_WRONLY | O_TRUNC | O_CREAT, 0755);
        dup2(fd_output, 1);

        system("./result");


        /* Comparing results */
        FILE* fp_in, *fp_out;

        fp_in = fopen(file_name, "r");
        fp_out = fopen(output_path, "r");


        char code_result[32], answer[32];
        while(1){
                fgets(code_result, sizeof(code_result), fp_out);
                fgets(answer, sizeof(answer), fp_in);

                //printf("code_result: %s, answer: %s\n", code_result, answer);

                //printf("code_result: %s, answer: %s\n", code_result, answer);
                int a = atoi(code_result);
                int b = atoi(answer);

                //printf("a: %d, b: %d\n", a, b);

                //printf("Start Comparing!\n");
                if(a == b){
                  right_cnt++;
                }
                else{
                  wrong_cnt++;
                }

                if(feof(fp_out) || feof(fp_in))
                        break;
        }

        close(fd_input);
        close(fd_output);
        fclose(fp_in);
        fclose(fp_out);
}




int
main(int argc, char const *argv[])
{
    int listen_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt;
    int port_num;
    char testcase_path[64];


    /* Parsing the argument of the file */
    while((opt = getopt(argc, argv, "p:")) != -1){
      	switch(opt){
      	    case 'p':
            		port_num = atoi(optarg);
            		break;
      	}
    }
    /* End Parsing */


    /* Listening from the Instagrapd */
    listen_fd = socket(AF_INET /*IPv4*/, SOCK_STREAM /*TCP*/, 0 /*IP*/);
    if(listen_fd == 0){
      	perror("socket failed: ");
      	exit(EXIT_FAILURE);
    }

    memset(&address, '0', sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY /* the localhost */;
    address.sin_port = htons(port_num);
    if(bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
      	perror("bind failed: ");
      	exit(EXIT_FAILURE);
    }

  	if(listen(listen_fd, 16 /* the size of waiting queue */) < 0){
  	    perror("listen failed: ");
  	    exit(EXIT_FAILURE);
  	}

  	new_socket = accept(listen_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen);
  	if(new_socket < 0){
  	    perror("accept");
  	    exit(EXIT_FAILURE);
  	}

    /* Reading C source code file */
    char buf[1024] = {0};
    char *data = 0x0, * orig = 0x0;
    int len = 0;
    int s;


    while((s = recv(new_socket, buf, 1023, 0)) > 0){
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
    printf("Given Source File(Worker):\n>%s\n", data);//data is a string containing the source code from the submitter


    /* Getting Testcase Directory Path Info From Named Pipe */
    int pd, info;
    char buf_pipe[64];
    if((pd = open("./TESTCASE", O_RDONLY)) == -1){
      	perror("open");
      	exit(1);
    }

    while((info = read(pd, buf_pipe, sizeof(buf_pipe))) > 0)
	       ;

    if(info == -1){
      	perror("read");
      	exit(1);
    }

    strcpy(testcase_path, buf_pipe); // Copying the directory path
    //printf("Testcase Path: %s\n", testcase_path);
    close(pd);
    /* End Reading from Pipe */

    //pthread여기서 인풋 개수만큼 생성해서 build파일을 호출하면 될듯 인풋파일 파라미터로 넘겨주고
    //그리고 마지막에 조인하고 결과 마무리
    build_file(data);


    system("gcc -o result result.c"); // Program Build


    //pthread 사용해서 input파일 여러개 돌려라.
    //printf("Testing Start Using Pthread\n");
    int q;
    char path[32];
    char tmp[16];
    char *testcase[10];

    for(int i = 0; i < 10; i++){
      testcase[i] = (char *)malloc(sizeof(char) * 32);
      sprintf(tmp, "%d", i+1);
      strcpy(testcase[i], testcase_path);
      strcat(testcase[i], tmp);
    }
    //printf("Path to 1.in: %s\n", testcase[0]);

    // Argument로 하나만 넘겨주면 괜찮은데, 여러개를 넘겨주니까 오류가 난다.
    for(q = 0; q < 10; q++){
        //pthread_create(&thread_list[q], NULL, exe_file, (void *)testcase[q]);
        exe_file(testcase[q], q);
        //printf("Executing the function!\n");
    }

    // for(q = 0; q < 3; q++){
    //     pthread_join(thread_list[q], NULL);
    // }

    //right_cnt에 맞은 테스트케이스 개수가 담겨져 있다.

    // MSG send back to the submitter
    //This part should send back the message given from the worker process

    char result_msg[64];
    char temp_str[32];
    strcpy(result_msg, "Total ");
    sprintf(temp_str, "%d", right_cnt);
    strcat(result_msg, temp_str);
    strcat(result_msg, " test cases pass!!");
    // data = result_msg;
    len = strlen(result_msg);

    data = result_msg;
    orig = data;
    while(len > 0 && (s = send(new_socket, data, len, 0)) > 0){
	     data += s;
	     len -= s;
    }
    shutdown(new_socket, SHUT_WR);


    // if (orig != 0x0)
    //    free(orig);

    close(new_socket);
}

