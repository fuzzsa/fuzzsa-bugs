# Driver: rocker

# Reported bugs
## 1. Device-shared pointer

The driver allocated a shared buffer of descriptors in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/net/ethernet/rocker/rocker_main.c#L444, and each descriptor contains a field `cookie` as shown in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/net/ethernet/rocker/rocker_hw.h#L102.
The cookie can contain a pointer to an skb: https://elixir.bootlin.com/linux/v5.10.9/source/drivers/net/ethernet/rocker/rocker_main.c#L724.
Later the driver can access the pointer and try to free the memory at the address, as shown in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/net/ethernet/rocker/rocker_main.c#L747.
This is a severe security issue.

Fix:
The device does not need to have access to the cookie, so do not store it.

## 2. Out-of-bounds access

Possible out of bounds access in https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/net/ethernet/rocker/rocker_main.c#L98.

Root cause:
The value of `msix_entries` in https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/net/ethernet/rocker/rocker_main.c#L2684 is device-controlled.
The value of `rocker->port_count` is also device-controlled and set in https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/net/ethernet/rocker/rocker_main.c#L2944.
The device can provide a high value for `port_count` which will lead to an overflow in the `ROCKER_MSIX_VEC_COUNT` macro shown in https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/net/ethernet/rocker/rocker_hw.h#L44.
Thus, a small array would be allocated in https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/net/ethernet/rocker/rocker_main.c#L2691, but the device can bypass checks like https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/net/ethernet/rocker/rocker_main.c#L965 and still access memory beyond what was allocated.

Fix:
Validate `port_count` that it is within bounds when initializing.

## 3. Misc bug found while manual inspection of the driver.

The Rocker driver fails initialization when the SWIOTLB DMA implementation is used.`

The reason for this is that the sanity tests during initialization incorrectly handle DMA
operations. The test which would fail is here:
https://elixir.bootlin.com/linux/v5.10/source/drivers/net/ethernet/rocker/rocker_main.c#L213

Cause:
The buffer made available to the device is allocated via `dma_map_single` [1] which would
allocate a buffer in the SWIOTLB SLAB instead of directly returning the physical address of
the kzallocated buffer. Thus, the kzallocated buffer and the buffer given to the device
reside in different physical memory locations and are not coherent.
When the test does the memcmp in [2], the test would fail because the memory is not synced.

Fix:
A fix is to call `dma_sync_single_for_cpu` [3] before doing the memcmp. Under SWIOTLB, this
would copy the memory from the SWIOTLB SLAB to the original buffer.


[1]: https://elixir.bootlin.com/linux/v5.10/source/drivers/net/ethernet/rocker/rocker_main.c#L203
[2]: https://elixir.bootlin.com/linux/v5.10/source/drivers/net/ethernet/rocker/rocker_main.c#L172
[3]: https://elixir.bootlin.com/linux/v5.10/source/include/linux/dma-mapping.h#L117
