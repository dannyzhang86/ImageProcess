cc = gcc

OBJS = ImageProcess.o\
       RAW2RGB24.o\
       Gamma.o\
	   BMPOperation.o


main: $(OBJS) 
%.o : %.c
    $(cc) -c $^ -o $@

clean:
    -rm -rf $(OBJS)


.PHONY : clean
clean:
	rm tmp/*

