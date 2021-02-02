# Driver: nvme

# Unreported bugs
## 1. Invalid memory access (null ptr dereference)

Fault happens in https://elixir.bootlin.com/linux/v5.10.10/source/block/blk-exec.c#L30

The root cause investigation is incomplete, but the likely cause is that the driver reads an index value from the device in https://elixir.bootlin.com/linux/v5.10.10/source/drivers/nvme/host/pci.c#L985.
Then, the driver access an array with index and returns the request as shown in https://elixir.bootlin.com/linux/latest/source/block/blk-mq.c#L862.
We suspect the request may be not completely initialized which causes the null pointer dereference.


