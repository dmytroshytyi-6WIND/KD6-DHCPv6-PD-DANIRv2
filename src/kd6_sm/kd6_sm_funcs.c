/**
 * File              : kd6_sm_funcs.c
 * Author            : Dmytro Shytyi
 * Site              : https://dmytro.shytyi.net
 * Date              : 11.03.2020
 * Last Modified Date: 18.03.2020
 * Last Modified By  : Dmytro Shytyi
 */
#ifndef KD6_H
	#define KD6_H
	#include "../../include/kd6.h"
#endif
struct in6_addr KD6_LINK_LOCAL_MULTICAST = {{{ 0xff,02,0,0,0,0,0,0,0,0,0,0,0,1,0,2 }}}; 
struct in6_addr KD6_LINK_LOCAL = {{{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }}}; 
struct in6_addr KD6_LINK_NULL = {{{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }}}; 
unsigned long start_jiffies;        // Start timestamp. 
static  DEFINE_SPINLOCK(kd6_recv_lock);

bool kd6_reply_received = false;


static struct in6_addr kd6_servaddr; 
struct kd6_server_id kd6_global_server_id;
struct kd6_ia_prefix kd6_global_ia_prefix;
struct kd6_cb_strct{
	u8 xid[3];
	bool pkt_rcvd;
};
/*
 * Function generates a link local addresses on the interfaces
 */
void	kd6_generate_ll		(char *kd6_if){
	printk(KERN_INFO "[KD6] if=%s Generating ll",kd6_if);
}

/*
 * Convert Month name to number
 */
int GetMon (const char *str){
        const char * month_names[] = {
                "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
                "Sep", "Oct", "Nov", "Dec", NULL };
        int i = 0;
        while (i < 12) {
                if (strcasecmp(month_names[i],str) == 0)
                        break;
                ++i;
        }
        if (i == 12) {
                return 0;
        } else {
                return i + 1; 
        }
}
/*
 * Convert timeDate to seconds
 */
u32 convertTimeDateToSeconds(const struct tm date){
        u32 y;
        u32 m;
        u32 d;
        u32 t;

        //Year
        y = date.tm_year-2000;
        //Month of year
        m = date.tm_mon-1;
        //Day of month
        d = date.tm_mday-2;

        //January and February are counted as months 13 and 14 of the previous year
        if(m <= 2)
        {
                m += 12;
                y -= 1;
        }
        //Convert years to days
	/* Sakamotos Algorithm.
	* Explanation https://geeksforgeeks.org/tomohiko-sakamotos-algorithm-finging-day-week/
	* Jan 1st 1 AD is a Monday in Gregorian calendar.
	* Let us consider the first case in which we do not have leap years, hence
	* total number of days in each year is 365.January has 31 days i.e 7*4+3 days
	* so the day on 1st feb will always be 3 days ahead of the day on 1st January.
	* Now february has 28 days(excluding leap years) which is exact multiple 
	* of 7 (7*4=28) Hence there will be no change in the month of march and it 
	* will also be 3 days ahead of the day on 1st January of that respective year.
	* Considering this pattern, if we create an array of the leading number of days
	* for each month then it will be given as t[] = {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5}.
	* Now let us look at the real case when there are leap years. Every 4 years, our 
	* calculation will gain one extra day. Except every 100 years when it doesnt. 
	* Except every 400 years when it does. How do we put in these additional days? 
	* Well, just add y/4 <96> y/100 + y/400. Note that all division is integer division. 
	* This adds exactly the required number of leap days.But here is a problem, the leap 
	* day is 29 Feb and not 0 January.This means that the current year should not be 
	* counted for the leap day calculation for the first two months.Suppose that if 
	* the month were January or February, we subtracted 1 from the year. This means that
	* during these months, the y/4 value would be that of the previous year and would 
	* not be counted. If we subtract 1 from the t[] values of every month after February?
	* That would fill the gap, and the leap year problem is solved.That is, we need to make 
	* the following changes:
	* 1.t[] now becomes {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4}.
	* 2.if m corresponds to Jan/Feb (that is, month<3) we decrement y by 1.
	* 3.the annual increment inside the modulus is now y + y/4 <96> y/100 + y/400 
	* in place of y.
	*/	
        t = (365 * y) + (y / 4) - (y / 100) + (y / 400);
        //Convert months to days
        t += (30 * m) + (3 * (m + 1) / 5) + d;
        //Convert days to seconds
        t *= 86400;
        //Add hours, minutes and seconds
        t += (3600 * (date.tm_hour-1)) + (60 * date.tm_min) + date.tm_sec;

        //Return Unix time
        return t;
}


/*
 * Function is sending solicit message from RR to SR
 */
void	kd6_send_solicit	(char* kd6_if, u8 kd6_xid[3]){
	struct tm kd6_ktime = {0};
	char kd6_time_month[3];
	char* ver;
        long kernelCompilationTimeStartingFrom2000;
	struct net_device *dev;
        struct sk_buff *skb;
	u32 duid_time;
	u8  val_time[4] = {0x00,0x17,0x00,0x18};
        u8 t1[4]={0x00,0x00,0x0e,0x10};
        u8 t2[4]={0x00,0x00,0x15,0x18};
        u16 msecs_htons;
        u8 ipv6_my_multicast[6]={0x33,0x33,0x00,0x01,0x00,0x02};
	
	struct kd6_packet dhp;
 	struct udphdr *udph;
        struct ipv6hdr *ipv6h;
        struct ethhdr *ethh;
	__wsum csum;

	printk(KERN_INFO "[KD6] if=%s Sending solicit from RR to SR",kd6_if);

	rtnl_lock();
	dev = kmalloc(sizeof (struct net_device),GFP_KERNEL);
	//retrieve the device strucutre that has our wan_if_name
	/*
	for_each_netdev(&init_net, kd6_dev){
		//strcmp returns 0 if strings are same
		if (!strcmp(kd6_dev->name,kd6_if)){
			memcpy(dev,kd6_dev,sizeof(struct net_device));
			printk(KERN_INFO "IF %s FOUND", dev);
			break;
			return;
		}
	}
	*/
        dev=dev_get_by_name(&init_net,kd6_if);
        skb = alloc_skb(sizeof(struct ethhdr) +
			sizeof(struct udphdr) +
			sizeof(struct ipv6hdr) +
			sizeof(struct kd6_packet_sol), GFP_KERNEL);
	if (!skb)
        	return;

	//memset (skb, 0, sizeof(*skb));

        skb->dev = dev;
        skb->pkt_type = PACKET_OUTGOING;
        skb->protocol = htons(ETH_P_IPV6);
        skb->no_fcs = 1;

        skb_reserve(skb, sizeof (struct ethhdr) + sizeof(struct udphdr)+ sizeof(struct ipv6hdr));
	dhp.kd6_sol = (struct kd6_packet_sol *) skb_put (skb, sizeof (struct kd6_packet_sol));
	memset (dhp.kd6_sol, 0, sizeof(struct kd6_packet_sol)); 
	dhp.kd6_sol->msg_type = KD6_SOLICIT;
	memcpy(dhp.kd6_sol->transaction_id,kd6_xid,sizeof(u8)*3);

	//client id option
	dhp.kd6_sol->my_client_id.option_client_id = htons(1);
	dhp.kd6_sol->my_client_id.option_len = htons(14);
	dhp.kd6_sol->my_client_id.duid_type = htons(1);
	dhp.kd6_sol->my_client_id.hw_type = htons(1);
	
	//DUID time convert kernel vertsion to compatible option value 
	ver = utsname()->version;
	sscanf(ver, "%*s %*s %*s %*s %s %d %d:%d:%d %*s %ld",kd6_time_month, &kd6_ktime.tm_mday, &kd6_ktime.tm_hour, &kd6_ktime.tm_min, &kd6_ktime.tm_sec, &kd6_ktime.tm_year);
	kd6_ktime.tm_mon=GetMon(kd6_time_month);
	kernelCompilationTimeStartingFrom2000 = convertTimeDateToSeconds(kd6_ktime);
	duid_time=htonl( kernelCompilationTimeStartingFrom2000 & 0xffffffff);
	memcpy (&(dhp.kd6_sol->my_client_id.duid_time),&duid_time,sizeof(duid_time));
	memcpy (dhp.kd6_sol->my_client_id.my_hw_addr, dev->dev_addr, 48);

	//oro option
	dhp.kd6_sol->oro.option = htons(6);
	dhp.kd6_sol->oro.option_len = htons(4);
	memcpy(dhp.kd6_sol->oro.value,val_time,sizeof(val_time));

	//time option
	dhp.kd6_sol->time.option_time = htons(8);
	dhp.kd6_sol->time.option_len = htons(2);
	msecs_htons = htons(jiffies_to_msecs(jiffies-start_jiffies));
	memcpy(&(dhp.kd6_sol->time.value),&msecs_htons,sizeof(msecs_htons));

	//ia pd option
	dhp.kd6_sol->ia_pd.option_ia_pd = htons(0x19);
	dhp.kd6_sol->ia_pd.option_len = htons(0x0c);
	dhp.kd6_sol->ia_pd.iaid[0]=dev->dev_addr[2];
	dhp.kd6_sol->ia_pd.iaid[1]=dev->dev_addr[3];
	dhp.kd6_sol->ia_pd.iaid[2]=dev->dev_addr[4];
	dhp.kd6_sol->ia_pd.iaid[3]=dev->dev_addr[5];
	memcpy(dhp.kd6_sol->ia_pd.t1,t1,sizeof(t1));
	memcpy(dhp.kd6_sol->ia_pd.t2,t2,sizeof(t2));
	
	//udp
	ipv6_dev_get_saddr(&init_net, dev, &KD6_LINK_LOCAL_MULTICAST, 0, &KD6_LINK_LOCAL);
	udph = (struct udphdr *) skb_push (skb, sizeof (struct udphdr));
	udph->source = htons(546);
	udph->dest = htons(547);
	udph->len = htons(sizeof(struct udphdr)+sizeof(struct kd6_packet_sol));
	udph->check = 0;
	csum = csum_partial((char *) udph, sizeof(struct udphdr)+sizeof(struct kd6_packet_sol),0);
	udph->check = csum_ipv6_magic(	&KD6_LINK_LOCAL,
		       			&KD6_LINK_LOCAL_MULTICAST, 
					sizeof(struct udphdr)+sizeof(struct kd6_packet_sol),
					IPPROTO_UDP,csum);
	
	//ipv6
	ipv6h = (struct ipv6hdr *) skb_push (skb,sizeof (struct ipv6hdr));
	ipv6h->version = 6;
	ipv6h->nexthdr = IPPROTO_UDP;
	ipv6h->payload_len = htons(sizeof(struct udphdr)+sizeof(struct kd6_packet_sol));
	ipv6h->daddr = KD6_LINK_LOCAL_MULTICAST;
	ipv6h->saddr = KD6_LINK_LOCAL;
	ipv6h->hop_limit = 255;
	
	//eth
 	ethh = (struct ethhdr *) skb_push (skb, sizeof(struct ethhdr));
        ethh->h_proto = htons(ETH_P_IPV6);
        memcpy(ethh->h_source, dev->dev_addr, ETH_ALEN);

        memcpy (ethh->h_dest, ipv6_my_multicast, sizeof(ipv6_my_multicast));
	if (dev_queue_xmit(skb) < 0)
                printk(KERN_INFO "[KD6] if=%s Error-dev_queue_xmit failed",kd6_if);
	else
		printk(KERN_INFO "[KD6] if=%s Solicit packet is sent",kd6_if);
	rtnl_unlock();
}
/*
 * Function is polling on SR side for received solicit message sent by RR.
 */
void	kd6_receive_solicit	(char *kd6_if){
	printk(KERN_INFO "[KD6] if=%s Receiving solicit sent by RR on the SR side",kd6_if);
}
/*
 * Function sends advertise from SR to RR. 
 */
void	kd6_send_advertise	(char *kd6_if){
	printk(KERN_INFO "[KD6] if=%s Send advertise from SR to RR",kd6_if);

}

/*
 * Function to parse on RR side received advertise from SR
 */
int kd6_parse_received_advertise(u8 *kd6_packet, u8 len,u8 kd6_xid[3]){
        u8 pointer=0;
        u16 kd6_option;
        u16 dns_size;
        u16 domain_list;
	u16 kd6_opt_type;

        struct kd6_client_id *kd6_server_id;
        struct kd6_client_id *kd6_client_id;
        struct kd6_ia_pd *kd6_ia_pd;
        struct kd6_ia_prefix *kd6_ia_prefix;
        kd6_ia_pd = kmalloc (sizeof (struct kd6_ia_pd),GFP_KERNEL);
        kd6_ia_prefix = kmalloc (sizeof (struct kd6_ia_prefix),GFP_KERNEL);
        kd6_client_id = kmalloc (sizeof (struct kd6_client_id),GFP_KERNEL);
        kd6_server_id = kmalloc (sizeof (struct kd6_client_id),GFP_KERNEL);

        kd6_packet+=4; //first option
        while (pointer<len){
                memcpy(&kd6_option,kd6_packet+pointer, sizeof(u16));
                pr_debug("KD6: kd6_op_ntohs %d",ntohs(kd6_option));
                switch (ntohs(kd6_option)){
                        case 1: // Client identifier
                                memcpy(kd6_client_id, kd6_packet+pointer, sizeof (struct kd6_client_id));
                                pointer+=sizeof(struct kd6_client_id);
                                break;
                        case 2: // Server identifier
                                memcpy(kd6_server_id, kd6_packet+pointer, sizeof (struct kd6_client_id));
                                pointer+=sizeof(struct kd6_client_id);
                                break;
                        case 23:        // DNS recursive nameserver
                                memcpy(&dns_size,kd6_packet+pointer+2,sizeof (u16));
                                pointer+=ntohs(dns_size)+4;
                                break;
                        case 24:        // Domain search list
                                memcpy(&domain_list,kd6_packet+pointer+2,sizeof (u16));
                                pointer+=ntohs(domain_list)+4;
                                break;
                        case 25:        // IA PD
                                memcpy(kd6_ia_pd,kd6_packet+pointer,sizeof(struct kd6_ia_pd));
                                pointer+=sizeof(struct kd6_ia_pd);

                                memcpy(&kd6_opt_type,kd6_packet+pointer,sizeof (u16));
				if (ntohs(kd6_opt_type) == 13){
					pr_err("[KD6] ERROR - Server responded: No prefix available");
					goto ex;
				}else{
				memcpy(kd6_ia_prefix,kd6_packet+pointer,sizeof(struct kd6_ia_prefix));
				pointer+=sizeof(struct kd6_ia_prefix);
				}
                                break;
                        default:
                                pr_err("[KD6] ERROR - unknown option |function kd6_parse_received_advertise");
                                pr_info("[KD6] DEFAULT: %x",kd6_option);
                                pr_info("[KD6] pointer: %d", pointer);
                                goto ex;
                }
        }

        memcpy(&kd6_global_server_id,kd6_server_id,sizeof(struct kd6_server_id));
        memcpy(&kd6_global_ia_prefix,kd6_ia_prefix,sizeof(struct kd6_ia_prefix));
        //memcpy(kd6_servaddr_hw,kd6_server_id->my_hw_addr,sizeof(kd6_servaddr_hw));
ex:
        return 0;
}
/*
 * Function processes advertise on the RR side sent from SR
 */
static unsigned int _kd6_receive_advertise(void *priv, struct sk_buff *skb, const struct nf_hook_state *state){
        struct udphdr *udph;
        struct ipv6hdr *ipv6h;
	struct kd6_cb_strct *kd6_strct;
	u8 kd6_xid[3];
        u8 rx_xid[3];
	u8 kd6_msg_size;
	u8 msg_type;
	long dh6_offset;
        u8 *dhp;
	kd6_strct=(struct kd6_cb_strct *)priv;
	
	memset(kd6_xid,0,sizeof(u8)*3);	
	memcpy(kd6_xid,kd6_strct->xid,sizeof(u8)*3);

        if (skb->pkt_type == PACKET_OTHERHOST){
                goto drop;
        }

        skb = skb_share_check(skb, GFP_ATOMIC);
        if (!skb)
               return NET_RX_DROP;

        spin_lock(&kd6_recv_lock);
        if (!pskb_may_pull(skb,
                                sizeof(struct ipv6hdr) +
                                sizeof(struct udphdr))){
                //goto drop;
		goto drop_unlock;
        }

        ipv6h = (struct ipv6hdr*) skb_network_header(skb);
        udph = (struct udphdr*) skb_transport_header(skb);

        if (udph->source != htons(547) || udph->dest != htons(546)){
               // goto drop;
	        goto drop_unlock;
        }
        // Ok the front looks good, make sure we can get at the rest.  
        if (!pskb_may_pull(skb, skb->len))
                //goto drop;
	        goto drop_unlock;

        kd6_msg_size =
                skb->len -
                (sizeof(struct ipv6hdr)+
                 sizeof(struct udphdr)+
                 4);//message type + transaction id

        dh6_offset = sizeof (struct udphdr);
        dhp = (u8 *) udph + dh6_offset;
        dhp+=1; //Transaction ID pointer

        memcpy(rx_xid,dhp,sizeof(u8)*3);
        if (memcmp(rx_xid, kd6_xid,3) != 0){
               net_err_ratelimited("KD6: Reply not for us, ,rx_xid[%x%x%x],internal_xid [%x%x%x]\n",
               rx_xid[0],rx_xid[1],rx_xid[2],kd6_xid[0],kd6_xid[1],kd6_xid[2]);
               goto drop_unlock;
	}


        memcpy(&msg_type,dhp-=1,sizeof(u8));

        if (msg_type!=KD6_ADVERTISE)
	        goto drop_unlock;
	pr_info ("[KD6] advertise packed succesfully received");
	kd6_strct->pkt_rcvd = true;
        kd6_parse_received_advertise(dhp,kd6_msg_size,kd6_xid);
        spin_unlock(&kd6_recv_lock);
	return NF_DROP;	
drop_unlock:
        /* Show's over.  Nothing to see here.  */
        spin_unlock(&kd6_recv_lock);
drop:
        return NF_ACCEPT;

}
/*
 * Function receives advertise on the RR side sent from SR
 */
int    kd6_receive_advertise   (char* kd6_if,u8 kd6_xid[3],bool (*ptr_th_should_stop)(void)){
	int kd6_cnt=0;	
	bool kd6_advertise_received=false;
	struct net_device* kd6_dev;
        static struct nf_hook_ops kd6_rcv_advertise_hook = {
                .hook = _kd6_receive_advertise,
                //.dev = kd6_dev,
                ////.pf = NFPROTO_IPV6,
                .pf = PF_INET6,
                .hooknum = (1 << NF_INET_PRE_ROUTING),
                .priority = NF_IP6_PRI_FIRST,
        };
	struct kd6_cb_strct kd6_strct;
        printk(KERN_INFO "[KD6] if=%s Receiving advertise on the RR side sent from SR",kd6_if); 
        kd6_dev = kmalloc(sizeof (struct net_device),GFP_KERNEL);
        kd6_dev=dev_get_by_name(&init_net,kd6_if);
	kd6_rcv_advertise_hook.dev = kd6_dev;
        
	memcpy(kd6_strct.xid,kd6_xid,sizeof (u8)*3);
	kd6_strct.pkt_rcvd=kd6_advertise_received;
	memcpy (kd6_strct.xid,kd6_xid,sizeof(u8)*3);
	kd6_rcv_advertise_hook.priv = kmalloc (sizeof (kd6_strct),GFP_KERNEL);
	kd6_rcv_advertise_hook.priv = &kd6_strct;
       
       	nf_register_net_hook (&init_net, &kd6_rcv_advertise_hook);
        
	while (!(kd6_strct.pkt_rcvd || (*ptr_th_should_stop)())){
		pr_info ("[KD6] if=%s Waiting for advertise packet...",kd6_if);
		ssleep(1);
		kd6_cnt++;
		if (kd6_cnt > 15){
			nf_unregister_net_hook (&init_net, &kd6_rcv_advertise_hook);
			return 1;
		}
	}
	nf_unregister_net_hook (&init_net, &kd6_rcv_advertise_hook);
	return 0;
}

/*
 * Function receives the request on the SR side sent by RR
 */
void	kd6_receive_request	(char *kd6_if){
	printk(KERN_INFO "[KD6] if=%s Receiving the request on the SR side sent by RR",kd6_if);
}
/*
 * Function sends the request the from RR to SR
 */
void	kd6_send_request	(char *kd6_if, u8 kd6_xid[3]){
	struct tm kd6_ktime = {0};
	char kd6_time_month[3];
	char* ver;
        long kernelCompilationTimeStartingFrom2000;
	struct net_device *dev;
        struct sk_buff *skb;
	u32 duid_time;
	u8  val_time[4] = {0x00,0x17,0x00,0x18};
        u8 t1[4]={0x00,0x00,0x0e,0x10};
        u8 t2[4]={0x00,0x00,0x15,0x18};
        u8 ipv6_my_multicast[6]={0x33,0x33,0x00,0x01,0x00,0x02};
	u16 msecs_htons;
	__wsum csum; 
	struct kd6_packet dhp;
 	struct udphdr *udph;
        struct ipv6hdr *ipv6h;
        struct ethhdr *ethh;

	printk(KERN_INFO "[KD6] if=%s Send request from RR to SR",kd6_if);
	rtnl_lock();
	dev = kmalloc(sizeof (struct net_device),GFP_KERNEL);
        dev=dev_get_by_name(&init_net,kd6_if);
        skb = alloc_skb(sizeof(struct ethhdr) +
			sizeof(struct udphdr) +
			sizeof(struct ipv6hdr) +
			sizeof(struct kd6_packet_req), GFP_KERNEL);
	if (!skb)
        	return;

	//memset (skb, 0, sizeof(*skb));

        skb->dev = dev;
        skb->pkt_type = PACKET_OUTGOING;
        skb->protocol = htons(ETH_P_IPV6);
        skb->no_fcs = 1;

        skb_reserve(skb, sizeof (struct ethhdr) + sizeof(struct udphdr)+ sizeof(struct ipv6hdr));
	dhp.kd6_req = (struct kd6_packet_req *) skb_put (skb, sizeof (struct kd6_packet_req));
        memset (dhp.kd6_req, 0, sizeof(struct kd6_packet_req));
	
        dhp.kd6_req->msg_type = KD6_REQUEST;
        memcpy(dhp.kd6_req->transaction_id,kd6_xid,sizeof(u8)*3);
	
        //client id option
	dhp.kd6_req->my_client_id.option_client_id = htons(1);
	dhp.kd6_req->my_client_id.option_len = htons(14);
	dhp.kd6_req->my_client_id.duid_type = htons(1);
	dhp.kd6_req->my_client_id.hw_type = htons(1);
	//DUID time convert kernel vertsion to compatible option value 
	ver = utsname()->version;
	sscanf(ver, "%*s %*s %*s %*s %s %d %d:%d:%d %*s %ld",kd6_time_month, &kd6_ktime.tm_mday, &kd6_ktime.tm_hour, &kd6_ktime.tm_min, &kd6_ktime.tm_sec, &kd6_ktime.tm_year);
	kd6_ktime.tm_mon=GetMon(kd6_time_month);
	kernelCompilationTimeStartingFrom2000 = convertTimeDateToSeconds(kd6_ktime);
	duid_time=htonl( kernelCompilationTimeStartingFrom2000 & 0xffffffff);
	memcpy (&(dhp.kd6_req->my_client_id.duid_time),&duid_time,sizeof(duid_time));
	memcpy (dhp.kd6_req->my_client_id.my_hw_addr, dev->dev_addr, 48);
	
	//server id option
	memcpy (&(dhp.kd6_req->my_server_id),&kd6_global_server_id ,sizeof(dhp.kd6_req->my_server_id));
	
	//oro option
	dhp.kd6_req->oro.option = htons(6);
	dhp.kd6_req->oro.option_len = htons(4);
	memcpy(dhp.kd6_req->oro.value,val_time,sizeof(val_time));
	
	
	//time option
	dhp.kd6_req->time.option_time = htons(8);
	dhp.kd6_req->time.option_len = htons(2);
	msecs_htons = htons(jiffies_to_msecs(jiffies-start_jiffies));
	memcpy(&(dhp.kd6_req->time.value),&msecs_htons,sizeof(msecs_htons));
	
	
	//ia pd option
	dhp.kd6_req->ia_pd.option_ia_pd = htons(0x19);
	dhp.kd6_req->ia_pd.option_len = htons(0x0c);
	dhp.kd6_req->ia_pd.iaid[0]=dev->dev_addr[2];
	dhp.kd6_req->ia_pd.iaid[1]=dev->dev_addr[3];
	dhp.kd6_req->ia_pd.iaid[2]=dev->dev_addr[4];
	dhp.kd6_req->ia_pd.iaid[3]=dev->dev_addr[5];
	memcpy(dhp.kd6_req->ia_pd.t1,t1,sizeof(t1));
	memcpy(dhp.kd6_req->ia_pd.t2,t2,sizeof(t2));
	
	//ia pd prefix
	memcpy (&(dhp.kd6_req->ia_prefix),&kd6_global_ia_prefix ,sizeof(dhp.kd6_req->ia_prefix));

	//udp
	ipv6_dev_get_saddr(&init_net, dev, &KD6_LINK_LOCAL_MULTICAST, 0, &KD6_LINK_LOCAL);
	udph = (struct udphdr *) skb_push (skb, sizeof (struct udphdr));
	udph->source = htons(546);
	udph->dest = htons(547);
	udph->len = htons(sizeof(struct udphdr)+sizeof(struct kd6_packet_req));
	udph->check = 0;
	csum = csum_partial((char *) udph, sizeof(struct udphdr)+sizeof(struct kd6_packet_req),0);
	udph->check = csum_ipv6_magic(	&KD6_LINK_LOCAL,
		       			&KD6_LINK_LOCAL_MULTICAST, 
					sizeof(struct udphdr)+sizeof(struct kd6_packet_req),
					IPPROTO_UDP,csum);
	
	//ipv6
	ipv6h = (struct ipv6hdr *) skb_push (skb,sizeof (struct ipv6hdr));
	ipv6h->version = 6;
	ipv6h->nexthdr = IPPROTO_UDP;
	ipv6h->payload_len = htons(sizeof(struct udphdr)+sizeof(struct kd6_packet_req));
	ipv6h->daddr = KD6_LINK_LOCAL_MULTICAST;
	ipv6h->saddr = KD6_LINK_LOCAL;
	ipv6h->hop_limit = 255;
	
	//eth
 	ethh = (struct ethhdr *) skb_push (skb, sizeof(struct ethhdr));
        ethh->h_proto = htons(ETH_P_IPV6);
        memcpy(ethh->h_source, dev->dev_addr, ETH_ALEN);

        memcpy (ethh->h_dest, ipv6_my_multicast, sizeof(ipv6_my_multicast));
	if (dev_queue_xmit(skb) < 0)
                printk(KERN_INFO "[KD6] if=%s Error-dev_queue_xmit failed",kd6_if);
	else
		printk(KERN_INFO "[KD6] if=%s Solicit packent is sent",kd6_if);
	rtnl_unlock();
}
/*
 * Function sends reply from SR to RR 
 */
void	kd6_send_reply		(char *kd6_if){
	printk(KERN_INFO "[KD6] if=%s Send reply from SR to RR",kd6_if);
}
/*
 * Function processes reply on the RR side sent from SR
 */
static unsigned int _kd6_receive_reply(void *priv, struct sk_buff *skb, const struct nf_hook_state *state){
        struct udphdr *udph;
        struct ipv6hdr *ipv6h;
	struct kd6_cb_strct *kd6_strct;
	u8 kd6_msg_size;
	u8 kd6_xid[3];
	//pr_info ("[KD6] reply processing started");
        u8 rx_xid[3];
	u8 msg_type;
	long dh6_offset;
        u8 *dhp;

	kd6_strct=(struct kd6_cb_strct *)priv;
	memcpy(kd6_xid,kd6_strct->xid,sizeof(u8)*3);
        if (skb->pkt_type == PACKET_OTHERHOST){
                goto drop;
        }

        skb = skb_share_check(skb, GFP_ATOMIC);
        if (!skb){
		pr_info("[KD6] skb issue");
        	return NET_RX_DROP;
	}
       	spin_lock(&kd6_recv_lock);
        if (!pskb_may_pull(skb,
                                sizeof(struct ipv6hdr) +
                                sizeof(struct udphdr))){
                //goto drop;
		goto drop_unlock;
        }
        ipv6h = (struct ipv6hdr*) skb_network_header(skb);
        udph = (struct udphdr*) skb_transport_header(skb);

        if (udph->source != htons(547) || udph->dest != htons(546)){
               // goto drop;
	        goto drop_unlock;
        }
        // Ok the front looks good, make sure we can get at the rest.  
        if (!pskb_may_pull(skb, skb->len))
                //goto drop;
	        goto drop_unlock;

        kd6_msg_size =
                skb->len -
                (sizeof(struct ipv6hdr)+
                 sizeof(struct udphdr)+
                 4);//message type + transaction id

        dh6_offset = sizeof (struct udphdr);

        dhp = (u8 *) udph + dh6_offset;
        dhp+=1; //Transaction ID pointer
	memset(rx_xid,dhp,3*sizeof(u8));
        memcpy(rx_xid,dhp,3*sizeof(u8));
        if (memcmp(rx_xid, kd6_xid,3) != 0){
               net_err_ratelimited("[KD6] Reply not for us, ,rx_xid[%x%x%x],internal_xid [%x%x%x]\n",
               rx_xid[0],rx_xid[1],rx_xid[2],kd6_xid[0],kd6_xid[1],kd6_xid[2]);
               goto drop_unlock;
	}

        memcpy(&msg_type,dhp-=1,sizeof(u8));

        if (msg_type!=KD6_REPLY)
	        goto drop_unlock;
	//pr_info ("[KD6] reply packed succesfully received");
	kd6_strct->pkt_rcvd = true;
        kd6_parse_received_advertise(dhp,kd6_msg_size,kd6_xid);
	memcpy (&kd6_servaddr,&ipv6h->saddr,sizeof(struct in6_addr));
        spin_unlock(&kd6_recv_lock);
	return NF_DROP;	
drop_unlock:
        /* Show's over.  Nothing to see here.  */
        spin_unlock(&kd6_recv_lock);
drop:
        return NF_ACCEPT;

}
/*
 * Function is processing for reply from SR on the RR side.
 */
void 	kd6_receive_reply 	(char* kd6_if,u8 kd6_xid[3],bool (*ptr_th_should_stop)(void)){
	struct net_device* kd6_dev;
        static struct nf_hook_ops kd6_rcv_reply_hook = {
                .hook = _kd6_receive_reply,
                //.dev = kd6_dev,
                ////.pf = NFPROTO_IPV6,
                .pf = PF_INET6,
                .hooknum = (1 << NF_INET_PRE_ROUTING),
                .priority = NF_IP6_PRI_FIRST,
        };
	struct kd6_cb_strct kd6_strct;

	printk(KERN_INFO "[KD6] if=%s waiting for reply from SR on the RR side",kd6_if);

        kd6_dev = kmalloc(sizeof (struct net_device),GFP_KERNEL);
        kd6_dev=dev_get_by_name(&init_net,kd6_if);
	kd6_rcv_reply_hook.dev = kd6_dev;

	memcpy(kd6_strct.xid,kd6_xid,sizeof(u8)*3);
	kd6_strct.pkt_rcvd=kd6_reply_received;

	kd6_rcv_reply_hook.priv = kmalloc (sizeof (kd6_strct),GFP_KERNEL);
	kd6_rcv_reply_hook.priv = &kd6_strct;
        

        nf_register_net_hook (&init_net, &kd6_rcv_reply_hook);
        
	while (!(kd6_strct.pkt_rcvd || (*ptr_th_should_stop)())){

		pr_info ("[KD6] if=%s Waiting for reply packet...",kd6_if);
		ssleep(1);
	}
	nf_unregister_net_hook (&init_net, &kd6_rcv_reply_hook);
}
/*
 * Function configures the interface
 */
void	 kd6_setup_ifs	(char *kd6_if_wan,char *kd6_if_lan_all[10]){
	struct net_device *kd6_dev;
        struct prefix_info *pinfo;
        bool sllao = false;
        long subprefix_dec;
        char *subprefix_dec_s;
        long subprefix_hex;
        char *subprefix_hex_s;
	int kd6_dev_cnt;

	printk(KERN_INFO "[KD6] if=%s Configuring the interface",kd6_if_wan);
        subprefix_dec_s=kmalloc (2*sizeof (char),GFP_KERNEL);
        subprefix_hex_s=kmalloc (2*sizeof (char),GFP_KERNEL);
        pinfo = kmalloc (sizeof (struct prefix_info),GFP_KERNEL);
        pinfo->prefix_len = 64;//kd6_global_ia_prefix.prefix_len;
        pinfo->valid = (kd6_global_ia_prefix.valid_lifetime);
        //pr_info ("valid_lifetime %d \n",pinfo->valid); 
        pinfo->prefered = (kd6_global_ia_prefix.prefered_lifetime);
        //pr_info("preffered_lifetime %d \n",pinfo->prefered);
        pinfo->onlink = 1;
        pinfo->autoconf = 1;

        memcpy(&(pinfo->prefix.in6_u.u6_addr8),&(kd6_global_ia_prefix.prefix_addr),sizeof(pinfo->prefix.in6_u.u6_addr8));
        memcpy(&(pinfo->prefix.in6_u.u6_addr16),&(kd6_global_ia_prefix.prefix_addr),sizeof(pinfo->prefix.in6_u.u6_addr16));
        memcpy(&(pinfo->prefix.in6_u.u6_addr32),&(kd6_global_ia_prefix.prefix_addr),sizeof(pinfo->prefix.in6_u.u6_addr32));

        rtnl_lock();
	for (kd6_dev_cnt=0;kd6_dev_cnt<10;kd6_dev_cnt++) {
	   if (kd6_if_lan_all[kd6_dev_cnt] != NULL){
		kd6_dev = kmalloc(sizeof (struct net_device),GFP_KERNEL);
        	kd6_dev=dev_get_by_name(&init_net,kd6_if_lan_all[kd6_dev_cnt]);
                
		subprefix_hex=pinfo->prefix.in6_u.u6_addr8[7];
                //pr_info ("subprefix is: %x",subprefix_hex);
                sprintf(subprefix_dec_s,"%ld",subprefix_hex);
                //pr_info("subprefix_dec str %s", subprefix_dec_s);
                kstrtol(subprefix_dec_s,10,&subprefix_dec);
                subprefix_dec=subprefix_dec+1;
                //pr_info("subprefix_dec %d", subprefix_dec);
                sprintf(subprefix_hex_s,"%lx",subprefix_dec);
                kstrtol(subprefix_hex_s,16,&subprefix_hex);
                //pr_info("subprefix_hex %x",subprefix_hex);
                pinfo->prefix.in6_u.u6_addr8[7]=subprefix_hex;

                pr_info("[KD6] if=%s assigning to dev %s prefix %pI64 \n",(char*) kd6_if_wan,(char*)kd6_dev,pinfo->prefix.in6_u.u6_addr32);
                addrconf_prefix_rcv(kd6_dev, (u8 *)pinfo, jiffies + msecs_to_jiffies(999*1000), sllao);
		}
	}
        rtnl_unlock();
	// Let the address of interface be configured before sending the advertise.
	ssleep(5);
}

/*
 * Function configures the default IPv6 rotue
 */
void	kd6_setup_def_route	(char *kd6_if){
	struct net_device *kd6_dev;
	struct fib6_info *rt = NULL;

	printk(KERN_INFO "[KD6] if=%s Configuring default route",kd6_if);
        kd6_dev = kmalloc(sizeof (struct net_device),GFP_KERNEL);
        kd6_dev=dev_get_by_name(&init_net,kd6_if);
	//need to setup default route
        rt = rt6_add_dflt_router(&init_net, &kd6_servaddr, kd6_dev, ICMPV6_ROUTER_PREF_MEDIUM);
        if (!rt) {
                pr_info("KD6: failed to add default route\n");
                return;
        }
        if (rt)
                //set infinite timeout
                fib6_clean_expires(rt);
        //set finite timeout
        //fib6_set_expires(rt, jiffies + msecs_to_jiffies(ntohl(kd6_global_ia_prefix.valid_lifetime)*1000));

}
/*
 * Function sends the config from RR to CT via multicast
 */
void	kd6_send_ra_multicast	(char *kd6_if_lan_all[10]){
	struct sk_buff *skb;
        struct ipv6hdr* ipv6h;
        struct ethhdr *ethh;
        struct net_device *dev;
	int kd6_dev_cnt;
        struct in6_addr LINK_LOCAL_ALL_NODES_MULTICAST = {{{ 0xff,02,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }}};
        struct in6_addr LINK_GLOBAL_UNICAST = {{{ 0x20,0x01,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}};
        struct in6_addr kd6_if_addr_global = {{{ 0, }}};
        struct in6_addr kd6_if_addr_ll = {{{ 0, }}};
        struct icmp6sup_hdr *icmp6h;
	u8 lladdr[6];
	__wsum csum;
	u8 ipv6_my_multicast[6]={0x33,0x33,0x00,0x00,0x00,0x01};
	
	printk(KERN_INFO "[KD6] send config via multicast from RR to CT");
	
	rtnl_lock();
	
	for (kd6_dev_cnt=0;kd6_dev_cnt<10;kd6_dev_cnt++) {
	   if (kd6_if_lan_all[kd6_dev_cnt] != NULL){
		dev = kmalloc(sizeof (struct net_device),GFP_KERNEL);
        	dev=dev_get_by_name(&init_net,kd6_if_lan_all[kd6_dev_cnt]);
		
		skb = alloc_skb(sizeof(struct ethhdr) +
	                        sizeof(struct ipv6hdr)+
	                        sizeof(struct icmp6hdr), GFP_KERNEL);
	        if (!skb)
	                return;
	        //memset (skb, 0, sizeof(*skb));
	
	        skb->dev = dev;
	        skb->pkt_type = PACKET_OUTGOING;
	        skb->protocol = htons(ETH_P_IPV6);
	        skb->no_fcs = 1;
	
	        skb_reserve(skb, sizeof (struct ethhdr) + sizeof(struct icmp6sup_hdr)+ sizeof(struct ipv6hdr));
	        //      ipv6_dev_get_saddr(&init_net, dev, &LINK_LOCAL_ALL_NODES_MULTICAST, 0, &LINK_LOCAL); 
	
	        //icmpv6 base
		icmp6h = (struct icmp6sup_hdr*) skb_push (skb, sizeof (struct icmp6sup_hdr));
	
		icmp6h->icmp6h_base.icmp6_type = 134; //icmp ra type
	        icmp6h->icmp6h_base.icmp6_code = 0;
	        icmp6h->icmp6h_base.icmp6_dataun.u_nd_ra.router_pref = 3;
	        icmp6h->icmp6h_base.icmp6_dataun.u_nd_ra.hop_limit = 64;
	        icmp6h->icmp6h_base.icmp6_dataun.u_nd_ra.rt_lifetime = htons(86400);

		icmp6h->reachable_time = 0;
	        icmp6h->retransmit_timer = 0;
	
	       //icmpv6 slla opt
	
	        icmp6h->type_slla = 1;
	        icmp6h->len = 1;

	        lladdr[0]=0x08;
	        lladdr[1]=0x00;
	        lladdr[2]=0x27;
	        lladdr[3]=0x5e;
	        lladdr[4]=0x28;
	        lladdr[5]=0x56;
	
	        memcpy(&(icmp6h->eth_addr),lladdr,sizeof(icmp6h->eth_addr));
	
	        //      memcpy(&(icmp6h->eth_addr),&(dev->dev_addr),sizeof(icmp6h->eth_addr));
	
	        //icmpv6 prefix opt
	        icmp6h->type_prefix             = 3;
	        icmp6h->len_prefix_opt          = 4;
	        icmp6h->prefix_len              = 64;
	        icmp6h->flag                    = 0xe0;
	        icmp6h->valid_lifetime          = htonl(86400);
	        icmp6h->prefered_lifetime       = htonl(14400);
	
	        ipv6_dev_get_saddr(&init_net, dev, &LINK_GLOBAL_UNICAST, 0, &kd6_if_addr_global);
	        ipv6_dev_get_saddr(&init_net, dev, &LINK_LOCAL_ALL_NODES_MULTICAST, 0, &kd6_if_addr_ll);
	
	        memset(icmp6h->prefix, 0, sizeof(icmp6h->prefix));
	        memcpy(icmp6h->prefix,&kd6_if_addr_global.in6_u.u6_addr16,(sizeof (icmp6h->prefix))/2);
	       	//icmpv6 base
	        icmp6h->icmp6h_base.icmp6_cksum = 0;
	        csum = csum_partial((char *) icmp6h, sizeof(struct icmp6sup_hdr),0);
	        icmp6h->icmp6h_base.icmp6_cksum = csum_ipv6_magic(&kd6_if_addr_ll, &LINK_LOCAL_ALL_NODES_MULTICAST,
	                        sizeof(struct icmp6sup_hdr),IPPROTO_ICMPV6,csum);
	
	        //ipv6
	        ipv6h = (struct ipv6hdr *) skb_push (skb,sizeof (struct ipv6hdr));
	        ipv6h->version = 6;
	        ipv6h->nexthdr = IPPROTO_ICMPV6;
	        ipv6h->payload_len = htons(sizeof(struct icmp6sup_hdr));
	        ipv6h->daddr = LINK_LOCAL_ALL_NODES_MULTICAST;
	        ipv6h->saddr= kd6_if_addr_ll;
	        ipv6h->hop_limit = 255;
	
	        //eth
	        ethh = (struct ethhdr *) skb_push (skb, sizeof(struct ethhdr));
	        ethh->h_proto = htons(ETH_P_IPV6);
	        memcpy(ethh->h_source, dev->dev_addr, ETH_ALEN);

	        memcpy (ethh->h_dest, ipv6_my_multicast, sizeof(ipv6_my_multicast));
		if (dev_queue_xmit(skb) < 0)
	         	pr_err("[KD6] Error-dev_queue_xmit failed");
	   }
	}
	rtnl_unlock();
}
/*
 * Function sends the config from RR to CT via unicast
 */
void	kd6_send_ra_unicast	(struct kd6_rcvd_rs_ip_dev_strct *kd6_rcvd_rs_ip_dev){
	struct sk_buff *skb;
        struct ipv6hdr* ipv6h;
        struct ethhdr *ethh;
        struct net_device *dev;
        int kd6_rs_rcvd_frm_ct;
        struct in6_addr LINK_UNICAST;
	struct in6_addr kd6_if_addr_global = {{{ 0, }}};
        struct in6_addr kd6_if_addr_ll = {{{ 0, }}};
        struct icmp6sup_hdr *icmp6h;
	u8 lladdr[6];
	__wsum csum; 
	u8 ipv6_my_multicast[6]={0x33,0x33,0x00,0x00,0x00,0x01};
	
	
	printk(KERN_INFO "[KD6] send config via unicast from RR to CT");
	
	rtnl_lock();
	for (kd6_rs_rcvd_frm_ct=0; kd6_rs_rcvd_frm_ct<KD6_MAX_RS_PER_MOMENT; kd6_rs_rcvd_frm_ct++){
		dev = kmalloc(sizeof (struct net_device),GFP_KERNEL);
        	dev = &kd6_rcvd_rs_ip_dev->dev[kd6_rs_rcvd_frm_ct];
		
		skb = alloc_skb(sizeof(struct ethhdr) +
	                        sizeof(struct ipv6hdr)+
	                        sizeof(struct icmp6hdr), GFP_KERNEL);
	        if (!skb)
	                return;
	        //memset (skb, 0, sizeof(*skb));
	
	        skb->dev = dev;
	        skb->pkt_type = PACKET_OUTGOING;
	        skb->protocol = htons(ETH_P_IPV6);
	        skb->no_fcs = 1;
	
	        skb_reserve(skb, sizeof (struct ethhdr) + sizeof(struct icmp6sup_hdr)+ sizeof(struct ipv6hdr));
	        //      ipv6_dev_get_saddr(&init_net, dev, &LINK_LOCAL_ALL_NODES_MULTICAST, 0, &LINK_LOCAL); 
	
	        //icmpv6 base
		icmp6h = (struct icmp6sup_hdr*) skb_push (skb, sizeof (struct icmp6sup_hdr));
	
		icmp6h->icmp6h_base.icmp6_type = 134; //icmp ra type
	        icmp6h->icmp6h_base.icmp6_code = 0;
	        icmp6h->icmp6h_base.icmp6_dataun.u_nd_ra.router_pref = 3;
	        icmp6h->icmp6h_base.icmp6_dataun.u_nd_ra.hop_limit = 64;
	        icmp6h->icmp6h_base.icmp6_dataun.u_nd_ra.rt_lifetime = htons(86400);

		icmp6h->reachable_time = 0;
	        icmp6h->retransmit_timer = 0;
	
	       //icmpv6 slla opt
	
	        icmp6h->type_slla = 1;
	        icmp6h->len = 1;

	        lladdr[0]=0x08;
	        lladdr[1]=0x00;
	        lladdr[2]=0x27;
	        lladdr[3]=0x5e;
	        lladdr[4]=0x28;
	        lladdr[5]=0x56;
	
	        memcpy(&(icmp6h->eth_addr),lladdr,sizeof(icmp6h->eth_addr));
	
	        //      memcpy(&(icmp6h->eth_addr),&(dev->dev_addr),sizeof(icmp6h->eth_addr));
	
	        //icmpv6 prefix opt
	        icmp6h->type_prefix             = 3;
	        icmp6h->len_prefix_opt          = 4;
	        icmp6h->prefix_len              = 64;
	        icmp6h->flag                    = 0xe0;
	        icmp6h->valid_lifetime          = htonl(86400);
	        icmp6h->prefered_lifetime       = htonl(14400);
	
	        ipv6_dev_get_saddr(&init_net, dev, &LINK_UNICAST, 0, &kd6_rcvd_rs_ip_dev->addr[kd6_rs_rcvd_frm_ct]);
	        //ipv6_dev_get_saddr(&init_net, dev, &LINK_LOCAL_ALL_NODES_MULTICAST, 0, &kd6_if_addr_ll);
	
	        memset(icmp6h->prefix, 0, sizeof(icmp6h->prefix));
	        memcpy(icmp6h->prefix,&kd6_if_addr_global.in6_u.u6_addr16,(sizeof (icmp6h->prefix))/2);
	       	//icmpv6 base
	        icmp6h->icmp6h_base.icmp6_cksum = 0;
	        csum = csum_partial((char *) icmp6h, sizeof(struct icmp6sup_hdr),0);
	        icmp6h->icmp6h_base.icmp6_cksum = csum_ipv6_magic(&kd6_if_addr_ll, &LINK_UNICAST,
	                        sizeof(struct icmp6sup_hdr),IPPROTO_ICMPV6,csum);
	
	        //ipv6
	        ipv6h = (struct ipv6hdr *) skb_push (skb,sizeof (struct ipv6hdr));
	        ipv6h->version = 6;
	        ipv6h->nexthdr = IPPROTO_ICMPV6;
	        ipv6h->payload_len = htons(sizeof(struct icmp6sup_hdr));
	        ipv6h->daddr = LINK_UNICAST;
	        ipv6h->saddr= kd6_if_addr_ll;
	        ipv6h->hop_limit = 255;
	
	        //eth
	        ethh = (struct ethhdr *) skb_push (skb, sizeof(struct ethhdr));
	        ethh->h_proto = htons(ETH_P_IPV6);
	        memcpy(ethh->h_source, dev->dev_addr, ETH_ALEN);

	        memcpy (ethh->h_dest, ipv6_my_multicast, sizeof(ipv6_my_multicast));
		if (dev_queue_xmit(skb) < 0)
	         	pr_err("[KD6] Error-dev_queue_xmit failed");
		
	}	
	rtnl_unlock();
}
/*
 * Function is poling on CT side for ra from RR 
 */
void	kd6_receive_ra		(char *kd6_if){
	printk(KERN_INFO "[KD6] if=%s Poling on CT side for ra from RR",kd6_if);
}
/*
 * Function of CT to request config from RR.
 */
void	kd6_send_rs		(char *kd6_if){
	printk(KERN_INFO "[KD6] if=%s CT requests config from RR",kd6_if);
}
/*
 * Function tests if structure contains receved rs from ip/dev
 */

bool kd6_received_rs(struct kd6_rcvd_rs_ip_dev_strct *kd6_rcvd_rs_ip_dev){
	int ct_rs_num_per_moment;
	int memcmp_dev;
	int memcmp_addr;
	struct net_device dev_null;
	struct in6_addr addr_null;
	memset(&dev_null, 0, sizeof (struct net_device));
	memset(&addr_null, 0, sizeof (struct in6_addr));
	for (ct_rs_num_per_moment=0;ct_rs_num_per_moment<KD6_MAX_RS_PER_MOMENT;ct_rs_num_per_moment++){
		memcmp_dev=memcmp(&(kd6_rcvd_rs_ip_dev->dev[ct_rs_num_per_moment]),&dev_null,sizeof(struct net_device));
		memcmp_addr=memcmp(&(kd6_rcvd_rs_ip_dev->addr[ct_rs_num_per_moment]),&addr_null, sizeof (struct in6_addr));
		if ((memcmp_addr != 0) && (memcmp_dev != 0)){
			printk(KERN_INFO "[KD6] CT(s) request(s) config from RR");
			return true;
		}
	}
	printk(KERN_INFO "[KD6] NO CT(s) request(s) recieved by RR");
	return false;
}

/*
 * Function processes rs on the RR side sent from CT
 */
static unsigned int _kd6_receive_rs(void *priv, struct sk_buff *skb, const struct nf_hook_state *state){
	int ct_rs_num_per_moment=0;

        struct ipv6hdr *ipv6h;
	struct kd6_rcvd_rs_ip_dev_strct *kd6_rcvd_rs_ip_dev;

	kd6_rcvd_rs_ip_dev=(struct kd6_rcvd_rs_ip_dev_strct *)priv;
        
	if (skb->pkt_type == PACKET_OTHERHOST){
                goto drop;
        }
	
	pr_info("[KD6] if=%s Received packet",skb->dev->name);
        skb = skb_share_check(skb, GFP_ATOMIC);
        if (!skb){
		pr_info("[KD6] skb issue");
        	return NET_RX_DROP;
	}
       	
	
	spin_lock(&kd6_recv_lock);
        if (!pskb_may_pull(skb,sizeof(struct ipv6hdr))){
		goto drop_unlock;
        }
        ipv6h = (struct ipv6hdr*) skb_network_header(skb);

        // Ok the front looks good, make sure we can get at the rest.  
        if (!pskb_may_pull(skb, skb->len))
	        goto drop_unlock;

	// Put the counter in the position of last added rs to the array.
	while ((kd6_rcvd_rs_ip_dev->dev != NULL) && (ct_rs_num_per_moment < KD6_MAX_RS_PER_MOMENT-1))
		ct_rs_num_per_moment++;

	// to many clients per moment of time (ignoring loosers... Please wait next moment of time))
	if (ct_rs_num_per_moment >= KD6_MAX_RS_PER_MOMENT-1){
		goto drop_unlock;

	}
	
	// Get device which received the packet
	kd6_rcvd_rs_ip_dev->dev[ct_rs_num_per_moment]=*skb->dev;
        
	// Get senders (clients) ipv6 adress.
        ipv6h = (struct ipv6hdr *) skb_pull (skb,sizeof (struct ipv6hdr));
	kd6_rcvd_rs_ip_dev->addr[ct_rs_num_per_moment]=ipv6h->saddr;


        spin_unlock(&kd6_recv_lock);
	pr_info("[KD6] if=%s RS received on RR side sent from CT",skb->dev->name);
	return NF_DROP;	
drop_unlock:
        /* Show's over.  Nothing to see here.  */
       	spin_unlock(&kd6_recv_lock);
drop:

	pr_info("[KD6] PACKET NOT OURS OR ERROR");
        return NF_ACCEPT;

}
/*
 * Function is poling on RR side for received rs from CT
 */
int	kd6_receive_rs_init		(char* kd6_if_wan, char* kd6_if_lan_all[10],struct kd6_rcvd_rs_ip_dev_strct* kd6_rcvd_rs_ip_dev){
	// WARNING
	// To be able receive multicast (ff02::2) packets by this function you have enable forwarding sysctl: #net.ipv6.conf.all.forwarding=1
	
	
	//struct net_device* kd6_dev;
        static struct nf_hook_ops kd6_rcv_rs_hook = {
                .hook = _kd6_receive_rs,
                //.dev = kd6_dev,
                .pf = NFPROTO_IPV6,
                //.pf = PF_INET6,
                .hooknum = (1 << NF_INET_PRE_ROUTING),
		.priority = NF_IP6_PRI_FIRST,
        };
	//we pass the pointer of kd6_rcvd_rs_ip_dev to callback to fill this structure inside callback.
	kd6_rcv_rs_hook.priv=kd6_rcvd_rs_ip_dev;
	printk(KERN_INFO "[KD6] if=%s Poling on RR side for received rs from CT",kd6_if_wan);
        nf_register_net_hook (&init_net, &kd6_rcv_rs_hook);
	return 0;
}
/*
 * Function cleans the rs packet listener on RR side.
 */
void 	kd6_receive_rs_cleanup(char *kd6_if){	

        static struct nf_hook_ops kd6_rcv_rs_hook = {
                .hook = _kd6_receive_rs,
                //.dev = kd6_dev,
                .pf = NFPROTO_IPV6,
                //.pf = PF_INET6,
                .hooknum = (1 << NF_INET_PRE_ROUTING),
		.priority = NF_IP6_PRI_FIRST,
        };
	printk(KERN_INFO "[KD6] if=%s Cleanup of kd6_rcv_rs_hook.",(char *)kd6_if);
	nf_unregister_net_hook (&init_net, &kd6_rcv_rs_hook);
}
/*
 * Function is poling on the RR side for renew from SR
 */
void	kd6_receive_renew	(char *kd6_if){
	printk(KERN_INFO "[KD6] if=%s Poling on the SR side for renew from RR",kd6_if);
}
/*
 * Function sends renew from SR to RR. 
 */
void	kd6_send_renew		(char *kd6_if){
	printk(KERN_INFO "[KD6] if=%s Send renew from SR to RR",kd6_if);
}
/*
 * Function cleans NetFilter hookup. 
 */
void	kd6_nethook_cleanup	(char *kd6_if){
	printk(KERN_INFO "[KD6] if=%s Cleaning NetFilter hookup",kd6_if);
}
/*
 * Function closes the devices/interfaces. 
 */
void	kd6_close_ifs	(char* kd6_if_wan,char* kd6_if_lan_all[10]){
	int kd6_dev_cnt;
	struct net_device *kd6_dev;

	printk(KERN_INFO "[KD6] if=%s Close device/interface",kd6_if_wan);
        /* WAN IF */
	rtnl_lock();
		kd6_dev = kmalloc(sizeof (struct net_device),GFP_KERNEL);
		kd6_dev=dev_get_by_name(&init_net,kd6_if_wan);
		if (!kd6_dev){
			printk(KERN_INFO "[KD6] if=%s IF name doesn't exists",kd6_if_wan);
			rtnl_unlock();
			return;
		}
		dev_change_flags(kd6_dev, kd6_dev->flags & ~IFF_UP,NULL);
        rtnl_unlock();
	/* LAN IFs */
	rtnl_lock();         
	/* bring loopback and DSA master network devices up first */
	for (kd6_dev_cnt=0;kd6_dev_cnt<10;kd6_dev_cnt++) {
	   if (kd6_if_lan_all[kd6_dev_cnt] != NULL){
		kd6_dev = kmalloc(sizeof (struct net_device),GFP_KERNEL);
        	kd6_dev=dev_get_by_name(&init_net,kd6_if_lan_all[kd6_dev_cnt]);
		if (!kd6_dev){
			printk(KERN_INFO "[KD6] if=%s IF name doesn't exists",kd6_if_wan);
			return;
		}
		dev_change_flags(kd6_dev, kd6_dev->flags & ~IFF_UP,NULL);

	   }
	}
	rtnl_unlock();
}
/*
 * Function init dev
 */
static bool  _kd6_is_init_dev(struct net_device *dev){
	char kd6_user_dev_name[IFNAMSIZ] ;

        if (dev->flags & IFF_LOOPBACK)
                return false;
        return kd6_user_dev_name[0] ? !strcmp(dev->name, kd6_user_dev_name) :
                (!(dev->flags & IFF_LOOPBACK) &&
                 (dev->flags & (IFF_POINTOPOINT|IFF_BROADCAST)) &&
                 strncmp(dev->name, "dummy", 5));
}
/*
 * Function changes state of WAN IFs to "UP"
 */
int _kd6_open_if_wan(char* kd6_if_wan){
	struct net_device* kd6_dev;
        unsigned long start=0; 
	unsigned long next_msg=0;
	bool kd6_if_wan_up = false;
	
	/* WAN IF */
	rtnl_lock();         
        /* bring loopback and DSA master network devices up first */
	kd6_dev = kmalloc(sizeof (struct net_device),GFP_KERNEL);
	kd6_dev=dev_get_by_name(&init_net,kd6_if_wan);
	if (!kd6_dev){
		printk(KERN_INFO "[KD6] if=%s IF name doesn't exists",kd6_if_wan);
		rtnl_unlock();
		return 1;
	}
	pr_info ("[KD6] if=%s inspecting dev", kd6_dev->name);
	//if (!(kd6_dev->flags & IFF_LOOPBACK) && !netdev_uses_dsa(kd6_dev))
	//       return;
	if (dev_change_flags(kd6_dev, kd6_dev->flags | IFF_UP,NULL) < 0)
	        pr_err("[KD6] if=%s failed to open n", kd6_dev->name);
	else
	        kd6_if_wan_up = true;
	if (kd6_dev->mtu >= 364){
	        pr_info("[KD6] if=%s device is suitable to send", kd6_dev->name);
	}else{
	        pr_warn("[KD6] if=%s ignoring device, MTU %d too small\n",
	                        kd6_dev->name, kd6_dev->mtu);
	}

        if (kd6_if_wan_up){
                rtnl_unlock();
                return 1;
	}
	while (time_before(jiffies, start +
	                        msecs_to_jiffies(KD6_CARRIER_TIMEOUT))) {
	        int wait, elapsed;
		kd6_dev = kmalloc(sizeof (struct net_device),GFP_KERNEL);
		kd6_dev=dev_get_by_name(&init_net,kd6_if_wan);
		if (_kd6_is_init_dev(kd6_dev) && netif_carrier_ok(kd6_dev)){
			rtnl_unlock();
			return 1;
		}
	        if (time_before(jiffies, next_msg))
	                continue;
	
	        elapsed = jiffies_to_msecs(jiffies - start);
	        wait = (KD6_CARRIER_TIMEOUT - elapsed + 500)/1000;
	        pr_info("[KD6] if=%s Waiting up to %d more seconds for network.\n", kd6_if_wan,wait);
	        next_msg = jiffies + msecs_to_jiffies(KD6_CARRIER_TIMEOUT/12);
	}

	rtnl_unlock();
	return 0;
}
/*
 * Function changes state of WAN IFs to "UP"
 */
int _kd6_open_if_lan(char* kd6_if_lan_all[10]){
	struct net_device* kd6_dev;
	int kd6_dev_cnt;
	/* LAN IFs */
	rtnl_lock();         
	/* bring loopback and DSA master network devices up first */
	for (kd6_dev_cnt=0;kd6_dev_cnt<10;kd6_dev_cnt++) {
	    if (kd6_if_lan_all[kd6_dev_cnt] != NULL){
		kd6_dev = kmalloc(sizeof (struct net_device),GFP_KERNEL);
        	kd6_dev=dev_get_by_name(&init_net,kd6_if_lan_all[kd6_dev_cnt]);
		if (!kd6_dev){
			printk(KERN_INFO "[KD6] if=%s IF name doesn't exists",kd6_dev->name);
			rtnl_unlock();
			return 1;
		}
		pr_info ("[KD6]  Inspecting dev %s", kd6_dev->name);
                //if (!(kd6_dev->flags & IFF_LOOPBACK) && !netdev_uses_dsa(kd6_dev))
                //        continue;
                if (dev_change_flags(kd6_dev, kd6_dev->flags | IFF_UP, NULL) < 0)
                        pr_err("[KD6] Failed to open %s\n", kd6_dev->name);
		if (kd6_dev->mtu >= 364){
                        pr_info("[KD6] Device %s is suitable to send",kd6_dev->name);
                }else{
                        pr_warn("[KD6] Ignoring device %s, MTU %d too small\n",
                                        kd6_dev->name, kd6_dev->mtu);
                }
	   }
	}
	rtnl_unlock();
	return 0;
}

/*
 * Function opens the devices/interfaces. 
 */
int	kd6_open_ifs	(char* kd6_if_wan,char* kd6_if_lan_all[10]){
	printk(KERN_INFO "[KD6] if=%s Open device/interface",kd6_if_wan);
	if ((_kd6_open_if_lan(kd6_if_lan_all)>0) || (_kd6_open_if_wan(kd6_if_wan)>0) )
		return 1;
	//if (_kd6_open_if_lan(kd6_if_lan_all)>0)
	//	return 1;
	//
	
	//wait until wan if is up and has ll_addr before to continue
	ssleep (5);
	return 0;
}
/*
 * Function generates xid
 */
void kd6_generate_xid(u8 kd6_xid[3]){
	get_random_bytes(kd6_xid,sizeof(sizeof(u8)*3));
}



