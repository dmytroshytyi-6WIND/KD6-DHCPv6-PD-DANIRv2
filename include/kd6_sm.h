/**
 * File              : kd6_prototypes.h
 * Author            : Dmytro Shytyi
 * Site              : https://dmytro.shytyi.net
 * Date              : 25.02.2020
 * Last Modified Date: 25.02.2020
 * Last Modified By  : Dmytro Shytyi
 */

/*
 * Definitions:
 * CT - ClienT
 * RR - Requesting Router
 * SR - ServeR 
*/
#include <linux/types.h> 	// For bool types
#include <linux/in6.h>		// For struct KD6_LINK_LOCAL_MULTICAST etc..
#include <linux/icmpv6.h>       // For icmpv6hdr
/*
 * States of the state machine
 */

#define kd6_prepare 		0
#define kd6_init 		1
#define kd6_assigning_sr_to_rr	2
#define kd6_rr_assigned_with_sr	3
#define kd6_config_in_sync	4
#define kd6_config_not_in_sync	5
#define kd6_config_exchange	6
#define kd6_release_state	7
#define kd6_kd6_end_rr_rs	8
#define kd6_ct_pre_end		9
#define kd6_sr_rr_pre_end	10
#define kd6_sr_rr_end		11
#define kd6_end			12
/*
 * KD6 message types 
 */
#define KD6_SOLICIT              1
#define KD6_ADVERTISE            2
#define KD6_REQUEST              3
#define KD6_CONFIRM              4
#define KD6_RENEW                5
#define KD6_REBIND               6
#define KD6_REPLY                7
#define KD6_RELEASE              8
#define KD6_DECLINE              9
#define KD6_RECONFIGURE         10

#define KD6_MAX_RCV_RS		1000 		/* Maximum unicast RS packets will be handled */
#define KD6_MAX_RS_PER_MOMENT 	KD6_MAX_RCV_RS	/* Max number of rs handled per moment by RR */
#define KD6_CARRIER_TIMEOUT 	120000 		/* Wait for carrier timeout */
#define KD6_SEND_RA_DELAY	30*1000 	/* milliseconsd */		

/*
 * This section defines the variables passed to the moule externally.
 * One option per machine could be selected.
 */
/* 
 *  The extern declaration is not used for structure definitions,
 *  but is instead used for variable declarations (that is, some 
 *  data value with a structure type that you have defined). If 
 *  you want to use the same variable across more than one source file,
 *  declare it as extern in a header file
 */
extern 	bool	ct; 			// current code is executed on CT
extern 	bool 	sr;		 	// current code is executed on SR
extern 	bool 	rr;		 	// current code is executed on RR
/*
 * Generall variables section
 */
extern 	bool	kd6_done;		// signal to finish the program
extern 	bool	kd6_released; 	 	// signal from RR to SR to clear the connection
extern 	bool 	kd6_gua_expired;	// signal when GUA expired
extern 	bool 	kd6_sigkill_rcv;	// signal to stop kd6
extern 	bool 	kd6_timer_renew; 	// when timer expired. RA send. Timer reseted and started
extern 	int	kd6_state; 		// set kd6_state to start
//extern	bool	kd6_advertise_received; // set kd6_state to start
extern 	unsigned long start_jiffies;	// Start timestamp.

struct kd6_sm_args{
     	char*	wan_interfaces[4];	// wan interfaces used by lkm
	char* 	lan_interfaces[10];	// lan interfaces used by lkm
     	bool 	ct;        	        // current code is executed on CT
    	bool 	sr;        	        // current code is executed on SR
     	bool 	rr;	    	        // current code is executed on RR
};

extern struct 	tm 	date;			// To use tm date in function convertTimeDateToSeconds
extern struct in6_addr KD6_LINK_LOCAL_MULTICAST;
extern struct in6_addr KD6_LINK_LOCAL;
extern struct in6_addr KD6_LINK_NULL;

/*
 * kd6 structures to exchange betwen SR and RR.
 */
struct kd6_server_id{
        u16 option_client_id;
        u16 option_len;
        u16 duid_type;
        u16 hw_type;
        u32 duid_time;
        u8 my_hw_addr[6];
}__attribute__((packed));

struct kd6_client_id{
	u16 option_client_id;
	u16 option_len;
	u16 duid_type;
	u16 hw_type;
	u32 duid_time;
	u8 my_hw_addr[6];
}__attribute__((packed));


struct kd6_time{
	u16 option_time;
	u16 option_len;
	u16 value;
};

struct kd6_oro{
	u16 option;
	u16 option_len;
	u8 value[4];
};

struct kd6_ia_prefix{
        u16 option_prefix;
        u16 option_len;
        u32 prefered_lifetime;
        u32 valid_lifetime;
        u8 prefix_len;
        u8 prefix_addr[16];

}__attribute__((packed));

struct kd6_ia_pd{
	u16 option_ia_pd;
	u16 option_len;
	u8 iaid[4];
	u8 t1[4];
	u8 t2[4];
};


struct  kd6_packet_sol {
	u8 msg_type;
	u8 transaction_id[3];
	struct kd6_client_id my_client_id;
	struct kd6_oro oro;
	struct kd6_time time;
	struct kd6_ia_pd ia_pd;        
};

struct kd6_packet_req {
        u8 msg_type;
        u8 transaction_id[3];
        struct kd6_client_id my_client_id;
        struct kd6_client_id my_server_id;
        struct kd6_oro oro;
        struct kd6_time time;
        struct kd6_ia_pd ia_pd;
        struct kd6_ia_prefix ia_prefix;
};

struct kd6_packet{
        struct kd6_packet_sol *kd6_sol;
        struct kd6_packet_req *kd6_req;
};
struct icmp6sup_hdr{ 
         //base icmpv6 
         struct icmp6hdr __attribute__((packed)) icmp6h_base; 
         u32 reachable_time; 
         u32 retransmit_timer; 
         //prefix opt 
         u8 type_prefix; 
         u8 len_prefix_opt; 
         u8 prefix_len; 
         u8 flag; 
         u32 valid_lifetime; 
         u32 prefered_lifetime; 
         u32 reserved; 
         u8 prefix[16]; 
         //slla opt 
         u8 type_slla; 
         u8 len; 
         u8 eth_addr[6]; 
}; 
        struct  kd6_rcvd_rs_ip_dev_strct{
                struct net_device       dev[KD6_MAX_RCV_RS];
                struct in6_addr         addr[KD6_MAX_RCV_RS];
        };


/*
 * Function generates a link local addresses on the interfaces
 */
void	kd6_generate_ll		(char *kd6_if);
/*
 * Function is sending solicit message from RR to SR
 */
void	kd6_send_solicit	(char *kd6_if, u8 kd6_xid[3]);
/*
 * Function is polling on SR side for received solicit message sent by RR.
 */
void	kd6_receive_solicit	(char *kd6_if);
/*
 * Function sends advertise from SR to RR. 
 */
void	kd6_send_advertise	(char *kd6_if);
/*
 * Wrapper of function receives advertise on the RR side sent from SR
 */
int	kd6_receive_advertise	(char *kd6_if,u8 kd6_xid[3],bool (*ptr_th_should_stop)(void));
/*
 * Function receives advertise on the RR side sent from SR
 */
//static unsigned int _kd6_receive_advertise(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);
/*
 * Function receives the reqest on the SR side sent by RR
 */
void	kd6_receive_request	(char *kd6_if);
/*
 * Function sends the request the from RR to SR
 */
void	kd6_send_request	(char *kd6_if, u8 kd6_xid[3]);
/*
 * Function sends reply from SR to RR 
 */
void	kd6_send_reply		(char *kd6_if);
/*
 * Function is pooling for reply from SR on the RR side.
 */
void	kd6_receive_reply	(char *kd6_if,u8 kd6_xid[3],bool (*ptr_th_should_stop)(void));
/*
 * Function configures the interface
 */
void	 kd6_setup_ifs		(char *kd6_if, char *kd6_if_lan_all[10]);
/*
 * Function configures the default IPv6 rotue
 */
void	kd6_setup_def_route	(char *kd6_if);
/*
 * Function sends the config from RR to CT via multicast
 */
void	kd6_send_ra_multicast		(char *kd6_if_lan_all[10]);
/*
 * Function sends the config from RR to CT via unicast
 */
void	kd6_send_ra_unicast		(struct kd6_rcvd_rs_ip_dev_strct *kd6_rcvd_rs_ip_dev);
/*
 * Function is poling on CT side for ra from RR
 * */
void	kd6_receive_ra		(char *kd6_if);
/*
 * Function of CT to request config from RR.
 */
void	kd6_send_rs		(char *kd6_if);
/*
 * Function is poling on RR side for received rs from CT
 */
int	kd6_receive_rs		(char* kd6_if_lan_all[10]);
/*
 * Function is pooling on the RR side for renew from SR
 */
void	kd6_receive_renew	(char *kd6_if);
/*
 * Function sends renew from SR to RR. 
 */
void	kd6_send_renew		(char *kd6_if);
/*
 * Function cleans NetFilter hookup. 
 */
void	kd6_nethook_cleanup	(char *kd6_if);
/*
 * State machine, the core mechanism of the kd implementation.
 */
int	kd6_state_machine	(void *data);
/*
 * Generate xid
 */
void kd6_generate_xid(u8 xid[3]);
/*
 * Open interfaces
 */
int kd6_open_ifs(char* kd6_if_wan, char* kd6_if_lan_all[10]);
/* 
 * Close interfaces 
 */ 
void kd6_close_ifs(char* kd6_if_wan, char* kd6_if_lan_all[10]); 
/*
 * Convert month's name to month's number
 */
int GetMon (const char *str);
/*
 * Convert Time Date to Seconds
 */
u32 convertTimeDateToSeconds(const struct tm date);
/*
 * Function to parse on RR side received advertise from SR
 */
int kd6_parse_received_advertise(u8* dhp,u8 kd6_msg_size,u8 kd6_xid[3]);
/*
 * Function test if structure contains received rs from ip/dev
 */
bool kd6_received_rs(struct kd6_rcvd_rs_ip_dev_strct *kd6_rcvd_rs_ip_dev);
/*
 * Function is poling on RR side for received rs from CT
 */
int kd6_receive_rs_init(char* kd6_if_wan, char* kd6_if_lan_all[10],struct kd6_rcvd_rs_ip_dev_strct* kd6_rcvd_rs_ip_dev);
/*
 * Function cleans the rs packet listener on RR side.
 */
void    kd6_receive_rs_cleanup(char *kd6_if);
