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

#include "rtems_netconfig.h"

#define stringOf(x) #x
#define STRING(x) stringOf(x)

/*
 * The following configures up to 2 network interface card(s) using
 * settings in either configure/os/CONFIG_SITE.Common.RTEMS or in a
 * BSP-specific configure/os/CONFIG_SITE.Common.RTEMS-<bsp> file.
 * If no settings are provided, it uses the BSP's defaults instead.
 */

#if defined(RTEMS_NETWORK_DRIVER_NAME_1)

  #if defined(RTEMS_NETWORK_DRIVER_NAME_2)
    static struct rtems_bsdnet_ifconfig netdriver_config_2 = {
        STRING(RTEMS_NETWORK_DRIVER_NAME_2),
    #if defined(RTEMS_NETWORK_DRIVER_ATTACH_2)
        RTEMS_NETWORK_DRIVER_ATTACH_2,  /* specific attach function */
    #else
        RTEMS_BSP_NETWORK_DRIVER_ATTACH,    /* default attach function */
    #endif
        NULL,                               /* last interface */
    #if defined(RTEMS_NETWORK_IP4_ADDR_2)
        STRING(RTEMS_NETWORK_IP4_ADDR_2),
      #if defined(RTEMS_NETWORK_IP4_MASK_2)
        STRING(RTEMS_NETWORK_IP4_MASK_2),
      #endif
    #endif
    };
  #endif /* RTEMS_NETWORK_DRIVER_NAME_2 */

  static struct rtems_bsdnet_ifconfig netdriver_config = {
      STRING(RTEMS_NETWORK_DRIVER_NAME_1),
  #if defined(RTEMS_NETWORK_DRIVER_ATTACH_1)
      RTEMS_NETWORK_DRIVER_ATTACH_1,  /* specific attach function */
  #else
      RTEMS_BSP_NETWORK_DRIVER_ATTACH,    /* default attach function */
  #endif
  #if defined(RTEMS_NETWORK_DRIVER_NAME_2)
      &netdriver_config_2,                /* link to next interface */
  #else
      NULL,                               /* last interface */
  #endif
  #if defined(RTEMS_NETWORK_IP4_ADDR_1)
      STRING(RTEMS_NETWORK_IP4_ADDR_1),
    #if defined(RTEMS_NETWORK_IP4_MASK_1)
      STRING(RTEMS_NETWORK_IP4_MASK_1),
    #endif
  #endif
  };
  #define FIRST_DRIVER_CONFIG &netdriver_config

#else /* RTEMS_NETWORK_DRIVER_NAME_1 */

/* Use the BSP-provided standard macros */
static struct rtems_bsdnet_ifconfig bsp_driver_config = {
    RTEMS_BSP_NETWORK_DRIVER_NAME,      /* name */
    RTEMS_BSP_NETWORK_DRIVER_ATTACH,    /* attach function */
    NULL,                               /* last interface */
};
#define FIRST_DRIVER_CONFIG &bsp_driver_config

#endif

/*
 * Allow configure/os/CONFIG_SITE.Common.RTEMS to provide domain name
 */
#ifdef RTEMS_NETWORK_CONFIG_DNS_DOMAINNAME
# define MY_DOMAINNAME STRING(RTEMS_NETWORK_CONFIG_DNS_DOMAINNAME)
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
