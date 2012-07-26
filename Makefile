CC=nqc
CFLAGS=-Trcx2

all:
		$(CC) $(CFLAGS) -d test.c

lasm:
		$(CC) $(CFLAGS) 

firmware:
		$(CC) -firmware firmware/firm0328.lgo

symlink:
		sudo ln -s /dev/usb/legousbtower0 /dev/usb/lego0
		sudo chmod 777 /dev/usb/lego0

clean:
		rm -rf *.c.rcx
