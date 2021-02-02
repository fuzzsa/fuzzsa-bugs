# Driver: virtio common code

# Reported bugs
## 1. Out-of-bounds access

The common virtio driver code would call dma_unmap_page with a device-controlled length in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/virtio/virtio_ring.c#L385, which would eventuall call `swiotlb_tbl_unmap_single`, shown in https://elixir.bootlin.com/linux/v5.10.9/source/kernel/dma/swiotlb.c#L595.
`swiotlb_bounce` would copy a buffer with controlled length and data by the device.

Fix: keep track of actual length of the mapped buffer and avoid overflow.

## 2. Out-of-bounds access

A local index variable `i` would be set with the value of `desc[i].next` which is device-controlled, as shown in https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/virtio/virtio_ring.c#L491.
This can lead to a buffer overflow into the private memory of the VM.

Fix: check that `i` is not greater than the number of descriptors.

## 3. Double free / memory leak

If virtqueue initialization fails, then three `vring_free_queue` calls would be made with the same dma address as shown in https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/virtio/virtio_ring.c#L1678.
However, memory pointed by `driver_event_dma_addr` and `device_event_dma_addr` never gets cleared, and this this would leak memory.

Fix: use the correct dma addresses.

## 4. Use after free

The virtqueue is added to a list in https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/virtio/virtio_ring.c#L1611, and if the initialization fails the `vq` object would be freed in https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/virtio/virtio_ring.c#L1677. However, the virtqueue is not removed from the list.
When all vqs are deleted later in https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/virtio/virtio_mmio.c#L342, there would be an invalid memory access due to UAF.

Fix: remove the virtqueue from the list if virtqueue initialization fails.


