// Copyright: Ramiro Polla
// License: WTFPL

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lp.h"

int main(int argc, char *argv[])
{
	int n = 1;

	if ( argc > 3 )
	{
		char *endptr = NULL;
		n = strtol(argv[3], &endptr, 0);
		if ( endptr == argv[3] || *endptr != '\0' )
			n = -1;
	}

	if ( argc < 3 || n <= 0 )
	{
		fprintf(stderr, "usage: %s <txt> <lp> [repeat]\n", argv[0]);
		return -1;
	}

	int ret = print(argv[1], argv[2], n);

	return ret;
}
