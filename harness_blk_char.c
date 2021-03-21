#define _GNU_SOURCE
#include <time.h>
#include <dlfcn.h>
#include <elf.h>
#include <errno.h>
#include <sys/personality.h>
#include <fcntl.h>
#include <link.h>
#include <lkl.h>
#include <lkl_host.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include "fuzz_interface.h"
#include "util.h"

static pthread_mutex_t ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ready_cond = PTHREAD_COND_INITIALIZER;
static int unload_module = 0;
static int mod_loaded = 0;

int done_cb(void) {
  pthread_mutex_lock(&ready_mutex);
  unload_module = 1;
  pthread_cond_signal(&ready_cond);
  pthread_mutex_unlock(&ready_mutex);
  return 0;
}

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

void* thread_done_cb(void *arg)
{
  int err;
  while(true) {
     pthread_mutex_lock(&ready_mutex);
     while(!unload_module) {
        pthread_cond_wait(&ready_cond, &ready_mutex);
     }
     pthread_mutex_unlock(&ready_mutex);
     if(__atomic_exchange_n(&mod_loaded, 0, __ATOMIC_SEQ_CST)) {
        do {
           err = lkl_sys_uninit_loaded_module(this_module);
           if(err!=0) {
              trigger_irqs_virtio();
              trigger_irqs_pci();
           }
        } while(err);
     }
     unload_module = 0;
  }
  return NULL;
}

static pthread_t tid;
void start_thread_done_cb(void) {
  pthread_create(&tid, NULL, &thread_done_cb, NULL);
}

static int irq_cb(void *arg) {
  if(lkl_fuzz_has_waiters() || lkl_fuzz_wait_for_wait(1)) {
    if(devtype==1) { // VIRTIO
      trigger_irqs_virtio();
    }
    trigger_irqs_pci();
    return 1;
  }
  return 0;
}

int mod_init(void) {
  if(n_request_irqs>0) {
     start_irqthread_cb(irq_cb);
  }
  lkl_fuzz_set_done_callback(done_cb);
  start_thread_done_cb();
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
   lkl_fuzz_get_n(buf, 64);
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
  request_irqs(-1);
  lkl_fuzz_set_buf((uint8_t*)data, size);

  err = lkl_sys_init_loaded_module(this_module);
  if(err!=0) {
    goto out_noinit;
  }
  __atomic_store_n (&mod_loaded, 1, __ATOMIC_SEQ_CST);

  if(devtype==1) trigger_irqs_virtio();
  trigger_irqs_pci();
  n_dn = make_devnodes(fds);
  if(n_dn > 0) {
     write_sth(n_dn, fds);
     read_sth(n_dn, fds);
     clean_devnodes(n_dn, fds);
  }
  if(__atomic_exchange_n(&mod_loaded, 0, __ATOMIC_SEQ_CST)) {
     do {
        err = lkl_sys_uninit_loaded_module(this_module);
        if(err!=0) {
           trigger_irqs_virtio();
           trigger_irqs_pci();
        }
     } while(err!=0);
  }

out_noinit:
  while(unload_module) {
    struct timespec sleepValue = {0};
    sleepValue.tv_nsec = DRAIN_DELAY_NS;
    nanosleep(&sleepValue, NULL);
  }
  cancel_irqs();
  lkl_fuzz_unset_buf();
  update_hwm();
  return ret;
}

