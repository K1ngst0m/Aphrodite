#include <catch2/catch_all.hpp>
#include "app/input/event.h"
#include "app/input/eventManager.h"

using namespace aph;
using namespace Catch;

TEST_CASE("Single Event Push and Process")
{
    EventManager& manager = EventManager::GetInstance();

    struct TestEvent
    {
        int value;
    };
    bool handlerCalled = false;

    manager.registerEventHandler<TestEvent>([&handlerCalled](const TestEvent& event) {
        REQUIRE(event.value == 42);
        handlerCalled = true;
        return true;
    });

    manager.pushEvent(TestEvent{42});
    manager.processAll();

    REQUIRE(handlerCalled);
}

TEST_CASE("Multiple Event Push and Process")
{
    EventManager& manager = EventManager::GetInstance();

    struct TestEvent
    {
        int value;
    };
    int handlerCallCount = 0;

    manager.registerEventHandler<TestEvent>([&handlerCallCount](const TestEvent& event) {
        handlerCallCount++;
        return true;
    });

    for(int i = 0; i < 5; ++i)
    {
        manager.pushEvent(TestEvent{i});
    }

    manager.processAll();

    REQUIRE(handlerCallCount == 5);
}

TEST_CASE("Multi-Threaded Event Push")
{
    EventManager& manager = EventManager::GetInstance();

    struct TestEvent
    {
        int value;
    };
    int handlerCallCount = 0;

    manager.registerEventHandler<TestEvent>([&handlerCallCount](const TestEvent& event) {
        handlerCallCount++;
        return true;
    });

    std::vector<std::thread> threads;
    threads.reserve(5);
    for(int i = 0; i < 5; ++i)
    {
        threads.emplace_back([&manager, i]() { manager.pushEvent(TestEvent{i}); });
    }

    for(auto& t : threads)
    {
        t.join();
    }

    manager.processAll();

    REQUIRE(handlerCallCount == 5);
}

TEST_CASE("Event Processing in Multi-Threaded Environment")
{
    EventManager& manager = EventManager::GetInstance();

    struct TestEvent
    {
        int value;
    };
    std::atomic<int> handlerCallCount = 0;

    manager.registerEventHandler<TestEvent>([&handlerCallCount](const TestEvent& event) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Simulate work
        handlerCallCount++;
        return true;
    });

    for(int i = 0; i < 10; ++i)
    {
        manager.pushEvent(TestEvent{i});
    }

    auto start = std::chrono::high_resolution_clock::now();
    manager.processAll();
    auto end = std::chrono::high_resolution_clock::now();

    REQUIRE(handlerCallCount == 10);
    REQUIRE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() < 150);
}

TEST_CASE("Mouse Button Event Push and Process")
{
    EventManager& manager = EventManager::GetInstance();

    bool handlerCalled = false;
    manager.registerEventHandler<MouseButtonEvent>([&handlerCalled](const MouseButtonEvent& event) {
        REQUIRE(event.m_button == MouseButton::Left);
        REQUIRE(event.m_absX == Approx(10.0));
        REQUIRE(event.m_absY == Approx(20.0));
        REQUIRE(event.m_pressed);
        handlerCalled = true;
        return true;
    });

    manager.pushEvent(MouseButtonEvent(MouseButton::Left, 10.0, 20.0, true));
    manager.processAll();

    REQUIRE(handlerCalled);
}

TEST_CASE("Mouse Move Event Push and Process")
{
    EventManager& manager = EventManager::GetInstance();

    bool handlerCalled = false;
    manager.registerEventHandler<MouseMoveEvent>([&handlerCalled](const MouseMoveEvent& event) {
        REQUIRE(event.m_deltaX == Approx(5.0));
        REQUIRE(event.m_deltaY == Approx(7.0));
        REQUIRE(event.m_absX == Approx(15.0));
        REQUIRE(event.m_absY == Approx(25.0));
        handlerCalled = true;
        return true;
    });

    manager.pushEvent(MouseMoveEvent(5.0, 7.0, 15.0, 25.0));
    manager.processAll();

    REQUIRE(handlerCalled);
}

TEST_CASE("Keyboard Event Push and Process")
{
    EventManager& manager = EventManager::GetInstance();

    bool handlerCalled = false;
    manager.registerEventHandler<KeyboardEvent>([&handlerCalled](const KeyboardEvent& event) {
        REQUIRE(event.m_key == Key::A);
        REQUIRE(event.m_state == KeyState::Pressed);
        handlerCalled = true;
        return true;
    });

    manager.pushEvent(KeyboardEvent(Key::A, KeyState::Pressed));
    manager.processAll();

    REQUIRE(handlerCalled);
}

TEST_CASE("Window Resize Event Push and Process")
{
    EventManager& manager = EventManager::GetInstance();

    bool handlerCalled = false;
    manager.registerEventHandler<WindowResizeEvent>([&handlerCalled](const WindowResizeEvent& event) {
        REQUIRE(event.m_width == 800);
        REQUIRE(event.m_height == 600);
        handlerCalled = true;
        return true;
    });

    manager.pushEvent(WindowResizeEvent(800, 600));
    manager.processAll();

    REQUIRE(handlerCalled);
}

TEST_CASE("Multi-Threaded Event Push with Different Event Types")
{
    EventManager& manager = EventManager::GetInstance();

    std::atomic<int> handlerCallCount = 0;

    manager.registerEventHandler<KeyboardEvent>([&handlerCallCount](const KeyboardEvent&) {
        handlerCallCount++;
        return true;
    });
    manager.registerEventHandler<MouseMoveEvent>([&handlerCallCount](const MouseMoveEvent&) {
        handlerCallCount++;
        return true;
    });

    std::vector<std::thread> threads;
    threads.emplace_back([&manager]() { manager.pushEvent(KeyboardEvent(Key::A, KeyState::Pressed)); });
    threads.emplace_back([&manager]() { manager.pushEvent(MouseMoveEvent(5.0, 7.0, 15.0, 25.0)); });

    for(auto& t : threads)
    {
        t.join();
    }

    manager.processAll();

    REQUIRE(handlerCallCount == 2);
}
