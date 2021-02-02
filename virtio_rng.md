# Driver: virtio_rng

# Bugs

## 1. Buffer overflow when mixing bytes into the entropy pool

Overflow would happen in https://elixir.bootlin.com/linux/v5.10.11/source/drivers/char/random.c#L2312 where the length is device-controlled.

Root cause:
A buffer of size 64 is allocated in https://elixir.bootlin.com/linux/v5.10.11/source/drivers/char/hw_random/core.c#L621
When the device updates the used ring, the device can specify a larger length which will be propagated to set `dava_avail` in https://elixir.bootlin.com/linux/v5.10.11/source/drivers/char/hw_random/virtio-rng.c#L35.
When `rng_get_data` finally returns in https://elixir.bootlin.com/linux/v5.10.11/source/drivers/char/hw_random/core.c#L442, the value of `rc` is device-controlled and would be passed to `add_hwgenerator_randomness` which cause the buffer overflow

Fix:
This can be addressed in a couple of places: 1) the common virtio level to protect the other virtio drivers, 2) the virtio-rng driver, 3) the hwrng code.
The fix can be to check if the length is larger than the buffer.
It may be better to address at the hwrng level.
