//---------------------------------------------------------------------------
//
// SCSI Target Emulator PiSCSI
// for Raspberry Pi
//
// Copyright (C) 2022 Uwe Seimet
//
//---------------------------------------------------------------------------

#include "mocks.h"

TEST(ScsiHostBridgeTest, Inquiry)
{
	TestInquiry(SCBR, device_type::COMMUNICATIONS, scsi_level::SCSI_2, "PiSCSI  RASCSI BRIDGE   ", 0x27, false);
}
