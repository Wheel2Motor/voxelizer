CC:=gcc -std=c99
AR:=ar
CFLAG:=-O3 -I. -fPIC -DNDEBUG -DVL_HIGHP

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
	.$(SEP)$(EXAMPLE)
	@echo ---- Python DLL ----
	python .$(SEP)example$(SEP)example.py

clean:
	@$(DELETE) $(EXAMPLE)
	@$(DELETE) $(DYNAMIC)
	@$(DELETE) $(STATIC)
