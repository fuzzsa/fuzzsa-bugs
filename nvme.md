# Driver: nvme

# Unreported bugs
## 1. Invalid memory access (null ptr dereference)

Fault happens in https://elixir.bootlin.com/linux/v5.10.10/source/block/blk-exec.c#L30

The root cause investigation is incomplete, but the likely cause is that the driver reads an index value from the device in https://elixir.bootlin.com/linux/v5.10.10/source/drivers/nvme/host/pci.c#L985.
Then, the driver access an array with index and returns the request as shown in https://elixir.bootlin.com/linux/latest/source/block/blk-mq.c#L862.
We suspect the request may be not completely initialized which causes the null pointer dereference.

## 2. Use after free

Fault happens in https://github.com/torvalds/linux/blob/5e46d1b78a03d52306f21f77a4e4a144b6d31486/drivers/nvme/host/pci.c#L596
This is especially critical since the data written is completely under device control.

The same request is added twice here: https://github.com/torvalds/linux/blob/5e46d1b78a03d52306f21f77a4e4a144b6d31486/drivers/nvme/host/pci.c#L1024
This is possible, since `command_id` is device controlled.
The free happens in the block device core code.
