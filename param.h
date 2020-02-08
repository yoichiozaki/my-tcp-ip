#define DEFAULT_MTU (ETHERMTU)
#define DEFAULT_IP_TTL (64)
#define DEFAULT_PING_SIZE (64)
#define DUMMY_WAIT_MS (100)
#define RETRY_COUNT (3)

typedef struct {
    char *device;
    u_int8_t mymac[6]; // 6 bytes for mac address
    struct in_addr myip; // ip address
    u_int8_t vmac[6];
    struct in_addr vip;
    struct in_addr vmask;
    int IP_TTL;
    int MTU;
    struct in_addr gateway;
} PARAM;

int set_default_param();
int read_param(char *fname);
int is_target_ip_addr(struct in_addr *addr);
int is_in_same_subnet(struct in_addr *addr);
