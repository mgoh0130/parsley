# parsley
***PLEASE EMAIL ME AT michelle.goh@yale.edu IF YOU WOULD LIKE TO SEE MY CODE FOR THIS***

Write a bash command-line parser "parsley" that prompts for and
reads lines from stdin, breaks the lines into tokens, parses the tokens into a
tree of commands, and writes the tree in in-order to stdout.  Here a token is
one of the following:

(1) a maximal contiguous sequence of non-whitespace printing characters (see
    "man 3 isgraph") other than the metacharacters <, >, ;, &, |, (, and )
    [a TEXT token];

(2) a redirection symbol [<, <<, >, >>, 2>, 2>>, or &>];

(3) a pipeline symbol [|];

(4) a command operator [&& or ||];

(5) a command terminator [; or &];

(6) a left or right parenthesis [used to group commands into subcommands];

Whitespace may be used to separate tokens from each other but is otherwise
ignored.  Nonprinting characters are treated as spaces.

Example:  The command line

    < A B | ( C 2> D & E < F ) > G ; H=I J K

  results in the following command tree

  .            ;
  .          /   \
  .      PIPE     H=I J K
  .     /    \
  .   B <A    SUB >G
  .          /
  .         &
  .        / \
  .   C 2>D   E <F

  which is printed as

    CMD (Depth = 2):  SIMPLE,  argv[0] = B  <A
    CMD (Depth = 1):  PIPE
    CMD (Depth = 4):  SIMPLE,  argv[0] = C  2>D
    CMD (Depth = 3):  SEP_BG
    CMD (Depth = 4):  SIMPLE,  argv[0] = E  <F
    CMD (Depth = 2):  SUBCMD  >G
    CMD (Depth = 0):  SEP_END
    CMD (Depth = 1):  SIMPLE,  argv[0] = J,  argv[1] = K
	     LOCAL: H=I,
