#include "fuzz_interface.h"
#include "util.h"

static char buf[32];
static struct lkl_icmphdr icmp;
static struct lkl_ifreq ifr;
static int nl_sock;
static int raw_sock;
static int sock;
static unsigned int addr;
static struct lkl_sockaddr_in saddr;
int mod_init(void) {
  int err;
  if(n_request_irqs!=0) {
     start_irqthread_default();
  }
  start_thread_done_default();
  icmp.type = LKL_ICMP_ECHO;
  icmp.code = 0;
  icmp.checksum = 0;
  icmp.un.echo.sequence = 0;
  icmp.un.echo.id = 0;
  strcpy(ifr.lkl_ifr_name, interface_ptr);
  sock = lkl_sys_socket(LKL_AF_INET, LKL_SOCK_DGRAM, 0);
  if (sock < 0) {
     return -1;
  }
  nl_sock = lkl_netlink_sock(0);
  if (nl_sock < 0) {
     return -1;
  }
  raw_sock = lkl_sys_socket(LKL_AF_INET, LKL_SOCK_RAW, LKL_IPPROTO_ICMP);
  if (raw_sock < 0) {
     return -1;
  }
  addr = inet_addr("192.168.0.2");
  memset(&saddr, 0, sizeof(saddr));
  err = inet_aton("192.168.0.2", (struct in_addr*)&saddr.sin_addr.lkl_s_addr);
  if(err != 1) {
     return -1;
  }
  return 0;
}

int mod_fuzz(const uint8_t *data, size_t size) {
  int err, this_mtu;

  start_fuzz(data, size);
  err = init_module();
  if(err!=0) {
    goto out_noinit;
  }

  err = lkl_sys_ioctl(sock, LKL_SIOCGIFINDEX, (long)&ifr);
  if (err < 0) {
     goto out;
  }

  lkl_fuzz_get_n(&this_mtu, sizeof(this_mtu));
  ifr.lkl_ifr_mtu = this_mtu;
  lkl_sys_ioctl(sock, LKL_SIOCSIFMTU, (long)&ifr); // can fail, is ok
  ifr.lkl_ifr_flags |= LKL_IFF_UP;
  err = lkl_sys_ioctl(sock, LKL_SIOCSIFFLAGS, (long)&ifr);
  if (err < 0) {
     goto out;
  }
  err = ipaddr_modify_sock(LKL_RTM_NEWADDR, LKL_NLM_F_CREATE | LKL_NLM_F_EXCL, ifr.lkl_ifr_ifindex, LKL_AF_INET, &addr, 24, nl_sock);
  if (err < 0) {
     goto out;
  }
  lkl_sys_sendto(raw_sock, &icmp, sizeof(icmp), 0,(struct lkl_sockaddr*)&saddr, sizeof(saddr));

out:
  trigger_one_irq();
  uninit_module();

out_noinit:
  end_fuzz();
  return 0;
}

