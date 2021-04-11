# ...
NAME = hyl
DFLAGS = -g -O0 -DDEBUG_H -fsanitize=address -fsanitize-undefined-trap-on-error 
CFLAGS = -Wall -Werror -Wno-unused -Wno-format-security -fPIC -std=c99 -Ivendor
SFLAGS = -Wall -Werror -Wno-unused -Wno-format-security -std=c99 -Ivendor
CC = clang
SRC = \
	vendor/zhttp.c \
	vendor/zmime.c \
	vendor/zwalker.c \
	vendor/zdb.c \
	vendor/ztable.c \
	vendor/zrender.c \
	vendor/router.c \
	vendor/megadeth.c \
	src/lua.c \
	src/lib.c \
	src/db.c \
	src/echo.c
OBJ = $(SRC:.c=.o)
TARGET=


# lib - build shared object for use with hypno
lib: $(OBJ)
	-@test ! -d lib && mkdir lib/
	$(CC) $(SFLAGS) -shared -llua -lsqlite3 -o lib/lib$(NAME).so $(OBJ)


# debug - turn debugging on
debug: CFLAGS += $(DFLAGS)
debug: SFLAGS += $(DFLAGS)
debug: $(TARGET) 
	@printf '' > /dev/null


# clean - remove all objects
clean:
	-find -type f -name "*.o" | xargs rm
	-rm lib/lib$(NAME).so

.PHONY: lib