#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>


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


int getLineComponents(char *line, struct line_struct *lineStruct) {
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
		return 1;
	}

	*line = '\0';
	line++;

	while (1) {
		if  (*line != ' ' && *line != '\t') {
			break;
		}
		line++;
	}


	// TODO
	if (*line == '\0') {
		printAndExit("Data field can't be the empty string");
	}

	dataStart = line;

	lineStruct->fileName = fileNameStart;
	lineStruct->data = dataStart;

	return 0;
}

int parseEscapeSquence(char **startOfSequence, char **currEscapedFileName) {
	char *currSeq = *startOfSequence;
	char *currEscaped = *currEscapedFileName;
	int done = 0;
	char digit[2];
	digit[1] = '\0';

	if (*currSeq != '\\') {
		printf("Not escape sequence\n");
		return 1;
	}

	
	currSeq++;

	switch (*currSeq) {
		case 'n':
			*currEscaped = '\n';
			done = 1;
			break;

		case 't':
			*currEscaped = '\t';
			done = 1;
			break;

		case 'r':
			*currEscaped = '\r';
			done = 1;
			break;

		case ' ':
			*currEscaped = ' ';
			done = 1;
			break;

		case '\\':
			*currEscaped = '\\';
			done = 1;
			break;

		case '"':
			*currEscaped = '"';
			done = 1;
			break;

		default:
			if (isdigit(*currSeq)) {
				done = 0;
				break;
			}

			*currEscaped = *currSeq;
                        done = 1;
                        break;
	}

        if (done) {
                *startOfSequence = currSeq;
                return 0;
        }


	//Octal
	if (isdigit(*currSeq)) {
		int numDigits = 1;
		int num = 0;

		while (isdigit(*currSeq)) {
			printf("Analyzinng (%c)\n", *currSeq);


			digit[0] = *currSeq;
			int digitVal = atoi(digit);

			if (digitVal > 7) {
				break;
			}

			num = num * 8 + digitVal;

			if (numDigits >= 3) {
				break;
			}

			numDigits++;

			currSeq++;
		}

		if (num == 0) {
			printf("Error: can't have NUL char\n");
			return 1;
		}

		*currEscaped = (char) num;
	}

	*startOfSequence = currSeq;

	return 0;
}

int parseFileName(char *fileName, char **escapedFileName) {
	int fileNameLen = strlen(fileName);
	char *curr = fileName;
	char *currEscaped;
	char c, prev = 0;

	*escapedFileName = safeMalloc(fileNameLen + 1);
	currEscaped = *escapedFileName;


	// TODO realloc escapedFileName if necessary
	while (1) {
		printf("c (%c)\n", *curr);
		c = *curr;

		if (c == '\0') {
			*currEscaped = '\0';
			break;
		}

		if (isalnum(c)) {
			*currEscaped = *curr;
			prev = *curr;

			printf("currEscaped (%c)\n", *(currEscaped - 1));

			curr++;
			currEscaped++;

			continue;
		}

		switch (c) {
			case '\\':
				parseEscapeSquence(&curr, &currEscaped);
				break;
			default:
				*currEscaped = *curr;
				break;

		}


		prev = *curr;
		curr++;
		currEscaped++;
	}

	return 0;
}

void parseLine(char *line) {
	int error;
	struct line_struct lineStruct;
	lineStruct.fileName = NULL;
	lineStruct.data = NULL;
	char *escapedFileName;
	char *command;

	printf("line: (%s)\n", line);

	error = getLineComponents(line, &lineStruct);

	if (error) {
		printf("Error returned");
		return;
	}

	printf("Unescaped filename: (%s)\n", lineStruct.fileName);
	printf("Unescaped Data: (%s)\n", lineStruct.data);

	error = parseFileName(lineStruct.fileName, &escapedFileName);

	printf("Escaped filename: (%s)\n", escapedFileName);


	command = safeMalloc(10 + strlen(escapedFileName));

	sprintf(command, "echo \"%s\"", escapedFileName);

	printf("Command: %s\n", command);

	system(command);

	printf("\n---------------------------------------------\n\n");

	free(escapedFileName);
}

int main(int argc, char **argv) {

	while (!endOfInput) {
		char *line = getLine();

		if (endOfInput && *line == '\0') {
			free(line);
			break;
		}

		parseLine(line);

		free(line);

		if (endOfInput) {
			break;
		}
	}

	return 0;
}
