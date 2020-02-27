cc = gcc

OBJS = ImageProcess.o\
       RAW2RGB24.o\
       Gamma.o\
	   BMPOperation.o


main: $(OBJS) 
%.o : %.c
    $(cc) -c $^ -o $@
	mkdir tmp
	mv $(OBJS) ./tmp
	mv 
	
.PHONY : clean
clean:
	rm tmp/*

