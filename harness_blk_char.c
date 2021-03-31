#include "fuzz_interface.h"
#include "util.h"

int mod_init(void) {
  if(n_request_irqs!=0) {
     start_irqthread_default();
  }
  start_thread_done_default();
  return 0;
}

static int make_devnodes(int *fds) {
   int res;
   char buf[128];
   struct lkl_fuzz_devnode *dn;
   int n_fd = 0;
   int n_dn = lkl_fuzz_get_devnodes(&dn);
   for(int i=0;  i<n_dn; i++) {
      snprintf(buf, 128, "/dev/fdev_%d", i);
      res = lkl_sys_mknod(buf, dn->type | 0600, new_encode_dev(MAJOR(dn->devt), MINOR(dn->devt)));
      fds[n_fd++] = lkl_sys_open(buf, LKL_O_RDWR, 0);
   }
   return n_dn;
}

static int clean_devnodes(int n_dn, int *fds) {
   char buf[128];
   for(int i=0;  i<n_dn; i++) {
      snprintf(buf, 128, "/dev/fdev_%d", i);
      if(fds[i] >= 0) {
         lkl_sys_close(fds[i]);
      }
      lkl_sys_unlink(buf);
   }
   return 0;
}

static int write_sth(int n_dn, int *fds) {
   char buf[64];
   for(int i=0;  i<n_dn; i++) {
      if(fds[i] > 0) {
         lkl_sys_write(fds[i], buf, 64);
      }
   }
   return 0;
}

static int read_sth(int n_dn, int *fds) {
   char buf[64];
   for(int i=0;  i<n_dn; i++) {
      if(fds[i] > 0) {
         lkl_sys_read(fds[i], buf, 64);
      }
   }
   return 1;
}

int mod_fuzz(const uint8_t *data, size_t size) {
  int err = 0, ret = 0;
  int fds[LKL_FUZZ_MAX_DEVT];
  int n_dn;

  start_fuzz(data, size);
  err = init_module();
  if(err!=0) {
    goto out_noinit;
  }

  n_dn = make_devnodes(fds);
  if(n_dn > 0) {
     write_sth(n_dn, fds);
     read_sth(n_dn, fds);
     clean_devnodes(n_dn, fds);
  }
  trigger_one_irq();

  uninit_module();

out_noinit:
  end_fuzz();
  return ret;
}


