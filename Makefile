main:
	gcc main.c stack.c -o output.exe -Iinclude -Llib -lraylib -lopengl32 -lgdi32 -lwinmm

test:
	gcc test.c -o output.exe
