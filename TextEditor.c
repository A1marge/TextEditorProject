//** Includes **//
#include <stdio.h>     // Standard input-output functions (printf, scanf, etc.)
#include <unistd.h>    // Provides access to system calls like `read()`
#include <termios.h>   // Contains terminal control settings (tcgetattr, tcsetattr)
#include <stdlib.h>    // Used for `atexit()`
#include <ctype.h>     // Used for identifying control characters
#include <errno.h>     // Used for error codes like EAGAIN
#include <sys/ioctl.h> // Used for terminal window size manipulation

//** Defines **//

#define CTRL_KEY(k) ((k) & 0x1f) // Perfoms bitwise AND with value 00011111

//** Data **//

struct editorConfig
{
    int screenRows;
    int screenCols;
    struct termios orig_termios; // Global variable to store the original terminal settings
};

struct editorConfig E;

//** Function Declarations **//

void enableRawMode();                    // Turns on raw mode
void disableRawMode();                   // Restores normal mode when exiting
void die(const char *s);                 // Prints error messages and exits program
char editorReadKey();                    // Wait for a keypress then return it
void editorProcessKeypress();            // Waits for a keypress then processes it
void editorRefreshScreen();              // Erase the screen
void editorDrawRows();                   // Draws column of ~ on left hand side
int getWindowSize(int *rows, int *cols); // Gets the size of the terminal window

//** Main Function *//

void initEditor()
{
    if (getWindowSize(&E.screenRows, &E.screenCols) == -1)
        die("getWindowSize");
}

int main()
{
    enableRawMode();
    initEditor();

    while (1)
    {
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}

//** Terminal Settings **//

void enableRawMode()
{
    // Check for failure in setting the attributes for terminal
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
        die("tcsetattr");

    atexit(disableRawMode);

    struct termios raw = E.orig_termios; // Create a copy of the original settings to modify

    raw.c_iflag &= ~(ICRNL | IXON | BRKINT | ISTRIP | INPCK); // Modify the flag to turn of Ctrl-S, Ctrl-Q, and carrige return
    raw.c_oflag &= ~(OPOST);                                  // Modify the flag to turn off post-processing of output
    raw.c_cflag |= (CS8);                                     // Turning off the bit mask CS8
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);          // Modify the local mode flags to disable echo (so characters won't be displayed when typed)

    raw.c_cc[VMIN] = 0;  // Min number of bytes of input needed before read() can return set to 0
    raw.c_cc[VTIME] = 1; // Max amount of time to wait before read() returns set to 1/10 of a second

    // Apply the new terminal settings
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

void disableRawMode()
{
    // Restore the terminal to its original settings
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

void die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

char editorReadKey()
{
    int nread; // Stores number of bytes read from input
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if (nread == -1 && errno != EAGAIN)
            die("read");
    }
    return c;
}

int getWindowSize(int *rows, int *cols)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        return -1;
    }
    else
    {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

//** Output **//

void editorRefreshScreen()
{
    write(STDOUT_FILENO, "\x1b[2J", 4); // x1b = <esc> | 2J = erase entire screen
    write(STDOUT_FILENO, "\x1b[H", 3);  // Resets the cursor position

    editorDrawRows();

    write(STDOUT_FILENO, "\x1b[H", 3);
}

void editorDrawRows()
{
    int y;
    for (y = 0; y < E.screenRows; y++)
    {
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

//** Input **//

void editorProcessKeypress()
{
    char c = editorReadKey();

    switch (c)
    {
    // Exit on Ctrl + q
    case CTRL_KEY('q'):
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDIN_FILENO, "\x1b[H", 3);
        exit(0);
        break;
    }
}