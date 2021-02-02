# Overview
In this document we describe the modifications, that we applied to driver code in order to aid the analysis.
The modifications fall into two categories:
* Patches to fix bugs that were identified during research
* Fuzzing optimizations (such as removing checks on checksum comparisons, that are difficult to  solve for a fuzzer)

## virtio_blk
### Patches
1. [low severity] Avoid reachable assertion:
```
@@ -228,7 +230,13 @@ static blk_status_t virtio_queue_rq(struct blk_mq_hw_ctx *hctx,
   bool unmap = false;
   u32 type;

-  BUG_ON(req->nr_phys_segments + 2 > vblk->sg_elems);
+   if(lkl_ops->fuzz_ops->apply_patch()) {
+      if(req->nr_phys_segments + 2 > vblk->sg_elems) {
+         return BLK_STS_IOERR;
+      }
+   } else {
+      BUG_ON(req->nr_phys_segments + 2 > vblk->sg_elems);
+   }
```
2. [high severity] Handle invalid number of virtqueues to avoid null pointer dereference:
```
@@ -442,6 +450,7 @@ static void virtblk_update_capacity(struct virtio_blk *vblk, bool resize)
   struct request_queue *q = vblk->disk->queue;
   char cap_str_2[10], cap_str_10[10];
   unsigned long long nblocks;
   u64 capacity;

   /* Host must always specify the capacity. */
@@ -506,6 +515,11 @@ static int init_vq(struct virtio_blk *vblk)
      num_vqs = 1;

   num_vqs = min_t(unsigned int, nr_cpu_ids, num_vqs);
+   if(lkl_ops->fuzz_ops->apply_patch_2()) {
+      if(num_vqs<1) {
+         num_vqs=1;
+      }
+   }

```
3. [high severity] Sanitize block size to avoid oob-access
```
@@ -824,11 +839,18 @@ static int virtblk_probe(struct virtio_device *vdev)
 	/* Host can optionally specify the block size of the device */
 	err = virtio_cread_feature(vdev, VIRTIO_BLK_F_BLK_SIZE,
 				   struct virtio_blk_config, blk_size,
-				   &blk_size);
-	if (!err)
-		blk_queue_logical_block_size(q, blk_size);
-	else
-		blk_size = queue_logical_block_size(q);
+               &blk_size);
+   if (!err) {
+      if(lkl_ops->fuzz_ops->apply_patch_2()) {
+         if(blk_size < 512 || blk_size > PAGE_SIZE) {
+            pr_err("%s:%d WARNING: fixing blk size %d\n", __FUNCTION__, __LINE__, blk_size);
+            blk_size = 512;
+         }
+      }
+      blk_queue_logical_block_size(q, blk_size);
+   } else {
+      blk_size = queue_logical_block_size(q);
+   }
```

## virtio_rng
### Patches
1. [high severity] Sanitize rng buffer size in core code to avoid oob-access
```
--- a/drivers/char/hw_random/core.c
+++ b/drivers/char/hw_random/core.c
@@ -448,6 +448,12 @@ static int hwrng_fillfn(void *unused)
 			msleep_interruptible(10000);
 			continue;
 		}
+      if(lkl_ops->fuzz_ops->apply_patch_2()) {
+         if(rc >= rng_buffer_size()) {
+            pr_err("%s warning %ld >= %ld", __FUNCTION__, rc, rng_buffer_size());
+            rc = rng_buffer_size();
+         }
+      }
 		/* Outside lock, sure, but y'know: randomness. */
 		add_hwgenerator_randomness((void *)rng_fillbuf, rc,
 					   rc * current_quality * 8 >> 10);
```

## virtio_crypto
### Patches
1. [low severity] limit number of queues reported by device to avoid running out of memory
```
@@ -375,6 +375,10 @@ static int virtcrypto_probe(struct virtio_device *vdev)
 	vcrypto->hash_algo = hash_algo;
 	vcrypto->aead_algo = aead_algo;

+   if(lkl_ops->fuzz_ops->apply_patch()) {
+      if(vcrypto->max_data_queues > 128)
+         vcrypto->max_data_queues = 128;
+   }

 	dev_info(&vdev->dev,
 		"max_queues: %u, max_cipher_key_len: %u, max_auth_key_len: %u, max_size 0x%llx\n",
```

## e1000
### Optimizations
1. Disable checksum validation of EEPROM data
```
@@ -3959,6 +3959,9 @@ s32 e1000_validate_eeprom_checksum(struct e1000_hw *hw)
 	u16 checksum = 0;
 	u16 i, eeprom_data;

+	if(lkl_ops->fuzz_ops->apply_hacks()) {
+		return E1000_SUCCESS;
+	}
 	for (i = 0; i < (EEPROM_CHECKSUM_REG + 1); i++) {
 		if (e1000_read_eeprom(hw, i, 1, &eeprom_data) < 0) {
 			e_dbg("EEPROM Read Error\n");
```
### Patches
1. [high severity] validate offset before read array access
```
@@ -4090,7 +4091,16 @@ static bool e1000_tbi_should_accept(struct e1000_adapter *adapter,
 				    u32 length, const u8 *data)
 {
 	struct e1000_hw *hw = &adapter->hw;
-	u8 last_byte = *(data + length - 1);
+   u8 last_byte;
+   if(lkl_ops->fuzz_ops->apply_patch_2()) {
+      if(length >= adapter->rx_buffer_len) {
+         last_byte = *(data + 0);
+      } else {
+         last_byte = *(data + length - 1);
+      }
+   } else {
+      last_byte = *(data + length - 1);
+   }
```
2. [high severity] Update pointer to fix use after free
```
@@ -4185,7 +4195,11 @@ static bool e1000_clean_jumbo_rx_irq(struct e1000_adapter *adapter,
 				/* an error means any chain goes out the window
 				 * too
 				 */
-				dev_kfree_skb(rx_ring->rx_skb_top);
+            if(lkl_ops->fuzz_ops->apply_patch_2()) {
+               napi_free_frags(&adapter->napi);
+            } else {
+               dev_kfree_skb(rx_ring->rx_skb_top);
+            }
 				rx_ring->rx_skb_top = NULL;
 				goto next_desc;
 			}
```
## rocker
### Patches
1. [high severity] Fix oob access due to integer underflow
```
@@ -2942,6 +2957,11 @@ static int rocker_probe(struct pci_dev *pdev, const struct pci_device_id *id)
 	pci_set_drvdata(pdev, rocker);

 	rocker->port_count = rocker_read32(rocker, PORT_PHYS_COUNT);
+   if(lkl_ops->fuzz_ops->apply_patch()) {
+      if(rocker->port_count == 0) {
+         rocker->port_count = 2;
+      }
+   }

 	err = rocker_msix_init(rocker);
 	if (err) {
```
2. [low severity] Fix missing dma memory synchronization
```
@@ -207,6 +213,9 @@ static int rocker_dma_test_offset(const struct rocker *rocker,
 		goto free_alloc;
 	}

+   if(lkl_ops->fuzz_ops->apply_patch_2()) {
+      dma_sync_single_for_cpu(&pdev->dev, dma_handle, ROCKER_TEST_DMA_BUF_SIZE, DMA_BIDIRECTIONAL);
+   }
 	rocker_write64(rocker, TEST_DMA_ADDR, dma_handle);
 	rocker_write32(rocker, TEST_DMA_SIZE, ROCKER_TEST_DMA_BUF_SIZE);

@@ -218,6 +227,9 @@ static int rocker_dma_test_offset(const struct rocker *rocker,
 		goto unmap;

 	memset(expect, 0, ROCKER_TEST_DMA_BUF_SIZE);
+   if(lkl_ops->fuzz_ops->apply_patch_2()) {
+      dma_sync_single_for_cpu(&pdev->dev, dma_handle, ROCKER_TEST_DMA_BUF_SIZE, DMA_BIDIRECTIONAL);
+   }
 	err = rocker_dma_test_one(rocker, wait, ROCKER_TEST_DMA_CTRL_CLEAR,
 				  dma_handle, buf, expect,
 				  ROCKER_TEST_DMA_BUF_SIZE);
```
### Optimizations
1. Disable complex comparison on dma memory
```
@@ -161,6 +164,9 @@ static int rocker_dma_test_one(const struct rocker *rocker,
 	const struct pci_dev *pdev = rocker->pdev;
 	int i;

+	if(lkl_ops->fuzz_ops->apply_hacks()) {
+		return 0;
+	}
 	rocker_wait_reset(wait);
 	rocker_write32(rocker, TEST_DMA_CTRL, test_type);
```

## sungem
### Optimizations
1. Set device controlled fifo sizes correctly
```
@@ -2027,6 +2027,16 @@ static int gem_check_invariants(struct gem *gp)
 	gp->tx_fifo_sz = readl(gp->regs + TXDMA_FSZ) * 64;
 	gp->rx_fifo_sz = readl(gp->regs + RXDMA_FSZ) * 64;

+	if(lkl_ops->fuzz_ops->apply_hacks()) {
+		if (pdev->device == PCI_DEVICE_ID_SUN_GEM) {
+			gp->tx_fifo_sz = 9 * 1024;
+			gp->rx_fifo_sz = 20 * 1024;
+		} else {
+			gp->tx_fifo_sz = 2 * 1024;
+			gp->rx_fifo_sz = 2 * 1024;
+		}
+	}
+
 	if (pdev->vendor == PCI_VENDOR_ID_SUN) {
 		if (pdev->device == PCI_DEVICE_ID_SUN_GEM) {
 			if (gp->tx_fifo_sz != (9 * 1024) ||
```

## sunhme
### Patches
1. [low severity] Free dma memory on failed initialization
```
@@ -3163,7 +3181,11 @@ static int happy_meal_pci_probe(struct pci_dev *pdev,
 	if (err) {
 		printk(KERN_ERR "happymeal(PCI): Cannot register net device, "
 		       "aborting.\n");
-		goto err_out_iounmap;
+      if(lkl_ops->fuzz_ops->apply_patch()) {
+         goto err_out_free_dma;
+      } else {
+         goto err_out_iounmap;
+      }
 	}

 	pci_set_drvdata(pdev, hp);
@@ -3196,6 +3218,10 @@ static int happy_meal_pci_probe(struct pci_dev *pdev,

 	return 0;

+err_out_free_dma:
+	dma_free_coherent(hp->dma_dev, PAGE_SIZE,
+			  hp->happy_block, hp->hblock_dvma);
+
 err_out_iounmap:
 	iounmap(hp->gregs);
```

2. [low severity] Restrict number of allocations initiated by device
```
@@ -1993,12 +1999,19 @@ static void happy_meal_rx(struct happy_meal *hp, struct net_device *dev)
 	struct happy_meal_rxd *this;
 	int elem = hp->rx_new, drops = 0;
 	u32 flags;
+   long total_len = 0;

 	RXD(("RX<"));
 	this = &rxbase[elem];
 	while (!((flags = hme_read_desc32(hp, &this->rx_flags)) & RXFLAG_OWN)) {
 		struct sk_buff *skb;
 		int len = flags >> 16;
+      if(lkl_ops->fuzz_ops->apply_patch()) {
+         total_len += len;
+         if(total_len > PAGE_SIZE * 32) {
+            break;
+         }
+      }
 		u16 csum = flags & RXFLAG_CSUM;
 		u32 dma_addr = hme_read_desc32(hp, &this->rx_addr);
```

## virtio_net
### Patches
1. [high severity] Set proper error code to avoid use after free later
```
@@ -3072,6 +3074,9 @@ static int virtnet_probe(struct virtio_device *vdev)
 			dev_err(&vdev->dev,
 				"device MTU appears to have changed it is now %d < %d",
 				mtu, dev->min_mtu);
+         if(lkl_ops->fuzz_ops->apply_patch_2()) {
+            err = -EINVAL;
+         }
 			goto free;
 		}
```

## vmxnet3
### Patches
1. [low severity] Avoid reachable assertion:
```
@@ -603,8 +603,13 @@ vmxnet3_rq_alloc_rx_buf(struct vmxnet3_rx_queue *rq, u32 ring_idx,
 			}
 			val = VMXNET3_RXD_BTYPE_HEAD << VMXNET3_RXD_BTYPE_SHIFT;
 		} else {
-			BUG_ON(rbi->buf_type != VMXNET3_RX_BUF_PAGE ||
-			       rbi->len  != PAGE_SIZE);
+			if(lkl_ops->fuzz_ops->apply_patch()) {
+				rbi->buf_type = VMXNET3_RX_BUF_PAGE;
+				rbi->len = PAGE_SIZE;
+			} else {
+				BUG_ON(rbi->buf_type != VMXNET3_RX_BUF_PAGE ||
+						rbi->len  != PAGE_SIZE);
+			}

 			if (rbi->page == NULL) {
 				rbi->page = alloc_page(GFP_ATOMIC);
```

## virtio_balloon
### Patches
1. [high severity] Fix race condition on queued work struct
```
@@ -1014,6 +1047,9 @@ static int virtballoon_probe(struct virtio_device *vdev)
 	if (virtio_has_feature(vdev, VIRTIO_BALLOON_F_FREE_PAGE_HINT))
 		destroy_workqueue(vb->balloon_wq);
 out_iput:
+   if(lkl_ops->fuzz_ops->apply_patch_2()) {
+      cancel_work_sync(&vb->update_balloon_size_work);
+   }
 #ifdef CONFIG_BALLOON_COMPACTION
 	iput(vb->vb_dev_info.inode);
 out_kern_unmount:
```
2. [low severity] Avoid deadlocks
```
@@ -161,8 +162,16 @@ static void tell_host(struct virtio_balloon *vb, struct virtqueue *vq)
 	virtqueue_kick(vq);

 	/* When host has read buffer, this completes via balloon_ack */
-	wait_event(vb->acked, virtqueue_get_buf(vq, &len));
-
+	// Patch(feli): the virtqueues might be broken which leads to a deadlock here
+	// Note(feli): alternatively one could also wrap the kernel waitqueue interface
+	// in order to forcefully cancel these, but I guess such deadlocks are technically
+	// bugs to patching them is probably the better way
+   if(lkl_ops->fuzz_ops->apply_patch()) {
+      wait_event(vb->acked, true);
+      virtqueue_get_buf(vq, &len);
+   } else {
+      wait_event(vb->acked, virtqueue_get_buf(vq, &len));
+   }
 }

@@ -187,7 +196,12 @@ static int virtballoon_free_page_report(struct page_reporting_dev_info *pr_dev_i
 	virtqueue_kick(vq);

 	/* When host has read buffer, this completes via balloon_ack */
-	wait_event(vb->acked, virtqueue_get_buf(vq, &unused));
+   if(lkl_ops->fuzz_ops->apply_patch()) {
+      wait_event(vb->acked, true);
+      virtqueue_get_buf(vq, &unused);
+   } else {
+      wait_event(vb->acked, virtqueue_get_buf(vq, &unused));
+   }

 	return 0;
 }

```
3. [low severity] Avoid huge allocations triggered by device
```
@@ -402,7 +416,19 @@ static inline s64 towards_target(struct virtio_balloon *vb)
 	virtio_cread_le(vb->vdev, struct virtio_balloon_config, num_pages,
 			&num_pages);

+	// Patch(feli): this can get out of hand if the fuzzer
+	// asks for insane amounts of mem, lets keep the diff
+	// reasonalbe (not a bug though)
 	target = num_pages;
+   if(lkl_ops->fuzz_ops->apply_patch()) {
+      if(vb->num_pages > target && vb->num_pages - target > 16) {
+         return -16;
+      }
+      if(vb->num_pages < target && target - vb->num_pages > 16) {
+         return 16;
+      }
+   }
+
 	return target - vb->num_pages;
 }

```

## virtio_ring
### Patches
1. [high severity] Fix use after free due to premature queueing
```
@@ -1608,7 +1610,9 @@ static struct virtqueue *vring_create_virtqueue_packed(
 	vq->num_added = 0;
 	vq->packed_ring = true;
 	vq->use_dma_api = vring_use_dma_api(vdev);
-	list_add_tail(&vq->vq.list, &vdev->vqs);
+   if(!lkl_ops->fuzz_ops->apply_patch()) {
+      list_add_tail(&vq->vq.list, &vdev->vqs);
+   }
 #ifdef DEBUG
 	vq->in_use = false;
 	vq->last_add_time_valid = false;

@@ -1669,6 +1673,9 @@ static struct virtqueue *vring_create_virtqueue_packed(
 			cpu_to_le16(vq->packed.event_flags_shadow);
 	}

+   if(lkl_ops->fuzz_ops->apply_patch()) {
+      list_add_tail(&vq->vq.list, &vdev->vqs);
+   }
 	return &vq->vq;

 err_desc_extra:

@@ -2085,7 +2094,9 @@ struct virtqueue *__vring_new_virtqueue(unsigned int index,
 	vq->last_used_idx = 0;
 	vq->num_added = 0;
 	vq->use_dma_api = vring_use_dma_api(vdev);
-	list_add_tail(&vq->vq.list, &vdev->vqs);
+   if(!lkl_ops->fuzz_ops->apply_patch()) {
+      list_add_tail(&vq->vq.list, &vdev->vqs);
+   }
 #ifdef DEBUG
 	vq->in_use = false;
 	vq->last_add_time_valid = false;
@@ -2127,6 +2138,9 @@ struct virtqueue *__vring_new_virtqueue(unsigned int index,
 	memset(vq->split.desc_state, 0, vring.num *
 			sizeof(struct vring_desc_state_split));

+   if(lkl_ops->fuzz_ops->apply_patch()) {
+      list_add_tail(&vq->vq.list, &vdev->vqs);
+   }
 	return &vq->vq;
 }
 EXPORT_SYMBOL_GPL(__vring_new_virtqueue);

```
