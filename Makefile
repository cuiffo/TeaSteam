GCC = gcc
FLAGS = -Wall
LIBS = -lncurses

################ IMPORTANT #####################################################

# Sequentially list all target binaries
apps = connectFour connectFour_server speedTyper speedTyper_server teasteam tsserv

# Specify the names of the binaries as such
all: $(apps)

connectFour: bin/connectFour

connectFour_server: bin/connectFour_server

speedTyper: bin/speedTyper

speedTyper_server: bin/speedTyper_server

teasteam: bin/teasteam

tsserv: bin/tsserv

# Copy this rule, adding any obj/*.o object files as needed before the |
bin/%: obj/%.o | bin
	$(GCC) $(FLAGS) -o $@ $^ $(LIBS)

############ Don't worry too much about the stuff below here ###################

targets = $(addprefix bin/,$(apps))

.PHONY: clean all $(apps)

all: $(apps)

obj/%.o: src/%.c | obj
	$(GCC) $(FLAGS) -MMD -c -o $@ $< $(LIBS)

clean:
	rm -r obj $(targets)

-include obj/*.d

print-%:
    @echo '$*=$($*)'

bin:
	mkdir -p bin

obj:
	mkdir -p obj
