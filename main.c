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

struct field_struct {
	char *field;
	int quoted;
	char quote;
};

struct file_node {
	struct file_node *next;
	struct file_node *prev;
	char *fileName;
};

struct cmp_list {
	struct file_node *head;
	struct file_node *tail;
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

  //free(line);

  return returnLine;
}

/**
 * Returns the end of the component
 */
char *getInputComponent(char *line) {
	 int lineLen = strlen(line);
        char *start = line;
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
				if (inQuotes) {
                                	// TODO display error
                                	printf("Error. Unexepected end of input\n");
				}
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
		printf("Quotes error\n");
        	return NULL; 
        }

	return line;
}

int getLineComponents(char *line, struct line_struct *lineStruct) {
	int lineLen = strlen(line);
	char *fileNameStart = line;
	char *dataStart = NULL;
	char quoteChar = 0;
	char prevChar = 0;
	int inQuotes = 0;
	int done = 0;


	line = getInputComponent(line);

	if (line == NULL) {
		return 1;
	}


	if (*line != ' ' && *line != '\t') {
		printf("Invalid: Expected space of tab at (%s)\n", line);
		return 1;
	}

	*line = '\0';
	line++;

	printf("filename: (%s)\n", fileNameStart);

	while (1) {
		if  (*line != ' ' && *line != '\t') {
			break;
		}
		line++;
	}


	// TODO
	if (*line == '\0') {
		printf("Data field can't be the empty string\n");
		return 1;
	}

	dataStart = line;

	line = getInputComponent(line);

	if (line == NULL) {
		return 1;
	}

	if (*line != '\0') {
		printf("Unexpected (%s)\n", line);
		return 1;
	}

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


int isValidCharRange(char c) {
	unsigned char uc = c;

	if (uc >=1 && uc <= 255) {
		return 0;
	}

	return 1;
}


int isInAllLettersRange(char c) {
	unsigned char uc = c;


	if (isalnum(uc)) {
		printf("(%c) is alphanum\n", c);
		return 1;
	}

	switch (uc) {
		case 131: // Letters
		case 138:
		case 140:
		case 142:
		case 154:
		case 156:
		case 158:
		case 159:
		case 170:
		case 186: // letters
		case 178: //Superscripts 
		case 179:
		case 185:
			printf("(%c) is in range of switch\n", c);
			return 1;

		case 215: // Exceptions
		case 247:
			return 0;

		default:
			if (uc >= 192 && uc <= 255) {
				printf("(%c) is in range\n", c);
				return 1;
			}

	}

	printf("(%c) is not alphanum\n", c);
        return 0;
}

int parseField(char *fieldText, struct field_struct *fieldStruct) {
	int fieldLen = strlen(fieldText);
	char *curr = fieldText;
	char *currEscaped;
	char *escapedField;
	char c, prev = 0;

	escapedField = safeMalloc(fieldLen + 1);
	currEscaped = escapedField;

	fieldStruct->quoted = 0;
	fieldStruct->quote = 0;

	if (*curr == '"' || *curr == '\'') {
		fieldStruct->quoted = 1;
		fieldStruct->quote = *curr;
		curr++;
	}	

	while (1) {
		c = *curr;

		if (c == '\0') {
			*currEscaped = '\0';
			break;
		}


		if (!fieldStruct->quoted) {
			if (!isInAllLettersRange(c)) {
				free(escapedField);
                        	return 1;
			}
			
			*currEscaped = *curr;
                        prev = *curr;
                        curr++;
                        currEscaped++;

                        continue;
		}


		if (isalnum(c)) {
			*currEscaped = *curr;
			prev = *curr;
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


	if (fieldStruct->quoted) {
		currEscaped--;
		*currEscaped = '\0';
	}

	fieldStruct->field = escapedField;

	return 0;
}

void pushCmp(struct cmp_list *cmpList, char *cmpStart, int cmpLen) {
	struct file_node *newNode = safeMalloc(sizeof(struct file_node));
	char *cmp;

	cmp = strndup(cmpStart, cmpLen);

	newNode->fileName = cmp;
	newNode->next = cmpList->head;
	newNode->prev = NULL;

	if (cmpList->head == NULL) {
		cmpList->tail = newNode;
	} else {
		cmpList->head->prev = newNode;
	}

	cmpList->head = newNode;
}

void popCmp(struct cmp_list *cmpList) {
	if (cmpList->head == NULL) {
		printf("Going beyond /\n");
		return;
	}

	struct file_node *toDelete = cmpList->head;

	cmpList->head = toDelete->next;

	if (cmpList->head != NULL) {
		cmpList->head->prev = NULL;
	}

	free(toDelete->fileName);
	free(toDelete);
}

char *getAbsolutePathFromList(struct cmp_list *cmpList) {
	int memLen = 100;
	char *path = safeMalloc(memLen);
	struct file_node *window;
	int currLen = 0;

	for (window = cmpList->tail; window != NULL; window = window->prev) {
		int cmpLen = strlen(window->fileName);

		if (currLen + cmpLen + 2 >= memLen) {
			memLen = 2 * (currLen + cmpLen + 2);
			path = realloc(path, memLen);

			if (path == NULL) {
				printAndExit(NULL);
			}
		}

		

		char *ret = strcat(path, "/");

		if (ret == NULL) {
			printAndExit(NULL);
		}

		ret = strcat(path, window->fileName);

		if (ret == NULL) {
			printAndExit(NULL);
		}
	}


	return path;
}

int validateAbsolutePath(char *path) {
	char *dir = "/tmp/";

	if (strncmp(path, dir, strlen(dir)) != 0) {
		printf("Error: Not in /tmp or cwd (%s)\n", path);
		return 1;
	}

	path += strlen(dir);


	while (*path != '\0') {
		if (*path == '/') {
			printf("Error: Not in /tmp or cwd (%s)\n", path);
			return 1;
		}
		path++;
	}

	return 0;
}

int getAbsolutePath(char *fileName, char **absolutePath) {
	struct cmp_list cmpList;
	int isAbsolute = 0;
	char *currFileName = fileName;
	char c, prev = 0;
	int twoConsecutiveDots = 0;
	char *cmpStart = NULL;
	int done = 0;		

	cmpList.head = NULL;
	cmpList.tail = NULL;

	if (*fileName == '/') {
		isAbsolute = 1;
		prev = '/';
		currFileName++;
	}

	cmpStart = currFileName;

	while (1) {
		c = *currFileName;

		if (prev == '/') {
			cmpStart = currFileName;
		}

		int currCmpLen = currFileName - cmpStart;

		switch(c) {
			case '\0':
				// TODO free list
				if (prev == '/') {
					printf("Error. Can't end in slash\n");
					return 1;
				}

				if (currCmpLen == 0) {
					printf("Error. Can't have an empty file name\n");
					return 1;
				}
				
				pushCmp(&cmpList, cmpStart, currCmpLen);
				done = 1;
				break;
			case '/':
				if (prev == '.' && currCmpLen == 1) {
					twoConsecutiveDots = 0;
					break;
				}

				if (twoConsecutiveDots && currCmpLen == 2) {
					popCmp(&cmpList);
				} else if (prev && prev != '/') {
					pushCmp(&cmpList, cmpStart, currCmpLen);
				}
				twoConsecutiveDots = 0;
				break;
			case '.':
				if (prev == '.') {
					twoConsecutiveDots = 1;
				}
				break;
			default:
				twoConsecutiveDots = 0;
				break;
		}


		if (done) {
			break;
		}

		prev = c;
		currFileName++;
	}

	char *path = getAbsolutePathFromList(&cmpList);

	if (isAbsolute) {
		if (validateAbsolutePath(path)) {
			return 1;
		}
	} else {
		printf("Validate relative path\n");
	}

	*absolutePath = path;

	return 0;
}

int escapeShellChars(char *line, char **escapedLine) {
	*escapedLine = safeMalloc(strlen(line) * 2);
	char *currEscaped = *escapedLine;
	char c;

	while ((c = *line) != '\0') {
		switch (c) {
			case '\'':
			case '"':
			case '\\':
			case '|':
			case ';':
			case '>':
			case '<':
			case '(':
			case ')':
			case '*':
			case '?':
			case '`':
			case '#':
			case '!':
			case '+':
			case '-':
			case '{':
			case '}':
			case '[':
			case ']':
			case '~':
			case '&':
			case '^':
			case '$':
				*currEscaped = '\\';
				currEscaped++;
				*currEscaped = c;
				break;
			default:
				*currEscaped = c;
				break;

		}

		currEscaped++;
		line++;
	}

	printf("Escaeped string (%s)\n", *escapedLine);

	return 0;
}

void parseLine(char *line) {
	int error;
	struct line_struct lineStruct;
	lineStruct.fileName = NULL;
	lineStruct.data = NULL;
	struct field_struct fileNameStruct;
	struct field_struct dataStruct;
	char *escapedFileName;
	char *escapedData;
	char *command;
	char quote = 0;

	printf("line: (%s)\n", line);

	error = getLineComponents(line, &lineStruct);


	if (error) {
		printf("Error returned\n");
		return;
	}

	printf("Unescaped filename: (%s)\n", lineStruct.fileName);
	printf("Unescaped Data: (%s)\n", lineStruct.data);

	error = parseField(lineStruct.fileName, &fileNameStruct);

	if (error) {
		return;
	}

	escapedFileName = fileNameStruct.field;

	error = parseField(lineStruct.data, &dataStruct);


	if (error) {
		return;
	}


	char *absolutePath;
	error = getAbsolutePath(escapedFileName, &absolutePath);

	if (error) {
		return;
	}

	printf("Absolute path (%s)\n", absolutePath);

	escapeShellChars(absolutePath, &escapedFileName);
	escapeShellChars(dataStruct.field, &escapedData);

	int escapedFileNameLen = strlen(escapedFileName);
        int escapedDataLen = strlen(escapedData);

	
	command = safeMalloc(50 + escapedFileNameLen + escapedDataLen);

	sprintf(command, "echo \"%s\" >> \"%s.nm2805\"", escapedData, escapedFileName);


	printf("Command: %s\n", command);

	
	system(command);

	printf("\n---------------------------------------------\n\n");

	free(command);
	free(fileNameStruct.field);
	free(dataStruct.field);
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
