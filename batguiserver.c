//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2013 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

#define _GNU_SOURCE

#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "backgroundLayer.h"
#include "imageLayer.h"
#include "key.h"
#include "loadpng.h"

#include "bcm_host.h"

//-------------------------------------------------------------------------

#define NDEBUG

//-------------------------------------------------------------------------

#define MAX_STR_SIZE 100

const char *program = NULL;
const char *programpath = NULL;

//-------------------------------------------------------------------------

volatile bool run = true;

//-------------------------------------------------------------------------

static void
signalHandler(
    int signalNumber)
{
    switch (signalNumber)
    {
    case SIGINT:
    case SIGTERM:

        run = false;
        break;
    };
}

//-------------------------------------------------------------------------

void usage(void)
{
    fprintf(stderr, "Usage: %s ", program);
    fprintf(stderr, "[-b <RGBA>] [-d <number>] [-l <layer>] ");
    fprintf(stderr, "[-x <offset>] [-y <offset>] <file.png>\n");
    fprintf(stderr, "    -b - set background colour 16 bit RGBA\n");
    fprintf(stderr, "         e.g. 0x000F is opaque black\n");
    fprintf(stderr, "    -d - Raspberry Pi display number\n");
    fprintf(stderr, "    -l - DispmanX layer number\n");
    fprintf(stderr, "    -x - offset (pixels from the left)\n");
    fprintf(stderr, "    -y - offset (pixels from the top)\n");
    fprintf(stderr, "    -t - timeout in ms\n");
    fprintf(stderr, "    -n - non-interactive mode\n");

    exit(EXIT_FAILURE);
}

//-------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    uint16_t background = 0x0;
    int32_t layer = 3000;
    uint32_t displayNumber = 0;
    int32_t xOffset = 0;
    int32_t yOffset = 0;
    int32_t batterysize = 0;
    uint32_t timeout = 0;

    bool interactive = true;
    bool animate = false;
    bool pipemsg = true;

    program = basename(argv[0]);
	programpath = dirname(argv[0]);
    //---------------------------------------------------------------------

    int opt = 0;

	//printf("basename =%s\r\n", program);
	//printf("dirname =%s\r\n", programpath);
	
	//exit(0);

    while ((opt = getopt(argc, argv, "s:b:d:l:x:y:t:nap")) != -1)
    {
        switch(opt)
        {
		case 'p':
			pipemsg = true;
			break;
		case 'a':
			animate = true;
			break;
		case 's':
			batterysize = strtol(optarg, NULL, 10);
			if (batterysize > 2)
				batterysize=0;
			break;
        	case 'b':

            		background = strtol(optarg, NULL, 16);
            		break;

        case 'd':

            displayNumber = strtol(optarg, NULL, 10);
            break;

        case 'l':

            layer = strtol(optarg, NULL, 10);
            break;

        case 'x':

            xOffset = strtol(optarg, NULL, 10);

            break;

        case 'y':

            yOffset = strtol(optarg, NULL, 10);

            break;
        
        case 't':

            timeout = atoi(optarg);
            break;

        case 'n':

            interactive = false;
            break;

        default:

            //usage();
            break;
        }
    }


    IMAGE_LAYER_T imageLayer[5];

    //const char *imagePath = argv[optind];
	const char batteryfolder[3][10]={"small", "medium", "large"};
	char imagePath[256];
	int i;
	
	i=0;
	//for (i=0; i < 5; i++)
	//{
		sprintf(imagePath, "%s/icons/%s/battery%d.png", programpath, batteryfolder[batterysize], i * 25);
		
		// Load image from path
		if (loadPng(&(imageLayer[i].image), imagePath) == false)
		{
			fprintf(stderr, "unable to load %s\n", imagePath);
			exit(EXIT_FAILURE);
		}
	//}

    //---------------------------------------------------------------------

    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        perror("installing SIGINT signal handler");
        exit(EXIT_FAILURE);
    }

    //---------------------------------------------------------------------

    if (signal(SIGTERM, signalHandler) == SIG_ERR)
    {
        perror("installing SIGTERM signal handler");
        exit(EXIT_FAILURE);
    }

    //---------------------------------------------------------------------

    bcm_host_init();

    //---------------------------------------------------------------------

    DISPMANX_DISPLAY_HANDLE_T display
        = vc_dispmanx_display_open(displayNumber);
    assert(display != 0);

    //---------------------------------------------------------------------

    DISPMANX_MODEINFO_T info;
    int result = vc_dispmanx_display_get_info(display, &info);
    assert(result == 0);

    //---------------------------------------------------------------------

    BACKGROUND_LAYER_T backgroundLayer;

    if (background > 0)
    {
        initBackgroundLayer(&backgroundLayer, background, 0);
    }

	//for (i=0; i<5; i++)
	//{
		createResourceImageLayer(&imageLayer[0], layer + 0);
	//}
	
    //---------------------------------------------------------------------

    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
    assert(update != 0);

    if (background > 0)
    {
        addElementBackgroundLayer(&backgroundLayer, display, update);
    }
/*
    if (xOffsetSet == false)
    {
        xOffset = (info.width - imageLayer.image.width) / 2;
    }

    if (yOffsetSet == false)
    {
        yOffset = (info.height - imageLayer.image.height) / 2;
    }
*/
	//for(i=0; i<5; i++)
	//{
		addElementImageLayerOffset(&imageLayer[0],
								   xOffset,
								   yOffset,
								   display,
								   update);
	//}
	
    result = vc_dispmanx_update_submit_sync(update);
    assert(result == 0);

    //---------------------------------------------------------------------

    //int32_t step = 1;
    uint32_t currentTime = 0;

    // Sleep for 10 milliseconds every run-loop
    const int sleepMilliseconds = 10;
	
	int batcnt=0;
	int backbatcnt=0;

	char str_read[MAX_STR_SIZE];
	char ch;
	//int Exit=0;
	int cmdcnt=0;
	
	memset(str_read, 0, sizeof(str_read));
	
    while (run)
    {
        int c = 0;
        bool moveLayer = false;
	    bool changeLayer = false;		
		
		if (pipemsg) // python to c by pipe
		{
				ch = fgetc(stdin);
				if (ch!=EOF)
				{
					str_read[cmdcnt++] = ch;
					str_read[cmdcnt] = 0;

					if (strcmp(str_read, "[b:0]") == 0)
					{
					  cmdcnt = 0;
					  
					  backbatcnt = batcnt;
					  batcnt = 0;
					  changeLayer = true;
					}
					else  if (strcmp(str_read, "[b:25]") == 0)
					{
					  cmdcnt = 0;
					  
					  backbatcnt = batcnt;
					  batcnt = 1;
					  changeLayer = true;
					}
					else  if (strcmp(str_read, "[b:50]") == 0)
					{
					  cmdcnt = 0;
					  
					  backbatcnt = batcnt;
					  batcnt = 2;
					  changeLayer = true;
					}
					else  if (strcmp(str_read, "[b:75]") == 0)
					{
					  cmdcnt = 0;
					  
					  backbatcnt = batcnt;
					  batcnt = 3;
					  changeLayer = true;
					}
					else  if (strcmp(str_read, "[b:100]") == 0)
					{
					  cmdcnt = 0;
					  
					  backbatcnt = batcnt;
					  batcnt = 4;
					  changeLayer = true;
					}
					else if (strcmp(str_read, "[exit]") == 0)
					{
					  run = false;
					  cmdcnt = 0;
					}		

					if (changeLayer)
					{
						sprintf(imagePath, "%s/icons/%s/battery%d.png", programpath, batteryfolder[batterysize], batcnt * 25);
						
						// Load image from path
						if (loadPng(&(imageLayer[batcnt].image), imagePath) == false)
						{
							fprintf(stderr, "unable to load %s\n", imagePath);
							exit(EXIT_FAILURE);
						}
						
						createResourceImageLayer(&imageLayer[batcnt], layer + batcnt);
						
						update = vc_dispmanx_update_start(0);
						assert(update != 0);		
						/*void changeSourceImageLayer(IMAGE_LAYER_T *il, DISPMANX_UPDATE_HANDLE_T update);
						==> not ok
										void
						changeSourceAndUpdateImageLayer(
							IMAGE_LAYER_T *il);
						*/
						//changeSourceImageLayer(&imageLayer[batcnt], update);

					   addElementImageLayerOffset(&imageLayer[batcnt],
												   xOffset,
												   yOffset,
												   display,
												   update);		
												   
						result = vc_dispmanx_update_submit_sync(update);
						assert(result == 0);

						destroyImageLayer(&imageLayer[backbatcnt]);
						changeLayer = false;
					
					}	
				}
		}
		else
		{
			if (interactive && keyPressed(&c))
			{
				c = tolower(c);


				
				switch (c)
				{
				case 27:
					run = false;
					if (animate)
						animate = false;
					
			
					break;
					
				case 'c':
					changeLayer = true;
					
					backbatcnt = batcnt;

					batcnt = (batcnt + 1) % 5;
					break;
				}

				if (moveLayer)
				{
					update = vc_dispmanx_update_start(0);
					assert(update != 0);

					moveImageLayer(&imageLayer[batcnt], xOffset, yOffset, update);

					result = vc_dispmanx_update_submit_sync(update);
					assert(result == 0);
				}
			}
			
			if (animate)
			{
				backbatcnt = batcnt;

				batcnt = (batcnt + 1) % 5;
			}
			
			if (changeLayer || animate)
			{		
				sprintf(imagePath, "%s/icons/%s/battery%d.png", programpath, batteryfolder[batterysize], batcnt * 25);
				
				// Load image from path
				if (loadPng(&(imageLayer[batcnt].image), imagePath) == false)
				{
					fprintf(stderr, "unable to load %s\n", imagePath);
					exit(EXIT_FAILURE);
				}
				
				createResourceImageLayer(&imageLayer[batcnt], layer + batcnt);
				
				update = vc_dispmanx_update_start(0);
				assert(update != 0);		
				/*void changeSourceImageLayer(IMAGE_LAYER_T *il, DISPMANX_UPDATE_HANDLE_T update);
				==> not ok
								void
				changeSourceAndUpdateImageLayer(
					IMAGE_LAYER_T *il);
				*/
				//changeSourceImageLayer(&imageLayer[batcnt], update);

			   addElementImageLayerOffset(&imageLayer[batcnt],
										   xOffset,
										   yOffset,
										   display,
										   update);		
										   
				result = vc_dispmanx_update_submit_sync(update);
				assert(result == 0);

				destroyImageLayer(&imageLayer[backbatcnt]);
			
			}
			

			//---------------------------------------------------------------------
			
			if (animate)
			{
				usleep(1000000);
			}
			else
			{
				usleep(sleepMilliseconds * 1000);
			}
			
			currentTime += sleepMilliseconds;
			if (timeout != 0 && currentTime >= timeout) {
				run = false;
			}
			
		}
	
	
	
	
    } // end of loop while

    //---------------------------------------------------------------------

    keyboardReset();

    //---------------------------------------------------------------------

    if (background > 0)
    {
        destroyBackgroundLayer(&backgroundLayer);
    }
	
	//for (i=0; i<5; i++)
	//{
		destroyImageLayer(&imageLayer[batcnt]);
	//}
	
    //---------------------------------------------------------------------

    result = vc_dispmanx_display_close(display);
    assert(result == 0);

    //---------------------------------------------------------------------

    return 0;
}

