all:
	gcc -c -o DashGL/dashgl.o DashGL/dashgl.c -lGL -lepxoy -lpng -lm
	gcc `pkg-config --cflags gtk+-3.0` main.c DashGL/dashgl.o `pkg-config --libs gtk+-3.0` -lepoxy -lGL -lm -lpng
