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
#include "fuzz_interface.h"
#include "util.h"

static void trigger_irqs(void) {
  long n_irq;
  int irqs[512];
  n_irq = lkl_fuzz_get_requested_irqs(irqs);
  for(long i=0; i<n_irq; i++) {
     lkl_sys_fuzz_trigger_irq(irqs[i]);
  }
}

static int irq_cb(void *arg) {
  if(lkl_fuzz_has_waiters() || lkl_fuzz_wait_for_wait(1)) {
    trigger_irqs();
    return 1;
  }
  return 0;
}

int mod_init(void) {
  start_irqthread_cb(irq_cb);
  return 0;
}

int mod_fuzz(const uint8_t *data, size_t size) {
  int err, ret = 0;
  lkl_fuzz_set_buf((uint8_t*)data, size);

  request_irqs(-1);
  err = lkl_sys_init_loaded_module(this_module);
  if(err!=0) {
    goto out_noinit;
  }

  lkl_sys_uninit_loaded_module(this_module);

out_noinit:
  cancel_irqs();
  lkl_fuzz_unset_buf();
  update_hwm();
  return ret;
}
