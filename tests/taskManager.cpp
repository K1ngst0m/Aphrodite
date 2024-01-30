#include <catch2/catch_all.hpp>

#include "threads/taskManager.h"

using namespace aph;

TEST_CASE("Basic Task Creation and Execution")
{
    TaskManager taskManager;

    auto taskGroup = taskManager.createTaskGroup("BasicGroup");
    bool executed  = false;

    taskGroup->addTask([&executed]() { executed = true; }, "SimpleTask");
    taskGroup->flush();
    taskGroup->wait();

    REQUIRE(executed);
    taskManager.removeTaskGroup(taskGroup);
}

TEST_CASE("Task Dependencies")
{
    TaskManager taskManager;

    auto mainTaskGroup      = taskManager.createTaskGroup("MainGroup");
    auto dependentTaskGroup = taskManager.createTaskGroup("DependentGroup");

    int value = 0;

    dependentTaskGroup->addTask([&value]() { value = 10; }, "SetTask");
    mainTaskGroup->addTask([&value]() { value += 5; }, "AddTask");

    taskManager.setDependency(mainTaskGroup, dependentTaskGroup);

    dependentTaskGroup->flush();
    mainTaskGroup->flush();
    mainTaskGroup->wait();

    REQUIRE(value == 15);

    taskManager.removeTaskGroup(mainTaskGroup);
    taskManager.removeTaskGroup(dependentTaskGroup);
}

TEST_CASE("Multiple Tasks Execution")
{
    TaskManager taskManager;

    auto taskGroup = taskManager.createTaskGroup("MultiGroup");
    int  count     = 0;

    for(int i = 0; i < 5; i++)
    {
        taskGroup->addTask([&count]() { count++; }, "CountTask" + std::to_string(i));
    }
    taskGroup->flush();
    taskGroup->wait();

    REQUIRE(count == 5);

    taskManager.removeTaskGroup(taskGroup);
}

TEST_CASE("Polling for Task Completion")
{
    TaskManager taskManager;

    auto taskGroup = taskManager.createTaskGroup("PollGroup");
    taskGroup->addTask([]() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }, "SleepTask");
    taskGroup->flush();

    REQUIRE(!taskGroup->poll());
    taskGroup->wait();
    REQUIRE(taskGroup->poll());

    taskManager.removeTaskGroup(taskGroup);
}
