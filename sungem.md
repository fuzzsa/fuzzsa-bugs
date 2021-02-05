# Driver: sungem

# Bugs
## 1. Null pointer dereference
Fault in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/net/ethernet/sun/sungem.c#L648

Root cause:
The driver exposes the regs buffer for IO in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/net/ethernet/sun/sungem.c#L2925.
The value of `gp->status` is controlled by the device in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/net/ethernet/sun/sungem.c#L940 and https://elixir.bootlin.com/linux/v5.10.9/source/drivers/net/ethernet/sun/sungem.c#L915.
The device can inject an interrupt to tell that packets are processed, which would call `gem_tx` with status controlled by the device in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/net/ethernet/sun/sungem.c#L903.
The number of loop iterations is determined in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/net/ethernet/sun/sungem.c#L636, which means that the driver will access skbs which are not allocated yet.

Fix:
Keep track of what the end of the ring queue can be and validate the results provided by the device.

## 2. Unbounded Allocation
*DMA* memory is allocated but not freed
https://elixir.bootlin.com/linux/latest/source/drivers/net/ethernet/sun/sunhme.c#L3124
but not freed if initialization fails at
https://elixir.bootlin.com/linux/latest/source/drivers/net/ethernet/sun/sunhme.c#L3166
