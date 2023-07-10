CC:=gcc -std=c99
AR:=ar
CFLAG:=-O3 -I. -fPIC -DNDEBUG -DVL_HIGHP
ifeq ($(OS), Windows_NT)
	SEP:=\\
	DYNAMIC:=.$(SEP)voxelizer.dll
	STATIC:=.$(SEP)voxelizer.lib
	EXAMPLE:=.$(SEP)example.exe
	DEL:=cmd /c del
else
	SEP:=/
	DYNAMIC:=.$(SEP)libvoxelizer.so
	STATIC:=.$(SEP)libvoxelizer.a
	EXAMPLE:=.$(SEP)example.out
	DEL:=rm
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


run: $(EXAMPLE) $(DYNAMIC)
	@echo "---- Binary ----"
	$(EXAMPLE)
	@echo "---- Python ----"
	python .$(SEP)example$(SEP)example.py


clean:
	$(DEL) $(EXAMPLE)
	$(DEL) $(DYNAMIC)
	$(DEL) $(STATIC)


.PHONY: run clean
.INTERMEDIATE: voxelizer.o
