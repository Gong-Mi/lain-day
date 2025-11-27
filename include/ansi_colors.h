#ifndef ANSI_COLORS_H
#define ANSI_COLORS_H

// ANSI Escape Codes for colors
#define ANSI_COLOR_RED      "\x1b[31m"
#define ANSI_COLOR_GREEN    "\x1b[32m"
#define ANSI_COLOR_YELLOW   "\x1b[33m"
#define ANSI_COLOR_BLUE     "\x1b[34m"
#define ANSI_COLOR_MAGENTA  "\x1b[35m"
#define ANSI_COLOR_CYAN     "\x1b[36m"
#define ANSI_COLOR_WHITE    "\x1b[37m"

// Bright Colors
#define ANSI_COLOR_BRIGHT_BLACK   "\x1b[90m" // Gray

#define ANSI_COLOR_RESET    "\x1b[0m" // Reset to default color

// Add more specific character colors as needed
#define ALICE_COLOR    ANSI_COLOR_CYAN
#define CHISA_COLOR    ANSI_COLOR_MAGENTA
#define FATHER_COLOR   ANSI_COLOR_BLUE
#define FUYUKO_MIRA_COLOR ANSI_COLOR_YELLOW // Doctor
#define LAINS_SISTER_MIRA_COLOR ANSI_COLOR_GREEN // Lain's Sister

#endif // ANSI_COLORS_H
