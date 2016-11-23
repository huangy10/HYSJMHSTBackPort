//
// Created by 黄延 on 2016/11/2.
//
// Copy Right Reserved.
//
// We implement the TMDA mechanism here.
//

#ifndef BACKPORTS_TDMA_H
#define BACKPORTS_TDMA_H

#endif //BACKPORTS_TDMA_H

#define TDMA_H_DATA 0x0C
#define TDMA_H_SCHEDULE 0x08

struct tdma_hdr {
    u8 fractel_frame;
    u8 packet_type;
    u16 reserved;

    union {
        struct {
            u16 node_id;
            u32 offset;
            u64 reserved;
            u64 hw_ts;
            u64 slot_start;
            u16 this_slot;
            u16 rt_length;
        } schedule_hdr;

        struct {
            u16 flow_id;
            u8 rx[ETH_ALEN];
            u8 tx[ETH_ALEN];
            u8 end_src[ETH_ALEN];
            u8 end_des[ETH_ALEN];
        } data_hdr;
    };
};

bool tdma_is_data(struct tdma_hdr *hdr);
bool tdma_is_schedule(struct tdma_hdr *hdr);

struct tdma_queue {
    sk_buff_head skbs;

    // 考虑中，是否要给每个sta或者vif一个队列？
    struct ieee80211_vif *vif;
    struct ieee80211_sta *sta;

    u8 ac;
    atomic_t len = 0;
    u8 capacity = 0;

    // 给队列定义了一个自旋锁，那么当不同线程在访问队列的时候可以保证安全。
    // 当访问队列之前，应当用spin_lock来上锁
    spinlock_t lock;

    //
    struct ieee80211_local *local;
};

void tdma_queue_tail(struct tdma_queue *txq, struct sk_buff *skb);
struct sk_buff* tdma_dequeue(struct tdma_queue *txq);


#define TOPOLOGY_CAPACITY 100
#define NODE_MAX_DEGREE 8


// tdma的状态机
struct tdma_sc {
    u64 ts_offset;  // 时钟同步偏移

    tdma_queue *txq;    // 暂时定义一个全局队列
    tdma_queue *txq_h;  // 高优先级队列

    u8 nodes_count;       // 节点的数量
    u8 topology[TOPOLOGY_CAPACITY];
    int topo_len;
    u8 neighors[NODE_MAX_DEGREE];

    timer_list timer;

    net_device* dev;
};
