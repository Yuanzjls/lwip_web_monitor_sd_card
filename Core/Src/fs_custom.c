/*
 * fs_custom.c
 *
 *  Created on: Apr 3, 2020
 *      Author: Yuan
 */

#include "lwip/apps/httpd_opts.h"
#include "lwip/def.h"
#include "lwip/apps/fs.h"
#include <string.h>
#include "ff.h"

char filename_http[100];
char filenmae_http_pre[] = "html";

static FIL fi_http;

int fs_open_custom(struct fs_file *file, const char *name)
{
	strcpy(filename_http, filenmae_http_pre);
	strcat(filename_http, name);

	if (f_open(&fi_http, filename_http, FA_READ) == FR_OK)
	{
		file->data = NULL;
		file->flags = 0;
		file->index = 0;
		file->len = f_size(&fi_http);
		file->pextension = NULL;
		return 1;
	}
	else
	{
		return 0;
	}
}
void fs_close_custom(struct fs_file *file)
{
	f_close(&fi_http);
}

int fs_read_custom(struct fs_file *file, char *buffer, int count)
{
	unsigned int length;

	f_read(&fi_http, (void*)buffer, count, &length);
	return length;
}
