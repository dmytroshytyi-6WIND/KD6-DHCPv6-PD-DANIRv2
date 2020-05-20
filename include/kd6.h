/**
 * File              : kd6.h
 * Author            : Dmytro Shytyi
 * Site              : https://dmytro.shytyi.net
 * Date              : 26.02.2020
 * Last Modified Date: 26.02.2020
 * Last Modified By  : Dmytro Shytyi
 */
#ifndef KD6_H
        #define KD6_H
#endif

#include "./kd6_sm.h"			// State Machine header.
#include <linux/init.h>			// Load/unload module
#include <linux/kernel.h>		// printk
#include <linux/module.h>		// Core header for loading LKMs into the kernel 
#include <linux/kthread.h>		// Thread support
#include <linux/slab.h>			// kmalloc
#include <linux/moduleparam.h>		// For module_param
#include <linux/netdevice.h>		// For struct net_device and for "for_each_netdev" looper
#include <linux/utsname.h>		// For utsname()
#include <net/addrconf.h>		// For function ipv6_dev_get_saddr()
#include <linux/netfilter.h>		// For nf_hook
#include <linux/netfilter_ipv6.h>	// For netfilter ipv6
#include <net/ip6_fib.h>		// For fib6_clean_expires
#include <net/ip6_route.h>		// For rt6_add_dflt_router
#include <net/dsa.h> 			// For netdev_uses_dsa
#include <linux/netdevice.h>		// For dev_change_flags
#include <linux/jiffies.h>		// For msecs_to_jiffies

extern int kd6_mode;			// varialbe kernel param passed via cli 
extern int kd6_prefix;			// varialbe kernel param passed via cli
extern struct task_struct *kd6_thread[4];// Thread structure used in the thread creation
/*
 * WAN interfaces names passed to the module as param
 */
extern char    *kd6_if_w0;
extern char    *kd6_if_w1;
extern char    *kd6_if_w2;
extern char    *kd6_if_w3;
/*
 * LAN interfaces names passed to the moude as param
 */ 
extern char    *kd6_if_l0;
extern char    *kd6_if_l1;
extern char    *kd6_if_l2;
extern char    *kd6_if_l3;
extern char    *kd6_if_l4;
extern char    *kd6_if_l5;
extern char    *kd6_if_l6;
extern char    *kd6_if_l7;
extern char    *kd6_if_l8;
extern char    *kd6_if_l9;

/*
 * Exit the KD6_LKM
 */
void kd6_lkm_exit_run(void);
/*
 * Start the KD6_LKM
 */
int  kd6_lkm_init_run(void);
/*
 * Start the task in the thread
 */
int kd6_thread_init(void);
/*
 * Stop the thread.
 */
void kd6_thread_cleanup(void);
/*
 * Run the module in the specified in the cli mode: CT/RR/SR
 */
void kd6_set_mode(struct kd6_sm_args* args);

void kd6_lkm_params(void);
void kd6_print_ifs (char** kd6_if_l_all);
