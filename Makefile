.PHONY: all compile run clean

EXE = runfile

all:
	if [ -f ${EXE} ]; \
	then \
		rm ${EXE}; \
	fi;
	cc -o ${EXE} main.c threads.c ncurses.c waveform.c miscfunc.c -lm -lncurses
	./${EXE}

compile: 
	cc -o ${EXE} main.c threads.c ncurses.c waveform.c miscfunc.c -lm -lncurses

run:
	./${EXE}

load:
	./${EXE} -l

daq:
	./${EXE} -d

clean:
	rm ${EXE}
	# if [ -f ${TEST} ]; \
	# then \
	# 	rm ${TEST}; \
	# fi;
