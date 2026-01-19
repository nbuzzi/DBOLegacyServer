#pragma once

#include "NtlString.h"
#include <cctype>
#include <algorithm>

// Centralized channel utility for feature gating based on channel names
// This helps optimize performance by skipping event/arena logic on other channels
class CChannelUtility
{
public:
	// Check if a channel name contains a specific substring (case-insensitive)
	static bool ChannelNameContains(const CNtlString& channelName, const char* substring)
	{
		if (!channelName.c_str() || channelName.c_str()[0] == '\0' || !substring || *substring == '\0')
			return false;

		std::string channelLower = ToLower(channelName);
		std::string substringLower = ToLower(substring);

		return channelLower.find(substringLower) != std::string::npos;
	}

	// Check if current channel is an Arena channel
	static bool IsArenaChannel(const CNtlString& channelName)
	{
		return ChannelNameContains(channelName, "ARENA");
	}

	// Check if current channel is an Events channel
	static bool IsEventsChannel(const CNtlString& channelName)
	{
		return ChannelNameContains(channelName, "EVENTS");
	}

	// Check if current channel is the Dojo/Budokai channel
	static bool IsDojoChannel(const CNtlString& channelName)
	{
		return ChannelNameContains(channelName, "DOJO");
	}

	// Check if current channel is a custom drop event channel (usually EVENTS)
	static bool IsCustomDropEventChannel(const CNtlString& channelName)
	{
		// Custom drop events typically run on EVENTS channel
		// Can be configured to allow other channels if needed
		return IsEventsChannel(channelName);
	}

	// Generic helper: check if channel matches any of the specified names
	static bool ChannelMatchesAny(const CNtlString& channelName, const char** names, size_t count)
	{
		for (size_t i = 0; i < count; ++i)
		{
			if (ChannelNameContains(channelName, names[i]))
				return true;
		}
		return false;
	}

private:
	// Convert string to lowercase for case-insensitive comparison
	static std::string ToLower(const CNtlString& str)
	{
		std::string result;
		const char* p = str.c_str();
		while (*p)
		{
			result += (char)std::tolower((unsigned char)*p);
			++p;
		}
		return result;
	}

	static std::string ToLower(const char* str)
	{
		std::string result;
		while (*str)
		{
			result += (char)std::tolower((unsigned char)*str);
			++str;
		}
		return result;
	}
};
