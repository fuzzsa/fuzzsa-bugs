# Driver: acpi

# Unreported bugs
## 1. Out of bounds access

`table->length` is device controlled and would lead to out-bounds-accesses below.
See https://elixir.bootlin.com/linux/v5.10.10/source/drivers/acpi/acpica/tbutils.c#L310
