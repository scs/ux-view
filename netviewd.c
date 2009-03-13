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

void * hFramework;

/*! @brief The framework module dependencies of this application. */
struct OSC_DEPENDENCY deps[] = {
	{ "log", OscLogCreate, OscLogDestroy },
	{ "sup", OscSupCreate, OscSupDestroy },
	{ "cam", OscCamCreate, OscCamDestroy },
	{ "gpio", OscGpioCreate, OscGpioDestroy }
};

OSC_ERR OscVisFastDebayerRGB_raw(uint8 const * const pIn, uint16 const width, uint16 const height, uint8 * const pOut) 
{
	uint16 x, y;
	uint8 * p = pOut;

	for (y = 0; y < height; y += 2) {
		for (x = 0; x < width; x += 2) {
			*p++ = pIn[y * width + width + x + 1];
			*p++ = (pIn[y * width + x + 1] + pIn[y * width + width + x]) / 2;
			*p++ = pIn[y * width + x];
		}
	}
	
	return 0;
}

/*!
 * @brief This is the main loop which is called after the initialisation of the application.
 *
 * This loop alternately takes pictures and calls parts of the aplication to process the image, handle the valves and read and write configuration data.
 */
OSC_ERR mainLoop(int32 opt_shutterWidth, bool opt_debayer) {
	OSC_ERR err;
	static uint8 const frameBuffers[2][WIDTH_CAPTURE * HEIGHT_CAPTURE];
	uint8 const multiBufferIds[] = { 0, 1 };
	
	/* Create the framework */
	err = OscCreate(&hFramework);
	if (err != SUCCESS)
		fail_m(err, "OscCreate()");
	
	/* Load the framework module dependencies. */
	err = OscLoadDependencies(hFramework, deps, length(deps));
	if (err != SUCCESS)
		fail_m(err, "OscLoadDependencies()");
	
	/* Set log levels. */
	OscLogSetConsoleLogLevel(EMERG);
	OscLogSetFileLogLevel(INFO);
	
	/* Set the camera registers to sane default values. */
	err = OscCamPresetRegs();
	if (err != SUCCESS)
		fail_m(err, "OscCamPresetRegs()");
	
	err = OscCamSetupPerspective(OSC_CAM_PERSPECTIVE_180DEG_ROTATE);
	if (err != SUCCESS)
		fail_m(err, "OscCamSetupPerspective()");
	
	/* This sets the sensor area to capture the picture from. */
	err = OscCamSetAreaOfInterest((OSC_CAM_MAX_IMAGE_WIDTH - WIDTH_CAPTURE) / 2, (OSC_CAM_MAX_IMAGE_HEIGHT - HEIGHT_CAPTURE) / 2, WIDTH_CAPTURE, HEIGHT_CAPTURE);
	if (err != SUCCESS)
		fail_m(err, "OscCamSetAreaOfInterest()");
	
	/* This set the exposure time to a reasonable value for the linghting in the sorter. */
	err = OscCamSetShutterWidth(opt_shutterWidth);
	if (err != SUCCESS)
		fail_m(err, "OscCamSetShutterWidth()");
	
	/* This sets the frame buffers to store the captured picture in. */
	err = OscCamSetFrameBuffer(0, sizeof frameBuffers[0], frameBuffers[0], TRUE);
	if (err != SUCCESS)
		fail_m(err, "OscCamSetFrameBuffer()");
	
	err = OscCamSetFrameBuffer(1, sizeof frameBuffers[1], frameBuffers[1], TRUE);
	if (err != SUCCESS)
		fail_m(err, "OscCamSetFrameBuffer()");
	
	err = OscCamCreateMultiBuffer(2, multiBufferIds);
	if (err != SUCCESS)
		fail_m(err, "OscCamCreateMultiBuffer()");
	
	err = OscCamSetupCapture(OSC_CAM_MULTI_BUFFER);
	if (err != SUCCESS)
		fail_m(err, "OscCamSetupCapture()");
	
	err = OscGpioTriggerImage();
	if (err != SUCCESS)
		fail_m(err, "OscGpioTriggerImage()");
	
	loop {
		uint8 * pFrameBuffer;
		static uint8 frameBuffer_color[WIDTH_CAPTURE * HEIGHT_CAPTURE / 4 * 3];
		
		err = OscCamSetupCapture(OSC_CAM_MULTI_BUFFER);
		if (err != SUCCESS)
			trace_e(err, "OscCamSetupCapture()");
	
		err = OscGpioTriggerImage();
		if (err != SUCCESS)
			trace_e(err, "OscGpioTriggerImage()");
		
		/* Here we wait for the picture to be availible in the frame buffer. */
		err = OscCamReadPicture(OSC_CAM_MULTI_BUFFER, &pFrameBuffer, 0, 0);
		if (err != SUCCESS)
			fail_m(err, "OscCamReadPicture()");
		
		if (opt_debayer) {
			err = OscVisFastDebayerRGB_raw(pFrameBuffer, WIDTH_CAPTURE, HEIGHT_CAPTURE, frameBuffer_color);
			if (err != SUCCESS)
				fail_m(err, "OscVisFastDebayerRGB_raw()");
			
			err = write(1, frameBuffer_color, sizeof frameBuffer_color);
			if (err != sizeof frameBuffer_color)
				fail_m(err, "write()");
		}
		else
		{
			err = write(1, pFrameBuffer, sizeof frameBuffers[0]);
			if (err != sizeof frameBuffers[0])
				fail_m(err, "write()");
		}
	}
	
	return SUCCESS;

fail:
	/* Unload the framework module dependencies */
	OscUnloadDependencies(hFramework, deps, length (deps));
	OscDestroy(&hFramework);
	
	return err;
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
	int32 opt_shutterWidth = 20000;
	bool opt_debayer = false;
	int i;
	
	for (i = 1; i < argc; i += 1)
	{
		if (strcmp(argv[i], "-d") == 0)
		{
			opt_debayer = true;
		}
		else if (strcmp(argv[i], "-s") == 0)
		{
			i += 1;
			if (i >= argc)
			{
				fprintf(stderr, "Error: -s needs an argument.\n");
				return 1;
			}
			opt_shutterWidth = atoi(argv[i]);
		}
		else if (strcmp(argv[i], "-h") == 0)
		{
			fprintf(stderr,
				"Usage: hello-world [ -h ] [ -d ] [ -s <shutter-width> ]\n"
				"    -h: Prints this help.\n"
				"    -d: Debayers the image.\n"
				"    -s <shutter-width>: Sets the shutter with in us.\n"
			);
		}
		else
		{
			fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
			return 1;
		}
	}
	
	/* Calls the main loop. This only returns on an error. */
	mainLoop(opt_shutterWidth, opt_debayer);
	
	return 0;
}
