#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <netinet/ip_icmp.h>
#include <netinet/if_ether.h>
#include <linux/if.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <pthread.h>

#include "sock.h"
#include "ether.h"
#include "arp.h"
#include "ip.h"
#include "icmp.h"
#include "param.h"
#include "cmd.h"

// 終了フラグ
int end_flag = 0;

// 送受信するPF_PACKETのディスクリプタを格納する辺数
int device_sock;

// 設定を保持する構造体．param.hにて定義
PARAM param;

// ネットワークインタフェースを監視するスレッド
void *my_eth_thread(void *arg){
    int nready;
    struct pollfd targets[1];
    u_int8_t buf[2048];
    int len;

    targets[0].fd = device_sock;
    targets[0].events = POLLIN|POLLERR;

    while (end_flag == 0) {
        switch ((nready = poll(targets, 1, 1000))) {
        case -1:
            if(errno != EINTR) {
                perror("poll");
            }
            break;
        case 0:
            break;
        default:
            if(targets[0].revents & (POLLIN|POLLERR)) {
                if ((len = read(device_Sock, buf, sizeof(buf))) <= 0) {
                    perror("read");
                } else {
                    ether_recv(device_sock, buf, len);
                }
            }
            break;
        }
    }
    return NULL;
}

// 標準入力を監視するスレッド
void *stdin_thread(void *arg) {
    int nready;
    struct pollfd targets[1];
    char buf[2048];

    targets[0].fd = fileno(stdin);
    targets[0].events = POLLIN|POLLERR;

    while (end_flag == 0) {
        switch ((nready = poll(targets, 1, 1000))) {
        case -1:
            if (errno != EINTR) {
                perror("poll");
            }
            break;
        case 0:
            break;
        default:
            if (targets[0].revents & (POLLIN|POLLERR)) {
                fgets(buf, sizeof(buf), stdin);
                do_cmd(buf);
            }
            break;
        }
    }
    return NULL;
}

// シグナルハンドラ
void sig_term(int sig) {
    end_flag = 1;
}

// main()終了時処理
int ending() {
    struct ifreq if_req;
    printf("ending\n");

    if (device_sock != -1) {
        strcpy(if_req.ifr_name, param.device);
        if (ioctl(device_sock, SIOCGIFFLAGS, &if_req) < 0) {
            perror("ioctl");
        }

        if_req.ifr_flags = if_req.ifr_flags & ~IFF_PROMISC;
        if (ioctl(device_sock, SIOCSIFFLAGS, &if_req) < 0) {
            perror("ioctl");
        }

        close(device_sock);
        device_sock = -1;
    }
    return 0;
}

int show_ifreq(char *name) {
    char buf1[80];
    int sock;
    struct ifreq ifreq;
    struct sockaddr_in addr;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    strcpy(ifreq.ifr_name, name);

    if(ioctl(sock, SIOCGIFFLAGS, &ifreq) == -1) {
        perror("ioctl");
        close(sock);
        return -1;
    }

    if (ifreq.ifr_flags & IFF_UP) {
        printf("UP ");
    }
    if (ifreq.ifr_flags & IFF_BROADCAST) {
        printf("BROADCAST ");
    }
    if (ifreq.ifr_flags & IFF_PROMISC) {
        printf("PROMISC ");
    }
    if (ifreq.ifr_flag & IFF_LOOPBACK) {
        printf("LOOPBACK ");
    }
    if (ifreq.ifr_flag & IFF_POINTOPOINT) {
        printf("P2P ");
    }
    printf("\n");

    if (ioctl(sock, SIOCGIFMTU, &ifreq) == -1) {
        perror("ioctl::mtu");
    } else {
        printf("mtu = %d\n", ifreq.ifr_mtu);
    }

    if (ioctl(sock, SIOCGIFADDR, &ifreq) == -1) {
        perror("ioctl::addr");
    } else if (ifreq.ifr_addr.sa_family != AF_INET) {
        printf("NOT AF_INET\n");
    } else {
        memcpy(&addr, &ifreq.ifr_addr, sizeof(struct sockaddr_in));
        printf("myup = %s\n", inet_ntop(AF_INET, &addr.sin_addr, buf1, sizeof(buf1)));
        param.myip = addr.sin_addr;
    }

    close(sock);

    if (get_mac_address(name, param.mymac) == -1) {
        printf("get_mack_address");
    } else {
        printf("mymac = %s\n", my_ether_ntoa_r(param.mymac, buf1));
    }

    return 0;
}

int main(int argc, char *argv[]) {
    char buf1[80];
    int i;
    int param_flag;
    pthread_attr_t attr;
    pthread_t thread_id;

    set_default_param();

    param_flag = 0;
    for (i = 0; i < argc; i++) {
        if (read_param(argv[1]) == -1) {
            exit(-1);
        }
        param_flag = 1;
    }
    if (param_flag == 0) {
        if (read_param("./my-tcp-ip.conf") == -1) {
            exit(-1);
        }
    }

    printf("divice = %s\n", param.device);
    printf("++++++++++++++++++++++++++++++++++++++++\n");
    show_ifreq(param.device);
    printf("++++++++++++++++++++++++++++++++++++++++\n");

    printf("vmac = %s\n", my_ether_ntoa_r(param.vmac, buf1));
    printf("vip = %s\n", inet_ntop(AF_INET, &param.vip, buf1, sizeof(buf1)));
    printf("vmas = %s\n", inet_ntop(AF_INET, &param.vmask, buf1, sizeof(buf1)));
    printf("gateway = %s\n", inet_ntop(AF_INET, &param.gateway, buf1, sizeof(buf1)));

    signal(SIGINT, sig_term);
    signal(SIGTERM, sig_term);
    signal(SIGQUIT, sig_term);
    signal(SIGPIPE, SIG_IGN);

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 102400);
    pthread_attr_setdetachstate(&attrm PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread_id, &attr, my_eth_thread, NULL) != 0) {
        printf("pthread_create::error\n");
    }
    if (pthread_create(&thread_id, &attr, stdin_thread, NULL) != 0) {
        printf("pthread_create::error\n");
    }
    if (arp_check_garp(device) == 0) {
        printf("failed: garp check\n");
        return -1;
    }

    while (end_flag == 0) {
        sleep(1);
    }

    ending();

    return 0;
}
