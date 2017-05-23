// Copyright: Ramiro Polla
// License: WTFPL

#include <inttypes.h>

// Read text file <fname>.
// Returns dimensions in <width> and <height>.
// Return text data must be free()'d
char *txt_read(const char *fname, size_t *width, size_t *height);

// Write text from <txt> to <fname>.
int txt_write(const char *fname, const char *txt);
