// Copyright: Ramiro Polla
// License: WTFPL

#include <stdio.h>
#include <string.h>

//---------------------------------------------------------------------
int print(const char *txt, const char *lp, int n)
{
	FILE *fp_txt = NULL;
	FILE *fp_lp = NULL;
	int ret = -1;

	fp_txt = strcmp(txt, "-") ? fopen(txt, "r") : stdin;
	if ( fp_txt == NULL )
		goto the_end;
	fp_lp = strcmp(lp, "-") ? fopen(lp, "w") : stdout;
	if ( fp_lp == NULL )
		goto the_end;

	fprintf(fp_lp, "\x1B""0"); // spacing 1/8 (0.125)
	fprintf(fp_lp, "\x1B""M"); // 12 cpi

	char buf[0x1000];
	while ( fgets(buf, sizeof(buf), fp_txt) != NULL )
	{
		int l = strcspn(buf, "\r\n");
		buf[l] = '\r';
		if ( n > 1 )
		{
			buf[l+1] = '\0';
			for ( int i = 1; i < n; i++ )
				fputs(buf, fp_lp);
		}
		buf[l+1] = '\n';
		buf[l+2] = '\0';
		fputs(buf, fp_lp);
	}

	ret = 0;

the_end:
	if ( fp_lp != NULL )
		fclose(fp_lp);
	if ( fp_txt != NULL )
		fclose(fp_txt);

	return ret;
}
