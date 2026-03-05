#include "framework.h"

std::string DwordToHex(DWORD value) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%02X %02X %02X %02X",
        (value >> 24) & 0xFF,
        (value >> 16) & 0xFF,
        (value >> 8) & 0xFF,
        value & 0xFF);
    return buf;
}

DWORD HexToDword(std::string s) {
    std::string hex;
    hex.reserve(8);
    for (char c : s) {
        if (c != ' ' && c != '\0' && hex.size() < 8)
            hex += c;
    }
    return (DWORD)strtoul(hex.c_str(), nullptr, 16);
}

double DwordToFreq(DWORD offset) {
    return round(((signed long)offset / (double)MAXDWORD * 2.0 * 383.0) * 1e12) / 1e12;
}

std::string FormatFrequency(double hz) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.9f", hz);
    std::string s = buf;

    // split integer and fractional parts
    size_t dot = s.find('.');
    std::string intPart = s.substr(0, dot);
    std::string fracPart = s.substr(dot + 1);

    // handle negative
    bool negative = intPart[0] == '-';
    if (negative) intPart = intPart.substr(1);

    // pad integer part with leading zeros to multiple of 3
    while (intPart.size() % 3 != 0) intPart = "0" + intPart;

    // group integer part by 3
    std::string intGrouped;
    for (size_t i = 0; i < intPart.size(); i++) {
        if (i > 0 && i % 3 == 0) intGrouped += ' ';
        intGrouped += intPart[i];
    }

    // group fractional part by 3
    std::string fracGrouped;
    for (size_t i = 0; i < fracPart.size(); i++) {
        if (i > 0 && i % 3 == 0) fracGrouped += ' ';
        fracGrouped += fracPart[i];
    }

    return (negative ? "-" : "+") + intGrouped + " . " + fracGrouped;
}

std::string BytesToHex(const std::string data) {
    static const char hex[] = "0123456789ABCDEF";
    std::string result;
    result.reserve(data.size() * 2);

    for (unsigned char byte : data) {
        result += hex[byte >> 4];    // high nibble
        result += hex[byte & 0x0F]; // low nibble
        // result += " ";
    }

    return result;
}

std::string HexToBytes(const std::string hex)
{
    std::string result; 
    for (size_t i = 0; i + 1 < hex.size(); i += 2)
        result += (char)std::stoul(hex.substr(i, 2), nullptr, 16);

    return result;
}