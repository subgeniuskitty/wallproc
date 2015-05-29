# wallproc - bulk crop images to fixed aspect ratio
# See LICENSE file for copyright and license details.

PREFIX = /usr/local

MAGICKFLAGS = `pkg-config --cflags --libs MagickWand`
SDLFLAGS = -lSDL2 -lSDL2_image -I/usr/local/include/SDL2

CFLAGS = -std=gnu99 -Wall ${MAGICKFLAGS} ${SDLFLAGS} # -std=gnu99 included for dirent.h
CC = gcc

all: options wallproc

options:
	@echo wallproc build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "CC       = ${CC}"

wallproc:
	@${CC} -o $@ ${CFLAGS} main.c

clean:
	@rm wallproc

install: all
	@echo installing executable file to ${PREFIX}/bin
	@mkdir -p ${PREFIX}/bin
	@cp -f wallproc ${PREFIX}/bin
	@chmod 755 ${PREFIX}/bin/wallproc

uninstall:
	@echo removing executable file from ${PREFIX}/bin
	@rm -f ${PREFIX}/bin/wallproc
