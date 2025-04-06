CC = gcc
TARGET = server
SRC = src/server.c src/header_parser.c src/html_variables.c
TESTS = 9

$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	valgrind -s --leak-check=full --track-origins=yes ./$(TARGET) $(TESTS)

clear: $(TARGET)
	rm -f $(TARGET)
