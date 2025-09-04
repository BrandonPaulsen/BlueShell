# Terminal Shell

## Description

For this lab, you are tasked with making a terminal shell. The name is up to you, but it must end with 'sh' (this is tradition). A shell is complicated, but its core functionality can be summarized as follows:

- On startup, the shell prints a prompt and waits for user input
- The shell searches both the current directory and `PATH` environment variable for executables matching commands
	- Execvp is used to facilitate execution
- The shell prints an error if the desired command is not found

The prompt you display is up to you, though it should include at least 4 bits of information helpful to the user. The professor's example includes:

1. Whether or not the last command was successful (as an emoji)
2. The current command number (starting from 1)
3. The current username (root if running as root)
4. The host name
5. The current working directory

The shell must support a scripting mode to run (and pass test cases). Scripting mode is essentially the same as the regular mode except it does not print the prompt. The shell can determine if it should run in scripting mode if `stdin` is connected to a terminal (use `isatty` to ascertain this). Scripting mode should use `readline` instead of `getline` to read input. Note: close stdin on child process if call to `exec` fails to prevent infinite loops.

The shell should support the following built in commands:
 
- `cd` to change current working directory
	- `cd` with no arguments changes to users home directory
- `#` to write comments (ignored by shell)
- `history` which prints the last 100 commands with command numbers
	- Note that index in command array is not the same as command number
	- e.g. might have 100 historical commands with command numbers 600-699
- `!` reexecution
	- `!!` re-runs last command
	- `!39` re-runs 39th command
	- `!ls` re-runs last command that starts with ls
- `exit` to exit shell
 
The shell should keep a history of the last 100 commands. If a command is blank, do not include it in the history. Show and store original commands (not tokenized or modified). To do this, use `strdup`. Command reexecution should be supported with the `!` command in the following ways:

- `!13` reruns command 13
- `!cc` reruns the last command starting with "cc"

Reexecuted commands should add the referenced command and not the `!` command to the history to avoid loops.

The shell must support pipes and redirection using `pipe` and `dup2`. An arbitrary number of pipes should be supported, but only 1 redirection need to be supported. Do not worry about supporting `>` or `>>` in the middle of a pipeline. Some examples of pipelines and redirectionsare given below:

- `cat file.txt | sort`
- `echo 'Hello World! > my_file.txt`
- `sort < /etc/passwd > sorted_passwd.txt`

The shell should feature a signature feature - what this is is up to you. Make sure to get the signature feature checked off by the instructor before demoing.

Include a `README.md` that describes how the program works and any relevant details.

## Test breakdown

	- [ ] Basic features
		- [ ] Command execution
		- [ ] Scripting
		- [ ] Basic builtins
		- [ ] SIGINT HANDLER
	- [ ] History list
		- [ ] Small history list
		- [ ] Large history list
	- [ ] Re-run commands
		- [ ] !num
		- [ ] !prefix
		- [ ] !!
	- [ ] Pipes and redirection
		- [ ] Basic pipes
		- [ ] Multiple pipes
		- [ ] I/O redirection
		- [ ] Pipes and redirection
	- [X] Analysis
		- [X] Static analysis
		- [X] Leak check

## Grading

	- 90%
		- Pass all included test cases
	- 95%
		- All previous requirements
		- Add custom prompt
		- Project documentation
	- 100%
		- All previous requirements
		- Implement signature feature
		- Pass live demo
			- Each instance of shell crashing or failing to work properly carries a 1% deduction

## Code review

	- Code quality and stylistic consistency
	- Code reuse and abstraction
		- Design helper functions for repetitive portions of code
		- Design for use in future projects
	- Functions, structs, etc. have documentation
	- No dead, leftover, or unnecessary code
	- Will need to explain 1-3 parts of implementation
		- Describe high level design aspects and challenges faced when implementing
	
## Files

- [[vfile:Makefile|Makefile]]
- [[vfile:README.md|README.md]]
- [[vfile:logger.h|logger.h]]
- [[vfile:history.h|history.h]]
- [[vfile:history.c|history.c]]
- [[vfile:shell.c|shell.c]]

## TODO

- [ ] Prompt
	- [ ] Decide what information to include in prompt
		- [ ] Username
		- [ ] Hostname
		- [ ] Directory
	- [ ] Create function to construct prompt
- [o] Add scripting support
	- [X] Use `isatty` to determine if scripting mode is enabled
	- [X] Disable prompt when in scripting mode
	- [ ] Close stdin on child process when `exec` fails if in scripting mode
- [o] Built-in commands
	- [X] `cd`
		- [X] Add directory tracking
		- [X] Add cd function
	- [ ] # (comments)
		- [ ] Split lines by # first in order to get rid of stuff after comment
	- [X] `history`
		- [X] Add history printing
		- [X] Add command to show history
	- [ ] `!` commands
		- [ ] `!NUMBER` commands
			- [ ] Check if chars following `!` are numerical
			- [ ] Replace current command with historical command
			- [ ] Add command back to history
		- [ ] `!PREFIX`
			- [ ] Check if chars following `!` are non-numerical
			- [ ] Look up previous command with given prefix
			- [ ] Replace current command with historical command
			- [ ] Add command back to history
		- [ ] `!!`
			- [ ] Check if command is exactly `!!`
			- [ ] Look up last command
			- [ ] Replace current command with last command
			- [ ] Add command back to history
	- [X] `exit`
		- [X] Quit program nicely on exit
- [X] Handle Ctrl+c (`SIGINT`) (send signal to child) (from [Lab7](../../labs-Poly1581/lab7/notes.md))
- [ ] Redirection and pipelines
	- [ ] Add pipeline support (from [Lab8](../../labs-Poly1581/lab8/notes.md))
	- [ ] Add redirection support (`>` and `<`)
- [ ] Signature feature
	- [ ] Decide what signature feature to add
	- [ ] Ask professor Malensek to validate feature
	- [ ] Add signature feature
