# Driver: virtio_console

# Bugs

## 1. Invalid access

Root-cause:
The number of ports is provided by the device in [1].
The nummber of queues is calculated in [2]. The device can provide a large number which would result in interger overflow, and the driver would allocate zero queues.
The allocations would succeed but return `ZERO_SIZE_PTR` which will cause an invalid memory access later on.

Fix:
Add a check to verify that the number of ports is reasonable.

## 2. Potential out-of-bounds access
When a new port is added, `port->id` in coherent DMA memory.
There's validation in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/char/virtio_console.c#L1592, but then the value is read again in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/char/virtio_console.c#L1600 .

[1]: https://elixir.bootlin.com/linux/v5.10.9/source/drivers/char/virtio_console.c#L2041
[2]: https://elixir.bootlin.com/linux/v5.10.9/source/drivers/char/virtio_console.c#L1852
