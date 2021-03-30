// mainParsley.c                                  Stan Eisenstat (09/20/20)
//
// Prompts for commands, expands environment variables, parses them into
// command structures, and dumps the command structures to stdout.
//
// Bash version based on expression tree

#include "parsley.h"

int main()
{
    int nCmd = 1;                   // Command number
    CMD *cmd;                       // Parsed command

    char *line = NULL;                          // Space for line read
    size_t nLine = 0;                           // #chars allocated
    for ( ; ; ) {
        printf ("(%d)$ ", nCmd);                // Prompt for command
        fflush (stdout);

        if (getline (&line,&nLine, stdin) <= 0) // Read line
            break;                              //   Break on end of file

        if ((cmd = parse (line)) != NULL) {     // Parsed command?
            dumpTree (cmd, 0);                  //   Dump CMD as tree to stdout
            cmd = freeCMD (cmd);                //   Free associated storage
            nCmd++;                             // Adjust prompt
        }
    }

    printf ("\n");                              // Add final newline
    free (line);
    return EXIT_SUCCESS;
}


// Allocate, initialize, and return a pointer to a command structure of type
// TYPE with left child LEFT and right child RIGHT
CMD *mallocCMD (int type, CMD *left, CMD *right)
{
    CMD *new = malloc(sizeof(*new));

    new->type     = type;
    new->argc     = 0;
    new->argv     = malloc (sizeof(char *));
    new->argv[0]  = NULL;
    new->nLocal   = 0;
    new->locVar   = NULL;
    new->locVal   = NULL;
    new->fromType = NONE;
    new->fromFile = NULL;
    new->toType   = NONE;
    new->toFile   = NULL;
    new->errType  = NONE;
    new->errFile  = NULL;
    new->left     = left;
    new->right    = right;

    return new;
}


// Free tree of commands rooted at *C and return NULL
CMD *freeCMD (CMD *c)
{
    if (!c)
        return NULL;

    for (int i = 0; i < c->nLocal; i++) {
        free (c->locVar[i]);
        free (c->locVal[i]);
    }
    free (c->locVar);
    free (c->locVal);

    for (char **p = c->argv;  *p;  p++)
        free (*p);
    free (c->argv);

    free (c->fromFile);
    free (c->toFile);
    free (c->errFile);

    c->left = freeCMD (c->left);
    c->right = freeCMD (c->right);

    free (c);
    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// Dump CMD structure in tree format

// Print arguments in command data structure rooted at *C
void dumpArgs (CMD *c)
{
    if (c->argc < 0)
        fprintf (stdout, "  ARGC < 0");
    else if (c->argv == NULL)
        fprintf (stdout, "  ARGV = NULL");
    else if (c->argv[c->argc] != NULL)
        fprintf (stdout, "  ARGV[ARGC] != NULL");
    else {
////    fprintf (stdout, ",  argc = %d", c->argc);
        for (char **q = c->argv;  *q;  q++)
            fprintf (stdout, ",  argv[%ld] = %s", q-(c->argv), *q);
    }
}


// Print input/output redirections and local variables in command data
// structure rooted at *C
void dumpRedirect (CMD *c)
{
    if (c->fromType == NONE && c->fromFile == NULL)
        ;
    else if (c->fromType == RED_IN && c->fromFile != NULL)
        fprintf (stdout, "  <%s", c->fromFile);
    else if (c->fromType == RED_IN_HERE && c->fromFile != NULL)
        fprintf (stdout, "  <<HERE");
    else
        fprintf (stdout, "  ILLEGAL INPUT REDIRECTION");

    if (c->toType == NONE && c->toFile == NULL)
        ;
    else if (c->toType == RED_OUT && c->toFile != NULL)
        fprintf (stdout, "  >%s", c->toFile);
    else if (c->toType == RED_OUT_APP && c->toFile != NULL)
        fprintf (stdout, "  >>%s", c->toFile);
    else if (c->toType == RED_OUT_ERR && c->toFile != NULL)
        fprintf (stdout, "  &>%s", c->toFile);
    else
        fprintf (stdout, "  ILLEGAL OUTPUT REDIRECTION");

    if (c->errType == NONE && c->errFile == NULL)
        ;
    else if (c->errType == RED_ERR && c->errFile != NULL)
        fprintf (stdout, "  2>%s", c->errFile);
    else if (c->errType == RED_ERR_APP && c->errFile != NULL)
        fprintf (stdout, "  2>>%s", c->errFile);
    else if (c->errType == RED_OUT_ERR && c->errFile == NULL)
        fprintf (stdout, "  &>%s", c->toFile);
    else
        fprintf (stdout, "  ILLEGAL ERROR REDIRECTION");

    if (c->nLocal < 0) {
        fprintf (stdout, "  INVALID NLOCAL");
    } else if (c->nLocal == 0) {
        ;
    } else if (c->locVar == NULL || c->locVal == NULL) {
        fprintf (stdout, "  INVALID LOCVAL or LOCVAR");
    } else {
        fprintf (stdout, "\n         LOCAL: ");
        for (int i = 0; i < c->nLocal; i++)
            if (strchr (c->locVal[i], '='))
                fprintf (stdout, "%s = %s, ", c->locVar[i], c->locVal[i]);
            else
                fprintf (stdout, "%s=%s, ", c->locVar[i], c->locVal[i]);
    }

    if (c->fromType == RED_IN_HERE) {
        if (c->fromFile == NULL) {
            fprintf (stdout, "  INVALID FROMFILE FOR RED_IN_HERE");
        } else {
            fprintf (stdout, "\n         HERE:  ");
            for (char *s = c->fromFile; *s; s++) {
                if (*s != '\n')
                    fputc (*s, stdout);
                else if (s[1])
                    fprintf (stdout, "\n         HERE:  ");
                else
                    fprintf (stdout, "<newline>");
            }
        }
    }
}


// Print in in-order command data structure rooted at *C at depth LEVEL
void dumpTree (CMD *c, int level)
{
    if (!c)
        return;

    dumpTree (c->left, level+1);

////fprintf (stdout, "CMD (Level = %d):  ", level);
    fprintf (stdout, "CMD (Depth = %d):  ", level);

    if (c->type == SIMPLE) {
        if (c->left != NULL)
            fprintf (stdout, "  SIMPLE HAS LEFT CHILD");
        else if (c->right != NULL)
            fprintf (stdout, "  SIMPLE HAS RIGHT CHILD");
        else {
            fprintf (stdout, "SIMPLE");
            dumpArgs (c);
            dumpRedirect (c);
        }

    } else if (c->argc > 0) {
        fprintf (stdout, "  NON-SIMPLE HAS ARGUMENTS");

    } else if (c->type == SUBCMD) {
        if (c->right != NULL)
            fprintf (stdout, "  SUBCMD HAS RIGHT CHILD");
        else {
            fprintf (stdout, "SUBCMD");
            dumpRedirect (c);
        }

    } else if (c->fromType != NONE
            || c->fromFile != NULL
            || c->toType != NONE
            || c->toFile != NULL
            || c->errType != NONE
            || c->errFile != NULL) {
        fprintf (stdout, "  NON-SIMPLE, NON-SUBCMD HAS I/O REDIRECTION");

    } else if (c->nLocal > 0) {
        fprintf (stdout, "  NON-SIMPLE, NON-SUBCMD HAS LOCAL VARIABLES");

    } else if (c->type == PIPE) {
        fprintf (stdout, "PIPE");

    } else if (c->type == SEP_AND) {
        fprintf (stdout, "SEP_AND");

    } else if (c->type == SEP_OR) {
        fprintf (stdout, "SEP_OR");

    } else if (c->type == SEP_END) {
        fprintf (stdout, "SEP_END");

    } else if (c->type == SEP_BG) {
        fprintf (stdout, "SEP_BG");

    } else {
        fprintf (stdout, "NODE HAS INVALID TYPE");
    }

    fprintf (stdout, "\n");

    dumpTree (c->right, level+1);
}
