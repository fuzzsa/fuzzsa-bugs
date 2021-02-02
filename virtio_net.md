# Driver: virtio_net

# Reported bugs

## 1. Use after free

Memory can be freed when https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/net/virtio_net.c#L2039 is called.

Root-cause:
`virtnet_probe` would not set an error into the `err` variable if the MTU size, reported by the device, is invalid as shown in https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/net/virtio_net.c#L3075. This would result in memory being freed in https://github.com/torvalds/linux/blob/eccc876724927ff3b9ff91f36f7b6b159e948f0c/drivers/net/virtio_net.c#L3165.
However, initialization would succeed because `err` is earlier set to 0.

Fix:
Set `err` to `-EINVAL` and then jump to tear down code.

