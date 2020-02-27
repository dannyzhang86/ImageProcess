cc = gcc

OBJS = ImageProcess.o\
       RAW2RGB24.o\
       Gamma.o\
	   BMPOperation.o

ImageProcess : $(OBJS)
	cc -o ImageProcess $(OBJS)
	mkdir out
	mv $(OBJS) ./out
	mv ImageProcess ./out

ImageProcess.o : ImageProcess.h RAW2RGB24.h	Gamma.h BMPOperation.h
RAW2RGB24.o : RAW2RGB24.h  LUT10_8.h LUT12_8.h LUT14_8.h LUT16_8.h
Gamma.o : Gamma.h
BMPOperation.o : BMPOperation.h

.PHONY : clean
clean :
	rm -rf out


