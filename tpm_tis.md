# Driver: tpm_tis

# Bugs
## 1. Out-of-bounds access 

The driver would perform an out-of-bounds read access to a buffer on the stack.
The read would happen in https://github.com/torvalds/linux/blob/0477e92881850d44910a7e94fc2c46f96faa131f/drivers/char/tpm/tpm2-cmd.c#L593.

Root cause:
The contents of `buf.data` and thus `pcr_selection` are controlled by the device.
The device can specify a high `pcr_selection.size_of_select` and fill the `pcr_selection` with non-null bytes to cause the overflow.
The contents of the buffer is controlled when the driver decides to transmit a command in https://github.com/torvalds/linux/blob/0477e92881850d44910a7e94fc2c46f96faa131f/drivers/char/tpm/tpm2-cmd.c#L562.
When transmitting the command, the buffer will be filled at the very end of the operation with device-controlled data: https://elixir.bootlin.com/linux/v5.10.10/source/drivers/char/tpm/tpm-interface.c#L126.
The buffer is filled in https://elixir.bootlin.com/linux/v5.10.10/source/drivers/char/tpm/tpm_tis.c#L155.

## 2. Use after free
Due to a race condition the the device can trigger a use after free, by triggerin the removal of the driver while the character device can still be accessed.
The race condition happens because https://elixir.bootlin.com/linux/latest/source/drivers/char/tpm/tpm-chip.c#L274 frees the memory allocated for that chip, which containts the character device structure. 
A call to cdev_remove is missing.
