// Copyright: Ramiro Polla
// License: WTFPL

#include <stdio.h>

//---------------------------------------------------------------------
int copy_text_file(const char *from, const char *to)
{
	FILE *fp_in = NULL;
	FILE *fp_out = NULL;
	int ret = -1;

	fp_in = fopen(from, "r");
	if ( fp_in == NULL )
		goto the_end;
	fp_out = fopen(to, "w");
	if ( fp_out == NULL )
		goto the_end;

	char buf[0x1000];
	while ( fgets(buf, sizeof(buf), fp_in) != NULL )
		fputs(buf, fp_out);

	ret = 0;

the_end:
	if ( fp_out != NULL )
		fclose(fp_out);
	if ( fp_in != NULL )
		fclose(fp_in);

	return ret;
}
