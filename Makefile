CC := gcc -std=c99


all: voxelizer.dll voxelizer.exe


voxelizer.o: voxelizer.c
	$(CC) -fPIC -c $^ -O3 -DNDEBUG


voxelizer.dll: voxelizer.o
	$(CC) -shared -o $@ $^


voxelizer.exe: example/example.c voxelizer.c
	$(CC) -o $@ $^ -O3 -DNDEBUG -I.


.PHONY: run clean


run: voxelizer.exe
	voxelizer.exe

clean:
	del voxelizer.exe
	del voxelizer.dll
	del *.o
