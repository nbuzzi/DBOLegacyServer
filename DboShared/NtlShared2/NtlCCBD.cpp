#include "stdafx.h"
#include "NtlCCBD.h"



bool IsCCBDBossStage(BYTE byStage)
{
	// Boss stage every 5 floors (5, 10, 15, 20, etc.)
	// Dynamic calculation supports unlimited stages up to CCBD_MAX_STAGE
	return (byStage > 0 && byStage % 5 == 0 && byStage <= CCBD_MAX_STAGE);
}
