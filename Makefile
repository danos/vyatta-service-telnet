cpiop = find  . ! -regex '\(.*~\|.*\.bak\|.*\.swp\|.*\#.*\#\)' -print0 | \
  cpio -0pd

all: mini-inetd

mini-inetd:
	$(CC) $(CFLAGS) -o mini-inetd src/mini-inetd.c

install:
	mkdir -p $(DESTDIR)/usr/sbin
	install -m755 -t $(DESTDIR)/usr/sbin scripts/system/update-inetd
	mkdir -p $(DESTDIR)/opt/vyatta/sbin
	install -m755 -t $(DESTDIR)/opt/vyatta/sbin \
		scripts/system/vyatta_update_telnet
	install -m755 -t $(DESTDIR)/opt/vyatta/sbin mini-inetd
	mkdir -p $(DESTDIR)/opt/vyatta/bin
	install -m755 -t $(DESTDIR)/opt/vyatta/bin \
		scripts/system/vrouter-telnet
	mkdir -p $(DESTDIR)/opt/vyatta/share/vyatta-op/templates
	cd templates && $(cpiop) $(DESTDIR)/opt/vyatta/share/vyatta-op/templates
	mkdir -p $(DESTDIR)/opt/vyatta/share/tmplscripts
	cd tmplscripts && $(cpiop) $(DESTDIR)/opt/vyatta/share/tmplscripts
	mkdir -p $(DESTDIR)/usr/share/configd/yang
	install -m644 -t $(DESTDIR)/usr/share/configd/yang \
		yang/vyatta-service-telnet-v1.yang
	install -m644 -t $(DESTDIR)/usr/share/configd/yang \
		yang/vyatta-service-telnet-routing-instance-v1.yang
	mkdir -p $(DESTDIR)/opt/vyatta/preconfig.d
	cd preconfig.d && $(cpiop) $(DESTDIR)/opt/vyatta/preconfig.d
