
INCLUDES = -I./util
LIBS = -lpng

## On OS X, libpng is provided by XQuartz in /opt/X11.
UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
  INCLUDES += -I/opt/X11/include
  LIBS += -L/opt/X11/lib
endif

PRODUCTS = \
  play_barca \
  play_bejeweled \
  play_jorinapeka

UTILS = \
  util/ImageFmtc.o \
  util/Interact.o \
  util/SimplePng.o

all: $(PRODUCTS)

play_barca: Barca/main.o Barca/process_image.o Barca/ai.o $(UTILS)
	g++ $^ $(LIBS) -o $@

play_bejeweled: Bejeweled/main.o Bejeweled/process_image.o Bejeweled/ai.o $(UTILS)
	g++ $^ $(LIBS) -o $@

play_jorinapeka: Jorinapeka/main.o Jorinapeka/process_image.o Jorinapeka/ai.o $(UTILS)
	g++ $^ $(LIBS) -o $@

clean:
	rm -f {Barca,Bejeweled,Jorinapeka,util}/*.o $(PRODUCTS)

%.o:%.c
	gcc $(INCLUDES) -c $^ -o $@

%.o:%.cc
	g++ $(INCLUDES) -c $^ -o $@
