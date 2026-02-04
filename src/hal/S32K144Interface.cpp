#include "../../inc/hal/S32K144Interface.h"
#include "../../inc/utils/Logger.h"

// TODO: This is a stub implementation for S32K144 microcontroller
// Full implementation would require S32K144 SDK and hardware access

S32K144Interface::S32K144Interface() {
    Logger::getInstance().info("S32K144Interface initialized (STUB)");
}

S32K144Interface::~S32K144Interface() {
    Logger::getInstance().info("S32K144Interface destroyed");
}

bool S32K144Interface::init() {
    Logger::getInstance().warn("S32K144Interface::init() - STUB implementation");
    return false;
}

void S32K144Interface::updateDisplay(const std::string& line1, const std::string& line2) {
    Logger::getInstance().warn("S32K144Interface::updateDisplay() - STUB implementation");
}

int S32K144Interface::readButton() {
    return -1;  // No button pressed
}

void S32K144Interface::setLED(int ledId, bool state) {
    Logger::getInstance().warn("S32K144Interface::setLED() - STUB implementation");
}
