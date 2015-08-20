
#include <list>
#include "util.h"
#include <cstring>
#include <Windows.h>
#include "host.h"
#include <errno.h>
using namespace std;

#include "resource.h"

#define SERVER_PORT 7799

#define WM_SOCKET_NOTIFY (WM_USER + 1)

#define CGI_SOCKET_NOTIFY (WM_USER + 2)

BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
int EditPrintf(HWND, TCHAR *, ...);
//=================================================================
//	Global Variables
//=================================================================
list<SOCKET> Socks;

host_t* hosts[6];

int all_clear(host_t** hosts, int len) {
	int c;
	for (c = 0; c < len; c++) {
		if (hosts[c] != NULL) {
			return -1;
		}
	}
	return 1;
}

void process_qstring(char* query) {

	int c;
	int attr_count;
	char** attrs;
	str_split(query, "&", &attrs, &attr_count);


	for (c = 0; c < 6; c++) {
		hosts[c] = NULL;
	}


	for (c = 0; c<attr_count; c++) {
		if (strlen(attrs[c]) > 3) {
			int num_of_host = attrs[c][1] - '0';
			if (attrs[c][0] == 'h') {
				char hostname[17];
				memset(hostname, 0, 17);

				strcpy(hostname, &attrs[c][3]);
				create_host(&hosts[num_of_host], hostname, 0, "");
			}
			else if (attrs[c][0] == 'p') {
				hosts[num_of_host]->port = atoi(&attrs[c][3]);
			}
			else {
				strcpy(hosts[num_of_host]->filename, &attrs[c][3]);
				hosts[num_of_host]->host_file = fopen(hosts[num_of_host]->filename, "r");
			}
		}
	}

}

void send_cgi_header(SOCKET ev_sc) {
	char text_to_send[4096];
	int c;
	sprintf(text_to_send, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
	send(ev_sc, text_to_send, strlen(text_to_send), 0);

	sprintf(text_to_send, "<html>\n<head>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" />\n<title>Network Programming Homework 3</title>\n</head>\n<body bgcolor=#336699>\n<font face=\"Courier New\" size=2 color=#FFFF99>\n<table width=\"800\" border=\"1\">\n<tr>\n");
	send(ev_sc, text_to_send, strlen(text_to_send), 0);

	for (c = 1; c <= 5; c++) {
		if (hosts[c] != NULL) {
			sprintf(text_to_send, "<td>%s</td>\n", hosts[c]->hostname);
			send(ev_sc, text_to_send, strlen(text_to_send), 0);
		}
	}
	sprintf(text_to_send, "</tr>\n<tr>\n");
	send(ev_sc, text_to_send, strlen(text_to_send), 0);
	for (c = 1; c <= 5; c++) {
		if (hosts[c] != NULL) {
			sprintf(text_to_send, "<td valign=\"top\" id=\"m%d\"></td>\n", c - 1);
			send(ev_sc, text_to_send, strlen(text_to_send), 0);
		}
	}
	sprintf(text_to_send, "</tr>\n</table>\n");
	send(ev_sc, text_to_send, strlen(text_to_send), 0);
}

void connect_hosts(HWND hwnd) {
	int c;
	for (c = 1; c <= 5; c++) {
		if (hosts[c] == NULL) {
			continue;
		}
		SOCKET new_sc = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in sa;
		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_port = htons(hosts[c]->port);
		sa.sin_addr.s_addr = inet_addr(hosts[c]->hostname);

		int err = WSAAsyncSelect(new_sc, hwnd, CGI_SOCKET_NOTIFY, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);

		if (err == SOCKET_ERROR) {
			hosts[c] = NULL;
			return;
		}

		if (connect(new_sc, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
			if (errno != EINPROGRESS && errno != 0) {
				hosts[c] = NULL;
			}
		}

		if (hosts[c] != NULL) {
			hosts[c]->server_socket = new_sc;
		}
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{

	return DialogBox(hInstance, MAKEINTRESOURCE(ID_MAIN), NULL, MainDlgProc);
}

BOOL CALLBACK MainDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	WSADATA wsaData;

	static HWND hwndEdit;
	static SOCKET msock, ssock;
	static struct sockaddr_in sa;
	list<SOCKET> to_remove;
	int err;



	if (Message == WM_INITDIALOG){
		hwndEdit = GetDlgItem(hwnd, IDC_RESULT);
	}
	else if (Message == WM_COMMAND){

		if (LOWORD(wParam) == ID_LISTEN){

			WSAStartup(MAKEWORD(2, 0), &wsaData);

			//create master socket
			msock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			if (msock == INVALID_SOCKET) {
				EditPrintf(hwndEdit, TEXT("=== Error: create socket error ===\n"));
				WSACleanup();
				return TRUE;
			}

			err = WSAAsyncSelect(msock, hwnd, WM_SOCKET_NOTIFY, FD_ACCEPT | FD_CLOSE | FD_READ | FD_WRITE);

			if (err == SOCKET_ERROR) {
				EditPrintf(hwndEdit, TEXT("=== Error: select error ===\n"));
				closesocket(msock);
				WSACleanup();
				return TRUE;
			}

			//fill the address info about server
			sa.sin_family = AF_INET;
			sa.sin_port = htons(SERVER_PORT);
			sa.sin_addr.s_addr = INADDR_ANY;

			//bind socket
			err = bind(msock, (LPSOCKADDR)&sa, sizeof(struct sockaddr));

			if (err == SOCKET_ERROR) {
				EditPrintf(hwndEdit, TEXT("=== Error: binding error ===\n"));
				WSACleanup();
				return FALSE;
			}

			err = listen(msock, 2);

			if (err == SOCKET_ERROR) {
				EditPrintf(hwndEdit, TEXT("=== Error: listen error ===\n"));
				WSACleanup();
				return FALSE;
			}
			else {
				EditPrintf(hwndEdit, TEXT("=== Server START ===\n"));
			}

		}
		else if (LOWORD(wParam) == ID_EXIT){
			EndDialog(hwnd, 0);
		}

	}
	else if (Message == WM_CLOSE){
		EndDialog(hwnd, 0);
	}
	else if (Message == WM_SOCKET_NOTIFY) {
		if (WSAGETSELECTEVENT(lParam) == FD_ACCEPT){
			ssock = accept(msock, NULL, NULL);
			Socks.push_back(ssock);
			EditPrintf(hwndEdit, TEXT("=== Accept one new client(%d), List size:%d ===\n"), ssock, Socks.size());
		}
		else if (WSAGETSELECTEVENT(lParam) == FD_READ){
			//Write your code for read event here.
			SOCKET ev_sc = wParam;
			char buf[4096];
			memset(buf, 0, 4096);
			int r = recv(ev_sc, buf, 4096, 0);
			if (r == 0 || (r == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)) {
				return FALSE;
			}
			EditPrintf(hwndEdit, TEXT("r:%d\n"), r);
			char** tokens;
			int num_tokens;
			str_split(buf, "\n", &tokens, &num_tokens);
			char get_cmd[4096];
			memset(get_cmd, 0, 4096);
			strcpy(get_cmd, tokens[0]);
			// dirt remove '\r'
			get_cmd[strlen(get_cmd) - 1] = '\0';
			EditPrintf(hwndEdit, TEXT("request:\n%s\n"), get_cmd);

			str_split(get_cmd, " ", &tokens, &num_tokens);
			if (num_tokens < 3) {
				EditPrintf(hwndEdit, TEXT("Error, query str is empty"));
				return FALSE;
			}

			char q_string[4096];
			memset(q_string, 0, 4096);
			strcpy(q_string, tokens[1]);
			EditPrintf(hwndEdit, TEXT("Query String: %s\n"), q_string);

			if (is_match(q_string, ".*cgi.*") == 1) {
				// cgi
				// dirty remove "/hw3.cgi"
				int d;
				while (q_string[0] != '?') {
					for (d = 0; d < strlen(q_string); d++) {
						q_string[d] = q_string[d + 1];
					}
				}
				for (d = 0; d < strlen(q_string); d++) {
					q_string[d] = q_string[d + 1];
				}
				// ^ dirty remove "/hw3.cgi"

				process_qstring(q_string);
				send_cgi_header(ev_sc);
				connect_hosts(hwnd);

			}
			else {
				char text_to_send[4096];


				// dirt remove '/'
				int c;
				for (c = 0; q_string[c + 1] != '\0'; q_string[c] = q_string[c + 1], c++);
				q_string[c] = '\0';
				FILE* html_file = fopen(q_string, "r");
				if (html_file == NULL) {
					// 404 error
					sprintf(text_to_send, "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n");
					send(ev_sc, text_to_send, strlen(text_to_send), 0);
					closesocket(ev_sc);
					to_remove.push_back(ev_sc);
				}
				else {
					sprintf(text_to_send, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
					send(ev_sc, text_to_send, strlen(text_to_send), 0);
					while (fgets(text_to_send, 4096, html_file) != 0) {
						send(ev_sc, text_to_send, strlen(text_to_send), 0);
					}
					closesocket(ev_sc);
					to_remove.push_back(ev_sc);
				}

			}

		}
		else if (WSAGETSELECTEVENT(lParam) == FD_WRITE){
			//Write your code for write event here
		}
		else if (WSAGETSELECTEVENT(lParam) == FD_CLOSE){
		}



	}
	else if (Message == CGI_SOCKET_NOTIFY){
		if (WSAGETSELECTEVENT(lParam) == FD_READ){
			for (int c = 1; c <= 5; c++) {
				if (hosts[c] != NULL && hosts[c]->is_connect == 1) {
					SOCKET sc = hosts[c]->server_socket;

					char buf[4096];
					memset(buf, 0, 4096);

					int r = recv(sc, buf, 4096, 0);
					if (r <= 0) {
						continue;
					}

					char** res;
					int num_lines;

					str_split(buf, "\n", &res, &num_lines);


					for (int d = 0; d < num_lines; d++) {
						strcpy(buf, res[d]);
						if (strncmp(buf, "% ", 2) == 0) {

							char next_cmd[4096];
							memset(next_cmd, 0, 4096);
							fgets(next_cmd, 4096, hosts[c]->host_file);

							send(hosts[c]->server_socket, next_cmd, strlen(next_cmd), 0);
							replace_to_html(next_cmd);


							if (strncmp(next_cmd, "exit", 4) == 0) {
								closesocket(hosts[c]->server_socket);
								hosts[c] = NULL;
							}

							for (list<SOCKET>::iterator it = Socks.begin(); it != Socks.end(); ++it) {
								memset(buf, 0, 4096);
								sprintf(buf, "<script>document.all['m%d'].innerHTML += \"%% <b>%s</b>\";</script>\n", c - 1, next_cmd);
								SOCKET web_sc = *it;
								send(web_sc, buf, strlen(buf), 0);

							}
						}
						else {
							for (list<SOCKET>::iterator it = Socks.begin(); it != Socks.end(); ++it) {
								char text_to_send[4096];
								sprintf(text_to_send, "<script>document.all['m%d'].innerHTML += \"%s<br>\";</script>\n", c - 1, buf);
								SOCKET web_sc = *it;
								send(web_sc, text_to_send, strlen(text_to_send), 0);
							}
						}

					}

				}
			}

			if (all_clear(hosts, 6) == 1) {
				for (list<SOCKET>::iterator it = Socks.begin(); it != Socks.end(); ++it) {
					to_remove.push_back(*it);
					closesocket(*it);
				}
			}

		}
		else if (WSAGETSELECTEVENT(lParam) == FD_CONNECT) {
			for (int c = 1; c <= 5; c++) {
				if (hosts[c] != NULL && hosts[c]->server_socket == wParam) {
					EditPrintf(hwndEdit, TEXT("Connect from host %d\n"), c);
					hosts[c]->is_connect = 1;
				}
			}
		}
	}
	else {
		return FALSE;
	}

	while (to_remove.size() != 0) {
		SOCKET sc = to_remove.front();
		to_remove.pop_front();
		Socks.remove(sc);
	}
	return TRUE;
}

int EditPrintf(HWND hwndEdit, TCHAR * szFormat, ...)
{
	TCHAR   szBuffer[4096];
	va_list pArgList;

	va_start(pArgList, szFormat);
	wvsprintf(szBuffer, szFormat, pArgList);
	va_end(pArgList);

	SendMessage(hwndEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
	SendMessage(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)szBuffer);
	SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
	return SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
}