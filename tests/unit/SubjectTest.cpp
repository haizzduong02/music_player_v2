#include "utils/Subject.h"
#include "interfaces/IObserver.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockObserver : public IObserver
{
  public:
    MOCK_METHOD(void, update, (void *subject), (override));
};

// Concrete subject for testing
class TestSubject : public Subject
{
  public:
    void doSomething()
    {
        notify();
    }
};

class SubjectTest : public ::testing::Test
{
  protected:
    TestSubject subject;
    MockObserver observer1;
    MockObserver observer2;
    MockObserver observer3;
};

// Attach/Detach tests
TEST_F(SubjectTest, AttachSingleObserver)
{
    subject.attach(&observer1);

    EXPECT_CALL(observer1, update(&subject)).Times(1);
    subject.doSomething();
}

TEST_F(SubjectTest, AttachMultipleObservers)
{
    subject.attach(&observer1);
    subject.attach(&observer2);
    subject.attach(&observer3);

    EXPECT_CALL(observer1, update(&subject)).Times(1);
    EXPECT_CALL(observer2, update(&subject)).Times(1);
    EXPECT_CALL(observer3, update(&subject)).Times(1);
    subject.doSomething();
}

TEST_F(SubjectTest, DetachObserver)
{
    subject.attach(&observer1);
    subject.attach(&observer2);

    subject.detach(&observer1);

    EXPECT_CALL(observer1, update(&subject)).Times(0);
    EXPECT_CALL(observer2, update(&subject)).Times(1);
    subject.doSomething();
}

TEST_F(SubjectTest, DetachNonExistentObserver)
{
    subject.attach(&observer1);

    // Detaching observer that was never attached should not crash
    subject.detach(&observer2);

    EXPECT_CALL(observer1, update(&subject)).Times(1);
    subject.doSomething();
}

TEST_F(SubjectTest, DetachAllObservers)
{
    subject.attach(&observer1);
    subject.attach(&observer2);

    subject.detach(&observer1);
    subject.detach(&observer2);

    EXPECT_CALL(observer1, update(&subject)).Times(0);
    EXPECT_CALL(observer2, update(&subject)).Times(0);
    subject.doSomething();
}

TEST_F(SubjectTest, DoubleAttachSameObserver)
{
    subject.attach(&observer1);
    subject.attach(&observer1); // Attach same observer again - should prevent duplicates

    // Subject prevents duplicates, so should only notify once
    EXPECT_CALL(observer1, update(&subject)).Times(1);
    subject.doSomething();
}

// Notify tests
TEST_F(SubjectTest, NotifyWithNoObservers)
{
    // Should not crash
    EXPECT_NO_THROW(subject.doSomething());
}

TEST_F(SubjectTest, MultipleNotifications)
{
    subject.attach(&observer1);

    EXPECT_CALL(observer1, update(&subject)).Times(3);
    subject.doSomething();
    subject.doSomething();
    subject.doSomething();
}

// Null observer tests
TEST_F(SubjectTest, AttachNullObserver)
{
    // Should handle null gracefully (Subject prevents null observers)
    subject.attach(nullptr);
    EXPECT_NO_THROW(subject.doSomething());
}

TEST_F(SubjectTest, DetachNullObserver)
{
    subject.attach(&observer1);
    subject.detach(nullptr);

    EXPECT_CALL(observer1, update(&subject)).Times(1);
    subject.doSomething();
}

