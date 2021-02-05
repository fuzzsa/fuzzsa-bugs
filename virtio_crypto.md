# Driver: virtio_crypto

# Bugs

## 1. Invalid access

Root cause:
The device can provide invalid information, e.g. that device is not ready, which will cause this function to jump into driver teardown:
https://elixir.bootlin.com/linux/v5.10.8/source/drivers/crypto/virtio/virtio_crypto_core.c#L398

During this time, because the crypto engines are registered, the some other driver or user space can make a crypto request. It may be possible that the device can also inject an interrupt which would cause a similar failure.

## 2. Unbounded Allocation
`max_data_queues` is under device control
https://elixir.bootlin.com/linux/latest/source/drivers/crypto/virtio/virtio_crypto_core.c#L326
https://elixir.bootlin.com/linux/latest/source/drivers/crypto/virtio/virtio_crypto_core.c#L366
and used to allocate array
https://elixir.bootlin.com/linux/latest/source/drivers/crypto/virtio/virtio_crypto_core.c#L123
