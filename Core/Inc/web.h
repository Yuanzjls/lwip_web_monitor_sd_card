/*
 * web.h
 *
 *  Created on: Apr 5, 2020
 *      Author: Yuan
 */

#ifndef INC_WEB_H_
#define INC_WEB_H_


#define Sensor_STAT_json "{ \"Temp\": %d, \"Light\": %d, \"Button\": %d, \"L_alarm\": \"%s\", \"T_alarm\": \"%s\" }"

#ifdef __cplusplus
 extern "C" {
#endif

 typedef enum  {
 		HTTP_CONTENT_JSON = 0,
 		HTTP_CONTENT_HTML,
 		HTTP_CONTENT_CSS,
 		HTTP_CONTENT_JS,
 		HTTP_CONTENT_PNG,
 		HTTP_CONTENT_JPEG,
 		HTTP_CONTENT_GIF,
 		HTTP_CONTENT_PLAIN,
 		HTTP_CONTENT_ICO,
 		HTTP_CONTENT_DEFAULT,
 		HTTP_CONTENT_CNT,
 	}HTTP_content_type;

 void process_req (portCHAR * rq, struct netconn *current_conn);

#ifdef __cplusplus
}
#endif

#endif /* INC_WEB_H_ */
