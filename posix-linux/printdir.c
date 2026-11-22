#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

static void list_dir(char *path);

int main(int argc, char *argv[]) {

	//If no parameters are given, list current directory.
	if (argc == 1) {
		list_dir("./");
		return EXIT_SUCCESS;
	}

	//Iterate over directories passed as parameters.
	for(int i = 1; i < argc; i++) {
		list_dir( argv[i] );
	}

	return EXIT_SUCCESS;
}

static void list_dir(char *path) {

	int ret;
	int counter = 0;
	off_t total_size = 0;

	struct dirent *ent;

	DIR *dir = opendir(path);

	//Error handling for opendir()
	if(dir == NULL) {
		perror("opendir");
		return;
	}

	printf("%s:\n", path);

	while(1) {
		errno = 0;
		ent = readdir(dir);

		//Error or EOF?
		if(ent == NULL && errno != 0) { //Error
			perror("readdir");
			continue;
		}
		if(ent == NULL) break; //EOF

		if( (ent -> d_name)[0] != '.' ) {

			if( strlen(path) + 2 + strlen(ent -> d_name) > 1025) { // length of path + length of name + '/' in the path before the name + trailing '/0' in the path > 1024 printable characters + '/0'
				printf("Name is too long (over 1024 characters).\n");
				continue;
			}

			struct stat buf;

			//Build the path string for lstat() to be able to access file data
			char temp_path[1025]; //Max path length is 1024 characters, so 1025 including '\0'
			strcpy(temp_path, path);
			if( path[strlen(path) - 1] != '/') {
				strcat(temp_path, "/");
			}
			strcat(temp_path, ent -> d_name);

			ret = lstat( temp_path, &buf);
			if(ret < 0) {
				perror("lstat");
				continue;
			}

			//Only process regular files
			if( !S_ISREG(buf.st_mode) ) {
				continue;
			}

			counter++;
			total_size += buf.st_size;

			printf("%ldB\t%s\n", buf.st_size, ent -> d_name);
		}
	}

	printf("Number of entries: %d\tTotal size: %ldB\n\n", counter, total_size);

	ret = closedir(dir);
	if(ret < 0) {
		perror("closedir");
	}
}
