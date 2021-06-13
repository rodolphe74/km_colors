# The name of your C compiler:
CC= gcc

# You may need to adjust these cc options:
CFLAGS= -O3 -I.	-I.\SDL2_gfx-1.0.4\mingw64\include \
				-I.\SDL2-2.0.14\x86_64-w64-mingw32\include \
				-I.\SDL2-2.0.14\x86_64-w64-mingw32\include\SDL2 -std=c99 \
				-I.\jpeg-6b \
				-I.\Containers\src\include


# Link-time cc options:
LDFLAGS= 	-L.\SDL2-2.0.14\x86_64-w64-mingw32\lib -lmingw32 -lSDL2main -lSDL2 \
			-L.\SDL2_gfx-1.0.4\mingw64\lib -lSDL2_gfx \
			-L.\jpeg-6b -ljpeg \
			-L.\Containers -lcontainers

# To link any special libraries, add the necessary -l commands here.
LDLIBS= 

# miscellaneous OS-dependent stuff
# linker
LN= $(CC)
# file deletion command
RM= rm -f
# library (.a) file creation command
AR= ar rc
# second step in .a creation (use "touch" if not needed)
AR2= ranlib

all: km_colors

km_colors: log.o mathc.o 3d.o jpeg.o pixel.o kmean.o ./jpeg-6b/libjpeg.a km_colors.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)


log.o: log.c
	$(CC) $(CFLAGS) -c -o $@ $<

3d.o: 3d.c
	$(CC) $(CFLAGS) -c -o $@ $<

mathc.o: mathc.c
	$(CC) $(CFLAGS) -c -o $@ $<

jpeg.o: jpeg.c
	$(CC) $(CFLAGS) -c $< -o $@

pixel.o: pixel.c
	$(CC) $(CFLAGS) -c $< -o $@

kmean.o: kmean.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.a km_colors.exe

indent:
	uncrustify --replace -c /usr/share/doc/uncrustify/examples/linux.cfg *.c  *.h
	rm *unc-backup*

indent_clean:
	rm *unc-backup*
