/**
 * File              : kd6_sm.c
 * Author            : Dmytro Shytyi
 * Site              : https://dmytro.shytyi.net
 * Date              : 25.02.2020
 * Last Modified Date: 11.03.2020
 * Last Modified By  : Dmytro Shytyi
 */
#ifndef KD6_H
	#define KD6_H
	#include "../../include/kd6.h"
#endif



int kd6_state_machine (void* data){
	/*
	 * Variables section
	 */
	bool 	kd6_released	=	false;		// signal from RR to SR to clear the connection
	bool 	kd6_gua_expired	=	true;		// signal when GUA expired
	bool 	kd6_sigkill_rcv	=	false;		// signal to stop kd6
	unsigned long 	kd6_timer_renew = 0;		// when timer expired. RA send. Timer reseted and started
	bool	sr 		= 	NULL;		// current code is executed on SR
	bool 	rr 		= 	NULL;		// current code is executed on RR
	bool 	ct 		= 	NULL;		// current code is executed on CT
	int 	kd6_state	=	kd6_prepare;	// set kd6_state to start
	char*	kd6_if_wan	=	"";		// interface pointer
	u8	kd6_xid[3];				// xid 
	struct 	kd6_sm_args* kd6_sm_args_struct;	// struct passed from main to thread kd_state_machine;
	char* 	kd6_if_lan_all[10];			// all lan interfaces selected in the console;
//	bool 	kd6_advertise_not_received = true;	// when waiting for advertise(and not received) to go to solicit state.
//	unsigned long start_jiffies =	jiffies; 	// Start timestamp. 
	bool 	kd6_receive_rs_active 	=	false;	// RS receiving is active/Navtive
	struct 	kd6_rcvd_rs_ip_dev_strct kd6_rcvd_rs_ip_dev;		// This structure handles the dev and addr where rs received.
	bool 	(*ptr_th_should_stop)(void) = &kthread_should_stop;	// Pointer to the thread stop func.
	int 	ct_rs_num_per_moment;					// counter
	int 	kd6_rcv_rs_hook;					//contains pointer to registered hook
/*
  	struct in6_addr KD6_LINK_LOCAL_MULTICAST = {{{ 0xff,02,0,0,0,0,0,0,0,0,0,0,0,1,0,2 }}};
	struct in6_addr KD6_LINK_LOCAL = {{{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }}};
	struct in6_addr KD6_LINK_NULL = {{{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }}};
*/
	// Set struct fields to 0;
	//memset(&kd6_rcvd_rs_ip_dev,0,sizeof(struct kd6_rcvd_rs_ip_dev_strct));
	// dev init in rcvd_rs_ip_dev struct
	for (ct_rs_num_per_moment=0;ct_rs_num_per_moment < KD6_MAX_RS_PER_MOMENT;ct_rs_num_per_moment++)
		kd6_rcvd_rs_ip_dev.dev[ct_rs_num_per_moment]=kzalloc(sizeof (struct net_device), GFP_KERNEL);

/*
	for (ct_rs_num_per_moment=0;ct_rs_num_per_moment < KD6_MAX_RS_PER_MOMENT; ct_rs_num_per_moment++){
		memset(&(kd6_rcvd_rs_ip_dev.dev[ct_rs_num_per_moment]),0,sizeof(struct net_device));
		memset(&(kd6_rcvd_rs_ip_dev.addr[ct_rs_num_per_moment]),0,sizeof(struct in6_addr));
	}
*/
	kd6_sm_args_struct=(struct kd6_sm_args*) data;
	sr=kd6_sm_args_struct->sr;
	rr=kd6_sm_args_struct->rr;
	ct=kd6_sm_args_struct->ct;
	kd6_if_wan=kd6_sm_args_struct->wan_interfaces[0];
	memcpy (kd6_if_lan_all,kd6_sm_args_struct->lan_interfaces,sizeof(kd6_if_lan_all));
	get_random_bytes(kd6_xid,sizeof(u8)*3);

	while (!kthread_should_stop()){
        switch (kd6_state){
		case kd6_prepare:
			if (kd6_open_ifs(kd6_if_wan,kd6_if_lan_all)>0){
				kd6_state=kd6_end;
				break;
			}

//			kd6_generate_xid(kd6_xid);
			kd6_state=kd6_init;
			break;
                case kd6_init:
			//client
			if (ct){
				start_jiffies=jiffies;
				printk(KERN_INFO "[KD6] Client started");
				kd6_generate_ll(kd6_if_wan);
                        	kd6_state=kd6_config_not_in_sync;
			}
			//server	
			if (sr){
				printk(KERN_INFO "[KD6] Server started");
                        	if (kd6_released){
                        	        kd6_state=kd6_release_state;
                        	        break;
				}
                        	kd6_state=kd6_assigning_sr_to_rr;
			}
			//requesting router
			if (rr){
				printk(KERN_INFO "[KD6] Requesting Router started");
                        	if (kd6_released){
                        	        kd6_state=kd6_release_state;
                        	        break;
				}
				kd6_send_solicit(kd6_if_wan,kd6_xid);
				kd6_state=kd6_assigning_sr_to_rr;
			}
                        break;
                case kd6_assigning_sr_to_rr:
			//client
			if (ct) {
				//NONE
			} 
			//server	
			if  (sr){
                        	if (kd6_released){
                        	        kd6_state=kd6_release_state;
                         	        break;
			 	}
				kd6_receive_solicit(kd6_if_wan);
				kd6_send_advertise(kd6_if_wan);
                        	kd6_state=kd6_rr_assigned_with_sr;
			}
			//requesting router
			if (rr){
                        	if (kd6_released){
                        	        kd6_state=kd6_release_state;
                        	         break;
				}

				if (kd6_receive_advertise(kd6_if_wan,kd6_xid,ptr_th_should_stop)>0)
					kd6_state=kd6_init;
				else
                        		kd6_state=kd6_rr_assigned_with_sr;
			}
                        break; 
                case kd6_rr_assigned_with_sr:
			//client
			if (ct){ 
				//NONE
			}
			//server	
			if (sr){
                        	if (kd6_released){
                        	        kd6_state=kd6_release_state;
                        	        break;
				}
       
				kd6_receive_request(kd6_if_wan);
                        	kd6_state=kd6_config_exchange;
			}
			//requesting router
			if (rr){
                        	if (kd6_released){
                        	        kd6_state=kd6_release_state;
                        	        break;
				}
				kd6_send_request(kd6_if_wan,kd6_xid);
                        	kd6_state=kd6_config_exchange;
			}
                        break; 
                case kd6_config_in_sync:
			//client
			if (ct){
				if (kd6_gua_expired){
					kd6_state=kd6_config_not_in_sync;
					break;
				}
				if (kd6_sigkill_rcv){
					kd6_state=kd6_ct_pre_end;
					break;
				}
			}
			//server	
			if (sr){
				if (kd6_sigkill_rcv){
					kd6_state=kd6_sr_rr_pre_end;
					break;
				}
				kd6_receive_renew(kd6_if_wan);
				kd6_state=kd6_config_exchange;
				break;
			}
			//requesting router
			if (rr){
				if (kd6_sigkill_rcv){
					kd6_state=kd6_sr_rr_pre_end;
					break;
				}
				if (kd6_gua_expired){
					kd6_state=kd6_config_not_in_sync;
					break;
				}
				//time_after(a,b) "a" is_after "b" => return "true"
				if(time_after(jiffies,kd6_timer_renew+msecs_to_jiffies(KD6_SEND_RA_DELAY))){
					kd6_state=kd6_config_exchange;
					break;
				}	
				//activate the hook to sniff for received rs	
				if (!kd6_receive_rs_active){
					kd6_rcv_rs_hook=kd6_receive_rs_init(kd6_if_wan,kd6_if_lan_all,&kd6_rcvd_rs_ip_dev);
					kd6_receive_rs_active=true;
					break;
				}
				// analyse if we have received the rs on any of dev	
/*
				if (kd6_received_rs(&kd6_rcvd_rs_ip_dev)){
					kd6_state=kd6_config_exchange;
					break;
				}
*/
				ssleep (1);
			}
                        break;
		case kd6_config_not_in_sync:
			//client
			if (ct){
				kd6_send_rs(kd6_if_wan);
				kd6_state=kd6_config_exchange;
			}
			//server	
			if (sr){
				//NONE	
			}
			//requesting router
			if (rr){
                        	if (kd6_released){
                        	        kd6_state=kd6_release_state;
                        	        break;
				}
				kd6_send_renew(kd6_if_wan);
				//kd6_receive_rs(kd6_if_wan);
				kd6_state=kd6_config_exchange;
				break;
			}
                        break;

		case kd6_config_exchange:
			//client
			if (ct){
				kd6_receive_ra(kd6_if_wan);
                                kd6_setup_ifs(kd6_if_wan,kd6_if_lan_all);
                                kd6_setup_def_route(kd6_if_wan);
				
				kd6_state=kd6_config_in_sync;
				break;
				
			}
			//server	
			if (sr){
                        	if (kd6_released){
                        	        kd6_state=kd6_release_state;
                        	        break;
				}
				kd6_send_reply(kd6_if_wan);
				kd6_state=kd6_config_in_sync;
				break;
			}
			//requesting router
			if (rr){
                        	if (kd6_released){
                        	        kd6_state=kd6_release_state;
                        	        break;
				}
				if (kd6_gua_expired){
					//receive RS
			       		//bool (*ptr_th_should_stop)(void) = &kthread_should_stop;
					kd6_receive_reply(kd6_if_wan,kd6_xid,ptr_th_should_stop);
					kd6_gua_expired = false;
				}

                                kd6_setup_ifs(kd6_if_wan,kd6_if_lan_all);
                                kd6_setup_def_route(kd6_if_wan);

/*
				if (kd6_received_rs(&kd6_rcvd_rs_ip_dev)){
					kd6_send_ra_unicast(&kd6_rcvd_rs_ip_dev);
					// Set struct fields to 0;
					for (ct_rs_num_per_moment=0;ct_rs_num_per_moment<KD6_MAX_RS_PER_MOMENT;ct_rs_num_per_moment++){
						memset(&(kd6_rcvd_rs_ip_dev.dev[ct_rs_num_per_moment]),0,sizeof(struct net_device));
						memset(&(kd6_rcvd_rs_ip_dev.addr[ct_rs_num_per_moment]),0,sizeof(struct in6_addr));
					}
					kd6_state=kd6_config_in_sync;
					break;
				}
*/
				// if unicast is sent multicast isn't because of break in the unicast if-section.
				kd6_send_ra_multicast(kd6_if_lan_all);
				kd6_timer_renew=jiffies;
				kd6_state=kd6_config_in_sync;
			}
                        break;

                case kd6_release_state:
			//client
			if (ct){
				//NONE
			}
			//server	
			if (sr){
				kd6_send_reply(kd6_if_wan);
				kd6_state=kd6_sr_rr_end;
				break;
			}
			//requesting router
			if (rr){
			       	//bool (*ptr_th_should_stop)(void) = &kthread_should_stop;
				kd6_receive_reply(kd6_if_wan,kd6_xid,ptr_th_should_stop);
				kd6_state=kd6_sr_rr_end;
			}
                        break;
                case kd6_sr_rr_end:
			kd6_state=kd6_end;
                        break;
		case kd6_end:
			ssleep (2);
			// we don't waining to receive the rs packet when we finish.
			if (kd6_receive_rs_active){
				kd6_receive_rs_cleanup(kd6_if_wan,&kd6_rcvd_rs_ip_dev,kd6_rcv_rs_hook);
				kd6_receive_rs_active=false;
			}
			kd6_state=kd6_init;
			break;
	}
	}

	if (kd6_receive_rs_active){
		kd6_receive_rs_cleanup(kd6_if_wan,&kd6_rcvd_rs_ip_dev,kd6_rcv_rs_hook);
		kd6_receive_rs_active=false;
	}
	kd6_close_ifs(kd6_if_wan,kd6_if_lan_all);
	do_exit(0);
}
