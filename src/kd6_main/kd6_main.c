/**
 * File              : kd6_main.c
 * Author            : Dmytro Shytyi
 * Site              : https://dmytro.shytyi.net
 * Date              : 25.02.2020
 * Last Modified Date: 25.02.2020
 * Last Modified By  : Dmytro Shytyi
 */
#ifndef KD6_H
	#define KD6_H
	#include "../../include/kd6.h"
#endif

MODULE_LICENSE("GPL");                  // The license type -- this affects runtime behavior 
MODULE_AUTHOR("Dmytro Shytyi");         // The author -- visible when one use modinfo 
MODULE_DESCRIPTION("[KD6] DANIR Kernel IPv6 Prefix Delegation");
MODULE_VERSION("0.4");                  // The version of the module 
int    	kd6_mode = 0;
int    	kd6_prefix = 0;
int 	kd6_wan_if_mode = 0;		// 0==Run kd6 on all wan ports,1==Specified wan ports
int 	kd6_lan_if_mode = 0;		// 0==Run kd6 on all lan ports,1==Specified lan ports

typedef struct _kd6_ifs
{
    int kd6_if_index;
    int kd6_if_name;
} TMember;

/*
 * WAN interfaces names passed to the module as param
 */
char 	*kd6_if_w0;
char 	*kd6_if_w1;
char 	*kd6_if_w2;
char 	*kd6_if_w3;
/*
 * LAN interfaces names passed to the module as param
 */ 
char  	*kd6_if_l0;
char  	*kd6_if_l1;
char  	*kd6_if_l2;
char  	*kd6_if_l3;
char  	*kd6_if_l4;
char  	*kd6_if_l5;
char  	*kd6_if_l6;
char  	*kd6_if_l7;
char  	*kd6_if_l8;
char  	*kd6_if_l9;
/*
 * WAN if Mode
 */
module_param(kd6_wan_if_mode,int,0);
MODULE_PARM_DESC(kd6_wan_if_mode, "[KD6] 0==Run kd6 on all wan ports,1==Specified wan ports");
/*
 * LAN if mode
 */
module_param(kd6_lan_if_mode,int,0);
MODULE_PARM_DESC(kd6_lan_if_mode, "[KD6] 0==Run kd6 on all lan ports,1==Specified lan ports");
/*
 * KD6 mode (CT/RR/SR)
 */
module_param(kd6_mode,int,0);
MODULE_PARM_DESC(kd6_mode, "[KD6] Chose the mode: 1==Client,2==Requesting_Router,3=Server");
/*
 * Set if w0 name
 */
module_param(kd6_if_w0,charp,0);
MODULE_PARM_DESC(kd6_if_w0, "[KD6] when kd6_wan_if_mode == 1: set the wan0 interface name");
/*
 * Set if w1 name
 */
module_param(kd6_if_w1,charp,0);
MODULE_PARM_DESC(kd6_if_w1, "[KD6] when kd6_wan_if_mode == 1: set the wan1 interface name");
/*
 * Set if w2 name
 */
module_param(kd6_if_w2,charp,0);
MODULE_PARM_DESC(kd6_if_w2, "[KD6] when kd6_wan_if_mode == 1: set the wan2 interface name");
/*
 * set if w3 name
 */
module_param(kd6_if_w3,charp,0);
MODULE_PARM_DESC(kd6_if_w3, "[KD6] when kd6_wan_if_mode == 1: set the wan3 interface name");
/*
 * set if l0 name
 */
module_param(kd6_if_l0,charp,0);
MODULE_PARM_DESC(kd6_if_l0, "[KD6] when kd6_lan_if_mode == 1: set the lan0 interface name");
/*
 * set if l1 name
 */
module_param(kd6_if_l1,charp,0);
MODULE_PARM_DESC(kd6_if_l1, "[KD6] when kd6_lan_if_mode == 1: set the lan1 interface name");
/*
 * set if l2 name
 */
module_param(kd6_if_l2,charp,0);
MODULE_PARM_DESC(kd6_if_l2, "[KD6] when kd6_lan_if_mode == 1: set the lan2 interface name");
/*
 * set if l3 name
 */
module_param(kd6_if_l3,charp,0);
MODULE_PARM_DESC(kd6_if_l3, "[KD6] when kd6_lan_if_mode == 1: set the lan3 interface name");
/*
 * set if l4 name
 */
module_param(kd6_if_l4,charp,0);
MODULE_PARM_DESC(kd6_if_l4, "[KD6] when kd6_lan_if_mode == 1: set the lan4 interface name");
/*
 * set if l5 name
 */
module_param(kd6_if_l5,charp,0);
MODULE_PARM_DESC(kd6_if_l5, "[KD6] when kd6_lan_if_mode == 1: set the lan5 interface name");
/*
 * set if l6 name
 */
module_param(kd6_if_l6,charp,0);
MODULE_PARM_DESC(kd6_if_l6, "[KD6] when kd6_lan_if_mode == 1: set the lan6 interface name");
/*
 * set if l2 name
 */
module_param(kd6_if_l7,charp,0);
MODULE_PARM_DESC(kd6_if_l7, "[KD6] when kd6_lan_if_mode == 1: set the lan7 interface name");
/*
 * set if l8 name
 */
module_param(kd6_if_l8,charp,0);
MODULE_PARM_DESC(kd6_if_l8, "[KD6] when kd6_lan_if_mode == 1: set the lan8 interface name");
/*
 * set if l2 name
 */
module_param(kd6_if_l9,charp,0);
MODULE_PARM_DESC(kd6_if_l9, "[KD6] when kd6_lan_if_mode == 1: set the lan9 interface name");

static void __exit KD6_LKM_exit(void){
	kd6_lkm_exit_run();
}

static int __init KD6_LKM_init(void){
	kd6_lkm_params();
	kd6_lkm_init_run();
        return 0;
}
module_init(KD6_LKM_init);
module_exit(KD6_LKM_exit);
