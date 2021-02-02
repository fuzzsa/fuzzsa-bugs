# Driver: vmxnet3

# Reported bugs
## 1. Device-shared pointer

Occurs in many places and it's severe security issue for SEV / TDX.
Example occurrence is https://elixir.bootlin.com/linux/v5.10.9/source/drivers/net/vmxnet3/vmxnet3_drv.c#L1425

Found through: fuzzing and static analysis

Root cause:
The driver stores `skb` pointers in shared structures with the device.

Fixed in mainline.

# Unreported bugs
## 1. out of bounds access

Found through: static analysis 

Root cause:
The device controls the value of indTableSize which would lead to an out-of-bounds access in this memory copy: https://elixir.bootlin.com/linux/v5.10.9/source/drivers/net/vmxnet3/vmxnet3_ethtool.c#L1020

Fix:
Check that it's not larger than `UPT1_RSS_MAX_IND_TABLE_SIZE`.

## 2. out of bounds access

Found through: static analysis

Root cause:
The device controls the value of indTableSize which is to determine the number of iterations in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/net/vmxnet3/vmxnet3_drv.c#L2578.
While the value is set by the driver few lines above, the device can always change it immediatly after.

## 3. out of bounds access

Found through: static analysis

Fault in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/net/vmxnet3/vmxnet3_drv.c#L336 and later

Root cause:
The device controls the value of eop_idx while the size of buf_info and tx_ring.base can be smaller than the maximum value for eop_idx.
