CFLAGS := -g
targets = libdagame.a
src = $(wildcard *.c)
objs = $(pathsubst %.c,%.o$(src))

all: $(targets)
.PHONY : all

libdagame.a: $(objs)
	ar -c -r libdagame.a $(objs)

$(objs) : $(src)

clean:
	rm $(objs) $(targets)
