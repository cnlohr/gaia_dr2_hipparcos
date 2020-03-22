all : process crosscorrelate-hip

process : process.c
	gcc -o $@ $^

crosscorrelate-hip : crosscorrelate-hip.c
	gcc -O2 -I. -o $@ $^

clean :
	rm -rf *.o *~ process crosscorrelate-hip

