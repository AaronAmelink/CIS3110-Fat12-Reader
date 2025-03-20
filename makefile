CC = gcc
CFLAGS = -g

EXE_FAT12READER	= fat12reader
EXE_WRITEDATA	= writedata

OBJS_FAT12READER	= \
		main.o \
		commands.o \
		fat12fs.o

OBJS_WRITEDATA	= \
		writedata.o

all: $(EXE_FAT12READER) $(EXE_WRITEDATA)

$(EXE_FAT12READER) : $(OBJS_FAT12READER)
	$(CC) $(CFLAGS) -o $(EXE_FAT12READER) $(OBJS_FAT12READER)

$(EXE_WRITEDATA) : $(OBJS_WRITEDATA)
	$(CC) $(CFLAGS) -o $(EXE_WRITEDATA) $(OBJS_WRITEDATA)

clean :
	rm -f $(OBJS_FAT12READER)
	rm -f $(EXE_FAT12READER)
	rm -f $(OBJS_WRITEDATA)
	rm -f $(EXE_WRITEDATA)

tags : dummy
	ctags *.c 

dummy:
