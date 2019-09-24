/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/*
 *	Author:	Roger A. Cole
 *	Date:	07-20-91
 *
 */
/****************************************************************************
* TITLE	envDefs.h - definitions for environment get/set routines
*
* DESCRIPTION
*	This file defines the environment parameters for EPICS.  These
*	ENV_PARAM's are created in envData.c by bldEnvData for
*	use by EPICS programs running under UNIX and VxWorks.
*
*	User programs can define their own environment parameters for their
*	own use--the only caveat is that such parameters aren't automatically
*	setup by EPICS.
*
*****************************************************************************/

#ifndef envDefsH
#define envDefsH

#ifdef __cplusplus
extern "C" {
#endif

#include "libComAPI.h"

typedef struct envParam {
    char	*name;		/* text name of the parameter */
    char	*pdflt;
} ENV_PARAM;

/*
 * bldEnvData.pl looks for "LIBCOM_API extern const ENV_PARAM <name>;"
 */
LIBCOM_API extern const ENV_PARAM EPICS_CA_ADDR_LIST;
LIBCOM_API extern const ENV_PARAM EPICS_CA_CONN_TMO;
LIBCOM_API extern const ENV_PARAM EPICS_CA_AUTO_ADDR_LIST;
LIBCOM_API extern const ENV_PARAM EPICS_CA_REPEATER_PORT;
LIBCOM_API extern const ENV_PARAM EPICS_CA_SERVER_PORT;
LIBCOM_API extern const ENV_PARAM EPICS_CA_MAX_ARRAY_BYTES;
LIBCOM_API extern const ENV_PARAM EPICS_CA_AUTO_ARRAY_BYTES;
LIBCOM_API extern const ENV_PARAM EPICS_CA_MAX_SEARCH_PERIOD;
LIBCOM_API extern const ENV_PARAM EPICS_CA_NAME_SERVERS;
LIBCOM_API extern const ENV_PARAM EPICS_CA_MCAST_TTL;
LIBCOM_API extern const ENV_PARAM EPICS_CAS_INTF_ADDR_LIST;
LIBCOM_API extern const ENV_PARAM EPICS_CAS_IGNORE_ADDR_LIST;
LIBCOM_API extern const ENV_PARAM EPICS_CAS_AUTO_BEACON_ADDR_LIST;
LIBCOM_API extern const ENV_PARAM EPICS_CAS_BEACON_ADDR_LIST;
LIBCOM_API extern const ENV_PARAM EPICS_CAS_SERVER_PORT;
LIBCOM_API extern const ENV_PARAM EPICS_CA_BEACON_PERIOD; /* deprecated */
LIBCOM_API extern const ENV_PARAM EPICS_CAS_BEACON_PERIOD;
LIBCOM_API extern const ENV_PARAM EPICS_CAS_BEACON_PORT;
LIBCOM_API extern const ENV_PARAM EPICS_BUILD_COMPILER_CLASS;
LIBCOM_API extern const ENV_PARAM EPICS_BUILD_OS_CLASS;
LIBCOM_API extern const ENV_PARAM EPICS_BUILD_TARGET_ARCH;
LIBCOM_API extern const ENV_PARAM EPICS_TZ;
LIBCOM_API extern const ENV_PARAM EPICS_TS_NTP_INET;
LIBCOM_API extern const ENV_PARAM EPICS_IOC_IGNORE_SERVERS;
LIBCOM_API extern const ENV_PARAM EPICS_IOC_LOG_PORT;
LIBCOM_API extern const ENV_PARAM EPICS_IOC_LOG_INET;
LIBCOM_API extern const ENV_PARAM EPICS_IOC_LOG_FILE_LIMIT;
LIBCOM_API extern const ENV_PARAM EPICS_IOC_LOG_FILE_NAME;
LIBCOM_API extern const ENV_PARAM EPICS_IOC_LOG_FILE_COMMAND;
LIBCOM_API extern const ENV_PARAM EPICS_IOC_LOG_PREFIX;
LIBCOM_API extern const ENV_PARAM IOCSH_PS1;
LIBCOM_API extern const ENV_PARAM IOCSH_HISTSIZE;
LIBCOM_API extern const ENV_PARAM IOCSH_HISTEDIT_DISABLE;

LIBCOM_API extern const ENV_PARAM *env_param_list[];

struct in_addr;

LIBCOM_API char * LIBCOMSTD_API 
	envGetConfigParam(const ENV_PARAM *pParam, int bufDim, char *pBuf);
LIBCOM_API const char * LIBCOMSTD_API 
	envGetConfigParamPtr(const ENV_PARAM *pParam);
LIBCOM_API long LIBCOMSTD_API 
	envPrtConfigParam(const ENV_PARAM *pParam);
LIBCOM_API long LIBCOMSTD_API 
	envGetInetAddrConfigParam(const ENV_PARAM *pParam, struct in_addr *pAddr);
LIBCOM_API long LIBCOMSTD_API 
	envGetDoubleConfigParam(const ENV_PARAM *pParam, double *pDouble);
LIBCOM_API long LIBCOMSTD_API 
	envGetLongConfigParam(const ENV_PARAM *pParam, long *pLong);
LIBCOM_API unsigned short LIBCOMSTD_API envGetInetPortConfigParam 
        (const ENV_PARAM *pEnv, unsigned short defaultPort);
LIBCOM_API long LIBCOMSTD_API
    envGetBoolConfigParam(const ENV_PARAM *pParam, int *pBool);
LIBCOM_API long LIBCOMSTD_API epicsPrtEnvParams(void);
LIBCOM_API void LIBCOMSTD_API epicsEnvSet (const char *name, const char *value);
LIBCOM_API void LIBCOMSTD_API epicsEnvUnset (const char *name);
LIBCOM_API void LIBCOMSTD_API epicsEnvShow (const char *name);

#ifdef __cplusplus
}
#endif

#endif /*envDefsH*/

