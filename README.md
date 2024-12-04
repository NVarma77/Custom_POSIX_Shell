# MySH: My Custom POSIX Shell

MySH is a custom shell implementation inspired by Unix/Linux shells like Bash or Zsh. It supports interactive and batch modes, providing a simple yet functional command-line interface for executing commands, handling redirection, pipelines, and wildcards.

## Features

- **Interactive Mode**: 
  - Displays a prompt (`mysh>`) for user commands.
  - Provides feedback on abnormal command exit statuses.
  - Welcomes users on startup and displays an exit message when terminated.

- **Batch Mode**: 
  - Executes commands from a file without user interaction.
  - Suppresses prompts and additional messages.

- **Built-in Commands**:
  - `cd`: Changes the current working directory.
  - `pwd`: Prints the current working directory.
  - `which`: Finds the location of an executable in predefined paths.
  - `exit`: Exits the shell.

- **Redirection**:
  - Supports input (`<`) and output (`>`) redirection.

- **Pipelines**:
  - Allows chaining commands with `|` for data flow between processes.

- **Wildcard Expansion**:
  - Expands `*` for file matching in directories.

- **Error Handling**:
  - Prints meaningful messages for invalid commands or redirection failures.

## How to Use

### Interactive Mode
Run the shell interactively:

`./mysh`

### Batch Mode
Execute commands from a file:
`./mysh <command_file>`

## Testing

The following test cases are provided to validate the shell's functionality:

1. **Basic Commands**:
   - Verify execution of simple commands like `ls`, `echo`, etc.

2. **Redirection**:
   - Test input and output redirection with `>` and `<`.

3. **Pipelines**:
   - Chain commands using `|` (e.g., `ls | grep .txt`).

4. **Built-in Commands**:
   - Test `cd`, `pwd`, `which`, and `exit` for expected behavior.

5. **Wildcard Expansion**:
   - Test wildcard patterns like `*.txt` for correct expansion.

6. **Error Handling**:
   - Test invalid commands, redirection, and pipeline errors for appropriate error messages.

### Running Tests
Use the provided test scripts to validate the shell's behavior under different scenarios:
`./test_script.sh`

## Implementation Highlights

- **POSIX Compliance**: Implements POSIX features like unbuffered I/O, `dup2()`, and `pipe()`.
- **Dynamic Parsing**: Handles command parsing and tokenization dynamically for flexibility.
