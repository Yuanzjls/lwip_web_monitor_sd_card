#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__


#include "lwip/err.h"
#include "lwip/netif.h"
#include "cmsis_os.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Exported types ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
err_t ethernetif_init(struct netif *netif);
#ifdef __cplusplus
}
#endif
#endif
