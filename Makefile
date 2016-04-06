all:
	gcc -o notepad notepad.c `pkg-config --libs --cflags gtk+-2.0 gtksourceview-2.0`
clean:
	rm -rf notepad
