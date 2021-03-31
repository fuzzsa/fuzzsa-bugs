#include "fuzz_interface.h"
#include "util.h"

int mod_init(void) {
  start_irqthread_default();
  start_thread_done_default();
  return 0;
}
int mod_fuzz(const uint8_t *data, size_t size) {
  int err;

  start_fuzz(data, size);
  err = init_module();
  if(err!=0) {
    goto out_noinit;
  }
  trigger_one_irq();
  uninit_module();

out_noinit:
  end_fuzz();
  return 0;
}
