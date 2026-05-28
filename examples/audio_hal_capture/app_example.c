/******************************************************************************
*
* Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
*
******************************************************************************/
#include "ameba_soc.h"
#include "example_audio_hal_capture.h"

CMD_TABLE_DATA_SECTION
const COMMAND_TABLE  audio_capture_test_cmd_table[] = {
	{
		"AudioHalCapture", CmdAudioHalCaptureTest
	},
};
