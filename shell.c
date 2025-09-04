#include <pwd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/param.h>
#include <readline/readline.h>

#include "history.h"
#include "logger.h"



int active_process = -1;

// Handle Ctrl+c (send kill signal to child)
void child_killer(int signo) {
	// Only kill active process
	if(active_process != -1) {
		// Try to kill with sigterm
		if(kill(active_process, SIGTERM) == -1) {
			// Try to kill with sigkill if sigterm did not work
			if(kill(active_process, SIGKILL) == -1) {
				// If sigkill didn't work, we have an issue
				perror("kill");
				return;
			}
		}
		
		// Reset active process
		active_process = -1;
	}
}

// Change current_directory
void change_directory(const char* directory) {
	if(directory == NULL || strlen(directory) == 0) {
		// Change to home directory
		char* home = getenv("HOME");
		if(home == NULL) {
			LOG("No home directory foun%c\n", 'd');
		} else {
			LOG("Changing directory to \"%s\"\n", home);
			if(chdir(home) == -1) {
				perror("chdir");
				return;
			}
		}
	} else {
		// Change to given directory
		LOG("Changing directory to \"%s\"\n", directory);
		if(chdir(directory) == -1) {
			perror("chdir");
			return;
		}
	}
}


// Get the next command
char* get_input(char* prompt, bool scripting) {
	// Set up input
	char* input = NULL;
	size_t input_size = 0;

	// Get input
	if(scripting) {
		// Use getline when in scripting mode
		if(getline(&input, &input_size, stdin) == -1) {
			perror("getline");
			free(input);
			input = NULL;
		}
	} else {
		// Use readline when in normal mode
		input = readline(prompt);
	}

	// Get rid of trailing newline (if any)
	if(input != NULL) {
		input_size = strlen(input);
		if(input[input_size - 1] == '\n') {
			input[input_size - 1] = '\0';
		}
	}

	return input;
}

// Check if command is "history"
bool check_history(const char* input) {
	const char* history = "history";
	return input != NULL && strncmp(input, history, strlen(history)) == 0;
}

// Resize and strcat
void strcatr(char** string, const char* append) {
	// Cannot append nothing
	if(append == NULL) {
		return;
	}
	
	if(*string == NULL) {
		*string = strdup(append);
	} else {
		*string = realloc(*string, strlen(*string) + strlen(append) + 1);
		strcat(*string, append);
	}
}

// Construct prompt
// 	FORMAT: UNAME@HOSTNAME:DIRECTORY>
void update_prompt(char** prompt) {
	// Deallocate old prompt
	if(*prompt != NULL) {
		free(*prompt);
	}
	
	// Set prompt to null
	*prompt = NULL;

	strcatr(prompt, "[");
	
	// Add current directory to prompt
	const char* home_directory = getenv("HOME");
	char* current_directory = getcwd(NULL, 0);
	if(strncmp(home_directory, current_directory, strlen(home_directory)) == 0) {
		// current_directory starts with home_directory
		strcatr(prompt, "~");
		strcatr(prompt, current_directory + strlen(home_directory));
	} else {
		strcatr(prompt, current_directory);
	}
	free(current_directory);

	// Add user name to prompt
	strcatr(prompt, " | ");
	strcatr(prompt, getenv("LOGNAME"));
	strcatr(prompt, "@");

	// Add host name to prompt
	char host_name[128];
	host_name[0] = '\0';
	gethostname(host_name, 127);
	strcatr(prompt, host_name);

	// Add command number to prompt
	char* command_number = NULL;
	asprintf(&command_number, "%d", hist_last_cnum() + 1);
	strcatr(prompt, " | ");
	strcatr(prompt, command_number);
	free(command_number);

	// Terminate prompt with $
	strcatr(prompt, "]$");
}

/**
 * Retrieves the next token from a string.
 *
 * Parameters:
 * - str_ptr: maintains context in the string, i.e., where the next token in the
 *   string will be. If the function returns token N, then str_ptr will be
 *   updated to point to token N+1. To initialize, declare a char * that points
 *   to the string being tokenized. The pointer will be updated after each
 *   successive call to next_token.
 *
 * - delim: the set of characters to use as delimiters
 *
 * Returns: char pointer to the next token in the string.
 */
char *next_token(char **str_ptr, const char *delim)
{
    if (*str_ptr == NULL) {
        return NULL;
    }

    size_t tok_start = strspn(*str_ptr, delim);
    size_t tok_end = strcspn(*str_ptr + tok_start, delim);

    /* Zero length token. We must be finished. */
    if (tok_end  == 0) {
        *str_ptr = NULL;
        return NULL;
    }

    /* Take note of the start of the current token. We'll return it later. */
    char *current_ptr = *str_ptr + tok_start;

    /* Shift pointer forward (to the end of the current token) */
    *str_ptr += tok_start + tok_end;

    if (**str_ptr == '\0') {
        /* If the end of the current token is also the end of the string, we
         * must be at the last token. */
        *str_ptr = NULL;
    } else {
        /* Replace the matching delimiter with a NUL character to terminate the
         * token string. */
        **str_ptr = '\0';

        /* Shift forward one character over the newly-placed NUL so that
         * next_pointer now points at the first character of the next token. */
        (*str_ptr)++;
    }

    return current_ptr;
}

// Replace ! commands
void replace_historical(char** input) {
	// Delimeters to skip
	const char* delims = " ";
	
	// New input string
	char* new_input = malloc(1 * sizeof(char));
	if(new_input == NULL) {
		perror("malloc");
		return;
	}
	new_input[0] = '\0';
	
	// Loop variables
	char* curr_tok = "";
	char* next_tok = *input;
	char* whitespace;
	bool first_iteration = true;
	bool comment = false;
	
	do {
		bool replaced = false;
		// Replace historical commands
		if(strlen(curr_tok) >= 2 && curr_tok[0] == '!') {
			char* hist_command = NULL;
			if(curr_tok[1] == '!') {
				// Replace with previous command
				const char* prev_command = hist_search_cnum(hist_last_cnum());
				if(prev_command != NULL) {
					hist_command = strdup(prev_command);
				}
			} else if('0' <= curr_tok[1] && curr_tok[1] <= '9') {
				// Replace with numerical command
				const char* num_command = hist_search_cnum(atoi(curr_tok + 1));
				if(num_command != NULL) {
					hist_command = strdup(num_command);
				}
			} else {
				// Replace with prefix command
				const char* pref_command = hist_search_prefix(*input + 1);
				if(pref_command != NULL) {
					hist_command = strdup(pref_command);
				}
			}
			
			if(hist_command != NULL) {
				strcatr(&new_input, hist_command);
				free(hist_command);
			}
		} else {
			strcatr(&new_input, curr_tok);
		}
		
		if(curr_tok != NULL && curr_tok[0] != '\0' && replaced) {
			free(curr_tok);
		}

		// Handle comments and whitespaces
		if(next_tok != NULL) {
			// Append space if next_token has been called (replaced by '\0')
			if(!first_iteration) {
				strcatr(&new_input, " ");
			} else {
				first_iteration = false;
			}
			
			// Get amount of whitespace
			size_t whitespace_size = strspn(next_tok, delims);
		
			if(next_tok[whitespace_size] == '#') {
				// Append remainder if it is a comment
				strcatr(&new_input, next_tok);
				comment = true;
			} else {
				// Make a copy (avoid messing with original
				whitespace = strdup(next_tok);
				if(whitespace == NULL) {
					perror("strdup");
					return;
				}
	
				// Truncate to just whitespace
				whitespace[whitespace_size] = '\0';
			
				// Append leading whitespace
				strcatr(&new_input, whitespace);
				free(whitespace);
			}
		}
	} while((curr_tok = next_token(&next_tok, delims)) != NULL && !comment);
	
	// Avoid memory leaks
	free(*input);

	// Set input to new input (with historical commands replaced)
	*input = new_input;
}


// Struct to store commands
struct command_struct {
	char* tokens[128];			// Tokens for the command (i.e. argv) (initialize to large array to avoid resizing)
	char* stdin_file;			// Name of file to redirect first command stdin to (if exists)
	char* stdout_file;			// Name of file to redirect last command stdout to (if exists)
	int stdout_flags;			// Flags to redirect last command with (i.e. O_CREAT | O_TRUNC | O_CREAT | O_APPEND)
	struct command_struct* next_command;	// Pointer to next command (NULL if there are no more commands)
};

// Deallocate commands
void destroy_commands(struct command_struct* command) {
	// Free next command
	if(command->next_command != NULL) {
		destroy_commands(command->next_command);
	}
	
	// Free tokens
	size_t token_index = 0;
	char* curr_token = command->tokens[token_index++];
	while(curr_token != NULL) {
		free(curr_token);
		curr_token = command->tokens[token_index++];
	}

	// Free stdin file
	if(command->stdin_file != NULL) {
		free(command->stdin_file);
	}


	// Free stdout file
	if(command->stdout_file != NULL) {
		free(command->stdout_file);
	}

	// Free the command
	free(command);
}

// Print commands
void print_commands(struct command_struct* commands) {
	for(int command_number = 1; commands != NULL; command_number++, commands = commands->next_command) {
		LOG("Command %d: \n", command_number);
		size_t token_index = 0;
		const char* curr_token = commands->tokens[token_index++];
		while(curr_token != NULL) {
			LOG("\t%s \n", curr_token);
			curr_token = commands->tokens[token_index++];
		}

		LOG("\t\tSTDIN FILE%c ", ':');
		if(commands->stdin_file == NULL) {
			LOG("NUL%c\n", 'L');
		} else {
			LOG("\"%s\"\n", commands->stdin_file);
		}

		LOG("\t\tSTDOUT FILE%c ", ':');
		if(commands->stdout_file == NULL) {
			LOG("NUL%c\n", 'L');
		} else {
			LOG("\"%s\"\n", commands->stdout_file);
		}
	}
}

// Construct command pipeline
struct command_struct* construct_commands(char* input) {
	// Set up linked list of commands
	struct command_struct* commands = malloc(sizeof(struct command_struct));
	if(commands == NULL) {
		perror("malloc");
		return NULL;
	}
	struct command_struct* curr_command = commands;
	size_t curr_token = 0;
	
	// Set default values in curr_command
	curr_command->tokens[curr_token] = NULL;
	curr_command->stdin_file = NULL;
	curr_command->stdout_file = NULL;
	curr_command->next_command = NULL;

	// Delimeters to skip
	const char* delims = " \t";

	// Loop variables
	const char* curr_tok = "";
	char* next_tok = input;
	char* stdin_file = NULL;
	char* stdout_file = NULL;
	int stdout_flags = O_WRONLY | O_CREAT | O_TRUNC;

	// Extract tokens and build commands argument
	while((curr_tok = next_token(&next_tok, delims)) != NULL) {
		if(curr_tok[0] == '<') {
			// Set stdin_file to next token
			if((curr_tok = next_token(&next_tok, delims)) != NULL) {
				stdin_file = strdup(curr_tok);
				continue;
			} else {
				break;
			}
		}

		if(curr_tok[0] == '>') {
			// Set stdout_file to next token
			if(strlen(curr_tok) >= 2 && curr_tok[1] == '>') {
				stdout_flags = O_WRONLY | O_CREAT | O_APPEND;
			}
			if((curr_tok = next_token(&next_tok, delims)) != NULL) {
				stdout_file = strdup(curr_tok);
				continue;
			} else {
				break;
			}
		}

		if(curr_tok[0] == '#') {
			curr_tok = NULL;
			break;
		}

		if(curr_tok[0] == '|') {
			// Pipe into next command
			// 	Set current token to NULL
			// 	Set curr_token to 0
			// 	Create new command and append to curr_command
			// 	Update curr_command
			curr_tok = NULL;
			curr_command->tokens[curr_token] = NULL;
			curr_token = 0;
			curr_command->stdin_file = NULL;
			curr_command->stdout_file = NULL;
			curr_command->next_command = malloc(sizeof(struct command_struct));
			curr_command = curr_command->next_command;
			if(curr_command == NULL) {
				perror("malloc");
			}
		} else {
			// Append curr_tok to tokens array
			curr_command->tokens[curr_token++] = strdup(curr_tok);
		}
	}

	// Finish setup

	curr_command->tokens[curr_token] = NULL;
	curr_command->stdout_flags = stdout_flags;
	curr_command->next_command = NULL;

	// Free stdout file
	if(stdout_file != NULL) {
		curr_command->stdout_file = strdup(stdout_file);
		free(stdout_file);
	} else {
		curr_command->stdout_file = NULL;
	}

	// Free stdin file
	if(stdin_file != NULL) {
		commands->stdin_file = strdup(stdin_file);
		free(stdin_file);
	} else {
		commands->stdin_file = NULL;
	}

	// Return commands (NOT CURR_COMMAND)
	return commands;
}

// Execute commands
void execute_commands(struct command_struct* commands) {
	// Cannot run empty commands
	if(commands == NULL) {
		return;
	}

	// Set up stdin pipe (if applicable)
	if(commands->stdin_file != NULL) {
		LOG("Stdin file is \"%s\"\n", commands->stdin_file);
		int open_flags = O_RDONLY;
		int open_perms = 0666;
		int in_fd = open(commands->stdin_file, open_flags, open_perms);
		if(in_fd == -1) {
			perror("open");
		} else {
			LOG("Setting stdin to \"%s\"\n", commands->stdin_file);
			dup2(in_fd, STDIN_FILENO);
		}
	}

	// Set up pipe
	int fd[2];
	
	// Iterate over commands that need to be piped (nonterminal)
	while(commands->next_command != NULL) {
		pipe(fd);
		pid_t child = fork();
		
		if(child == -1) {
			perror("fork");
			exit(0);
		} else if(child == 0) {
			LOG("Executing %s\n", commands->tokens[0]);
			// Child - execute command
			close(fd[0]);
			dup2(fd[1], STDOUT_FILENO);
			execvp(commands->tokens[0], commands->tokens);
			close(fileno(stdin));
			exit(0);
		}
		
		// Parent (child never reaches here because execvp/exit)
		close(fd[1]);
		dup2(fd[0], STDIN_FILENO);
		commands = commands->next_command;
	}
	
	// Handle last command
	if(commands->stdout_file != NULL) {
		LOG("Stdout file is \"%s\"\n", commands->stdout_file);
		int open_flags = commands->stdout_flags;
		int open_perms = 0666;
		int out_fd = open(commands->stdout_file, open_flags, open_perms);
		if(out_fd == -1) {
			perror("open");
		} else {
			dup2(out_fd, STDOUT_FILENO);
		}
	}

	execvp(commands->tokens[0], commands->tokens);
	close(fileno(stdin));
	exit(0);
}

// Initialize readline (set up)
int readline_init(void) {
	rl_variable_bind("show-all-if-ambiguous", "on");
	rl_variable_bind("colored-completion-prefix", "on");
	return 0;
};

// Main function (i.e. entry point to shell)
int main(void) {
	hist_init(100);

	signal(SIGINT, child_killer);

	rl_startup_hook = readline_init;
	
	char* input;
	char* prompt = NULL;
	bool need_to_exit = false;
	bool scripting = !isatty(STDIN_FILENO);

	do {
		update_prompt(&prompt);
		input =  get_input(prompt, scripting);
		
		LOG("INPUT: \"%s\"\n", input);

		if(input == NULL || strncmp(input, "exit", strlen("exit")) == 0) {
			LOG("EXITIN%c\n", 'G');
			need_to_exit = true;
		} else if(strlen(input) != strspn(input, " \t")) {
			replace_historical(&input);
			
			LOG("REPLACED: \"%s\"\n", input);			

			if(strlen(input) != 0) {
				hist_add(input);
			}

			if(check_history(input)) {
				hist_print();
			} else {
				struct command_struct* commands = construct_commands(input);
				
				print_commands(commands);

				if(commands->tokens[0] != NULL) {
					if(strncmp(commands->tokens[0], "cd", strlen("cd")) == 0) {
						change_directory(commands->tokens[1]);
					} else {
						pid_t child = fork();
						if(child == -1) {
							perror("fork");
						} else if(child == 0) {
							// Child - execute commands, close stdin if failed and exit
							execute_commands(commands);
							close(fileno(stdin));
							exit(0);
						} else {
							// Parent - set active process and wait for child
							active_process = child;
							int status;
							wait(&status);
							active_process = -1;
						}
					}
				}

				destroy_commands(commands);
			}
		}
		free(input);
		free(prompt);
		prompt = NULL;
	} while(!need_to_exit);

	hist_destroy();

	return 0;
}
