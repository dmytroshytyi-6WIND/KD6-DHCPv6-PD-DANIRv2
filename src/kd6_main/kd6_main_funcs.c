/*
 * File              : kd6_main_funcs.c
 * Author            : Dmytro Shytyi
 * Site              : https://dmytro.shytyi.net
 * Date              : 26.02.2020
 * Last Modified Date: 26.02.2020
 * Last Modified By  : Dmytro Shytyi
 */
#ifndef KD6_H
	#define KD6_H
	#include "../../include/kd6.h"
#endif


struct task_struct* kd6_thread[4];          // Thread structure used in the thread creation
struct kd6_sm_args* kd6_sm_args_struct;

char* kd6_if_l_all[10];
char* kd6_if_w_all[4];


void kd6_lkm_exit_run(void){
	kd6_thread_cleanup();
        printk(KERN_INFO "[KD6] LKM is finished!\n");
}
int kd6_lkm_init_run(void){
        printk(KERN_INFO "[KD6] LKM is started!\n" );
	kd6_thread_init();
	return 0;
}

void kd6_set_mode(struct kd6_sm_args* args){
	args->ct=false;
	args->rr=false;
	args->sr=false;
	printk(KERN_INFO "[KD6] Mode selected - %i", kd6_mode);
	switch (kd6_mode){
		case 0:
			break;
		case 1:
			args->ct=true;
			break;
		case 2:
			args->rr=true;
			break;
		case 3:
			args->sr=true;
			break;
	}
}
void kd6_lkm_params (void){



/*
	int kd6_interface_num=0;
	int kd6_port;
	for (kd6_port=0; kd6_port < 4; kd6_port++){
		if (kd6_if_w_all[kd6_port] != NULL){
			kd6_if_w_active[kd6_interface_num]=kd6_if_w_all[kd6_port];
			kd6_interface_num++;
		}
	}
	kd6_interface_num=0;
	for (kd6_port=0; kd6_port < 10; kd6_port++){
		if (kd6_if_l_all[kd6_port] != NULL){
			kd6_if_l_active[kd6_interface_num]=kd6_if_l_all[kd6_port];
			kd6_interface_num++;
		}
	}
*/
}

void kd6_print_ifs (char** kd6_if_l_all){
	int kd6_port;
/*
	printk(KERN_INFO "[KD6] WAN interfaces: \n");
	for (kd6_port=0; kd6_port < 4; kd6_port++){
		if (kd6_if_w_all[kd6_port] != NULL){
			printk(KERN_INFO "%s,", kd6_if_w_all[kd6_port]);
		}
	}
*/
	for (kd6_port=0; kd6_port < 10; kd6_port++){
		if (kd6_if_l_all[kd6_port] != NULL){
			printk(KERN_CONT "%s, ",kd6_if_l_all[kd6_port]);
		}
	}
	printk("\n");
}

int kd6_thread_init (void) {
	int kd6_port;
	char*  kd6_lkm_thread[4]={"kd6_thread_if0","kd6_thread_if1","kd6_thread_if2","kd6_thread_if3"};
	/*
	 * LAN interfaces names passed to the module as params
	 */
	char* kd6_if_l_all[10]={kd6_if_l0,kd6_if_l1,kd6_if_l2,kd6_if_l3,kd6_if_l4,kd6_if_l5,kd6_if_l6,kd6_if_l7,kd6_if_l8,kd6_if_l9};
	/*
	 * WAN interfaces names passed to the module as params
	 */
	kd6_if_w_all[0]=kd6_if_w0;
	kd6_if_w_all[1]=kd6_if_w1;
	kd6_if_w_all[2]=kd6_if_w2;
	kd6_if_w_all[3]=kd6_if_w3;

	
	
	printk(KERN_INFO "[KD6] IPv6 Prefix Delegation init");

	
	for (kd6_port=0; kd6_port < 4; kd6_port++){
		if (kd6_if_w_all[kd6_port] != NULL){

			kd6_sm_args_struct=kmalloc(sizeof(struct kd6_sm_args),GFP_KERNEL);
			kd6_set_mode(kd6_sm_args_struct);
			memcpy (kd6_sm_args_struct->lan_interfaces,
			kd6_if_l_all,
			sizeof(kd6_if_l_all));

			kd6_sm_args_struct->wan_interfaces[0]=kd6_if_w_all[kd6_port];
			memcpy(kd6_sm_args_struct->lan_interfaces,kd6_if_l_all,sizeof (kd6_if_l_all[10]));
			kd6_thread[kd6_port] = kthread_create(kd6_state_machine,
					kd6_sm_args_struct,
					kd6_lkm_thread[kd6_port]);
			
			
			if(kd6_thread[kd6_port]){
				printk(KERN_CONT "[KD6] if=%s, Worker thread started. LAN Interfaces considered: ",kd6_if_w_all[kd6_port]);
				kd6_print_ifs(kd6_if_l_all);
				wake_up_process(kd6_thread[kd6_port]);
			}
		}
	}
	return 0;
}

void kd6_thread_cleanup(void) {
	int kd6_port;
	int ret[4];
	for (kd6_port=0; kd6_port < 4; kd6_port++){
		if (kd6_if_w_all[kd6_port] != NULL){
			ret[kd6_port] = kthread_stop(kd6_thread[kd6_port]);
			if(!ret[kd6_port])
				pr_info(KERN_INFO "[KD6] Worker thread %d stopped", kd6_port+1);
			else
				pr_err (KERN_INFO "[KD6] Error during stopping the thread, %d",ret[kd6_port+1]);

		}
	}
}
