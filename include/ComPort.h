#pragma once

#include <windows.h>
#include <iostream>
#include <thread>
#include <functional>
#include <string>
#include <vector>
#include <time.h>

class ComPort {
public:
    ComPort(void) 
        :portHandle_(INVALID_HANDLE_VALUE) {
    }

    ComPort(const std::string& portName, size_t baud, std::function<void(char)> callback)
        : callback_(callback), portHandle_(INVALID_HANDLE_VALUE) {
        OpenPort(portName, baud);
        if (portHandle_ != INVALID_HANDLE_VALUE) {
            listenerThread_ = std::thread(&ComPort::Listen, this);
        }
    }

    ~ComPort() {
        if (portHandle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(portHandle_);
        }
        if (listenerThread_.joinable()) {
            listenerThread_.join();
        }
    }

    bool open(const std::string& portName, size_t baud, std::function<void(char)> callback) {
        if(portHandle_ != INVALID_HANDLE_VALUE) 
            return false;
        
        callback_ = callback;
        portHandle_ = INVALID_HANDLE_VALUE;
        OpenPort(portName, baud);

        if (portHandle_ != INVALID_HANDLE_VALUE) {
            listenerThread_ = std::thread(&ComPort::Listen, this);
            return true;
        }

        return false;
    }

    bool is_opened(void) {
        return portHandle_ != INVALID_HANDLE_VALUE ? true : false;
    }

    void close()
    {
        if (portHandle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(portHandle_);
            portHandle_ = INVALID_HANDLE_VALUE;
        }
        if (listenerThread_.joinable()) {
            listenerThread_.join();
        }    
    }


    // Метод для записи данных в COM-порт
    bool Write(uint8_t buf[], size_t len) {
        if (portHandle_ == INVALID_HANDLE_VALUE) {
            std::cerr << "Port is not open" << std::endl;
            return false;
        }

        OVERLAPPED overlapped = {0};
        overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

        if (overlapped.hEvent == nullptr) {
            std::cerr << "Failed to create event for writing" << std::endl;
            return false;
        }

        DWORD bytesWritten;
        if (!WriteFile(portHandle_, buf, len, &bytesWritten, &overlapped)) {
            if (GetLastError() == ERROR_IO_PENDING) {
                // Ожидание завершения асинхронной записи
                if (WaitForSingleObject(overlapped.hEvent, INFINITE) == WAIT_OBJECT_0) {
                    if (!GetOverlappedResult(portHandle_, &overlapped, &bytesWritten, FALSE)) {
                        std::cerr << "Write operation failed" << std::endl;
                        CloseHandle(overlapped.hEvent);
                        return false;
                    }
                }
            } else {
                std::cerr << "WriteFile failed" << std::endl;
                CloseHandle(overlapped.hEvent);
                return false;
            }
        }

        CloseHandle(overlapped.hEvent);
        return true;
    }

    std::vector<std::string> GetCOMPortsFromRegistry() {
        std::vector<std::string> ports;
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char valueName[256];
            char valueData[256];
            DWORD valueNameLen = sizeof(valueName);
            DWORD valueDataLen = sizeof(valueData);
            DWORD index = 0;
    
            while (RegEnumValueA(hKey, index, valueName, &valueNameLen, NULL, NULL, (LPBYTE)valueData, &valueDataLen) == ERROR_SUCCESS) {
                ports.push_back(valueData);
                valueNameLen = sizeof(valueName);
                valueDataLen = sizeof(valueData);
                index++;
            }
            RegCloseKey(hKey);
        }
        return ports;
    }

    bool Write(const std::string& data) {
        if (portHandle_ == INVALID_HANDLE_VALUE) {
            std::cerr << "Port is not open" << std::endl;
            return false;
        }

        OVERLAPPED overlapped = {0};
        overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

        if (overlapped.hEvent == nullptr) {
            std::cerr << "Failed to create event for writing" << std::endl;
            return false;
        }

        DWORD bytesWritten;
        if (!WriteFile(portHandle_, data.c_str(), data.size(), &bytesWritten, &overlapped)) {
            if (GetLastError() == ERROR_IO_PENDING) {
                // Ожидание завершения асинхронной записи
                if (WaitForSingleObject(overlapped.hEvent, INFINITE) == WAIT_OBJECT_0) {
                    if (!GetOverlappedResult(portHandle_, &overlapped, &bytesWritten, FALSE)) {
                        std::cerr << "Write operation failed" << std::endl;
                        CloseHandle(overlapped.hEvent);
                        return false;
                    }
                }
            } else {
                std::cerr << "WriteFile failed" << std::endl;
                CloseHandle(overlapped.hEvent);
                return false;
            }
        }

        CloseHandle(overlapped.hEvent);
        return true;
    }

    void Write(const unsigned char b) {
        if (portHandle_ == INVALID_HANDLE_VALUE) {
            std::cerr << "Port is not open" << std::endl;
            return ;
        }

        OVERLAPPED overlapped = {0};
        overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

        if (overlapped.hEvent == nullptr) {
            std::cerr << "Failed to create event for writing" << std::endl;
            return ;
        }

        DWORD bytesWritten;
        if (!WriteFile(portHandle_, &b, 1, &bytesWritten, &overlapped)) {
            if (GetLastError() == ERROR_IO_PENDING) {
                // Ожидание завершения асинхронной записи
                if (WaitForSingleObject(overlapped.hEvent, INFINITE) == WAIT_OBJECT_0) {
                    if (!GetOverlappedResult(portHandle_, &overlapped, &bytesWritten, FALSE)) {
                        std::cerr << "Write operation failed" << std::endl;
                        CloseHandle(overlapped.hEvent);
                        return ;
                    }
                }
            } else {
                std::cerr << "WriteFile failed" << std::endl;
                CloseHandle(overlapped.hEvent);
                return ;
            }
        }

        CloseHandle(overlapped.hEvent);
    }
private:
    void OpenPort(const std::string& portName, size_t baud) {
        portHandle_ = CreateFile(portName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        if (portHandle_ == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to open port: " << portName << std::endl;
            return;
        }

        DCB dcb = {0};
        if (!GetCommState(portHandle_, &dcb)) {
            std::cerr << "Failed to get comm state" << std::endl;
            return;
        }

        dcb.BaudRate = baud;
        dcb.ByteSize = 8;
        dcb.StopBits = ONESTOPBIT;
        dcb.Parity = NOPARITY;

        if (!SetCommState(portHandle_, &dcb)) {
            std::cerr << "Failed to set comm state" << std::endl;
            return;
        }

        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout = MAXDWORD;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.ReadTotalTimeoutConstant = 0;
        timeouts.WriteTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 0;

        if (!SetCommTimeouts(portHandle_, &timeouts)) {
            std::cerr << "Failed to set comm timeouts" << std::endl;
            return;
        }

        if (!SetCommMask(portHandle_, EV_RXCHAR | EV_ERR)) {
            std::cerr << "Failed to set comm mask" << std::endl;
            return;
        }
    }

    void Listen() {
        OVERLAPPED overlapped = {0};
        overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

        if (overlapped.hEvent == nullptr) {
            std::cerr << "Failed to create event" << std::endl;
            return;
        }

        char buffer[1];
        DWORD dwEventMask;
        DWORD dwRead;

        while (true) {
            if (!WaitCommEvent(portHandle_, &dwEventMask, &overlapped)) {
                if (GetLastError() == ERROR_IO_PENDING) {
                    WaitForSingleObject(overlapped.hEvent, INFINITE);
                } else {
                    std::cerr << "WaitCommEvent failed" << std::endl;
                    break;
                }
            }

            if (dwEventMask & EV_ERR) {
                std::cout << "EV_ERR";
                continue;
            }

            if (dwEventMask & EV_RXCHAR) {
                if (ReadFile(portHandle_, buffer, 1, &dwRead, &overlapped)) {
                    if (dwRead > 0) {
                        callback_(buffer[0]);
                    }
                } else {
                    if (GetLastError() == ERROR_IO_PENDING) {
                        WaitForSingleObject(overlapped.hEvent, INFINITE);
                        if (GetOverlappedResult(portHandle_, &overlapped, &dwRead, FALSE)) {
                            if (dwRead > 0) {
                                callback_(buffer[0]);
                            }
                        }
                    } else {
                        std::cerr << "ReadFile failed" << std::endl;
                        break;
                    }
                }
            }
           
        }

        CloseHandle(overlapped.hEvent);
    }

    std::function<void(char)> callback_;
    HANDLE portHandle_;
    std::thread listenerThread_;
};