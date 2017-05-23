// Copyright: Ramiro Polla
// License: WTFPL

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "txt.h"

//---------------------------------------------------------------------
char *txt_read(const char *fname, size_t *width, size_t *height)
{
	FILE *fp = strcmp(fname, "-") ? fopen(fname, "rb") : stdin;
	if ( fp == NULL )
	{
		fprintf(stderr, "could not open '%s'\n", fname);
		return NULL;
	}

	*width = 0;
	*height = 0;
	char buf[0x1000];
	while ( fgets(buf, sizeof(buf), fp) != NULL )
	{
		buf[strcspn(buf, "\r\n")] = 0;
		size_t cur_w = strlen(buf);
		if ( *width < cur_w )
			*width = cur_w;
		(*height)++;
	}
	fseek(fp, 0, SEEK_SET);

	char *txt = (char *) malloc(*width * *height);
	if ( txt == NULL )
	{
		fprintf(stderr, "no mem\n");
		return NULL;
	}

	int y = 0;
	while ( fgets(buf, sizeof(buf), fp) != NULL )
	{
		buf[strcspn(buf, "\r\n")] = 0;
		size_t cur_w = strlen(buf);
		memcpy(txt + y * *width, buf, cur_w);
		memset(txt + y * *width + cur_w, 0xFF, *width - cur_w);
		y++;
	}

	fclose(fp);

	return txt;
}

//---------------------------------------------------------------------
int txt_write(const char *fname, const char *txt)
{
	FILE *fp = strcmp(fname, "-") ? fopen(fname, "wb") : stdout;
	if ( fp == NULL )
		return -1;
	fputs(txt, fp);
	fclose(fp);
	return 0;
}
