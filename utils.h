#pragma once

#include "framework.h"

std::string DwordToHex(DWORD);				// 0xDEADBEEF to "DE AD BE EF"
DWORD HexToDword(std::string);				// "DE AD BE EF" to 0xDEADBEEF
double DwordToFreq(DWORD);					// Calculates frequency offset in Hz
std::string FormatFrequency(double);		// Shows double value as easy-to-read string
std::string BytesToHex(const std::string);	// "\xDE\xAD\xBE\xEF" to "DEADBEEF"
std::string HexToBytes(const std::string);  // "DEADBEEF" to "\xDE\xAD\xBE\xEF"