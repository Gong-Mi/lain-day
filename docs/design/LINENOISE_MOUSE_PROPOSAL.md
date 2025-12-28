# Linenoise (Fork)

This directory contains a fork of the **Linenoise** library, a minimal, zero-config, BSD-licensed replacement for readline. It is designed to be small (less than 20k lines of code) and easy to embed in applications.

This specific version (located in `external/linenoise`) appears to be a vendored dependency for the `lain-day` project.

## Key Features

*   **Minimalistic:** Designed to be lightweight and easy to integrate.
*   **Cross-Platform:** Supports Linux/Unix and Win32 console.
*   **UTF-8 Support:** Full UTF-8 support (can be toggled).
*   **History:** Supports command history (add, save, load).
*   **Completion:** Supports tab completion with callbacks.
*   **Hints:** Supports hints (like autosuggestions).
*   **Multiline:** Supports multiline editing.
*   **Mouse Support:** (New) Supports SGR mouse protocol for touch/click interaction.

## Project Structure

*   **`linenoise.c` / `linenoise.h`**: The core source and header files.
*   **`utf8.c` / `utf8.h`**: implementation of UTF-8 handling.
*   **`stringbuf.c` / `stringbuf.h`**: Helper for string buffer management.
*   **`linenoise-win32.c`**: Windows-specific implementation details.
*   **`example.c`**: A sample application demonstrating how to use the library.
*   **`Makefile`**: Build script for the library and examples.
*   **`linenoise-ship.c`**: A generated file that concatenates sources for easier single-file distribution.

## Building and Running

To build the examples, you can use `make`:

```bash
# Build the standard example
make linenoise_example

# Build the example with UTF-8 support
make linenoise_utf8_example

# Clean build artifacts
make clean
```

Running the example:

```bash
./linenoise_example
# or
./linenoise_example --multiline
```

## Integration

To use this library in the main project:

1.  **Include Header:** `#include "linenoise.h"`
2.  **Compilation:** Compile `linenoise.c` (and `utf8.c`, `stringbuf.c` etc. as needed, or use `linenoise-ship.c`) along with your application source.
3.  **Basic Usage:**

```c
char *line;
while((line = linenoise("prompt> ")) != NULL) {
    if (line[0] != '\0') {
        printf("You typed: %s\n", line);
        linenoiseHistoryAdd(line);
        linenoiseHistorySave("history.txt");
    }
    free(line);
}
```

## API Overview

See `linenoise.h` for the full API. Key functions include:

*   `linenoise(const char *prompt)`: Read a line of input.
*   `linenoiseHistoryAdd(const char *line)`: Add a line to history.
*   `linenoiseHistorySave(const char *filename)`: Save history to disk.
*   `linenoiseHistoryLoad(const char *filename)`: Load history from disk.
*   `linenoiseSetCompletionCallback(...)`: Set a custom tab completion handler.
*   `linenoiseSetMultiLine(int enableml)`: Enable or disable multiline editing.

### Mouse Support API

*   `linenoiseSetMouseSupport(int enable)`:
    *   `0`: Disabled (default).
    *   `1`: Enabled (swallows events, does not interrupt input).
    *   `2`: Enabled (interrupts input, causing `linenoise` to return current buffer).
*   `linenoiseGetLastMouse(int *x, int *y, int *button, int *event_type)`: Retrieve details of the last mouse event.
    *   `x`, `y`: 1-based coordinates.
    *   `button`: Mouse button (0=left, etc.).
    *   `event_type`: 'M' (Press) or 'm' (Release).