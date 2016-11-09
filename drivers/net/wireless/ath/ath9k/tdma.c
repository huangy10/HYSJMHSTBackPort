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


struct tdma_txq {
    list_head head;
};

static void tdma_config() {

}


void tdma_xmit(struct ieee80211_sub_if_data *sdata, struct sta_info *sta, struct sk_buff *skb) {

}

