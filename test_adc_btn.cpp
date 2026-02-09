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
            
            if (event.command == HardwareCommand::BUTTON_PRESS) {
                std::cout << " -> Button " << (int)event.value << " pressed! (Current State: " << interface->readButton() << ")" << std::endl;
            }
            if (event.command == HardwareCommand::ADC_UPDATE) {
                std::cout << " -> ADC Updated: " << event.value << " (Current State: " << interface->readADC() << ")" << std::endl;
            }
        }
    }
};

int main()
{
    S32K144Interface hw;
    TestObserver observer;
    
    hw.attach(&observer);
    
    std::cout << "Initializing S32K144Interface (connecting to localhost:5002)..." << std::endl;
    if (!hw.initialize("127.0.0.1", 5002))
    {
        std::cerr << "Failed to initialize!" << std::endl;
        return 1;
    }
    
    hw.startListening();
    
    std::cout << "Waiting for Button and ADC events (8 seconds)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(8));
    
    hw.stopListening();
    hw.close();
    
    return 0;
}
