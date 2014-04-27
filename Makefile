GCC= gcc
FLAGS= -Wall
LIBS= -lncurses
apps = connectFour
targets = $(addprefix bin/,$(apps))

.PHONY: clean all $(apps)

all: $(apps)

$(apps): $(targets) 

bin/%: obj%.o | bin
	$(GCC) $(FLAGS) -o $@ $< $(LIBS)

obj/%.o: src/%.c | obj
	$(GCC) $(FLAGS) -MMD -c -o $@ $< $(LIBS)

clean:
	rm -r obj bin

-include obj/*.d

print-%:
    @echo '$*=$($*)'

bin:
	mkdir -p bin

obj:
	mkdir -p obj