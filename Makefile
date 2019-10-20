all: aprsudp aprstcp aprscmdtcp udptoaprs udptomysql udptolog udptolocal aprstomysql aprs.fi.toudp local.toudp local.toaprs gt02 udptomysql2 resendaprs

aprsudp: aprsudp.c sendudp.c util.c
	gcc -g -o aprsudp aprsudp.c	 -Wall -lm
aprstcp: aprstcp.c sendudp.c sock.h printaddr.c util.c
	gcc -g -o aprstcp aprstcp.c	 -Wall  -lm
aprscmdtcp: aprscmdtcp.c sendudp.c sock.h printaddr.c
	gcc -g -o aprscmdtcp aprscmdtcp.c	 -Wall  -lmysqlclient -L/usr/lib64/mysql/
udptoaprs: udptoaprs.c passcode.c
	gcc -o udptoaprs udptoaprs.c -Wall
aprstomysql: aprstomysql.c db.h tomysql.c sock.h sendudp.c
	gcc -g -o aprstomysql aprstomysql.c -Wall -lmysqlclient -lm -L/usr/lib64/mysql/
resendaprs: resendaprs.c sendudp.c
	gcc -g -o resendaprs resendaprs.c  -Wall


udptolocal: udptolocal.c passcode.c sock.h
	gcc -o udptolocal udptolocal.c -Wall

aprs.fi.toudp: aprs.fi.toudp.c passcode.c
	gcc -o aprs.fi.toudp aprs.fi.toudp.c -Wall
local.toudp: local.toudp.c passcode.c
	gcc -o local.toudp local.toudp.c -Wall
local.toaprs: local.toaprs.c passcode.c
	gcc -o local.toaprs local.toaprs.c -Wall

gt02: gt02.c printaddr.c
	gcc -g -o gt02 gt02.c	 -Wall -lm
udptomysql: udptomysql.c db.h tomysql.c
	gcc -g -o udptomysql udptomysql.c -Wall -lmysqlclient -lm -L/usr/lib64/mysql/

udptomysql2: udptomysql2.c db.h tomysql2.c
	gcc -g -o udptomysql2 udptomysql2.c -Wall -lmysqlclient -lm -L/usr/lib64/mysql/
indent:
	indent aprsudp.c sendudp.c aprs.fi.toudp.c  passcode.c local.toudp.c local.toaprs.c \
	aprscmdtcp.c aprstcp.c gt02.c udptoaprs.c udptolocal.c udptomysql.c tomysql.c aprstomysql.c \
	udptomysql2.c tomysql2.c resendaprs.c util.c \
	-nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 \
-cli0 -d0 -di1 -nfc1 -i8 -ip0 -l160 -lp -npcs -nprs -npsl -sai \
-saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8 -il1
clean:
	rm -f aprsudp.o
