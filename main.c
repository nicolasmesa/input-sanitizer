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

char *startDir = "/home/nm2805/";

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

/**
 * Performs a malloc and exits if it fails
 */
void *safeMalloc(size_t len) {
  void *new = malloc(len);

  if (new == NULL) {
    printAndExit(NULL);
  }

  return new;
}

/**
 * My own non-locale dependant version of isdigit
 */
int isDigit(char c) {
  if (c >= 48 && c <= 57) {
    return 1;
  }

  return 0;
}

/**
 * My own non locale dependant version of isalnum
 */
int isAlphaNum(char c) {
  if (isDigit(c)) {
    return 1;
  }

  if (c >= 65 && c <= 90) {
    return 1;
  }

  if (c >= 97 & c <= 122) {
    return 1;
  }

  return 0;
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

  // free(line);

  return returnLine;
}

/**
 * Returns the end of the component (component being a file path or data field)
 */
char *getInputComponent(char *line) {
  char quoteChar = 0;
  char prevChar = 0;
  char prevPrevChar = 0;
  int inQuotes = 0;
  int done = 0;
  int numEscapes = 0;

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
      done = 1;
      break;

    case '\t':
    case ' ':
      if (!inQuotes) {
        done = 1;
      }
      numEscapes = 0;
      break;

    case '\'': // Intentional fall through
    case '"':
      if (inQuotes && quoteChar == c && (numEscapes % 2 == 0)) {
        inQuotes = 0;
        line++;
        done = 1;
      }
      numEscapes = 0;
      break;

    case '\\':
      numEscapes++;
      break;

    default:
      numEscapes = 0;
      break;
    }

    if (done) {
      break;
    }

    prevPrevChar = prevChar;
    prevChar = *line;
    line++;
  }

  if (inQuotes) {
    return NULL;
  }

  return line;
}

/**
 * Sets the line components (file path and data field) to the
 * lineStruct. Returns 1 in case of failure, 0 otherwise
 */
int getLineComponents(char *line, struct line_struct *lineStruct) {
  char *fileNameStart = line;
  char *dataStart = NULL;

  line = getInputComponent(line);

  if (line == NULL) {
    return 1;
  }

  if (*line != ' ' && *line != '\t') {
    return 1;
  }

  *line = '\0';
  line++;

  while (1) {
    if (*line != ' ' && *line != '\t') {
      break;
    }
    line++;
  }

  // TODO
  if (*line == '\0') {
    return 1;
  }

  dataStart = line;

  line = getInputComponent(line);

  if (line == NULL) {
    return 1;
  }

  if (*line != '\0') {
    return 1;
  }

  lineStruct->fileName = fileNameStart;
  lineStruct->data = dataStart;

  return 0;
}

/**
 * Substitutes the escape sequence with the actual character
 */
int parseEscapeSquence(char **startOfSequence, char **currEscapedFileName,
                       char quote) {
  char *currSeq = *startOfSequence;
  char *currEscaped = *currEscapedFileName;
  int done = 0;
  char digit[2];
  digit[1] = '\0';

  if (*currSeq != '\\') {
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

  case '\\':
    *currEscaped = '\\';
    done = 1;
    break;

  case '"':
    if (quote != *currSeq) {
      return 1;
    }

    *currEscaped = '"';
    done = 1;
    break;

  case '\'':
    if (quote != *currSeq) {
      return 1;
    }

    *currEscaped = '\'';
    done = 1;
    break;

  default:
    if (isDigit(*currSeq)) {
      done = 0;
      break;
    }

    // Escaping something that should not be escaped
    return 1;
  }

  if (done) {
    *startOfSequence = currSeq;
    return 0;
  }

  // Octal
  if (isDigit(*currSeq)) {
    int numDigits = 0;
    int num = 0;

    while (isdigit(*currSeq)) {
      digit[0] = *currSeq;
      int digitVal = atoi(digit);

      if (digitVal > 7) {
        return 1;
      }

      num = num * 8 + digitVal;

      numDigits++;

      if (numDigits >= 3) {
        break;
      }

      currSeq++;
    }

    if (num == 0) {
      return 1;
    }

    if (num > 255) {
      return 1;
    }

    if (numDigits < 3) {
      return 1;
    }

    *currEscaped = (char)num;
  }

  *startOfSequence = currSeq;

  return 0;
}

/**
 * Validates that the passed char is in range
 * according to the spec
 */
int isValidCharRange(char c) {
  unsigned char uc = c;

  if (uc >= 1 && uc <= 255) {
    return 1;
  }

  return 0;
}

/**
 * Verifies that the character is a valid character
 * according to the specs
 */
int isInAllLettersRange(char c) {
  unsigned char uc = c;

  if (isAlphaNum(c)) {
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
  case 178: // Superscripts
  case 179:
  case 185:
    return 1;

  case 215: // Exceptions
  case 247:
    return 0;

  default:
    if (uc >= 192 && uc <= 255) {
      return 1;
    }
  }

  return 0;
}

/**
 * Parses a field performing the substitution of escape
 * characters. Returns 1 on error, 0 otherwise
 */
int parseField(char *fieldText, struct field_struct *fieldStruct) {
  int fieldLen = strlen(fieldText);
  char *curr = fieldText;
  char *currEscaped;
  char *escapedField;
  char c, prev = 0;
  int error = 0;

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

    if (fieldStruct->quoted) {
      if (c == '\\') {
        error = parseEscapeSquence(&curr, &currEscaped, fieldStruct->quote);

        if (error) {
          free(escapedField);
          return 1;
        }
      } else {
        *currEscaped = *curr;
      }
    } else {
      if (!isInAllLettersRange(c)) {
        free(escapedField);
        return 1;
      }

      *currEscaped = *curr;
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

/**
 * Pushes a component to the component stack, if there are any
 */
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

/**
 * Pops a component from the component stack
 */
void popCmp(struct cmp_list *cmpList) {
  if (cmpList->head == NULL) {
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

/**
 * Transforms the list into an absolute path;
 */
char *getAbsolutePathFromList(struct cmp_list *cmpList) {
  int memLen = 100;
  char *path = safeMalloc(memLen);
  struct file_node *window;
  int currLen = 0;

  *path = '/';
  *(path + 1) = '\0';
  currLen++;

  for (window = cmpList->tail; window != NULL; window = window->prev) {
    int cmpLen = strlen(window->fileName);

    if (currLen + cmpLen + 2 >= memLen) {
      memLen = 2 * (currLen + cmpLen + 2);
      path = realloc(path, memLen);

      if (path == NULL) {
        printAndExit(NULL);
      }
    }

    char *ret = strcat(path, window->fileName);

    if (ret == NULL) {
      printAndExit(NULL);
    }

    ret = strcat(path, "/");

    if (ret == NULL) {
      printAndExit(NULL);
    }

    currLen = strlen(path);
  }

  *(path + currLen - 1) = '\0';

  return path;
}

/**
 * Frees the cmp stack
 */
void freeCmpList(struct cmp_list *cmpList) {
  struct file_node *window;
  struct file_node *prev;

  for (window = cmpList->head; window != NULL;) {
    prev = window;
    window = window->next;
    free(prev);
  }

  cmpList->head = NULL;
  cmpList->tail = NULL;
}

/**
 * Verifies if the absolute path is in the CWD
 */
int absolutePathIsValidCwd(char *path) {
  char *dir = startDir;

  if (strncmp(path, dir, strlen(dir)) != 0) {
    return 0;
  }

  path += strlen(dir);

  while (*path != '\0') {
    if (*path == '/') {
      return 0;
    }
    path++;
  }

  return 1;
}

/**
 * Verifies that the absolute path is in /tmp/
 */
int absolutePathIsValidTmp(char *path) {
  char *dir = "/tmp/";

  if (strncmp(path, dir, strlen(dir)) != 0) {
    return 0;
  }

  path += strlen(dir);

  while (*path != '\0') {
    if (*path == '/') {
      return 0;
    }
    path++;
  }

  return 1;
}

/**
 * Makes sure that the path is in either /tmp/ or
 * the CWD
 */
int absolutePathIsValid(char *path) {
  if (absolutePathIsValidTmp(path)) {
    return 1;
  }

  if (absolutePathIsValidCwd(path)) {
    return 1;
  }

  return 0;
}

/**
 * Parses the path getting rid of ../, //, or ./ and
 * making it a single absolute path
 */
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
  } else {
    int size = strlen(startDir) + strlen(fileName) + 1;
    char *tempFileName = safeMalloc(size + 1);
    int ret = snprintf(tempFileName, size, "%s%s", startDir, fileName);

    if (ret > size) {
      free(tempFileName);
      return 1;
    }

    fileName = tempFileName;
    currFileName = fileName;
  }

  prev = '/';
  currFileName++;

  cmpStart = currFileName;

  while (1) {
    c = *currFileName;

    if (prev == '/') {
      cmpStart = currFileName;
    }

    int currCmpLen = currFileName - cmpStart;

    switch (c) {
    case '\0':
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

  if (!absolutePathIsValid(path)) {
    freeCmpList(&cmpList);

    if (!isAbsolute) {
      free(fileName);
    }

    return 1;
  }

  if (!isAbsolute) {
    free(fileName);
  }

  freeCmpList(&cmpList);

  *absolutePath = path;

  return 0;
}

/**
 * Escapes the dangerous characters of the shell
 */
int escapeShellChars(char *line, char **escapedLine) {
  *escapedLine = safeMalloc(strlen(line) * 2);
  char *currEscaped = *escapedLine;
  char c;

  while ((c = *line) != '\0') {
    switch (c) {
    // case '\'':
    // case '#':
    // case '!':
    // case '+':
    // case '-':
    // case '{':
    // case '}':
    // case '[':
    // case ']':
    // case '~':
    // case '&':
    // case '^':
    // case '|':
    // case ';':
    // case '>':
    // case '<':
    // case '(':
    // case ')':
    // case '*':
    // case '?':

    case '"':
    case '\\':
    case '`':
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

  *currEscaped = '\0';

  return 0;
}

/**
 * Parses the line and executes the command
 */
int parseLine(char *line) {
  int error;
  struct line_struct lineStruct;
  lineStruct.fileName = NULL;
  lineStruct.data = NULL;
  struct field_struct fileNameStruct;
  struct field_struct dataStruct;
  char *escapedFileName;
  char *escapedData;
  char *command;

  error = getLineComponents(line, &lineStruct);

  if (error) {
    return 1;
  }

  error = parseField(lineStruct.fileName, &fileNameStruct);

  if (error) {
    return 1;
  }

  error = parseField(lineStruct.data, &dataStruct);

  if (error) {
    free(fileNameStruct.field);
    return 1;
  }

  char *absolutePath;
  error = getAbsolutePath(fileNameStruct.field, &absolutePath);

  if (error) {
    free(fileNameStruct.field);
    free(dataStruct.field);
    return 1;
  }

  escapeShellChars(absolutePath, &escapedFileName);
  escapeShellChars(dataStruct.field, &escapedData);

  int escapedFileNameLen = strlen(escapedFileName);
  int escapedDataLen = strlen(escapedData);

  command = safeMalloc(50 + escapedFileNameLen + escapedDataLen);

  sprintf(command, "echo \"%s\" >> \"%s.nm2805\"", escapedData,
          escapedFileName);

  system(command);

  free(escapedFileName);
  free(escapedData);
  free(absolutePath);
  free(command);
  free(fileNameStruct.field);
  free(dataStruct.field);

  return 0;
}

/**
 * Main function. Reads lines and executes them
 */
int main(int argc, char **argv) {

  while (!endOfInput) {
    char *line = getLine();

    if (endOfInput && *line == '\0') {
      free(line);
      break;
    }

    printf("(%s):\t", line);

    int ret = parseLine(line);

    printf("%d\n", ret);

    free(line);

    if (endOfInput) {
      break;
    }
  }

  return 0;
}
