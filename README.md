# Blue Shell (bluesh)

## Description

Bluesh is a custom shell written in C. It has a number of notable features including:

- History:
	- The shell keeps track of the last 100 commands executed by the user
- Command reexecution via `!` commands:
	- `!<NUMBER>` reexecutes command NUMBER if it exists
	- `!!` reexecutes the last command if it exists
	- `!<PREFIX>` reexecutes the most recent command starting with PREFIX
- Pipelines and redirection:
	- Redirection from / to a file can be accomplished with `<` and `>`
	- Pipelines can be created with `|`

Bluesh is NOT intended as a replacement for bash, zsh, fish, or any other full shell and is intended merely as a proof of concept / academic exercise

## Implementation

### History

History is tracked using an array based queue. As such, new commands overwrite the oldest command when the number of commands exceeds the capacity of the queue.

### Tokenization

User input is tokenized using the next_token implementation available from the class website. Input is split on spaces, each token is checked to see if it is a reexecution command and then replaced with the according historical command (if valid) before being added to the history. This allows for pipelines to be constructed using multiple historical commands. Unfortunately, the input tokenization as it currently stands struggles with strings in user input (i.e. `git commit -m "commit message")`, though I hope to address this in future versions.

### Pipelines and redirection

Pipelines are implemented using `fork` and `pipe`, almost exactly like in lab 8 (though using a linked list of commands instead of an array to allow an arbitrary (memory allowing) number of commands to be compiled into a pipeline. In addition to the command execution, pipelines are constructed (much like in the historical command replacement) by iterating over each token in the input, adding it to the tokens array for the current command, and terminating the token array with a NUL byte when a `|` is found. Input and output redirection are accomplished by setting the `stdin_file` and `stdout_file` of the first and last command respectively when a `>`, `<`, or `>>` are found. Note that tokens (i.e. words) in each command are stored in an array, so there is a limited number permissible for each individual command.
