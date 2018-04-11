#include "stdafx.h"
#include <string.h>
#include <thread>
#define DEFAULT_PORT 5557
#define CONNECTION_QUEUE 100

bool parse_cmd(int argc, char* argv[], char* host, short* port);
void handle_connection(SOCKET, sockaddr_in*);
bool is_prime(int);
void error_msg(const char*);
void exit_handler();

SOCKET server_socket;
int main(int argc, char* argv[])
{
	atexit(exit_handler);
	short port = DEFAULT_PORT;
	char host[128] = "";
	bool parse_cmd_result = parse_cmd(argc, argv, host, &port);

	WSADATA ws;
	if (WSAStartup(MAKEWORD(2, 2), &ws)) {
		error_msg("Error init of WinSock2");
		return -1;
	}

	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket <= 0) {
		error_msg("Can't create socket");
		return -1;
	}

	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	if (parse_cmd && strlen(host) > 0) {
		server_addr.sin_addr.s_addr = inet_addr(host);
	}
	else {
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	if (bind(server_socket, (sockaddr*)&server_addr, sizeof(sockaddr))) {
		char err_msg[128] = "";
		sprintf(err_msg, "Can't bind socket to the port %d", port);
		error_msg(err_msg);
		return -1;
	}

	if (listen(server_socket, CONNECTION_QUEUE)) {
		error_msg("Error listening socket");
		return -1;
	}

	printf("Server running at the port %d\n", port);

	while (true)
	{
		sockaddr_in incom_addr;
		memset(&incom_addr, 0, sizeof(incom_addr));
		int len = sizeof(incom_addr);
		SOCKET socket = accept(server_socket, (sockaddr*)&incom_addr, &len);
		if (socket <= 0) {
			error_msg("Can't accept connection");
			return -1;
		}

		try
		{
			std::thread thr(handle_connection, std::ref(socket), &incom_addr);
			thr.detach();
		}
		catch (const std::exception &ex)
		{
			printf(ex.what());
		}
	}
	closesocket(server_socket);

	return 0;
}

bool parse_cmd(int argc, char* argv[], char* host, short* port)
{
	if (argc < 2) {
		return false;
	}

	char all_args[256];
	memset(all_args, 0, sizeof all_args);

	for (int i = 1; i < argc; ++i) {
		strcat(all_args, argv[i]);
		strcat(all_args, " ");
	}


	const int count_vars = 3;
	const int host_buf_sz = 128;
	int tmp_ports[count_vars] = { -1, -1, -1 };
	char tmp_hosts[count_vars][host_buf_sz];
	for (int i = 0; i < count_vars; ++i) {
		memset(tmp_hosts[i], 0, host_buf_sz);
	}
	char* formats[count_vars] = { "-h %s -p %d", "-p %d -h %s", "-p %d" };

	int results[] = {
		sscanf(all_args, formats[0], tmp_hosts[0], &tmp_ports[0]) - 2,
		sscanf(all_args, formats[1], &tmp_ports[1], tmp_hosts[1]) - 2,
		sscanf(all_args, formats[2], &tmp_ports[2]) - 1
	};

	for (int i = 0; i < sizeof(results) / sizeof(int); ++i) {
		if (!results[i]) {
			if (strlen(tmp_hosts[i]) > 0) {
				strcpy(host, tmp_hosts[i]);
			}
			if (tmp_ports[i] > 0) {
				*port = (short)tmp_ports[i];
				return true;
			}
		}
	}

	return false;

}

void handle_connection(SOCKET socket, sockaddr_in* addr) {
	FILE *f;
	int i = 1;
	char* str_in_addr = inet_ntoa(addr->sin_addr);
	printf("[%s]>>%s\n", str_in_addr, "Establish new connection\n");
	std::string fileName = "forRecive-[" + std::string(str_in_addr) + "].txt";
	f = fopen(fileName.c_str(), "ab");
	char response[25] = "ready";

	int rc = 0;
	do {
		char buf[4 * 1024];
		memset(buf, 0, sizeof(buf));
		rc = recv(socket, buf, sizeof(buf), 0);
		if (rc > 0) {
			fwrite(buf, sizeof(char), rc, f);
			printf("receive bytes: %d, part: %d\n", rc, i);
			rc = send(socket, response, strlen(response), 0);
			if (rc <= 0)
			{
				printf("Can't send response to client %s", str_in_addr);
				break;
			}
			i++;

		}
		else {
			break;
		}
	} while (rc > 0);
	fclose(f);
	closesocket(socket);
	printf("[%s]>>%s", str_in_addr, "Close incomming connection\n");
}

bool is_prime(int n)
{
	for (int i = 2; i < n; ++i)
	{
		if (n % i == 0)
			return false;
	}
	return true;
}

void error_msg(const char* msg) {
	printf("%s\n", msg);
}

void exit_handler()
{
	closesocket(server_socket);
	WSACleanup();
}
