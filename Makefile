project=ssh-bleach
prefix=/usr/local
bin=$(prefix)/bin
man=$(prefix)/share/man
man1=$(man)/man1
lib=$(prefix)/lib
etc=$(prefix)/etc
libdir=$(lib)/$(project)

OBJS = ssh-bleach.c

CFLAGS=-g -DCONFDIR=\"$(libdir)\"

$(project): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS)
