#ifndef __HTTPSERVER_NETCONN_H__
#define __HTTPSERVER_NETCONN_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "lwip/api.h"
void http_server_netconn_init(void);
void DynWebPage(struct netconn *conn);
#ifdef __cplusplus
}
#endif
#endif /* __HTTPSERVER_NETCONN_H__ */
