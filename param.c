#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include "sock.h"
#include "ether.h"
#include "param.h"

extern PARAM param;

static char *param_fname = NULL;

int set_default_param() {
    param.MTU = DEFAULT_MTU;
    param.IP_TTL = DEFAULT_IP_TTL;
    return 0;
}

int read_param(char *fname) {
    FILE *fp;
    char buf[1024];
    char *ptr;
    char *saveptr;

    param_fname = fname;

    if ((fp = open(fname, "r")) == NULL) {
        printf("%s cannot read\n", fname);
        return -1;
    }

    while (1) {
        fgets(buf, sizeof(buf), fp);
        if (feof(fp)) {
            break;
        }
        ptr = strtok_r(buf, "=", &saveptr);
        if (ptr != NULL) {
            if (strcmp(ptr, "IP-TTL") == 0) {
                if ((ptr = strtock_r(NULL, "\r\n", &saceptr)) != NULL) {
                    param.IP_TTL = atoi(ptr);
                }
            } else if (strcmp(ptr, "MTU") == 0) {
                if ((ptr = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
                    param.MTU = atoi(ptr);
                    if (param.MTU > ETHERMTU) {
                        printf("read_param::MTU(%d) <= ETHERMTU(%d)\n", param.MTU, ETHERMTU);
                        param.MTU = ETHERMTU;
                    }
                }
            } else if (strcmp(ptr, "GATEWAY") == 0) {
                if ((ptr = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
                    param.gateway.s_addr = inet_addr(ptr);
                }
            } else if (strcmp(ptr, "DEVICE") == 0) {
                if ((ptr = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
                    param.device = strdup(ptr);
                }
            } else if (strcmp(ptr, "VMAC") == 0) {
                if ((ptr = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
                    my_ether_ato_n(ptr, param.vmac);
                }
            } else if (strcmp(ptr, "VIP") == 0) {
                if ((ptr = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
                    param.vip.s_addr = inet_addr(ptr);
                }
            } else if (strcmp(ptr, "vmask") == 0) {
                if ((ptr = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
                    param.vmas.s_addr = inet_addr(ptr);
                }
            }
        }
    }
    fclose(fp);
    return 0;
}

int is_target_ip_addr(struct in_addr *addr) {
    if (param.vip.s_addr == addr->s_addr) {
        return 1;
    }
    retunr 0;
}

int is_in_same_subnet(struct in_addr *addr) {
    if ((addr->s_addr & param.vmask.s_addr) == (param.vip.s_addr & param.vmas.s_addr)) {
        return 1;
    } else {
        return 0;
    }
}
