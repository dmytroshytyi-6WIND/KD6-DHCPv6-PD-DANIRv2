# KD6-DHCPv6-PD-DANIRv2
Lite Kernel DHCPv6 Prefix Delegation &amp; Neighbour Discovery implementation v2.

This is second version (improved) of DHCPv6_PD+ND kernel module  based on the PoC (https://github.com/dmytroshytyi/KD6-DHCPv6-PD-DANIR).

![alt text](https://github.com/dmytroshytyi/KD6-DHCPv6-PD-DANIRv2/blob/dev/docs/kd6_state_machine.PNG "kd6_state_machine")

The kernel in the IoT Router issues a DHCPv6 PD request on its egress interface and obtains a /56. Further it splits multiple /64s out of it and sends RAs with /64 on the ingress interfaces. After clients receive the RA the default route is configured (link-local) address of IoT Requesting Router and /64 prefix is used to configure interface IP address.
Finally this implementation doesn't include cellular part.


# Dev branch.
Dev branch usually comprises a latest version of the code. Thus feel free to ckeckout it...

# Improvements:
1) Written from scratch.
2) State machine is designed for Client (CT), Server (SR) and Requesting Router (RR) and RR implementation follows it as close as possible regarding the time constraint.
3) C func prototyping is enabled in this version. File organisation follows the next pattern: (header.h + main.c + funcs.c)
4) Code is more clean (just few  orphan vars needed for new features like unicast ra on rs. Multicast ra is enabled)


# DOCS section
In the docs folder you may find the kd6_state_machine + software architecture of the kd6_kernel_module.


# TODO:
1. Validation.
2. Improve and modify: unicast ra as a result of CT rs.
3. CT and SR implementation.
4. Improve state machine.


# DEMO and IETF docs:
It is an implementation of draft-shytyi-v6ops-danir-03.txt: https://tools.ietf.org/html/draft-shytyi-v6ops-danir-03

Demo of implementation (v1) : https://www.youtube.com/watch?v=DymVQY7bCUM

The DEMO recorded the virtual machines output on the IoT Router, the Client device behind it, and the DHCPv6-PD server. 

# Compiled with kernel:
Linux ferby 5.5.10-patchedv5 #1 SMP PREEMPT Thu Mar 19 14:48:05 CET 2020 x86_64 GNU/Linux

# Suggestions:
Do not forget to enable ipv6 forwarding on the IoT Requesting Router(RR).

# Requirements:
Patched kernel 5.5.10+ 

# Problems and solutions:
Problem:

	insmod: ERROR: could not insert module danir.ko: Unknown symbol in module

Errors in the journalctl -f during insmod:

	Sep 16 23:48:17 ferby kernel: danir: Unknown symbol rt6_add_dflt_router (err 0)

	Sep 16 23:48:17 ferby kernel: danir: Unknown symbol addrconf_prefix_rcv (err 0)

Solution:

	Stock kernel doesn't expose some functions. Thus such errors appear.

	need to add few lines of the code to fix the errors.

	Example: EXPORT_SYMBOL(rt6_add_dflt_router);

	Example: EXPORT_SYMBOL(addrconf_prefix_rcv);

