/*!
 * @file main.c
 * @brief Main file of the template application. Mainly contains initialization code.
 *
 * This file defines the main funtion and helper function that initialize the application. It also contains the main loop that defines the processing cycle of the application.
 */

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include "support.h"

/*!
 * @brief Program entry
 *
 * @param argc Command line argument count.
 * @param argv Command line argument strings.
 * @return 0 on success
 */
int main(const int argc, const char ** argv)
{
	static uint8 * buffer;
	int err, num, count;
	
	if (argc != 2) {
		fprintf(stderr, "Wrong number of arguments!");
		return 1;
	}
	
	count = atoi(argv[1]);
	buffer = malloc(count);
	
	while (num < count) {
		int num_read = read(0, buffer + num, count - num);
		
		if (num_read <= 0)
			return 1;
		
		num += num_read;
	}
	
	close(0);
	
	err = write(1, buffer, count);
	if (err == -1)
		return 1;
	
	return 0;
}
