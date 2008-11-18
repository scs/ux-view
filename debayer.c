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

#define MARKERS_ENABLE

#include "support.h"

void *hFramework;

/*! @brief The framework module dependencies of this application. */
struct OSC_DEPENDENCY deps[] = {
	{ "log", OscLogCreate, OscLogDestroy },
	{ "sup", OscSupCreate, OscSupDestroy },
	{ "vis", OscVisCreate, OscVisDestroy }
};

/*!
 * @brief Initialize everything so the application is fully operable after a call to this function.
 *
 * @return SUCCESS or an appropriate error code.
 */
static OSC_ERR init(const int argc, const char * * argv)
{
	OSC_ERR err = SUCCESS;
	
	/* Set log levels. */
	OscLogSetConsoleLogLevel(EMERG);
	OscLogSetFileLogLevel(EMERG);
	
	/* Create the framework */
	err = OscCreate(&hFramework);
	if (err != SUCCESS)
	{
		fprintf(stderr, "%s: error: Unable to create framework.\n", __func__);
		return err;
	}
	
	/* Load the framework module dependencies. */
	err = OscLoadDependencies(hFramework, deps, length (deps));
	if (err != SUCCESS)
	{
		fprintf(stderr, "%s: error: Unable to load dependencies! (%d)\n", __func__, err);
		goto dep_err;
	}
	
	/* Seed the random generator */
	srand(OscSupCycGet());
		
	return SUCCESS;
		
dep_err:
	OscDestroy(&hFramework);
	
	return err;
}

OSC_ERR Unload()
{
	/* Unload the framework module dependencies */
	OscUnloadDependencies(hFramework, deps, length (deps));
	
	OscDestroy(hFramework);
	
	return SUCCESS;
}

/*!
 * @brief This is the main loop which is called after the initialisation of the application.
 *
 * This loop alternately takes pictures and calls parts of the aplication to process the image, handle the valves and read and write configuration data.
 */
OSC_ERR mainLoop() {
	OSC_ERR err = SUCCESS;
	static uint8 greyBuffer[WIDTH_CAPTURE * HEIGHT_CAPTURE];
	static uint8 colorBuffer[3 * WIDTH_CAPTURE * HEIGHT_CAPTURE];
	
	loop {
		int num = 0;
		
		while (num < sizeof greyBuffer) {
			int num_read = read(0, greyBuffer + num, (sizeof greyBuffer) - num);
			
			if (num_read == 0) {
				fprintf(stderr, "Reached end of file.\n");
				return SUCCESS;
			} else if (num_read == -1) {
				fprintf(stderr, "Error: Error %d.\n", errno);
				return ! SUCCESS;
			}
			
			num += num_read;
		}
		
		err = OscVisDebayer(greyBuffer, WIDTH_CAPTURE, HEIGHT_CAPTURE, ROW_RGRG, colorBuffer);
		if (err == -1)
			return err;
		
		err = write(1, colorBuffer, sizeof colorBuffer);
		if (err == -1)
			return err;
	}
	
	return SUCCESS;
}

/*!
 * @brief Program entry
 *
 * @param argc Command line argument count.
 * @param argv Command line argument strings.
 * @return 0 on success
 */
int main(const int argc, const char ** argv)
{
	OSC_ERR err = SUCCESS;
	
	/* This initializes various parts of the framework and the application. */
	err = init(argc, argv);
	if (err != SUCCESS)
	{
		OscLog(ERROR, "%s: Initialization failed!(%d)\n", __func__, err);
		return err;
	}
	OscLog(INFO, "Initialization successful!\n");
	
	/* Calls the main loop. This only returns on an error. */
	mainLoop();
	
	/* On an error, we unload the framework, before we exit. */
	Unload();
	return 0;
}
