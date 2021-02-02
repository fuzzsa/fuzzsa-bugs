# Driver: qemu_fw_cfg module

# Unreported bugs
## 1. Null-pointer dereference / Invalid memory access

Found through: fuzzing

Root cause:
The entry may not be added to the list when list_del is called in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/firmware/qemu_fw_cfg.c#L384.
The entry is enlisted in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/firmware/qemu_fw_cfg.c#L625, but that may never be called if the specified name by the device is invalid in https://elixir.bootlin.com/linux/v5.10.9/source/drivers/firmware/qemu_fw_cfg.c#L612.

Fix:
Probably call `INIT_LIST_HEAD` for `fw_cfg_sysfs_entry::list` after memory is first allocated.
