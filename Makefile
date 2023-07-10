CC:=gcc -std=c99
AR:=ar
CFLAG:=-O3 -I. -fPIC -DNDEBUG -DVL_HIGHP
ifeq ($(OS),Windows_NT)
	DYNAMIC:=voxelizer.dll
	STATIC:=voxelizer.lib
	EXAMPLE:=example.exe
else
	DYNAMIC:=libvoxelizer.so
	STATIC:=libvoxelizer.a
	EXAMPLE:=example.out
endif


all: $(DYNAMIC) $(STATIC) $(EXAMPLE)


voxelizer.o: voxelizer.c
	$(CC) -c $^ $(CFLAG)


$(DYNAMIC): voxelizer.o
	$(CC) -shared -o $@ $^


$(STATIC): voxelizer.o
	$(AR) crv $@ $^


$(EXAMPLE): example/example.c voxelizer.c
	$(CC) -o $@ $^ $(CFLAG) -DVL_TEST


.PHONY: run clean
.INTERMEDIATE: voxelizer.o


run: $(EXAMPLE) $(DYNAMIC)
	@echo ---- Binary ----
	./$(EXAMPLE)
	@echo ---- Python DLL ----
	python ./example/example.py

clean:
	ifeq ($(OS),Windows_NT)
		CMD_DEL = DEL
	else
		CMD_DEL = rm
	endif
	@$(CMD_DEL) $(EXAMPLE)
	@$(CMD_DEL) $(DYNAMIC)
	@$(CMD_DEL) $(STATIC)
