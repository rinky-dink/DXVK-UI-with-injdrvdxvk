#ifndef TASK_SCHEDULER_LIB_H
#define TASK_SCHEDULER_LIB_H

#include <string>

bool DoesScheduledTaskExist(const std::wstring& taskName);
bool CreateScheduledTask(const std::wstring& taskName, const std::wstring& executablePath, const std::wstring& arguments);
bool DeleteScheduledTask(const std::wstring& taskName);

#endif // TASK_SCHEDULER_LIB_H
