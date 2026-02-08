#ifndef MOCK_PERSISTENCE_H
#define MOCK_PERSISTENCE_H

#include "interfaces/IPersistence.h"
#include <gmock/gmock.h>

class MockPersistence : public IPersistence
{
  public:
    MOCK_METHOD(bool, saveToFile, (const std::string &, const std::string &), (override));
    MOCK_METHOD(bool, loadFromFile, (const std::string &, std::string &), (override));
    MOCK_METHOD(bool, fileExists, (const std::string &), (override));
    MOCK_METHOD(bool, deleteFile, (const std::string &), (override));
    MOCK_METHOD(std::string, serialize, (const void *), (override));
    MOCK_METHOD(bool, deserialize, (const std::string &, void *), (override));
};

#endif // MOCK_PERSISTENCE_H
