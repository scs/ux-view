/*!
 * @file main.c
 * @brief Main file of the template application. Mainly contains initialization code.
 *
 * This file defines the main funtion and helper function that initialize the application. It also contains the main loop that defines the processing cycle of the application.
 */

/*!
 * \mainpage
 * \image html sugus-orange.png
 * \section Introduction
 * This is the documentation of the LeanXsugus source code. LeanXsugus is a Project to port the Sugus sorting algorithm of the SCS Demo-Sorter to the LeanXcam.
 *
 */

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "support.h"

void *hFramework;

/*! @brief The framework module dependencies of this application. */
struct OSC_DEPENDENCY deps[] = {
	{ "log", OscLogCreate, OscLogDestroy },
	{ "sup", OscSupCreate, OscSupDestroy },
	{ "cam", OscCamCreate, OscCamDestroy }
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
	err = OscLoadDependencies (hFramework, deps, length (deps));
	if (err != SUCCESS)
	{
		fprintf(stderr, "%s: error: Unable to load dependencies! (%d)\n", __func__, err);
		goto dep_err;
	}
	
	/* Seed the random generator */
	srand(OscSupCycGet());
	
	/* Set the camera registers to sane default values. */
	err = OscCamPresetRegs();
	if (err != SUCCESS)
	{
		OscLog(ERROR, "%s: Unable to preset camera registers! (%d)\n", __func__, err);
		goto fb_err;
	}
	
	OscCamSetupPerspective(OSC_CAM_PERSPECTIVE_DEFAULT);
	
	return SUCCESS;
	
fb_err:
	OscUnloadDependencies(hFramework, deps, length (deps));
	
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
	static uint8 const frameBuffers[2][OSC_CAM_MAX_IMAGE_WIDTH * OSC_CAM_MAX_IMAGE_HEIGHT];
	uint8 const multiBufferIds[] = { 0, 1 };
	
	/* This sets the sensor area to capture the picture from. */
	err = OscCamSetAreaOfInterest((OSC_CAM_MAX_IMAGE_WIDTH - WIDTH_CAPTURE) / 2, (OSC_CAM_MAX_IMAGE_HEIGHT - HEIGHT_CAPTURE) / 2, WIDTH_CAPTURE, HEIGHT_CAPTURE);
	if (err != SUCCESS)
	{
		OscLog(ERROR, "%s: Unable to set the area of interest!\n", __func__);
		return err;
	}
	
	/* This set the exposure time to a reasonable value for the linghting in the sorter. */
	err = OscCamSetShutterWidth(15000);
	if (err != SUCCESS)
	{
		OscLog(ERROR, "%s: Unable to set the exposure time!\n", __func__);
		return err;
	}
	
	/* This sets the frame buffers to store the captured picture in. */
	err = OscCamSetFrameBuffer(0, sizeof frameBuffers[0], frameBuffers[0], TRUE);
	if (err != SUCCESS)
	{
		OscLog(ERROR, "%s: Unable to set up the frame buffer!\n", __func__);
		return err;
	}
	
	err = OscCamSetFrameBuffer(1, sizeof frameBuffers[1], frameBuffers[1], TRUE);
	if (err != SUCCESS)
	{
		OscLog(ERROR, "%s: Unable to set up the frame buffer!\n", __func__);
		return err;
	}
	
	err = OscCamCreateMultiBuffer(2, multiBufferIds);
	if(err != SUCCESS)
	{
		OscLog(ERROR, "%s: Unable to set up multi buffer!\n", __func__);
		return err;
	}
	
retry1:
	/* This starts the capture and records the time. */
	err = OscCamSetupCapture(OSC_CAM_MULTI_BUFFER, OSC_CAM_TRIGGER_MODE_MANUAL);
	if (err != SUCCESS)
	{
		OscLog(ERROR, "%s: Unable to trigger the capture (%d)!\n", __func__, err);
		usleep(100000);
		goto retry1;
	}
	
	loop {
		uint8 * pFrameBuffer;
		
	retry:
		/* This starts the capture and records the time. */
		err = OscCamSetupCapture(OSC_CAM_MULTI_BUFFER, OSC_CAM_TRIGGER_MODE_MANUAL);
		if (err != SUCCESS)
		{
			OscLog(ERROR, "%s: Unable to trigger the capture (%d)!\n", __func__, err);
			usleep(100000);
			goto retry;
		}
		
		/* Here we wait for the picture be availible in the frame buffer. */
		err = OscCamReadPicture(OSC_CAM_MULTI_BUFFER, &pFrameBuffer, 0, 0);
		if (err != SUCCESS)
		{
			OscLog(ERROR, "%s: Unable to read the picture (%d)!\n", __func__, err);
			return err;
		}
		
		err = write(1, pFrameBuffer, WIDTH_CAPTURE * HEIGHT_CAPTURE);
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
