#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#include "history.h"
#include "logger.h"

// History is effectively a queue - as such, we can use a circular array queue (like in 545)
struct history_struct {
	char** commands;				// Array of commands
	unsigned int size;				// The size of hist array
	unsigned int next_command;			// The next available place to put a command
	unsigned int oldest_command;			// The oldest command in history
	unsigned int oldest_command_number;		// The command number of the oldest command
	unsigned int num_commands;			// The number of commands in the history
};

struct history_struct history = {
	.next_command = 0,
	.oldest_command = 0,
	.oldest_command_number = 1,
	.num_commands = 0,
};

// Equivalent to i++ (but mod)
unsigned int increment(unsigned int* command) {
	unsigned int value = *command;
	*command = (*command + 1) % history.size;
	return value;
}

// Equivalent to command-- (but mod)
unsigned int decrement(unsigned int* command) {
	// (i - d) % m === (i + m - d) % m for all i, d, and m
	// if d <= m (i + m - d) % m >= 0
	unsigned int value = *command;
	*command = (*command + history.size - 1) % history.size;
	return value;
}

// Initialize history
void hist_init(unsigned int limit) {
	history.commands = malloc(limit * sizeof(char*));
	history.size = limit;
}

void hist_destroy(void)	{
	for(int i = 0; i < history.num_commands; i++) {
		if(history.commands[i] != NULL) {
			free(history.commands[i]);
		}
	}
	free(history.commands);
}

void hist_add(const char* cmd) {
	// Free oldest command if necessary
	if(history.num_commands == history.size) {
		free(history.commands[increment(&history.oldest_command)]);
		history.oldest_command_number++;
		history.num_commands--;
	}
	
	// Add newest command
	history.commands[increment(&history.next_command)] = strdup(cmd);
	history.num_commands++;
}

void hist_print(void) {
	// Print all commands
	LOG("PRINTING %u COMMANDS\n", history.num_commands);
	unsigned int command = history.oldest_command;
	for(unsigned int offset = 0; offset < history.num_commands; offset++) {
		printf("%3u %s\n", history.oldest_command_number + offset, history.commands[increment(&command)]);
	}
	fflush(stdout);
}

const char* hist_search_prefix(const char* prefix) {
	if(history.num_commands == 0) {
		return NULL;
	}
	unsigned int command = decrement(&history.next_command);
	increment(&history.next_command);
	decrement(&command);
	for(int offset = 0; offset < history.num_commands; offset++) {
		if(strlen(prefix) <= strlen(history.commands[command]) && strncmp(history.commands[command], prefix, strlen(prefix)) == 0) {
			return history.commands[command];
		}
		decrement(&command);
	}
	return NULL;
}

const char* hist_search_cnum(int command_number) {
	if(command_number < history.oldest_command_number || command_number > history.oldest_command_number + history.num_commands) {
		return NULL;
	}
	return history.commands[(history.oldest_command + command_number - history.oldest_command_number) % history.size];
}

unsigned int hist_last_cnum(void) {
	return history.oldest_command_number + history.num_commands - 1;
}
