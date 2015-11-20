#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define INITIAL_LINE_SIZE 100

struct line_struct {
	char *fileName;
	char *data;
};

int endOfInput = 0;


/**
 * Prints an error message if it is passed. It NULL is
 * passed instead, the strerror for errno is printed.
 * After that, the program exits
 */
void printAndExit(char *msg) {
  if (msg == NULL) {
    msg = strerror(errno);
  }

  printf("Error: %s\n", msg);
  exit(1);
}


void *safeMalloc(size_t len) {
  void *new = malloc(len);

  if (new == NULL) {
    printAndExit(NULL);
  }

  return new;
}


/**
 * Gets a line from STDIN. The caller is responsible for
 * freeing the memory of the line. If it reaches the end
 * of input, the endOfInput global variable is set
 */
char *getLine() {
  int len = INITIAL_LINE_SIZE;
  int index = 0;
  char *line = malloc(len);
  char *returnLine;
  char c;

  if (line == NULL) {
    printAndExit(NULL);
  }

  while ((c = getchar()) != EOF && c != '\n') {
    line[index] = c;
    index++;

    if (index > len - 1) {
      len *= 2;
      line = realloc(line, len);
    }
  }

  if (c == EOF) {
    endOfInput = 1;
  }

  line[index] = '\0';

  returnLine = strndup(line, index);

  if (returnLine == NULL) {
    printAndExit(NULL);
  }

  free(line);

  return returnLine;
}


struct line_struct *getLineComponents(char *line) {
	int lineLen = strlen(line);
	char *fileNameStart = line;
	char *dataStart = NULL;
	char quoteChar = 0;
	char prevChar = 0;
	int inQuotes = 0;
	int done = 0;


	if (*line == '"' || *line == '\'') {
		quoteChar = *line;
		prevChar = *line;
		line++;
		inQuotes = 1;
	}


	while (1) {
		char c = *line;

		switch (c) {
			case '\0':
				// TODO display error
				printf("Error. Unexepected end of input\n");
				done = 1;
				break;

			case '\t':
			case ' ':
				if (!inQuotes && prevChar != '\\') {
					done = 1;
				}
				break;

			case '\'':
				if (inQuotes && quoteChar == c && prevChar != '\\') {
					inQuotes = 0;
					line++;
					done = 1;
				}
				break;

			case '"':
                                if (inQuotes && quoteChar == c && prevChar != '\\') {
                                        inQuotes = 0;
					line++;
                                        done = 1;
                                }
                                break;

			default:
				break;
		}

		if (done) {
			break;
		}

		prevChar = *line;
		line++;
	}


	// Should not happen
	if (inQuotes) {
		printAndExit("Unexpected end of file name. Still in quotes\n");
	}	

	if (*line != ' ' && *line != '\t') {
		printf("Invalid (%s)\n", line);
		return NULL;
	}

	*line = '\0';
	line++;

	while (1) {
		if  (*line != ' ' && *line != '\t') {
			break;
		}
		line++;
	}


	dataStart = line;

	printf("Filename: (%s)\n", fileNameStart);
	printf("Data: (%s)\n", dataStart); 

	return NULL;
}

void parseLine(char *line) {
	struct line_struct *lineStruct = getLineComponents(line);
}

int main(int argc, char **argv) {

	while (!endOfInput) {
		char *line = getLine();

		parseLine(line);

		free(line);
	}

	return 0;
}
