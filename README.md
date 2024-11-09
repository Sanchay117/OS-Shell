# SimpleShell

This is a basic command-line shell program written in C. It can execute commands, supports piping, runs commands in the background, and maintains a command history.

## Features
- **Command Execution:** Executes single shell commands as well as commands with piping.
- **Piped Commands:** Supports chaining commands using `|`.
- **Background Execution:** Allows commands to run in the background using `&`.
- **Command History:** Maintains a history of commands with PID, start/end time, status, and duration.
- **Input Sanitization:** Handles leading/trailing spaces and ignores empty commands.

## Code Structure

### Struct Definitions
- **`CommandDetails`**: Stores command details such as `command`, `pid`, `start_time`, `end_time`, `duration`, and `status`.

### Functions
- **`add_to_history(CommandDetails command)`**: Adds details of executed commands to the command history.
- **`sig_child_handler(int signal)`**: Signal handler for child processes to manage background tasks.
- **`parse_command(char *input)`**: Parses the input command into tokens.
- **`execute_command(char **args)`**: Executes a command without piping.
- **`execute_piped_commands(char *input)`**: Executes commands with piping (`|`).
- **`execute_background_command(char **args)`**: Runs commands in the background.
- **`sanitize_input(char *input)`**: Trims spaces and handles empty inputs.

## Usage

1. **Compilation**
   ```bash
   gcc -o simpleshell simpleshell.c
   ```

2. **Running the Shell**
   ```bash
   ./simpleshell
   ```

3. **Example Commands**
   - **Single Command**: `ls -l`
   - **Piped Command**: `cat file.txt | grep "search"`
   - **Background Command**: `sleep 10 &`

4. **Viewing History**
   - Command history is automatically updated and stored in memory during runtime.
   - Each entry includes command details such as PID, execution time, status, and duration.

## Notes
- The program uses signal handling to manage background tasks and update command history.
- History is lost when the shell session ends (i.e., it is not persisted across sessions).

## License
This project is licensed under the MIT License.
