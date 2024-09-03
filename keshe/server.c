#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define SERVER_IP_ADDR "0.0.0.0"	//服务器IP地址
#define SERVER_PORT 8007				//服务器端口号
#define BACKLOG 10
#define BUF_SIZE 8192
#define OK 1
#define ERROR 0

#include <direct.h>  // For _getcwd
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> 
#include <time.h>
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")

const char* Server_name = "Server: Web Server 1.0 - BooLo\r\n";
//Web服务器信息 

int Server_Socket_Init(int port);
int Handle_Request_Message(char* message, int Socket);
int Judge_URI(char* URI, int Socket);
int Send_Ifon(int Socket, const char* sendbuf, int Length);
int Error_Request_Method(int Socket);
int Inquire_File(char* URI);
int File_not_Inquire(int Socket);
int Send_File(char* URI, int Socket);
int Logo();
const char* Judge_Method(char* method, int Socket);
const char* Judge_File_Type(char* URI, const char* content_type);
const char* Get_Data(const char* cur_time);
const char* Post_Value(char* message);

int Server_Socket_Init(int port) {
	// 初始化和构造套接字 
	WORD wVersionrequested;
	WSADATA wsaData;
	SOCKET ServerSock;
	struct sockaddr_in ServerAddr;
	int rval;

	/* 加载Winsock */
	wVersionrequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionrequested, &wsaData) != 0) {
		printf("Failed to load Winsock!\n");
		system("pause");
		return -1;
	}
	printf("Succeed to load Winsock!\n");

	/* 创建套接字 */
	ServerSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ServerSock == INVALID_SOCKET) {
		printf("Failed to create socket!\n");
		system("pause");
		exit(1);
	}
	printf("Succeed to create socket!\n");

	/* 配置服务器IP、端口信息 */
	memset(&ServerAddr, 0, sizeof(struct sockaddr));	//每一个字节都用0来填充
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(port);
	ServerAddr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDR);

	/* 绑定 */
	rval = bind(ServerSock, (SOCKADDR*)&ServerAddr, sizeof(struct sockaddr));
	if (rval == SOCKET_ERROR) {
		printf("Failed to bind stream socket!\n");
		system("pause");
		exit(1);
	}
	printf("Succeed to bind stream socket!\n");

	return ServerSock;
}


int Handle_Request_Message(char* message, int Socket) {
	//处理HTTP请求报文信息 
	int rval = 0;
	char Method[BUF_SIZE];
	char URI[BUF_SIZE];
	char Version[BUF_SIZE];

	if (sscanf(message, "%s %s %s", Method, URI, Version) != 3) {
		printf("Request line error!\n");
		return ERROR;
	} //提取"请求方法"、"URL"、"HTTP版本"三个关键要素 

	if (Judge_Method(Method, Socket) == ERROR) {
		return ERROR;
	}
	else if (Judge_Method(Method, Socket) == "POST") {
		Post_Value(message);
	} //判断处理"请求方法" 

	// 如果URI是根目录，则将其重定向到 /index.html
	if (strcmp(URI, "/") == 0) {
		strcpy(URI, "/index.html");
	}

	if (Judge_URI(URI, Socket) == ERROR) {
		return ERROR;
	} //判断处理"URI" 
	else
		rval = Send_File(URI, Socket);

	if (rval == OK) {
		printf("The process is successfully finished!\n");
	}

	return OK;
}

const char* Judge_Method(char* method, int Socket) {
	//判断请求方式 
	if (strcmp(method, "GET") == 0) {
		return "GET";
	}
	else if (strcmp(method, "POST") == 0) {
		return "POST";
	}
	else {
		Error_Request_Method(Socket);
		return ERROR;
	}
}

int Judge_URI(char* URI, int Socket) {
	//判断请求URI 
	if (Inquire_File(URI) == ERROR) {
		File_not_Inquire(Socket);
		return ERROR;
	}
	else
		return OK;
}

int Send_Ifon(int Socket, const char* sendbuf, int Length) {
	//发送信息到客户端 
	int sendtotal = 0, bufleft, rval = 0;

	bufleft = Length;
	while (sendtotal < Length) {
		rval = send(Socket, sendbuf + sendtotal, bufleft, 0);
		if (rval < 0) {
			break;
		}
		sendtotal += rval;
		bufleft -= rval;
	}

	Length = sendtotal;

	return rval < 0 ? ERROR : OK;
}

int Error_Request_Method(int Socket) {
	//501 Not Implemented响应 
	const char* Method_err_line = "HTTP/1.1 501 Not Implemented\r\n";
	const char* cur_time = "";
	const char* Method_err_type = "Content-type: text/plain\r\n";
	const char* File_err_length = "Content-Length: 41\r\n";
	const char* Method_err_end = "\r\n";
	const char* Method_err_info = "The request method is not yet completed!\n";

	printf("The request method from client's request message is not yet completed!\n");

	if (Send_Ifon(Socket, Method_err_line, strlen(Method_err_line)) == ERROR) {
		printf("Sending method_error_line failed!\n");
		return ERROR;
	}

	if (Send_Ifon(Socket, Server_name, strlen(Server_name)) == ERROR) {
		printf("Sending Server_name failed!\n");
		return ERROR;
	}

	cur_time = Get_Data(cur_time);
	Send_Ifon(Socket, "Data: ", 6);
	if (Send_Ifon(Socket, cur_time, strlen(cur_time)) == ERROR) {
		printf("Sending cur_time error!\n");
		return ERROR;
	}

	if (Send_Ifon(Socket, Method_err_type, strlen(Method_err_type)) == ERROR) {
		printf("Sending method_error_type failed!\n");
		return ERROR;
	}

	if (Send_Ifon(Socket, Method_err_end, strlen(Method_err_end)) == ERROR) {
		printf("Sending method_error_end failed!\n");
		return ERROR;
	}

	if (Send_Ifon(Socket, Method_err_info, strlen(Method_err_info)) == ERROR) {
		printf("Sending method_error_info failed!\n");
		return ERROR;
	}

	return OK;
}

int Inquire_File(char* URI) {
	// Get the current working directory
	char cwd[BUF_SIZE];
	if (_getcwd(cwd, sizeof(cwd)) == NULL) {
		perror("getcwd() error");
		return ERROR;
	}

	// Construct the absolute path
	char abs_path[BUF_SIZE];
	snprintf(abs_path, sizeof(abs_path), "%s%s", cwd, URI);

	struct stat File_info;
	if (stat(abs_path, &File_info) == -1) {
		return ERROR;
	}
	else {
		return OK;
	}
}


int File_not_Inquire(int Socket) {
    const char* File_err_line = "HTTP/1.1 404 Not Found\r\n";
    const char* cur_time = "";
    const char* File_err_type = "Content-type: text/html\r\n";
    const char* File_err_end = "\r\n";

    FILE* file;
    struct stat file_stat;
    char sendbuf[BUF_SIZE];
    int send_length;

    // Get the current working directory
    char cwd[BUF_SIZE];
    if (_getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return ERROR;
    }

    // Construct the absolute path to 404.html
    char abs_path[BUF_SIZE];
    snprintf(abs_path, sizeof(abs_path), "%s\\404.html", cwd);

    // Open and read 404.html
    file = fopen(abs_path, "rb");
    if (file != NULL) {
        fstat(_fileno(file), &file_stat);

        // Send 404 Not Found response
        if (Send_Ifon(Socket, File_err_line, strlen(File_err_line)) == ERROR) {
            printf("Sending file_error_line error!\n");
            fclose(file);
            return ERROR;
        }

        if (Send_Ifon(Socket, Server_name, strlen(Server_name)) == ERROR) {
            printf("Sending Server_name failed!\n");
            fclose(file);
            return ERROR;
        }

        cur_time = Get_Data(cur_time);
        Send_Ifon(Socket, "Date: ", 6);
        if (Send_Ifon(Socket, cur_time, strlen(cur_time)) == ERROR) {
            printf("Sending cur_time error!\n");
            fclose(file);
            return ERROR;
        }

        if (Send_Ifon(Socket, File_err_type, strlen(File_err_type)) == ERROR) {
            printf("Sending file_error_type error!\n");
            fclose(file);
            return ERROR;
        }

        // Calculate and send Content-Length
        char content_length[BUF_SIZE];
        snprintf(content_length, sizeof(content_length), "Content-Length: %lld\r\n", file_stat.st_size);
        if (Send_Ifon(Socket, content_length, strlen(content_length)) == ERROR) {
            printf("Sending file_error_length error!\n");
            fclose(file);
            return ERROR;
        }

        // Send end of header
        if (Send_Ifon(Socket, File_err_end, strlen(File_err_end)) == ERROR) {
            printf("Sending file_error_end error!\n");
            fclose(file);
            return ERROR;
        }

        // Send 404.html content
        while ((send_length = fread(sendbuf, 1, BUF_SIZE, file)) > 0) {
            if (Send_Ifon(Socket, sendbuf, send_length) == ERROR) {
                printf("Sending 404.html content error!\n");
                break;
            }
        }

        fclose(file);
    } else {
        printf("Failed to open 404.html!\n");
        return ERROR;
    }

    return OK;
}


int Send_File(char* URI, int Socket) {
	char cwd[BUF_SIZE];
	if (_getcwd(cwd, sizeof(cwd)) == NULL) {
		perror("getcwd() error");
		return ERROR;
	}

	char abs_path[BUF_SIZE];
	snprintf(abs_path, sizeof(abs_path), "%s%s", cwd, URI);

	// 200 OK响应
	const char* File_ok_line = "HTTP/1.1 200 OK\r\n";
	const char* cur_time = "";
	const char* File_ok_type = "";
	const char* File_ok_length = "Content-Length: ";
	const char* File_ok_end = "\r\n";

	FILE* file;
	struct stat file_stat;
	char Length[BUF_SIZE];
	char sendbuf[BUF_SIZE];
	int send_length;

	if (Judge_File_Type(abs_path, File_ok_type) == ERROR) {
		printf("The request file's type from client's request message is error!\n");
		return ERROR;
	}

	file = fopen(abs_path, "rb");
	if (file != NULL) {
		fstat(_fileno(file), &file_stat);
		_itoa(file_stat.st_size, Length, 10);

		if (Send_Ifon(Socket, File_ok_line, strlen(File_ok_line)) == ERROR) {
			printf("Sending file_ok_line error!\n");
			return ERROR;
		}

		if (Send_Ifon(Socket, Server_name, strlen(Server_name)) == ERROR) {
			printf("Sending Server_name failed!\n");
			return ERROR;
		}

		cur_time = Get_Data(cur_time);
		Send_Ifon(Socket, "Date: ", 6);
		if (Send_Ifon(Socket, cur_time, strlen(cur_time)) == ERROR) {
			printf("Sending cur_time error!\n");
			return ERROR;
		}

		File_ok_type = Judge_File_Type(abs_path, File_ok_type);
		if (Send_Ifon(Socket, File_ok_type, strlen(File_ok_type)) == ERROR) {
			printf("Sending file_ok_type error!\n");
			return ERROR;
		}

		if (Send_Ifon(Socket, File_ok_length, strlen(File_ok_length)) != ERROR) {
			if (Send_Ifon(Socket, Length, strlen(Length)) != ERROR) {
				if (Send_Ifon(Socket, "\n", 1) == ERROR) {
					printf("Sending file_ok_length error!\n");
					return ERROR;
				}
			}
		}

		if (Send_Ifon(Socket, File_ok_end, strlen(File_ok_end)) == ERROR) {
			printf("Sending file_ok_end error!\n");
			return ERROR;
		}

		while (file_stat.st_size > 0) {
			if (file_stat.st_size < 1024) {
				send_length = fread(sendbuf, 1, file_stat.st_size, file);
				if (Send_Ifon(Socket, sendbuf, send_length) == ERROR) {
					printf("Sending file information error!\n");
					continue;
				}
				file_stat.st_size = 0;
			}
			else {
				send_length = fread(sendbuf, 1, 1024, file);
				if (Send_Ifon(Socket, sendbuf, send_length) == ERROR) {
					printf("Sending file information error!\n");
					continue;
				}
				file_stat.st_size -= 1024;
			}
		}
	}
	else {
		printf("The file is NULL!\n");
		return ERROR;
	}

	return OK;
}



const char* Judge_File_Type(char* URI, const char* content_type) {
	//文件类型判断 
	const char* suffix;

	if ((suffix = strrchr(URI, '.')) != NULL)
		suffix = suffix + 1;

	if (strcmp(suffix, "html") == 0) {
		return content_type = "Content-type: text/html\r\n";
	}
	else if (strcmp(suffix, "jpg") == 0) {
		return content_type = "Content-type: image/jpg\r\n";
	}
	else if (strcmp(suffix, "png") == 0) {
		return content_type = "Content-type: image/png\r\n";
	}
	else if (strcmp(suffix, "gif") == 0) {
		return content_type = "Content-type: image/gif\r\n";
	}
	else if (strcmp(suffix, "txt") == 0) {
		return content_type = "Content-type: text/plain\r\n";
	}
	else if (strcmp(suffix, "xml") == 0) {
		return content_type = "Content-type: text/xml\r\n";
	}
	else if (strcmp(suffix, "rtf") == 0) {
		return content_type = "Content-type: text/rtf\r\n";
	}
	else if (strcmp(suffix, "js") == 0) {  // 添加对js文件的支持
		return content_type = "Content-type: application/javascript\r\n";
	}
	else if (strcmp(suffix, "css") == 0) {  // 添加对css文件的支持
		return content_type = "Content-type: text/css\r\n";
	}
	else if (strstr(URI, ".mp3") != NULL) {
		content_type = "Content-Type: audio/mpeg\r\n";
	}
	else
		return ERROR;
}

const char* Get_Data(const char* cur_time) {
	//获取Web服务器的当前时间作为响应时间 
	time_t curtime;
	time(&curtime);
	cur_time = ctime(&curtime);

	return cur_time;
}

const char* Post_Value(char* message) {
	//获取客户端POST请求方式的值 
	const char* suffix;

	if ((suffix = strrchr(message, '\n')) != NULL)
		suffix = suffix + 1;
	printf("\n\nPost Value: %s\n\n", suffix);

	return suffix;
}

int Logo() {
	//Web服务器标志信息 
	printf("／￣￣￣￣￣￣￣￣￣\n");
	printf("|　　程序启动了！\n");
	printf("＼\n");
	printf("　￣￣∨￣￣￣￣￣￣\n");
	printf("　 ∧＿∧\n");
	printf("　(　∧_∧)　\n");
	printf("　(　 つつヾ\n");
	printf("　 | ｜ |\n");
	printf("　(＿_)＿)\n");
	printf("\n");
	printf("___________________________________________________________\n\n");

	return OK;
}

int main(int argc, char* argv[]) {
	// 默认端口号
	int port = SERVER_PORT;

	// 如果通过命令行传入端口号，则使用该端口号
	if (argc == 2) {
		port = atoi(argv[1]);
	}

	// 初始化服务器套接字
	SOCKET ServerSock, MessageSock;
	struct sockaddr_in ClientAddr;
	int rval, Length;
	char revbuf[BUF_SIZE];

	Logo();
	printf("Web Server is starting on port %d...\n\n", port);
	ServerSock = Server_Socket_Init(port);
	printf("\n-----------------------------------------------------------\n");

	while (OK) {
		/* 启动监听 */
		rval = listen(ServerSock, BACKLOG);
		if (rval == SOCKET_ERROR) {
			printf("Failed to listen socket!\n");
			system("pause");
			exit(1);
		}
		printf("Listening the socket on port %d...\n", port);

		/* 接受客户端请求建立连接 */
		Length = sizeof(struct sockaddr);
		MessageSock = accept(ServerSock, (SOCKADDR*)&ClientAddr, &Length);
		if (MessageSock == INVALID_SOCKET) {
			printf("Failed to accept connection from client!\n");
			system("pause");
			exit(1);
		}
		printf("Succeed to accept connection from [%s:%d] !\n\n", inet_ntoa(ClientAddr.sin_addr), ntohs(ClientAddr.sin_port));

		/* 接收客户端请求数据 */
		memset(revbuf, 0, BUF_SIZE);	//每一个字节都用0来填充 
		rval = recv(MessageSock, revbuf, BUF_SIZE, 0);
		revbuf[rval] = 0x00;
		if (rval <= 0)
			printf("Failed to receive request message from client!\n");
		else {
			printf("%s\n", revbuf);	//输出请求数据内容
			rval = Handle_Request_Message(revbuf, MessageSock);
		}

		closesocket(MessageSock);
		printf("\n-----------------------------------------------------------\n");
	}

	closesocket(ServerSock);	//关闭套接字 
	WSACleanup();	//停止Winsock

	return OK;
}
