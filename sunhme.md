## 1. Unbounded Allocation
*DMA* memory is allocated
https://elixir.bootlin.com/linux/latest/source/drivers/net/ethernet/sun/sunhme.c#L3124
but not freed if initialization fails at
https://elixir.bootlin.com/linux/latest/source/drivers/net/ethernet/sun/sunhme.c#L3166

## 2. Unbounded Allocation
SKB bufs are initialized
https://elixir.bootlin.com/linux/latest/source/drivers/net/ethernet/sun/sunhme.c#L1451
but not freed on error
https://elixir.bootlin.com/linux/latest/source/drivers/net/ethernet/sun/sunhme.c#L1481
https://elixir.bootlin.com/linux/latest/source/drivers/net/ethernet/sun/sunhme.c#L1497
