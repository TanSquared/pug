/* --------------------------------------------------------------------------
 * completion.c: Tab completion support for Pug interpreter
 * 
 * This module provides intelligent tab completion for:
 * - Commands (starting with :)
 * - Function and variable names
 * - Filenames (for :load and :edit commands)
 * ------------------------------------------------------------------------*/

#include "prelude.h"
#include "storage.h"
#include "connect.h"
#include "errors.h"
#include "input.h"

#if USE_READLINE

#include <readline/readline.h>
#include <readline/history.h>

/* --------------------------------------------------------------------------
 * Command completion data
 * ------------------------------------------------------------------------*/

static char *commands[] = {
    ":?", ":type", ":load", ":also", ":reload", ":project",
    ":edit", ":find", ":names", ":set", ":quit", ":cd",
    ":!", ":info", ":gc", NULL
};

/* --------------------------------------------------------------------------
 * Forward declarations
 * ------------------------------------------------------------------------*/

static char **pug_completion(const char *text, int start, int end);
static char *command_generator(const char *text, int state);
static char *name_generator(const char *text, int state);
static int is_command_context(int start);
static int is_filename_context(void);

/* --------------------------------------------------------------------------
 * Main completion function - called by readline
 * ------------------------------------------------------------------------*/

static char **pug_completion(const char *text, int start, int end) {
    char **matches = NULL;
    
    /* Don't do filename completion by default */
    rl_attempted_completion_over = 1;
    
    /* If we're at the start and text begins with ':', complete commands */
    if (start == 0 && text[0] == ':') {
        matches = rl_completion_matches(text, command_generator);
    }
    /* If we're in a filename context, allow default filename completion */
    else if (is_filename_context()) {
        rl_attempted_completion_over = 0;  /* Allow filename completion */
        return NULL;
    }
    /* Otherwise, complete function/variable names */
    else {
        matches = rl_completion_matches(text, name_generator);
    }
    
    return matches;
}

/* --------------------------------------------------------------------------
 * Command completion generator
 * ------------------------------------------------------------------------*/

static char *command_generator(const char *text, int state) {
    static int index, len;
    char *name;
    
    /* Initialize on first call */
    if (state == 0) {
        index = 0;
        len = strlen(text);
    }
    
    /* Find next matching command */
    while ((name = commands[index++])) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }
    
    return NULL;
}

/* --------------------------------------------------------------------------
 * Name completion generator - completes function and variable names
 * ------------------------------------------------------------------------*/

static char *name_generator(const char *text, int state) {
    static List nameList;
    static List currentName;
    char *nameStr;
    int len;

    /* Initialize on first call - get all names and filter them */
    if (state == 0) {
        /* Get all names in scope */
        nameList = addNamesMatching((String)0, NIL);  /* NULL pattern matches all */
        currentName = nameList;
    }

    len = strlen(text);

    /* Search through the list for names that match the prefix */
    while (nonNull(currentName)) {
        Name nm = hd(currentName);
        nameStr = textToStr(name(nm).text);
        currentName = tl(currentName);

        if (nameStr && strncmp(nameStr, text, len) == 0) {
            return strdup(nameStr);
        }
    }

    return NULL;
}

/* --------------------------------------------------------------------------
 * Context detection functions
 * ------------------------------------------------------------------------*/

static int is_command_context(int start) {
    return (start == 0);
}

static int is_filename_context(void) {
    /* Check if the current line starts with :load, :also, :edit, or :project */
    char *line = rl_line_buffer;
    
    /* Fullform commands */
    if (strncmp(line, ":load ", 6) == 0 ||
        strncmp(line, ":also ", 6) == 0 ||
        strncmp(line, ":edit ", 6) == 0 ||
        strncmp(line, ":project ", 9) == 0) {
        return 1;
    }
    
    /* Short form commands */
    if (strncmp(line, ":l ", 3) == 0 ||
        strncmp(line, ":a ", 3) == 0 ||
        strncmp(line, ":e ", 3) == 0 ||
        strncmp(line, ":p ", 3) == 0) {
        return 1;
    }

    return 0;
}

/* --------------------------------------------------------------------------
 * Initialization function - sets up readline completion
 * ------------------------------------------------------------------------*/

void setup_readline_completion(void) {
    /* Set our completion function */
    rl_attempted_completion_function = pug_completion;
    
    /* Set word break characters - defines what separates "words" */
    rl_basic_word_break_characters = " \t\n\"\\'`@$><=;|&{(.";
    
    /* Don't break on dots since Pug uses dot notation */
    rl_special_prefixes = ".";
}

#else

/* Dummy function when readline is not available */
void setup_readline_completion(void) {
    /* Do nothing if readline is not compiled in */
}

#endif /* USE_READLINE */
