all: aprstcp aprscmdtcp aprsudp udptoaprs udptomysql udptolog udptolocal aprs.fi.toudp local.toudp local.toaprs gt02 aprstomysql udptomysql2

aprs.fi.toudp: aprs.fi.toudp.c passcode.c
	gcc -o aprs.fi.toudp aprs.fi.toudp.c -Wall
local.toudp: local.toudp.c passcode.c
	gcc -o local.toudp local.toudp.c -Wall
local.toaprs: local.toaprs.c passcode.c
	gcc -o local.toaprs local.toaprs.c -Wall
aprscmdtcp: aprscmdtcp.c
	gcc -g -o aprscmdtcp aprscmdtcp.c	 -Wall  -lmysqlclient -L/usr/lib64/mysql/
aprstcp: aprstcp.c
	gcc -g -o aprstcp aprstcp.c	 -Wall  

gt02: gt02.c
	gcc -g -o gt02 gt02.c	 -Wall -lm
aprsudp: aprsudp.c
	gcc -g -o aprsudp aprsudp.c	 -Wall
udptoaprs: udptoaprs.c passcode.c
	gcc -o udptoaprs udptoaprs.c -Wall
udptolocal: udptolocal.c passcode.c
	gcc -o udptolocal udptolocal.c -Wall
udptomysql: udptomysql.c db.h tomysql.c
	gcc -g -o udptomysql udptomysql.c -Wall -lmysqlclient -L/usr/lib64/mysql/
aprstomysql: aprstomysql.c db.h tomysql.c
	gcc -g -o aprstomysql aprstomysql.c -Wall -lmysqlclient -L/usr/lib64/mysql/

udptomysql2: udptomysql2.c db.h tomysql2.c
	gcc -g -o udptomysql2 udptomysql2.c -Wall -lmysqlclient -L/usr/lib64/mysql/
indent:
	indent aprs.fi.toudp.c  passcode.c local.toudp.c local.toaprs.c aprscmdtcp.c aprstcp.c aprsudp.c gt02.c udptoaprs.c udptolocal.c udptomysql.c tomysql.c aprstomysql.c udptomysql2.c tomysql2.c  -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4  \
-cli0 -d0 -di1 -nfc1 -i8 -ip0 -l160 -lp -npcs -nprs -npsl -sai \
-saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8 -il1
clean:
	rm -f aprsudp.o
