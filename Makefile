GCC = gcc
FLAGS = -Wall
LIBS = -lncurses

################ IMPORTANT #####################################################

# Sequentially list all target binaries
apps = connectFour connectFour_server speedtyper speedtyper_server

# Specify the names of the binaries as such
connectFour: bin/connectFour

connectFour_server: bin/connectFour_server

speedtyper: bin/speedtyper

speedtyper_server: bin/speedtyper_server

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
