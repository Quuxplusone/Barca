
## On OS X, libpng is provided by XQuartz in /opt/X11.
UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
  DASHI = -I/opt/X11/include
  DASHL = -L/opt/X11/lib
endif

barca: barca.o input.o ai.o ImageFmtc.o simple-png.o
	g++ $(DASHL) $^ -lpng -o barca

clean:
	rm -f barca.o input.o ai.o
	rm -f ImageFmtc.o simple-png.o
	rm -f barca

%.o:%.c
	gcc $(DASHI) -c $^ -o $@

%.o:%.cc
	g++ -c $^ -o $@
