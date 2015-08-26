# Declare the default build goal.
.PHONY: default
default: legacy

# Rule to link the "legacy" executable.
legacy: main.o
	$(CC) $(LDFLAGS) -o $@ $^

# Rule to build main.o.
main.o: main.c
	$(CC) $(CFLAGS) -c $<

# Rule for cleaning up build artifacts.
.PHONY: clean
clean:
	rm -f *.o legacy

