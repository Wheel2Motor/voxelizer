CC:=gcc -std=c99
AR:=ar
CFLAG:=-O3 -I. -DNDEBUG -fPIC

ifeq ($(OS),Windows_NT)
	SEP:=\\
	DELETE:=del
	DYNAMIC:=voxelizer.dll
	STATIC:=voxelizer.lib
	EXAMPLE:=example.exe
else
	SEP:=/
	DELETE:=rm
	DYNAMIC:=libvoxelizer.so
	STATIC:=libvoxelizer.a
	EXAMPLE:=example
endif


all: $(DYNAMIC) $(STATIC) $(EXAMPLE)


voxelizer.o: voxelizer.c
	$(CC) -c $^ $(CFLAG)


$(DYNAMIC): voxelizer.o
	$(CC) -shared -o $@ $^


$(STATIC): voxelizer.o
	$(AR) crv $@ $^


$(EXAMPLE): example/example.c voxelizer.c
	$(CC) -o $@ $^ $(CFLAG)


.PHONY: run clean
.INTERMEDIATE: voxelizer.o


run: $(EXAMPLE)
	.$(SEP)$(EXAMPLE)

clean:
	@$(DELETE) $(EXAMPLE)
	@$(DELETE) $(DYNAMIC)
	@$(DELETE) $(STATIC)
