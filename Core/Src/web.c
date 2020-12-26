/*
 * web.c
 *
 *  Created on: Apr 5, 2020
 *      Author: Yuan
 */
#include "ff.h"
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"
#include "lwip/ip.h"
#include "string.h"
#include "web.h"
#include "stdbool.h"
#include "stm32f769i_discovery.h"

#define webSERVER_PROTOCOL "HTTP/1.1"
#define webSERVER_SOFTWARE    "lwIP"
#define webMAX_DATA_TO_SEND 2048


static void parse_respond_fetch_set_req(portCHAR *gs_req, struct netconn *current_conn);
static unsigned char fetch_file_path_type(char* file_path);
static const char http_html_hdr_200[] = "HTTP/1.1 200 OK\r\n";
static const char http_html_hdr_404[] = "HTTP/1.1 404 Not Found\r\n";
static void send_requested_file(FIL *pFile, u8_t content_type, struct netconn *current_conn);
static unsigned portLONG prulweb_BuildHeaders(portCHAR * response, int s, char * title, char* extra_header, char* me, char* mt );
unsigned portLONG build_json_response(u8_t light, u8_t temperature, u8_t button_stat,
portCHAR *l_alarm, portCHAR *t_alarm, portCHAR *json_reply);

static struct {
	char *content; } http_html_hdr[] = {
	{"Content-type: application/json\r\n"},
	{"Content-type: text/html\r\n"},
	{"Content-type: text/css\r\n"},
	{"Content-type: text/javascript\r\n"},
	{"Content-type: image/png\r\n"},
	{"Content-type: image/jpeg\r\n"},
	{"Content-type: image/gif\r\n"},
	{"Content-type: text/plain\r\n"},
	{"Content-type: image/x-icon\r\n"},
	{""},
};
static const char conf_file[12] = "config.txt";

void process_req (portCHAR * rq, struct netconn *current_conn)
{
	//FATFS fs;

	/*--------------------------------------------------------------------------*/
	/* Waiting for SD card to mount*/
	/*--------------------------------------------------------------------------*/


	FIL pFile;
	portCHAR *path, *protocol;

	u8_t content_type;

	/* fetch first occurrence of space char or \t or \n or \r */
	path = strpbrk( rq, " \t\n\r" );
	if ( path == (char*) 0 )
	{
		return;
	}

	*path++ = '\0';
	protocol = strpbrk( path, " \t\r\n" );
	*protocol++ = '\0';

	/* if there is a ?, this is a fetch/set request */
	protocol = strpbrk( path, "?" );
	if (protocol != NULL)
	{
		parse_respond_fetch_set_req(protocol, current_conn);
		return;
	}
	else /* a file has been requested */
	{
		content_type = fetch_file_path_type(path);

		if (f_open (&pFile, (const TCHAR*)path , FA_READ )!=FR_OK)
		{
			if(netconn_write(current_conn, http_html_hdr_404, sizeof(http_html_hdr_404), NETCONN_NOCOPY) != ERR_OK)
			{
				LWIP_DEBUGF(LWIP_DBG_ON, ("Write error=%d\n",conn_check));
			}
		}
		else
		{
			send_requested_file(&pFile, content_type, current_conn);
		}
		f_close(&pFile);

	}
}
extern int32_t ConvertedValue;
#define TEMP_REFRESH_PERIOD   1000    /* Internal temperature refresh period */
#define MAX_CONVERTED_VALUE   4095    /* Max converted value */
#define AMBIENT_TEMP            25    /* Ambient Temperature */
#define VSENS_AT_AMBIENT_TEMP  760    /* VSENSE value (mv) at ambient temperature */
#define AVG_SLOPE               25    /* Avg_Solpe multiply by 10 */
#define VREF                  3300
extern uint32_t light_value;
static void parse_respond_fetch_set_req(portCHAR *gs_req, struct netconn *current_conn)
{
	portCHAR *gs_val,*gs_set_req, *set_req_response, *gs_set_req_val;
	portCHAR *gs_set_req_1, *gs_set_req_2;

	u8_t temperature_val, light_val;
	bool pb_stat = false;
	portCHAR *light_stat, *temp_stat;

	unsigned portLONG response_size,header_size;

	if (!strncmp( &gs_req[1], "fetch", 5 ))
	{
		if( !strncmp( &gs_req[1], "fetch_sensor", 11 ))
		{
			set_req_response = (char*) pvPortMalloc (240);
			light_stat = (char*) pvPortMalloc (12);
			temp_stat = (char*) pvPortMalloc (12);
			pb_stat = BSP_PB_GetState(BUTTON_WAKEUP);
			temperature_val = ((((ConvertedValue * VREF)/MAX_CONVERTED_VALUE) - VSENS_AT_AMBIENT_TEMP) * 10 / AVG_SLOPE) + AMBIENT_TEMP;;
			light_val = light_value;

			sprintf(light_stat, "Normal");

			sprintf(temp_stat, "Below_min");

			header_size = prulweb_BuildHeaders(set_req_response, 200, "OK", "", "", http_html_hdr[HTTP_CONTENT_JSON].content);

			response_size = build_json_response (light_val, temperature_val, pb_stat, light_stat, temp_stat, &set_req_response[header_size]);

			netconn_write(current_conn, set_req_response, (u16_t)(response_size+header_size),	NETCONN_COPY);

			vPortFree (set_req_response);
			vPortFree(light_stat);
			vPortFree(temp_stat);
		}
		else if( !strncmp( &gs_req[1], "fetch_conf", 10 ))
		{
			FIL pFile;
			f_open (&pFile, conf_file , FA_READ);
			send_requested_file(&pFile, HTTP_CONTENT_DEFAULT, current_conn);
			f_close(&pFile);
		}
	}
	else
	{
		if( netconn_write(current_conn, http_html_hdr_200, sizeof(http_html_hdr_200),
				NETCONN_COPY) != ERR_OK)
		{
			LWIP_DEBUGF(LWIP_DBG_ON, ("Write error=%d\n",conn_check));
		}
		if( !strncmp( &gs_req[1], "set_led_on", 10 ))
		{
			BSP_LED_On(LED1);
		}
		else if( !strncmp( &gs_req[1], "set_led_off", 11 ))
		{
			BSP_LED_Off(LED1);
		}

	}
}
static unsigned char fetch_file_path_type(char* file_path)
{
	HTTP_content_type content_type = 0;
	char * cont_type;

	if (file_path[1] == '\0')
	{
		strcpy(file_path, "index.htm");
		content_type = HTTP_CONTENT_HTML;
	}
	else
	{
		cont_type = strpbrk(file_path, ".");
		if (cont_type != NULL)
		{
			file_path = &(file_path[1]);
			if (strcmp(&cont_type[1], "jpg") == 0)
			{
				content_type = HTTP_CONTENT_JPEG;
			}
			else if (strcmp(&cont_type[1], "css") == 0)
			{
				content_type = HTTP_CONTENT_CSS;

			}
			else if (strcmp(&cont_type[1], "ico") == 0)
			{
				content_type = HTTP_CONTENT_ICO;
			}
			else if (strcmp(&cont_type[1], "gif") == 0)
			{
				content_type = HTTP_CONTENT_GIF;
			}
			else if (strcmp(&cont_type[1], "txt") == 0)
			{
				content_type = HTTP_CONTENT_PLAIN;
			}
			else if (strcmp(&cont_type[1], "js") == 0)
			{
				content_type = HTTP_CONTENT_JS;
			}
			else if (strncmp(&cont_type[1], "htm", 3) == 0)
			{
				content_type = HTTP_CONTENT_HTML;
			}
			else if (strcmp(&cont_type[1], "png") == 0)
			{
				content_type = HTTP_CONTENT_PNG;
			}
			else if (strcmp(&cont_type[1], "json") == 0)
			{
				content_type = HTTP_CONTENT_JSON;
			}
			else
			{
				content_type = HTTP_CONTENT_DEFAULT;
			}
		}
	}
	//printf(file_path);
	return content_type;
}
static void send_requested_file(FIL *pFile, u8_t content_type, struct netconn *current_conn)
{
	portCHAR *buffer;
	uint32_t result;
	portLONG header_size, fSize;
	unsigned portSHORT  i;
	int conn_check;
	// obtain file size:

	fSize = f_size(pFile);



	// allocate memory to contain the whole file:
	buffer = (char*) pvPortMalloc (webMAX_DATA_TO_SEND);

	/* add the header */
	header_size = prulweb_BuildHeaders(buffer, 200, "OK", "", "", http_html_hdr[content_type].content);

	/* check if file should be sent in a single frame */
	if ( (fSize+header_size) <= webMAX_DATA_TO_SEND)
	{
		// copy the file into the buffer:

		if ( f_read(pFile, &buffer[header_size],fSize, &result)!= FR_OK)
		{

			conn_check = netconn_write(current_conn, http_html_hdr_404, sizeof(http_html_hdr_404),NETCONN_NOCOPY);
			if( conn_check != ERR_OK)
			{
				LWIP_DEBUGF(LWIP_DBG_ON, ("Write error=%d\n",conn_check));
			}
			return;
		}
		/* Send the response. */
		conn_check = netconn_write(current_conn, buffer, (u16_t)(fSize+header_size), NETCONN_COPY);
		if( conn_check != ERR_OK)
		{
			LWIP_DEBUGF(LWIP_DBG_ON, ("Write error=%d\n",conn_check));
			return;
		}
	}
	else
	{
		/* Send the header. */
		conn_check = netconn_write(current_conn, buffer, (u16_t)header_size,NETCONN_COPY);
		if( conn_check != ERR_OK)
		{
			LWIP_DEBUGF(LWIP_DBG_ON, ("Write error=%d\n",conn_check));
			return;
		}

		while(f_tell(pFile) != f_size(pFile))
		{
			if (f_read(pFile, buffer, webMAX_DATA_TO_SEND, &result)!= FR_OK)
			{
				conn_check = netconn_write(current_conn, http_html_hdr_404, sizeof(http_html_hdr_404),NETCONN_NOCOPY);
				if( conn_check != ERR_OK)
				{
					LWIP_DEBUGF(LWIP_DBG_ON, ("Write error=%d\n",conn_check));
				}
				return;
			}
			netconn_write(current_conn, buffer, result, NETCONN_COPY);
		}
//		/* try to send the biggest frame contained in the file */
//		for (i = webMax_SECTORS_TO_SEND ; i > 0 ; i--)
//		{
//			/* get sectors of maximum size */
//			while(fSize > i * _MAX_SS)
//			{
//				// copy the file into the buffer:
//				result = fread (buffer,1,(i * _MAX_SS),pFile);
//				if (result != (i * _MAX_SS)) {
//					conn_check = netconn_write(current_conn, http_html_hdr_404, sizeof(http_html_hdr_404),NETCONN_COPY);
//					if( conn_check != ERR_OK)
//					{
//						LWIP_DEBUGF(LWIP_DBG_ON, ("Write error=%d\n",conn_check));
//					}
//				}
//				conn_check = netconn_write(current_conn, buffer, (u16_t)(i*_MAX_SS), NETCONN_COPY);
//				if( conn_check != ERR_OK)
//				{
//					LWIP_DEBUGF(LWIP_DBG_ON, ("Write error=%d\n",conn_check));
//				}
//				fSize -= (i * _MAX_SS);
//			}
//		}
//		/* finish with the few data remaining (less than 1 sector)*/
//		if ( fSize > 0 )
//		{
//				// copy the file into the buffer:
//				result = fread (buffer,1,fSize,pFile);
//				if (result != fSize) {
//					conn_check = netconn_write(current_conn, http_html_hdr_404, sizeof(http_html_hdr_404),NETCONN_COPY);
//					if( conn_check != ERR_OK)
//					{
//						LWIP_DEBUGF(LWIP_DBG_ON, ("Write error=%d\n",conn_check));
//					}
//				}
//			conn_check = netconn_write(current_conn, buffer, (u16_t)fSize,
//			NETCONN_COPY);
//			if( conn_check != ERR_OK)
//			{
//				LWIP_DEBUGF(LWIP_DBG_ON, ("Write error=%d\n",conn_check));
//			}
//		}
	}
	vPortFree (buffer);
}
static unsigned portLONG prulweb_BuildHeaders(portCHAR * response, int s, char * title, char* extra_header, char* me, char* mt )
{
unsigned portLONG retsize = 0;
portLONG s100;

  retsize = sprintf( response, "%s %d %s\r\n", webSERVER_PROTOCOL, s, title );
  retsize += sprintf( &response[retsize], "Server: %s\r\n", webSERVER_SOFTWARE );
  s100 = s / 100;
  if ( s100 != 3 )
  {
    retsize += sprintf( &response[retsize], "Cache-Control: no-cache,no-store\r\n" );
  }
  if ( extra_header != (char*) 0 && extra_header[0] != '\0' )
  {
    retsize += sprintf( &response[retsize], "%s\r\n", extra_header );
  }
  if ( me != (char*) 0 && me[0] != '\0' )
  {
    retsize += sprintf(&response[retsize], "Content-Encoding: %s\r\n", me );
  }
  if ( mt != (char*) 0 && mt[0] != '\0' )
  {
    retsize += sprintf( &response[retsize], "%s", mt );
  }
  retsize += sprintf( &response[retsize], "Connection: close\r\n\r\n" );

  return (retsize);
}
unsigned portLONG build_json_response(u8_t light, u8_t temperature, u8_t button_stat,
portCHAR *l_alarm, portCHAR *t_alarm, portCHAR *json_reply)
{
	unsigned portLONG response_size;
	response_size = sprintf(json_reply, Sensor_STAT_json , temperature, light, button_stat, l_alarm, t_alarm);
	return response_size;
}
