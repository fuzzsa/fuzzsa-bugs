# Driver: virtio_blk

# Unreported bugs

## 1. Invalid access
Access fault at https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/block/virtio_blk.c#L761

Root-cause:
Device can specify zero number of queues but provide the flag VIRTIO_BLK_F_MQ.
After the device query the size in [1], the driver would allocate memory of size 0 [2].
The allocations would succeed but return `ZERO_SIZE_PTR` which will cause an invalid memory access later on.

Fix:
Add a check to verify that the length is reasonable [1;small number].

[1]: https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/block/virtio_blk.c#L502,
[2]: https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/block/virtio_blk.c#L510


## 2. Divide by zero
Divide by zero bug in virtio_blk here https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/block/virtio_blk.c#L457
Block size read here https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/block/virtio_blk.c#L825

## 3. Invalid access / Null Pointer Dereference
alloc_page_buffers returns NULL if blocksize > PAGE_SIZE
https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/fs/buffer.c#L1561
https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/fs/buffer.c#L855

## 4. Invalid memory access
The device provides the value for `sg_elems` in https://elixir.bootlin.com/linux/v5.10.12/source/drivers/block/virtio_blk.c#L723.
When a request is created, `sg_init_table` will call with `sg_elems == 0` in https://elixir.bootlin.com/linux/v5.10.12/source/drivers/block/virtio_blk.c#L677
This will result in an underflow in https://elixir.bootlin.com/linux/v5.10.12/source/include/linux/scatterlist.h#L270, thus leading to an invalid memory access.

## 5. Out-of-bounds access
Similarly, the driver can specify a very high value for `sg_elems` which will cause an out-of-bounds access in https://elixir.bootlin.com/linux/v5.10.12/source/lib/scatterlist.c#L126.



