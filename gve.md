# Driver: GVE

# Reported bugs
## 1. Unsanitized input to `proc_mkdir`

Happens in https://github.com/torvalds/linux/blob/d76913908102044f14381df865bb74df17a538cb/kernel/irq/proc.c#L328. The name is generated in https://github.com/torvalds/linux/blob/d76913908102044f14381df865bb74df17a538cb/drivers/net/ethernet/google/gve/gve_main.c#L263 by the driver, but `block->name` resides in shared memory which allows the device to change it before `request_irq` is called in https://github.com/torvalds/linux/blob/d76913908102044f14381df865bb74df17a538cb/drivers/net/ethernet/google/gve/gve_main.c#L266.

Fix:
1) don't store the name in shared memory
or
2) copy it locally, sanitize the copy and use that one to create the proc entry.

## 2. Use-after-free

Happens in https://github.com/torvalds/linux/blob/d76913908102044f14381df865bb74df17a538cb/drivers/net/ethernet/google/gve/gve_main.c#L311 and other possible places.

Root cause:
If driver initialization fails, then `msix->vectors` would be freed in https://github.com/torvalds/linux/blob/d76913908102044f14381df865bb74df17a538cb/drivers/net/ethernet/google/gve/gve_main.c#L295.
The driver setup would fail an on teardown the function `gve_free_notify_blocks` would be called in https://github.com/torvalds/linux/blob/d76913908102044f14381df865bb74df17a538cb/drivers/net/ethernet/google/gve/gve_main.c#L358.

Fix:
Refactor the code to move the `msix_vectors` allocation in `gve_setup_device_resources`. Also, free it there in the teardown code.

## 3. Out of bounds access

Happens in https://github.com/torvalds/linux/blob/d76913908102044f14381df865bb74df17a538cb/drivers/net/ethernet/google/gve/gve_main.c#L243

Root cause:
`priv->num_nfty_blks` and `priv->mgmt_msix_idx` are initialized with a value provided by the device.

In `gve_alloc_notify_blocks` the array `priv->msix_vectors` is allocated, the size of the array is device controlled and equal to `priv->num_ntfy_blks + 1`: https://github.com/torvalds/linux/blob/d76913908102044f14381df865bb74df17a538cb/drivers/net/ethernet/google/gve/gve_main.c#L204.

If the number of enabled msi vectors is not equal to the number of requested vectors `(vecs_enabled != num_vecs_requested)`, then `priv->num_ntfy_blks` is updated: https://github.com/torvalds/linux/blob/d76913908102044f14381df865bb74df17a538cb/drivers/net/ethernet/google/gve/gve_main.c#L223

The two invocations of `gve_alloc_notify_blocks` are triggered via the paths:
`gve_probe` -> `gve_init_priv` -> `gve_alloc_notify_blocks`
`gve_service_task` -> `gve_handle_reset` -> `gve_reset` -> `gve_reset_recovery` -> `gve_init_priv` -> `gve_alloc_notify_blocks`
Note that `gve_reset_recovery` on patch b) sets the parameter `skip_describe_device` which skips the update of `priv->mgmt_msix_idx` that happens on path a).

## 4. Device-shared pointer

The GVE driver shares and dereferences many pointers contained in the shared structure `struct gve_notify_block` in https://elixir.bootlin.com/linux/v5.10.10/source/drivers/net/ethernet/google/gve/gve.h#L159.
The `ntfy_blocks` array is allocated in https://github.com/torvalds/linux/blob/d76913908102044f14381df865bb74df17a538cb/drivers/net/ethernet/google/gve/gve_main.c#L249.
One of many places where a pointer, possibly overwritten by the device, is used is https://github.com/torvalds/linux/blob/d76913908102044f14381df865bb74df17a538cb/drivers/net/ethernet/google/gve/gve_main.c#L397


## 5. Unbounded Allocation
`num_ntfy[_blks]` is under device control
https://elixir.bootlin.com/linux/latest/source/drivers/net/ethernet/google/gve/gve_main.c#L1096
https://elixir.bootlin.com/linux/latest/source/drivers/net/ethernet/google/gve/gve_main.c#L1114
and used to allocate *DMA* memory
https://elixir.bootlin.com/linux/latest/source/drivers/net/ethernet/google/gve/gve_main.c#L248
