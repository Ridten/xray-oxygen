﻿////////////////////////////////////////////////////////////////////////////
//	Module 		: date_time.h
//	Created 	: 23.05.2018
//	Author		: ForserX
//	Description : Ingame date and time generator. 
////////////////////////////////////////////////////////////////////////////
#pragma once

namespace GameTime
{
ENGINE_API	u64		generate_time(u32 years, u32 months, u32 days, u32 hours, u32 minutes, u32 seconds, u32 milliseconds = 0);
ENGINE_API	void	split_time(u64 time, u32 &years, u32 &months, u32 &days, u32 &hours, u32 &minutes, u32 &seconds, u32 &milliseconds);
			u32		extra_day_count(u32 years);
}

// Мне лень везде писать название namespace в вызовах.
using namespace GameTime;
