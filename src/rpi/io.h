// Copyright: Ramiro Polla
// License: WTFPL

int io_read(int pi, unsigned gpio);
int io_write(int pi, unsigned gpio, unsigned level);
int io_start(void);
void io_update(void);
void io_set_input(int pi, unsigned gpio);
void io_set_output(int pi, unsigned gpio);
void io_set_pull_up_down(int pi, unsigned gpio);
