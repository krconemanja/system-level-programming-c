#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

//Function declarations
static void die(const char message[]);
static void printStatus(pid_t pid, int status);
static size_t parseCommandLine(char *buffer, char **argv);
static pid_t handleTerminatedChild(void);
static pid_t executeCommand(char **argv, int background);
static void sigchld_handler(int sig);

//Volatile because it's accesses both by main() and the signal handler
static volatile sig_atomic_t child_terminated = 0;

// DESCRIPTION:
// in the main() function necessary initialization work is done, the prompt
// is printed, the user input is read and the corresponding functions
// are called to parse the user input and execute the entered commands.
//
// Furthermore, the code to collect child processes and, if necessary, wait
// for them is located in the main().
// Starting with subexercise b) waiting on child processes shall be
// implemented using the sigsuspend() method. Be aware of the correct
// synchronization to prevent lost-wakeup or other synchronization problems.
int main(void) {

	//Block SIGINT signals
	sigset_t set;
	sigset_t set_old;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigprocmask(SIG_BLOCK, &set, &set_old);

	//Installing a handler for SIGCHLD
	struct sigaction sigchld_action;
	sigchld_action.sa_handler = sigchld_handler;
	sigemptyset(&sigchld_action.sa_mask);
	sigchld_action.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &sigchld_action, NULL);

	char buf[1023]; //max length of input of 1023 characters assumed
	char *argv[512]; //If every word of input is one character long then we'll have half of the max length because of spaces

	printf("mish> ");
	fflush(stdout);

	while( fgets(buf, 1023, stdin) != NULL ) { //Keep running until we get an EOF (CTRL-D)

		if(child_terminated) { //Check if any children terminated (this is how we catch terminated background processes)
			while( handleTerminatedChild() > 0 ) {
				//Keep handling terminated children until there are none left
			}
			child_terminated = 0;
		}

		//Get the program name and additional arguments
		if( !parseCommandLine(buf, argv) ) { //If the input was just enter, print mish>
			printf("mish> ");
			fflush(stdout);
			continue;
		}

		//Determine if background or foreground
		//If background, delete '&' from argv
		int background = 0;
		int i = 1;
		while(argv[i] != NULL) {
			i++;
		}
		if( strcmp(argv[--i], "&") == 0) { //Compare two strings
			background = 1;
			argv[i] = NULL;
		}

		//Create new process
		executeCommand(argv, background);

		printf("mish> ");
		fflush(stdout);

	}

	//Error handling
	if( ferror(stdin) ) {
		die("stdin");
	}

	return EXIT_SUCCESS;
}

// Error function
static void die(const char message[]) {
    perror(message);
    exit(EXIT_FAILURE);
}

// DESCRIPTION:
//   printStatus() examines the termination of a child process and
//   prints the source of the exit (signal or exit) and the
//   exit code or signal number, respectively.
//
// PARAMETER:
//   pid:    PID of the exited child process
//   status: Status bits as retrieved from waitpid(2)
//
// RETURN VALUE:
//   void:   -
static void printStatus(pid_t pid, int status) {

	if( WIFEXITED(status) ) {
		printf("Exit status [%d] = %d\n", pid, WEXITSTATUS(status));
		return;
	}

	if( WIFSIGNALED(status) ) {
		printf("Signal [%d] = %d\n", pid, WTERMSIG(status));
		return;
	}

	printf("Process [%d] terminated due to an unknown reason.\n", pid);
	return;
}

// DESCRIPTION:
//   parseCommandLine() gets one line as read from stdin and parses
//   the command and its arguments.
//
// PARAMETER:
//   buffer:     pointer to the line as read from stdin
//   argv:       pointer to an array to store the command (argv[0])
//               and all of its arguments (argv[1] - argv[n-1]). The
//               last argument (argv[n]) must be a NULL pointer
// RETURN VALUE:
//   size_t:     number of parsed entries (command and arguments) excluding
//               the NULL pointer
static size_t parseCommandLine(char *buffer, char **argv) {

	argv[0] = strtok(buffer, " \t\n");

	//Nothing was passed, just Enter pressed
	if(argv[0] == NULL) {
		return 0;
	}

	int i = 1;
	while( (argv[i] = strtok(NULL, " \t\n")) != NULL ) {
		i++;
	}

	return i;
}

// DESCRIPTION:
//  Signal handler to handle SIGCHLD events.
static void sigchld_handler(int sig) {
	child_terminated = 1;
}

// DESCRIPTION:
// Subexercise a)
//   handleTerminatedChild() is called when the program waits for a child
//   to terminate with waitpid(2) (blocking).
// Subexercise b+c)
//   handleTerminatedChild() is called when a child process has potentially
//   been terminated. It is collected with waitpid(2) (non-blocking).
//
// If a child process is collected it's exit status is printed
// (see printStatus()).
//
// PARAMETER:
//   void: -
//
// RETURN VALUE:
//   pid_t: PID of the handled child process
//
static pid_t handleTerminatedChild(void) {

	int status; //Variable into which we store information on how the process terminated

	pid_t term_pid = waitpid(-1, &status, WNOHANG); //WNOHANG - non-blocking (returns immediately with 0 if no process is terminated)
	// -1 makes it so that waitpid looks for ANY process

	if (term_pid > 0) { //If a process has been handled, print its status
		printStatus(term_pid, status);
	}

	return term_pid;
}

// DESCRIPTION:
// executeCommand() forks the current process to execute argv. If it is
// a background process the parent process prints the PID of the child
// process to stdout.
//
// In the child process the program in argv is loaded and executed.
//
// PARAMETER:
//   argv:       The program to execute (argv[0]) and its arguments
//               (argv[1] - argv[n-1]) and a terminating NULL pointer
//               (argv[n]).
//   background: int value indicating if the program should be executed
//               as a foreground process (background = 0) or as a
//               background process (background = 1)
//
// RETURN VALUE:
//   pid_t:      PID of the started child
//
static pid_t executeCommand(char **argv, int background) {

	pid_t res = fork();
	if(res > 0) { //Parent process

		if(background == 1) {
			printf("Started [%d]\n", res); //The background process is started

		} else {
			//Passive waiting for the SIGCHLD signal, with protection from lost wakeup by masking and unmasking signals with sigprocmask
			sigset_t sync_mask, old_mask;
			sigemptyset(&sync_mask);
			sigaddset(&sync_mask, SIGCHLD);

			pid_t foreground_alive = 1;

			sigprocmask(SIG_BLOCK, &sync_mask, &old_mask);

			while(foreground_alive) { //Sleep until the foreground process dies

				while(!child_terminated) { //Sleep until ANY child has died
					sigsuspend(&old_mask);
				}
				child_terminated = 0; //Reset the global flag

				pid_t pid_terminated_child;
				while ((pid_terminated_child = handleTerminatedChild()) > 0) { //Handle all terminated children
					if (pid_terminated_child == res) { //If the child we just handled is the foreground process then we stop sleeping, if not, go back to sleep
						foreground_alive = 0;
					}
				}
			}
			sigprocmask(SIG_SETMASK, &old_mask, NULL);
		}

	} else if(res == 0) { //Child process

		//Allow (unmask) signals for all child processes
		sigset_t set_empty;
		sigemptyset(&set_empty);
		sigprocmask(SIG_SETMASK, &set_empty, NULL);

		//Declare the struct sigaction for SIGINT for child processes, and set mask and flags because they're the same for foreground and background processes.
		struct sigaction sigint_action;
		sigemptyset(&sigint_action.sa_mask);
		sigint_action.sa_flags = SA_RESTART;

		if (background == 1) {
			//Set default handling of SIGINT for the background processes
			sigint_action.sa_handler = SIG_IGN;
			sigaction(SIGINT, &sigint_action, NULL);

		} else {
			//Set default handling of SIGINT for the foreground processes
			sigint_action.sa_handler = SIG_DFL;
			sigaction(SIGINT, &sigint_action, NULL);
		}

		//Execute the child program
		execvp(argv[0], argv);
		die("execvp"); //Execute if there's been some error

	} else { //Error - No child process created
		die("fork");
	}

	return res;
}
