#include "hal/S32K144Interface.h"
#include <iostream>
#include <unistd.h>
#include <thread>
#include <chrono>

class TestObserver : public IObserver
{
public:
    void update(void *subject) override
    {
        auto *interface = static_cast<S32K144Interface *>(subject);
        if (interface)
        {
            HardwareEvent event = interface->getLastEvent();
            std::cout << "[Observer] Received event. Command: " << (int)event.command 
                      << ", Value: " << event.value << std::endl;
        }
    }
};

int main()
{
    S32K144Interface hw;
    TestObserver observer;
    
    hw.attach(&observer);
    
    std::cout << "Initializing S32K144Interface (connecting to localhost:5000)..." << std::endl;
    if (!hw.initialize("127.0.0.1", 5000))
    {
        std::cerr << "Failed to initialize!" << std::endl;
        return 1;
    }
    
    hw.startListening();
    
    std::cout << "Sending commands..." << std::endl;
    hw.sendCommand("HELLO_FROM_LINUX");
    hw.sendVolume(0.8f);
    hw.displayText("Testing Display");
    
    std::cout << "Waiting for events (5 seconds)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    hw.stopListening();
    hw.close();
    
    return 0;
}
