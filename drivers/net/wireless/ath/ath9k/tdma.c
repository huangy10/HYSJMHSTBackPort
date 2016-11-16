//
// Created by 黄延 on 2016/11/2.
//
// Copy Right Reserved.
//
// We implement the TMDA mechanism here.
//


#include "tdma.h"
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/bitmap.h>
#include <linux/rcupdate.h>
#include <linux/export.h>
#include <net/net_namespace.h>
#include <net/ieee80211_radiotap.h>
#include <net/cfg80211.h>
#include <net/mac80211.h>
#include <asm/unaligned.h>


//struct tdma_txq {
//    list_head head;
//};
//
//static void tdma_config() {
//
//}
//
//
//void tdma_xmit(struct ieee80211_sub_if_data *sdata, struct sta_info *sta, struct sk_buff *skb) {
//
//}
//

struct TDMA_packet {

};
struct TDMA{
	struct TDMA_packet *head;
	struct TDMA_packet *tail;
};

void TDMA_send_triggered(struct TDMA *tdma){
	//calculate current transmission rate

	while (TDMA_get_packet_params(head)){
		//calculate time required to send this packet

		if (packet_transmit_time < slot_interval){
			TDMA_remove_from_buffer(tdma->head);
			ath_tx_hardstart();
		}
		else
			break;
	}
}

/*
 * Created by Woody Huang, 2016.11.15
 *
 * 获取发送速率，这里的原理如下：
 *
 * 由于我们禁用了发送速率控制功能，即所有的包都会以一个恒定的速率进行发送，那么直接取出就可以了。
 */
struct ieee80211_tx_rate* TDMA_get_tx_rate(struct sk_buff *skb) {
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	return info->control.rates[0];
}

void fractel_TDMA_TDMA_add_to_buffer(struct TDMA_packet pkt, struct TDMA* tdma){
	//accquire spin-lock on TDMA queue data-structure
	if (tdma->head == NULL){
		struct TDMA_packet *new_packet = MALLOC(sizeof(struct TDMA_packet));
		tdma->head = new_packet;
		tdma->tail = new_packet;
	}
	else{
		struct TDMA_packet *new_packet = MALLOC(sizeof(struct TDMA_packet));
		tdma->tail = new_packet;
	}
	//release spin-lock
}

void TDMA_get_packet_params(struct TDMA *tdma){
	if (tdma->head == NULL)
		return 0;
	else{
		//write length of the packet pointed by TDMA->head into the address passed as argument
		return 1;
	}
}

void TDMA_remove_from_buffer(struct TDMA *tdma){
	if (tdma->head == NULL)
		return 0;
	else{
		//accquire spin-lock on TDMA queue data-structure

		//release spin-lock
	}
}

void fractel_prepare_schedule(){
	fractel_create_schedule();
}

void fractel_event_handler(int expected_next_slot_time, int slot_number, struct TDMA *tdma){
    while(current_time<expected_next_slot_time) {}
    if((current_time-expected_next_slot_time)>100)
        next_slot_interval=TDMA_SLOT_INTERVAL(INTERVAL-1);
    else
        next_slot_interval=TDMA_SLOT_INTERVAL(INTERVAL);
    mod_timer(&fractel_event_timer,next_slot_interval);
    slot_counter++;
    slot_counter=slot_number%total_slots;
    if (my_control_slot){
        if(root_node){

        }
        else{

        }
    }
    else if(contention_slot) {}
    else if(my_data_slot){
        TDMA_send_triggered(tdma);
    }

}


static int
ath_hardstart(struct sk_buff *skb, struct net_device *dev)
{
	struct ath_softc *sc = dev->priv;
	struct ieee80211_node *ni = NULL;
	struct ath_buf *bf = NULL;
	struct ether_header *eh;
	ath_bufhead bf_head;
	struct ath_buf *tbf, *tempbf;
	struct sk_buff *tskb;
	int framecnt;
	/* We will use the requeue flag to denote when to stuff a skb back into
	* the OS queues.  This should NOT be done under low memory conditions,
	* such as skb allocation failure.  However, it should be done for the
	* case where all the dma buffers are in use (take_txbuf returns null).
	*/
	int requeue = 0;
#ifdef ATH_SUPERG_FF
	unsigned int pktlen;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ath_node *an;
	struct ath_txq *txq = NULL;
	int ff_flush;
#endif

	/* If an skb is passed in directly from the kernel,
	* we take responsibility for the reference */
	ieee80211_skb_track(skb);

	if ((dev->flags & IFF_RUNNING) == 0 || sc->sc_invalid) {
		DPRINTF(sc, ATH_DEBUG_XMIT,
			"Dropping; invalid %d flags %x\n",
			sc->sc_invalid, dev->flags);
		sc->sc_stats.ast_tx_invalid++;
		return -ENETDOWN;
	}

	STAILQ_INIT(&bf_head);

	if (SKB_CB(skb)->flags & M_RAW) {
		bf = ath_take_txbuf(sc);
		if (bf == NULL) {
			/* All DMA buffers full, safe to try again. */
			requeue = 1;
			goto hardstart_fail;
		}
		ath_tx_startraw(dev, bf, skb);
		return NETDEV_TX_OK;
	}

	ni = SKB_CB(skb)->ni;		/* NB: always passed down by 802.11 layer */
	if (ni == NULL) {
		/* NB: this happens if someone marks the underlying device up */
		DPRINTF(sc, ATH_DEBUG_XMIT,
			"Dropping; No node in skb control block!\n");
		goto hardstart_fail;
	}

#ifdef ATH_SUPERG_FF
	if (M_FLAG_GET(skb, M_UAPSD)) {
		/* bypass FF handling */
		bf = ath_take_txbuf(sc);
		if (bf == NULL) {
			/* All DMA buffers full, safe to try again. */
			requeue = 1;
			goto hardstart_fail;
		}
		goto ff_bypass;
	}

	/*
	* Fast frames check.
	*/
	ATH_FF_MAGIC_CLR(skb);
	an = ATH_NODE(ni);

	txq = sc->sc_ac2q[skb->priority];

	if (txq->axq_depth > TAIL_DROP_COUNT) {
		/* Wish to reserve some DMA buffers, try again later. */
		requeue = 1;
		goto hardstart_fail;
	}
#endif

	/* If the skb data is shared, we will copy it so we can strip padding
	* without affecting any other bridge ports. */
	if (skb_cloned(skb)) {
		/* Remember the original SKB so we can free up our references */
		struct sk_buff *skb_orig = skb;
		skb = skb_copy(skb, GFP_ATOMIC);
		if (skb == NULL) {
			DPRINTF(sc, ATH_DEBUG_XMIT,
				"Dropping; skb_copy failure.\n");
			/* No free RAM, do not requeue! */
			goto hardstart_fail;
		}
		ieee80211_skb_copy_noderef(skb_orig, skb);
		ieee80211_dev_kfree_skb(&skb_orig);
	}
	eh = (struct ether_header *)skb->data;

#ifdef ATH_SUPERG_FF
	/* NB: use this lock to protect an->an_tx_ffbuf (and txq->axq_stageq)
	*     in athff_can_aggregate() call too. */
	ATH_TXQ_LOCK_IRQ(txq);
	if (athff_can_aggregate(sc, eh, an, skb,
		ni->ni_vap->iv_fragthreshold, &ff_flush)) {
		if (an->an_tx_ffbuf[skb->priority]) { /* i.e., frame on the staging queue */
			bf = an->an_tx_ffbuf[skb->priority];

			/* get (and remove) the frame from staging queue */
			TAILQ_REMOVE(&txq->axq_stageq, bf, bf_stagelist);
			an->an_tx_ffbuf[skb->priority] = NULL;

			/*
			* chain skbs and add FF magic
			*
			* NB: the arriving skb should not be on a list (skb->list),
			*     so "re-using" the skb next field should be OK.
			*/
			bf->bf_skb->next = skb;
			skb->next = NULL;
			skb = bf->bf_skb;
			ATH_FF_MAGIC_PUT(skb);

			DPRINTF(sc, ATH_DEBUG_XMIT | ATH_DEBUG_FF,
				"Aggregating fast-frame\n");
		}
		else {
			/* NB: Careful grabbing the TX_BUF lock since still
			*     holding the TXQ lock.  This could be avoided
			*     by always obtaining the TXBuf earlier, but
			*     the "if" portion of this "if/else" clause would
			*     then need to give the buffer back. */
			bf = ath_take_txbuf(sc);
			if (bf == NULL) {
				ATH_TXQ_UNLOCK_IRQ_EARLY(txq);
				/* All DMA buffers full, safe to try again. */
				goto hardstart_fail;
			}
			DPRINTF(sc, ATH_DEBUG_XMIT | ATH_DEBUG_FF,
				"Adding to fast-frame stage queue\n");

			bf->bf_skb = skb;
			bf->bf_node = ieee80211_ref_node(ni);
			bf->bf_queueage = txq->axq_totalqueued;
			an->an_tx_ffbuf[skb->priority] = bf;

			TAILQ_INSERT_HEAD(&txq->axq_stageq, bf, bf_stagelist);

			ATH_TXQ_UNLOCK_IRQ_EARLY(txq);

			return NETDEV_TX_OK;
		}
	}
	else {
		if (ff_flush) {
			struct ath_buf *bf_ff = an->an_tx_ffbuf[skb->priority];
			int success = 0;

			TAILQ_REMOVE(&txq->axq_stageq, bf_ff, bf_stagelist);
			an->an_tx_ffbuf[skb->priority] = NULL;

			/* NB: ath_tx_start -> ath_tx_txqaddbuf uses ATH_TXQ_LOCK too */
			ATH_TXQ_UNLOCK_IRQ_EARLY(txq);

			/* Encap. and transmit */
			bf_ff->bf_skb = ieee80211_encap(ni, bf_ff->bf_skb,
				&framecnt);
			if (bf_ff->bf_skb == NULL) {
				DPRINTF(sc, ATH_DEBUG_XMIT,
					"Dropping; fast-frame flush encap. "
					"failure\n");
				sc->sc_stats.ast_tx_encap++;
			}
			else {
				pktlen = bf_ff->bf_skb->len;	/* NB: don't reference skb below */
				if (!ath_tx_start(dev, ni, bf_ff,
					bf_ff->bf_skb, 0))
					success = 1;
			}

			if (!success) {
				DPRINTF(sc, ATH_DEBUG_XMIT | ATH_DEBUG_FF,
					"Dropping; fast-frame stageq flush "
					"failure\n");
				ath_return_txbuf(sc, &bf_ff);
			}
			bf = ath_take_txbuf(sc);
			if (bf == NULL) {
				/* All DMA buffers full, safe to try again. */
				requeue = 1;
				goto hardstart_fail;
			}

			goto ff_flush_done;
		}
		/* XXX: out-of-order condition only occurs for AP mode and
		*      multicast.  But, there may be no valid way to get
		*      this condition. */
		else if (an->an_tx_ffbuf[skb->priority]) {
			DPRINTF(sc, ATH_DEBUG_XMIT | ATH_DEBUG_FF,
				"Discarding; out of sequence fast-frame\n");
		}

		bf = ath_take_txbuf(sc);
		if (bf == NULL) {
			ATH_TXQ_UNLOCK_IRQ_EARLY(txq);
			/* All DMA buffers full, safe to try again. */
			requeue = 1;
			goto hardstart_fail;
		}
	}

	ATH_TXQ_UNLOCK_IRQ(txq);

ff_flush_done:
ff_bypass :

#else /* ATH_SUPERG_FF */

	bf = ath_take_txbuf(sc);
	if (bf == NULL) {
		/* All DMA buffers full, safe to try again. */
		requeue = 1;
		goto hardstart_fail;
	}

#endif /* ATH_SUPERG_FF */

	/*
	* Encapsulate the packet for transmission.
	*/
	skb = ieee80211_encap(ni, skb, &framecnt);
	if (skb == NULL) {
		DPRINTF(sc, ATH_DEBUG_XMIT,
			"Dropping; encapsulation failure\n");
		sc->sc_stats.ast_tx_encap++;
		goto hardstart_fail;
	}

	if (framecnt > 1) {
		unsigned int bfcnt;

		/*
		*  Allocate 1 ath_buf for each frame given 1 was
		*  already alloc'd
		*/
		ATH_TXBUF_LOCK_IRQ(sc);
		for (bfcnt = 1; bfcnt < framecnt; ++bfcnt) {
			tbf = ath_take_txbuf_locked(sc);
			if (tbf == NULL)
				break;
			tbf->bf_node = ieee80211_ref_node(SKB_CB(skb)->ni);
			STAILQ_INSERT_TAIL(&bf_head, tbf, bf_list);
		}

		if (bfcnt != framecnt) {
			ath_return_txbuf_list_locked(sc, &bf_head);
			ATH_TXBUF_UNLOCK_IRQ_EARLY(sc);
			STAILQ_INIT(&bf_head);
			goto hardstart_fail;
		}
		ATH_TXBUF_UNLOCK_IRQ(sc);

		while (((bf = STAILQ_FIRST(&bf_head)) != NULL) && (skb != NULL)) {
			unsigned int nextfraglen = 0;

			STAILQ_REMOVE_HEAD(&bf_head, bf_list);
			tskb = skb->next;
			skb->next = NULL;
			if (tskb)
				nextfraglen = tskb->len;

			if (ath_tx_start(dev, ni, bf, skb, nextfraglen) != 0) {
				STAILQ_INSERT_TAIL(&bf_head, bf, bf_list);
				skb->next = tskb;
				goto hardstart_fail;
			}
			skb = tskb;
		}
	}
	else {
		if (ath_tx_start(dev, ni, bf, skb, 0) != 0) {
			STAILQ_INSERT_TAIL(&bf_head, bf, bf_list);
			goto hardstart_fail;
		}
	}

	ni = NULL;

#ifdef ATH_SUPERG_FF
	/* flush out stale FF from staging Q for applicable operational modes. */
	/* XXX: ADHOC mode too? */
	if (txq && ic->ic_opmode == IEEE80211_M_HOSTAP)
		ath_ffstageq_flush(sc, txq, ath_ff_ageflushtestdone);
#endif

	return NETDEV_TX_OK;

hardstart_fail:
	/* Clear all SKBs from the buffers, we will clear them separately IF
	* we do not requeue them. */
	ATH_TXBUF_LOCK_IRQ(sc);
	STAILQ_FOREACH_SAFE(tbf, &bf_head, bf_list, tempbf) {
		tbf->bf_skb = NULL;
	}
	ATH_TXBUF_UNLOCK_IRQ(sc);
	/* Release the buffers, now that skbs are disconnected */
	ath_return_txbuf_list(sc, &bf_head);
	/* Pass control of the skb to the caller (i.e., resources are their
	* problem). */
	if (requeue) {
		/* Queue is full, let the kernel backlog the skb */
		netif_stop_queue(dev);
		sc->sc_devstopped = 1;
		/* Stop tracking again we are giving it back*/
		ieee80211_skb_untrack(skb);
		return NETDEV_TX_BUSY;
	}
	/* Now free the SKBs */
	ieee80211_dev_kfree_skb_list(&skb);
	return NETDEV_TX_OK;
}



static int
ath_tx_startraw(struct net_device *dev, struct ath_buf *bf, struct sk_buff *skb)
{
	struct ath_softc *sc = dev->priv;
	struct ath_hal *ah = sc->sc_ah;
	struct ieee80211_phy_params *ph = (struct ieee80211_phy_params *)
		(SKB_CB(skb) + sizeof(struct ieee80211_cb));
	const HAL_RATE_TABLE *rt;
	unsigned int pktlen, hdrlen, try0, power;
	HAL_PKT_TYPE atype;
	u_int flags;
	u_int8_t antenna, txrate;
	struct ath_txq *txq = NULL;
	struct ath_desc *ds = NULL;
	struct ieee80211_frame *wh;

	wh = (struct ieee80211_frame *)skb->data;
	try0 = ph->try0;
	rt = sc->sc_currates;
	txrate = dot11_to_ratecode(sc, rt, ph->rate0);
	power = ph->power > 60 ? 60 : ph->power;
	hdrlen = ieee80211_anyhdrsize(wh);
	pktlen = skb->len + IEEE80211_CRC_LEN;

	flags = HAL_TXDESC_INTREQ | HAL_TXDESC_CLRDMASK; /* XXX needed for crypto errs */

	bf->bf_skbaddr = bus_map_single(sc->sc_bdev,
		skb->data, pktlen, BUS_DMA_TODEVICE);
	DPRINTF(sc, ATH_DEBUG_XMIT, "skb=%p [data %p len %u] skbaddr %llx\n",
		skb, skb->data, skb->len, ito64(bf->bf_skbaddr));

	bf->bf_skb = skb;
	KASSERT((bf->bf_node == NULL), ("Detected node reference leak"));
#ifdef ATH_SUPERG_FF
	bf->bf_numdescff = 0;
#endif

	/* setup descriptors */
	ds = bf->bf_desc;
	rt = sc->sc_currates;
	KASSERT(rt != NULL, ("no rate table, mode %u", sc->sc_curmode));

	if (IEEE80211_IS_MULTICAST(wh->i_addr1)) {
		flags |= HAL_TXDESC_NOACK;	/* no ack on broad/multicast */
		sc->sc_stats.ast_tx_noack++;
		try0 = 1;
	}
	atype = HAL_PKT_TYPE_NORMAL;		/* default */
	txq = sc->sc_ac2q[skb->priority & 0x3];


	flags |= HAL_TXDESC_INTREQ;
	antenna = sc->sc_txantenna;

	/* XXX check return value? */
	ath_hal_setuptxdesc(ah, ds,
		pktlen,			/* packet length */
		hdrlen,			/* header length */
		atype,			/* Atheros packet type */
		power,			/* txpower */
		txrate, try0,		/* series 0 rate/tries */
		HAL_TXKEYIX_INVALID,	/* key cache index */
		antenna,			/* antenna mode */
		flags,			/* flags */
		0,				/* rts/cts rate */
		0,				/* rts/cts duration */
		0,				/* comp icv len */
		0,				/* comp iv len */
		ATH_COMP_PROC_NO_COMP_NO_CCS /* comp scheme */
		);

	if (ph->try1) {
		ath_hal_setupxtxdesc(sc->sc_ah, ds,
			dot11_to_ratecode(sc, rt, ph->rate1), ph->try1,
			dot11_to_ratecode(sc, rt, ph->rate2), ph->try2,
			dot11_to_ratecode(sc, rt, ph->rate3), ph->try3
			);
	}
	bf->bf_flags = flags;	/* record for post-processing */

	ds->ds_link = 0;
	ds->ds_data = bf->bf_skbaddr;

	ath_hal_filltxdesc(ah, ds,
		skb->len,	/* segment length */
		AH_TRUE,	/* first segment */
		AH_TRUE,	/* last segment */
		ds		/* first descriptor */
		);

	/* NB: The desc swap function becomes void,
	* if descriptor swapping is not enabled
	*/
	ath_desc_swap(ds);

	DPRINTF(sc, ATH_DEBUG_XMIT, "Q%d: %08x %08x %08x %08x %08x %08x\n",
		M_FLAG_GET(skb, M_UAPSD) ? 0 : txq->axq_qnum,
		ds->ds_link, ds->ds_data,
		ds->ds_ctl0, ds->ds_ctl1,
		ds->ds_hw[0], ds->ds_hw[1]);

	ath_tx_txqaddbuf(sc, NULL, txq, bf, ds, pktlen);
	return 0;
}


