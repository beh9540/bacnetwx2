all: library server


clean: lib/Makefile\
	demo/server/Makefile
	make -C lib clean
	make -C demo/server clean

library: lib/Makefile
	make -C lib all

server: demo/server/Makefile
	( cd demo/server ; make ; cp bacnetwx /usr/sbin ; cd ../.. ; cd doc ; cp weather.conf /etc/bacnetwx ; cp bacnetwx /etc/init.d/ ; update-rc.d bacnetwx defaults )

ports:	atmega168 at91sam7s bdk-atxx4-mstp
	echo "Built the ports"

atmega168: ports/atmega168/Makefile
	make -C ports/atmega168 clean all

at91sam7s: ports/at91sam7s/makefile
	make -C ports/at91sam7s clean all

bdk-atxx4-mstp: ports/bdk-atxx4-mstp/Makefile
	make -C ports/bdk-atxx4-mstp clean all




