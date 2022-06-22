# Example
# target: dependencies
	# command 1
	# command 2
          # .
          # .
          # .
	# command n

# link objects(binaries) together
a.out:	main.o
	nvc	main.o \
			-lm -acc -Minfo=accel -gpu=cc86 -o srad

# compile main function file into object (binary)
main.o: 	main.c \
				define.c \
				graphics.c
	nvc	main.c \
			-c -acc -Minfo=accel -gpu=cc86 -fast

# delete all object files
clean:
	rm *.o srad
