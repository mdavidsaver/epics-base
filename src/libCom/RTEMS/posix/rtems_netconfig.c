/*************************************************************************\
* Copyright (c) 2002 The University of Saskatchewan
* Copyright (c) 2017 Fritz-Haber-Institut der Max-Planck-Gesellschaft
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/*
 * RTEMS network configuration for EPICS
 *      Author: W. Eric Norum
 *              Heinz Junkes
 *
 * Version for RTEMS-4.12
 *
 * This file can be copied to an application source dirctory
 * and modified to override the values shown below.
 */
#include <stdio.h>
#include <bsp.h>
#include <rtems/rtems_bsdnet.h>

/*
 * The following conditionals select the network interface card.
 *
 * On RTEMS-pc386 targets all network drivers which support run-time
 * probing are linked. 
 * On other targets the network interface specified by the board-support
 * package is used.
 * To use a different NIC for a particular application, copy this file to the
 * application directory and make the appropriate changes.
 */

#if defined(__i386__)
extern int rtems_fxp_attach (struct rtems_bsdnet_ifconfig *, int);
static struct rtems_bsdnet_ifconfig fxp_driver_config = {
    "fxp1",                             /* name */
    rtems_fxp_attach,                   /* attach function */
    NULL,                               /* link to next interface */
};
static struct rtems_bsdnet_ifconfig e3c509_driver_config = {
    "ep0",                              /* name */
    rtems_3c509_driver_attach,          /* attach function */
    &fxp_driver_config,                 /* link to next interface */
};
#define FIRST_DRIVER_CONFIG &e3c509_driver_config
#endif

#if defined (__PPC)

# define XIFSTR(x) IFSTR(x)
# define IFSTR(x) #x

# if defined(ETH_NAME_2)
static struct rtems_bsdnet_ifconfig netdriver_config_2 = {
  XIFSTR(ETH_NAME_2),
  RTEMS_BSP_NETWORK_DRIVER_ATTACH,
  NULL,     /* no more interface */
  "141.14.128.11",      // IP address
  "255.255.240.0",      // net mask
  NULL,                 // Driver supplies hardware address
  0                     // Use default driver parameters
};
#endif /* ETH_NAME_2 */

# if defined(ETH_NAME_1)
static struct rtems_bsdnet_ifconfig netdriver_config = {
  XIFSTR(ETH_NAME_1),
  RTEMS_BSP_NETWORK_DRIVER_ATTACH,
  NULL, /* during tests, don't use econd interface : netdriver_config_2, */
  NULL,
  NULL,                 // ip address per bootp
  NULL,                 // Driver supplies hardware address
  0                     // Use default driver parameters
};
#define FIRST_DRIVER_CONFIG &netdriver_config
#endif /* ETH_NAME_1 */

#endif /* __PPC */

/*
 * Allow configure/os/CONFIG_SITE.Common.RTEMS to provide domain name
 */
#ifdef RTEMS_NETWORK_CONFIG_DNS_DOMAINNAME
# define XSTR(x) STR(x)
# define STR(x) #x
# define MY_DOMAINNAME XSTR(RTEMS_NETWORK_CONFIG_DNS_DOMAINNAME)
#else
# define MY_DOMAINNAME NULL
#endif

/*
 * Allow non-BOOTP network configuration
 */
#ifndef MY_DO_BOOTP
# define MY_DO_BOOTP rtems_bsdnet_do_bootp
#endif

/*
 * Allow site- and BSP-specific network buffer space configuration.
 * The macro values are specified in KBytes.
 */
#ifndef RTEMS_NETWORK_CONFIG_MBUF_SPACE
# define RTEMS_NETWORK_CONFIG_MBUF_SPACE 180
#endif
#ifndef RTEMS_NETWORK_CONFIG_CLUSTER_SPACE
# define RTEMS_NETWORK_CONFIG_CLUSTER_SPACE 350
#endif

/*
 * Network configuration
 */
struct rtems_bsdnet_config rtems_bsdnet_config = {
    FIRST_DRIVER_CONFIG,   /* Link to next interface */
    MY_DO_BOOTP,           /* How to find network config */
    10,                    /* If 0 then the network daemons will run at a */
                           /*   priority just less than the lowest-priority */
                           /*   EPICS scan thread. */
                           /* If non-zero then the network daemons will run */
                           /*   at this *RTEMS* priority */
    RTEMS_NETWORK_CONFIG_MBUF_SPACE*1024,
    RTEMS_NETWORK_CONFIG_CLUSTER_SPACE*1024,
    NULL,                  /* Host name */
    MY_DOMAINNAME,         /* Domain name */
};
