GCC = gcc
FLAGS = -Wall
LIBS = -lncurses

################ IMPORTANT #####################################################

# Sequentially list all target binaries
apps = connectFour

# Specify the names of the binaries as such
connectFour: bin/connectFour

# Copy this rule, adding any obj/*.o object files as needed before the |
bin/connectFour: obj/%.o | bin
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