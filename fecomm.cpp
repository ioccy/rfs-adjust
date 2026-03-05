#pragma once
#include "framework.h"

// Handles the protocol
class DeviceCommand {
public:
    WORD packetLength = 0;      // Length of the whole packet (command, data and checksums included)
    std::string packetString;   // Binary string ready to be sent to device
    std::string commandData;    // Data binary string (without checksum), reserved for yet unknown commands
    DWORD dataDword = 0;        // Data binary string converted to DWORD, if possible

private:
    BYTE commandCode = 0;       // First byte of data packet
    BYTE lenLoByte = 0;         // Low byte of length
    BYTE lenHiByte = 0;         // High byte of length
    BYTE commandXOR = 0;        // Command checksum
    BYTE dataXOR = 0;           // Data checksum

public:
    // Constructors to serialize command to device
    DeviceCommand(BYTE cmd, std::string data) {
        commandCode = cmd;
        commandData = data;
        packetLength = commandData.size() == 0 ? 4 : (WORD)commandData.size() + 5;
        lenLoByte = LOBYTE(packetLength);
        lenHiByte = HIBYTE(packetLength);
        commandXOR = commandCode ^ lenLoByte ^ lenHiByte;
        if (commandData.size() <= sizeof(DWORD)) dataDword = StringToDword(commandData);
        dataXOR = CheckSum(commandData);
        packetString = { (char)commandCode, (char)lenLoByte, (char)lenHiByte, (char)commandXOR };
        packetString += commandData.size() ? (commandData + (char)dataXOR) : "";
    }

    DeviceCommand(void) {};
    DeviceCommand(BYTE cmd) : DeviceCommand(cmd, "") {}
    DeviceCommand(BYTE cmd, DWORD value) : DeviceCommand(cmd, DwordToString(value)) {}

    // Constructor to parse device response
    DeviceCommand(std::string response) {
        ParseCommandString(response.substr(0, 4));
        if (packetLength <= 4) return;
        ParseDataString(response.substr(4));
    }

private:
    void ParseCommandString(std::string cmdString) {
        if (cmdString.size() < 4) return;
        commandCode = cmdString.at(0);
        lenLoByte = cmdString.at(1);
        lenHiByte = cmdString.at(2);
        packetLength = MAKEWORD(lenLoByte, lenHiByte);
        commandXOR = commandCode ^ lenLoByte ^ lenHiByte;
        if (cmdString.at(3) != commandXOR) throw std::runtime_error("Bad command XOR");
    }

    void ParseDataString(std::string dataString) {
        if (dataString.size() < 2) return;
        commandData = std::string(dataString.substr(0, dataString.size() - 1));
        dataDword = (commandData.size() <= sizeof(DWORD)) ? StringToDword(commandData) : 0;
        if ((BYTE)dataString.back() != CheckSum(commandData)) throw std::runtime_error("Bad data XOR");
    }

    BYTE CheckSum(std::string str) {
        BYTE b = 0;
        for (BYTE c : str) b ^= c;
        return b;
    }

    static DWORD StringToDword(std::string str) {
        if (str.length() < 4) return 0;
        return ((BYTE)str[0] << 24) |
            ((BYTE)str[1] << 16) |
            ((BYTE)str[2] << 8) |
            (BYTE)str[3];
    }

    static std::string DwordToString(DWORD value) {
        std::string result(4, '\0');
        result[0] = (value >> 24) & 0xFF;
        result[1] = (value >> 16) & 0xFF;
        result[2] = (value >> 8) & 0xFF;
        result[3] = (value) & 0xFF;
        return result;
    }
};

// Handles communications
class DeviceConnect {
private:
    HANDLE hCom = NULL;

    HANDLE OpenComPort(const char* portName)
    {
        HANDLE hComm = CreateFileA(
            portName,                     /* Port name (e.g., "COM1", "\\\\.\\COM10" for COM10+) */
            GENERIC_READ | GENERIC_WRITE, /* Read/Write access */
            0,                            /* No sharing */
            NULL,                         /* Default security */
            OPEN_EXISTING,                /* Open existing port only */
            0,                            /* Non-overlapped I/O */
            NULL                          /* Null template */
        );

        // Error opening port
        if (hComm == INVALID_HANDLE_VALUE)
            return INVALID_HANDLE_VALUE;

        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

        if (!GetCommState(hComm, &dcbSerialParams))
        {
            // Error getting COM state
            CloseHandle(hComm);
            return INVALID_HANDLE_VALUE;
        }

        // 9600 8N1
        dcbSerialParams.BaudRate = CBR_9600;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;

        if (!SetCommState(hComm, &dcbSerialParams))
        {
            // Error setting COM state
            CloseHandle(hComm);
            return INVALID_HANDLE_VALUE;
        }

        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 0; // For unknown length reads
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;

        if (!SetCommTimeouts(hComm, &timeouts))
        {
            // Error setting timeouts
            CloseHandle(hComm);
            return INVALID_HANDLE_VALUE;
        }

        return hComm;
    }

public:
    static constexpr BYTE FE_SAVE = 0x2C;
    static constexpr BYTE FE_POLL = 0x2D;
    static constexpr BYTE FE_SET = 0x2E;

    DeviceConnect(const char* string) {
        hCom = OpenComPort(string);
        if (hCom == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot open COM port");
    }

    ~DeviceConnect(void) {
        CloseHandle(hCom);
    }

    DWORD FEPoll(void) const {
        DeviceCommand command(FE_POLL);
        DeviceCommand response = CustomCommand(command);
        if (!response.packetLength) throw std::runtime_error("No response");
        return response.dataDword;
    }

    void FESet(DWORD value) const {
        DeviceCommand command(FE_SET, value);
        CustomCommand(command);
    }

    void FESave(DWORD value) const {
        DeviceCommand command(FE_SAVE, value);
        CustomCommand(command);
    }

    DeviceCommand CustomCommand(DeviceCommand command) const {

        DWORD bytesCount;
        char readBuffer[16];
        std::string received;

        PurgeComm(hCom, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

        if (!WriteFile(hCom, command.packetString.c_str(), command.packetLength, &bytesCount, NULL) ||
            (command.packetLength != bytesCount)) throw std::runtime_error("Error sending command");

        while (ReadFile(hCom, readBuffer, sizeof(readBuffer), &bytesCount, NULL) && bytesCount > 0 && received.size() < (MAXWORD + 5))
            received.append(readBuffer, bytesCount);

        DeviceCommand response(received);
        return response;
    }
};
