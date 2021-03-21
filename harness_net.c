#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <net/route.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/personality.h>
#include <fcntl.h>
#include <link.h>
#include <lkl.h>
#include <lkl_host.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include "fuzz_interface.h"
#include "util.h"

static void trigger_irqs_virtio(void) {
  for(int qidx=0; qidx<nqueues; qidx++) {
    lkl_virtio_send_to_queue(fuzz_dev_handle, qidx);
  }
}

static void trigger_irqs_pci(void) {
  long n_irq;
  int irqs[512];
  n_irq = lkl_fuzz_get_requested_irqs(irqs);
  for(long i=0; i<n_irq; i++) {
     lkl_sys_fuzz_trigger_irq(irqs[i]);
  }
}

static void trigger_irqs(void) {
  if(devtype==1) { // VIRTIO
    trigger_irqs_virtio();
  } else {
    trigger_irqs_pci();
  }
}

static int irq_cb(void *arg) {
  if(lkl_fuzz_has_waiters() || lkl_fuzz_wait_for_wait(1)) {
    trigger_irqs();
    return 1;
  }
  return 0;
}

static unsigned short in_cksum(const u_short *addr, register int len, u_short csum)
{
  int nleft = len;
  const u_short *w = addr;
  u_short answer;
  int sum = csum;
  while (nleft > 1)  {
    sum += *w++;
    nleft -= 2;
  }
  if (nleft == 1)
    sum += htons(*(u_char *)w << 8);
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  answer = ~sum;
  return answer;
}

static struct lkl_icmphdr icmp;
int mod_init(void) {
  if(devtype != 1 || n_request_irqs>0) {
     start_irqthread_cb(irq_cb);
  }
  icmp.type = LKL_ICMP_ECHO;
  icmp.code = 0;
  icmp.checksum = 0;
  icmp.un.echo.sequence = 0;
  icmp.un.echo.id = 0;
  icmp.checksum = in_cksum((u_short *)&icmp, sizeof(icmp), 0);
  return 0;
}

static int test_icmp(void)
{
  int sock, ret;
  struct lkl_sockaddr_in saddr;
  char buf[32];

  memset(&saddr, 0, sizeof(saddr));
  ret = inet_aton("192.168.0.3", (struct in_addr*)&saddr.sin_addr.lkl_s_addr);
  if(ret != 1) {
    return -1;
  }
  saddr.sin_family = AF_INET;

  sock = lkl_sys_socket(LKL_AF_INET, LKL_SOCK_RAW, LKL_IPPROTO_ICMP);
  if (sock < 0) {
    return -1;
  }

  ret = lkl_sys_sendto(sock, &icmp, sizeof(icmp), 0,
      (struct lkl_sockaddr*)&saddr,
      sizeof(saddr));
  if (ret < 0) {
    lkl_sys_close(sock);
    return -1;
  }

  ret = lkl_sys_recv(sock, buf, sizeof(buf), LKL_MSG_DONTWAIT);
  lkl_sys_close(sock);

  return ret;
}

int mod_fuzz(const uint8_t *data, size_t size) {
  int err;
  int ifidx;
  int this_mtu;
  lkl_fuzz_set_buf((uint8_t*)data, size);

  request_irqs(-1);
  err = lkl_sys_init_loaded_module(this_module);
  if(err!=0) {
    goto out_noinit;
  }

  ifidx = lkl_ifname_to_ifindex(interface_ptr);
  if(ifidx < 0) {
     goto out;
  }
  lkl_fuzz_get_n(&this_mtu, sizeof(this_mtu));
  lkl_if_set_mtu(ifidx, this_mtu);
  lkl_if_up(ifidx);
  lkl_if_set_ipv4(ifidx, inet_addr("192.168.0.2"), 24);
  lkl_set_ipv4_gateway(inet_addr("192.168.0.1"));

  test_icmp();
  trigger_irqs();
out:
  lkl_sys_uninit_loaded_module(this_module);

out_noinit:
  cancel_irqs();
  lkl_fuzz_unset_buf();
  update_hwm();
  return 0;
}

