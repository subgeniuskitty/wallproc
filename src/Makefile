# wallproc - bulk crop images to fixed aspect ratio
# See LICENSE file for copyright and license details.

PREFIX = /usr/local

MAGICKFLAGS = `pkg-config --cflags --libs MagickWand`
SDLFLAGS = -lSDL2 -lSDL2_image -I/usr/local/include/SDL2

CFLAGS = -std=gnu99 -Wall ${MAGICKFLAGS} ${SDLFLAGS} # -std=gnu99 included for dirent.h
CC = gcc

SRC_CROP = main_wallproc.c file_io.c imagick.c misc.c sdl.c selection_box.c startup_shutdown.c ui.c
SRC_MINSIZE = main_minsize.c file_io.c

all: options wp_crop wp_minsize

options:
	@echo wallproc build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "CC       = ${CC}"

wp_crop:
	@${CC} -o $@ ${CFLAGS} ${SRC_CROP}

wp_minsize:
	@${CC} -o $@ ${CFLAGS} ${SRC_MINSIZE}

clean:
	@rm wp_crop wp_minsize

install: all
	@echo installing executable file to ${PREFIX}/bin
	@mkdir -p ${PREFIX}/bin
	@cp -f wp_crop ${PREFIX}/bin
	@chmod 755 ${PREFIX}/bin/wp_crop
	@cp -f wp_minsize ${PREFIX}/bin
	@chmod 755 ${PREFIX}/bin/wp_minsize

uninstall:
	@echo removing executable file from ${PREFIX}/bin
	@rm -f ${PREFIX}/bin/wp_crop
	@rm -f ${PREFIX}/bin/wp_minsize
