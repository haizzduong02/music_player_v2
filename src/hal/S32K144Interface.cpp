#include "hal/S32K144Interface.h"
#include "utils/Logger.h"
#include <cstring>
#include <iostream>
#include <chrono>

S32K144Interface::S32K144Interface()
    : socketFd_(-1), port_(0), connected_(false), running_(false), connecting_(false), currentButton_(-1), currentAdc_(0.0f)
{
    Logger::info("S32K144Interface initialized");
}

S32K144Interface::~S32K144Interface()
{
    stopListening();
    close();
    Logger::info("S32K144Interface destroyed");
}

bool S32K144Interface::initialize(const std::string &port, int baudRate)
{
    ipAddress_ = port;
    port_ = baudRate;
    Logger::info("S32K144Interface configured for " + ipAddress_ + ":" + std::to_string(port_));
    return true;
}

bool S32K144Interface::connect()
{
    if (connected_) return true;

    Logger::info("Attempting to connect to S32K144 at " + ipAddress_ + ":" + std::to_string(port_));

    socketFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd_ < 0)
    {
        Logger::error("Failed to create socket");
        return false;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    if (inet_pton(AF_INET, ipAddress_.c_str(), &serverAddr.sin_addr) <= 0)
    {
        Logger::error("Invalid address/ Address not supported");
        ::close(socketFd_);
        return false;
    }

    if (::connect(socketFd_, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        // Don't log error every second to avoid spam
        // Logger::error("Connection Failed"); 
        ::close(socketFd_);
        return false;
    }

    Logger::info("Connected to S32K144 via TCP");
    return true;
}

void S32K144Interface::close()
{
    if (socketFd_ != -1)
    {
        ::close(socketFd_);
        socketFd_ = -1;
    }
    connected_ = false;
}

bool S32K144Interface::isConnected() const
{
    return connected_;
}

bool S32K144Interface::sendCommand(const std::string &command)
{
    if (!connected_) return false;
    return sendRaw(command) > 0;
}

std::string S32K144Interface::readData()
{
    // Not used in listener loop
    return "";
}

void S32K144Interface::sendVolume(float volume)
{
    // S32K firmware doesn't seem to support VOL cmd yet, but we keep format
    std::string cmd = "VOL:" + std::to_string(volume) + "\n";
    sendCommand(cmd);
}

void S32K144Interface::displayText(const std::string &text)
{
    std::string cmd = "TXT:" + text + "\n";
    sendCommand(cmd);
}

float S32K144Interface::readADC()
{
    return currentAdc_;
}

int S32K144Interface::readButton()
{
    return currentButton_;
}

HardwareEvent S32K144Interface::getLastEvent() const
{
    return lastEvent_;
}

void S32K144Interface::startListening()
{
    if (running_) return;
    running_ = true;
    listenerThread_ = std::thread(&S32K144Interface::listenerLoop, this);
}

void S32K144Interface::stopListening()
{
    running_ = false;
    if (listenerThread_.joinable())
    {
        listenerThread_.join();
    }
}

void S32K144Interface::listenerLoop()
{
    char buffer[1024];
    while (running_)
    {
        if (!connected_)
        {
            if (connect())
            {
                connected_ = true;
            }
            else
            {
                // Retry delay
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }
        }

        int bytesRead = recv(socketFd_, buffer, 1024, 0);
        if (bytesRead > 0)
        {
            std::string data(buffer, bytesRead);
            // Append new data to buffer
            receiveBuffer_.append(buffer, bytesRead);
            
            // Process all complete lines
            size_t pos = 0;
            while ((pos = receiveBuffer_.find("\n")) != std::string::npos) {
                std::string token = receiveBuffer_.substr(0, pos);
                // Remove \r if present
                if (!token.empty() && token.back() == '\r') {
                    token.pop_back();
                }
                
                HardwareEvent event = parseData(token);
                if (event.command != HardwareCommand::UNKNOWN) {
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        lastEvent_ = event;
                    }
                    notify();
                }
                // Remove processed line from buffer
                receiveBuffer_.erase(0, pos + 1);
            }
        }
        else
        {
            Logger::warn("S32K144 Disconnected");
            close();
            connected_ = false;
            // Sleep to avoid tight loop on error
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

HardwareEvent S32K144Interface::parseData(const std::string &data)
{
    HardwareEvent event;
    event.command = HardwareCommand::UNKNOWN;
    event.value = 0.0f;
    
    // Protocol from firmware:
    // "cmd:next"
    // "cmd:pause"
    // "cmd:play"
    // "VR: <int>"
    

    if (data.find("cmd:next") != std::string::npos)
    {
        event.command = HardwareCommand::NEXT;
    }
    else if (data.find("cmd:prev") != std::string::npos || data.find("cmd:previous") != std::string::npos)
    {
        event.command = HardwareCommand::PREVIOUS;
    }
    else if (data.find("cmd:pause") != std::string::npos)
    {
        event.command = HardwareCommand::PAUSE;
    }
    else if (data.find("cmd:play") != std::string::npos)
    {
        event.command = HardwareCommand::PLAY;
    }
    else if (data.find("VR:") != std::string::npos)
    {
        event.command = HardwareCommand::ADC_UPDATE;
        try
        {
            // Format "VR: 1234"
            size_t startInfo = data.find(":");
            if (startInfo != std::string::npos) {
                float val = std::stof(data.substr(startInfo + 1));
                // Normalize 0-4095 to 0.0-1.0
                float newValue = val / 4095.0f;
                
                // Filter jitter/noise: only update if change is > 0.5%
                // or if it's the first update (currentAdc_ might be 0)
                if (std::abs(newValue - currentAdc_) > 0.005f) {
                    event.value = newValue;
                    currentAdc_ = newValue;
                } else {
                    // Ignore insignificant change
                    event.command = HardwareCommand::UNKNOWN;
                }
            }
        }
        catch (...)
        {
            event.value = 0.0f;
        }
    }
    else if (data.find("BTN:") == 0) // Legacy/Mock support
    {
        event.command = HardwareCommand::BUTTON_PRESS;
        try { event.value = std::stof(data.substr(4)); currentButton_ = (int)event.value; } catch(...) {}
    }

    return event;
}

void S32K144Interface::connectLoop()
{
    // Not used, merged into listenerLoop
}

bool S32K144Interface::configureSerial(int fd, int baudRate)
{
    return true;
}

int S32K144Interface::sendRaw(const std::string &data)
{
    return ::send(socketFd_, data.c_str(), data.length(), 0);
}

int S32K144Interface::readRaw(char *buffer, size_t size)
{
    return ::recv(socketFd_, buffer, size, 0);
}

