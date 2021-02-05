# Driver: virtio_balloon

# Bugs

## 1. Use after free

Race Condition:
the virtio device can trigger this function (it is a virtqueue callback) which will add a work item to a workqueue https://elixir.bootlin.com/linux/latest/source/drivers/virtio/virtio_balloon.c#L376
this can already happen before the probe function has run through, if probe then fails, it frees 'vb' https://elixir.bootlin.com/linux/latest/source/drivers/virtio/virtio_balloon.c#L1025
so we get a use after free, because the work_struct is still queued in the work list

## 2. Unbounded Allocation
`num_pages` can freely be choosen by the device
https://elixir.bootlin.com/linux/latest/source/drivers/virtio/virtio_balloon.c#L402

## 3. Deadlock
If virtqueue is broken due to invalid data provided by the device the condition can never be true
https://elixir.bootlin.com/linux/latest/source/drivers/virtio/virtio_balloon.c#L164
https://elixir.bootlin.com/linux/latest/source/drivers/virtio/virtio_balloon.c#L190
