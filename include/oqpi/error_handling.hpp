#pragma once

#include <string>
#include <cassert>
#include <iostream>

#include "oqpi/platform.hpp"

//----------------------------------------------------------------------------------------------
#define OQPI_ASSERT_DO_NOT_USE(Condition, File, Line)                                                   \
    {                                                                                                   \
        static bool always_ignore = false;                                                              \
        if (!always_ignore && !(Condition))                                                             \
        {                                                                                               \
            eAssertResult assertResult = assert_function(File, Line, #Condition);                       \
            if (assertResult == eAR_Retry)                                                              \
            {                                                                                           \
                debugger_break();                                                                       \
            }                                                                                           \
            else if (assertResult == eAR_AlwaysIgnore)                                                  \
            {                                                                                           \
                always_ignore = true;                                                                   \
            }                                                                                           \
        }                                                                                               \
    }

//----------------------------------------------------------------------------------------------
#define OQPI_ASSERTF_DO_NOT_USE(Condition, Msg, File, Line, ...)                                        \
    {                                                                                                   \
        static bool always_ignore = false;                                                              \
        if (!always_ignore && !(Condition))                                                             \
        {                                                                                               \
            eAssertResult assertResult = assert_function(File, Line, #Condition, Msg, ##__VA_ARGS__);   \
            if (assertResult == eAR_Retry)                                                              \
            {                                                                                           \
                debugger_break();                                                                       \
            }                                                                                           \
            else if (assertResult == eAR_AlwaysIgnore)                                                  \
            {                                                                                           \
                always_ignore = true;                                                                   \
            }                                                                                           \
        }                                                                                               \
    }

//----------------------------------------------------------------------------------------------
// Always evaluate the condition
#define oqpi_verify(COND)               OQPI_ASSERT_DO_NOT_USE((COND), __FILE__, __LINE__)
#define oqpi_verifyf(COND, MSG, ...)    OQPI_ASSERTF_DO_NOT_USE((COND), (MSG), __FILE__, __LINE__, ##__VA_ARGS__)
//----------------------------------------------------------------------------------------------
// Assertion that will cause the program to crash (can be skipped)
#define oqpi_check(COND)                OQPI_ASSERT_DO_NOT_USE((COND), __FILE__, __LINE__)
#define oqpi_checkf(COND, MSG, ...)     OQPI_ASSERTF_DO_NOT_USE((COND), (MSG), __FILE__, __LINE__, ##__VA_ARGS__)
//----------------------------------------------------------------------------------------------
// Assertion used inside an if statement that will output an error log and evaluates to false
// if the condition is not met. Always returns true when deactivated.
#define oqpi_ensure(COND)               ((COND) ? true : assert_and_return_false(__FILE__, __LINE__, #COND))
#define oqpi_ensuref(COND, MSG, ...)    ((COND) ? true : assert_and_return_false(__FILE__, __LINE__, #COND, (MSG), ##__VA_ARGS__))
//----------------------------------------------------------------------------------------------
// Assertion used inside an if statement that will output an error log and evaluates to true
// if the condition is not met. Always returns false when deactivated.
#define oqpi_failed(COND)               ((COND) ? false : assert_and_return_true(__FILE__, __LINE__, #COND))
#define oqpi_failedf(COND, MSG, ...)    ((COND) ? false : assert_and_return_true(__FILE__, __LINE__, #COND, (MSG), ##__VA_ARGS__))
//----------------------------------------------------------------------------------------------
// Error logs
#define oqpi_error(MSG, ...)
#define oqpi_warning(MSG, ...)
//----------------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------------
enum eAssertResult
{
    eAR_Abort,
    eAR_Retry,
    eAR_Ignore,
    eAR_AlwaysIgnore
};
//--------------------------------------------------------------------------------------------------
inline eAssertResult assert_function(const char *file, int line, const char *expression, const char *description = "", ...)
{
    std::string strMessage;

    va_list argPtr;
    static const unsigned int bufferSize = 1 << 10;
    thread_local static char formattedMessage[bufferSize];
    va_start(argPtr, description);
    vsprintf_s(formattedMessage, bufferSize, description, argPtr);
    va_end(argPtr);


    strMessage = std::string(
        "*************************************************************************\n"
        "Assertion failed:\n"
        "=================\n"
        "[" + std::string(expression) + "]\n"
        "-------------------------------------------------------------------------\n"
        + formattedMessage + "\n"
        "-------------------------------------------------------------------------\n"
        + file + ", line " + std::to_string(line) + "\n\n"
        "*************************************************************************\n"
        "Stack trace:\n"
        "-------------------------------------------------------------------------\n");

#if OQPI_PLATFORM_WIN
    eAssertResult assertResult = eAR_Ignore;
    int result = MessageBoxA(0, strMessage.c_str(), "Assertion Failed", MB_ABORTRETRYIGNORE);
    switch (result)
    {
    case IDABORT:
        assertResult = eAR_Abort;
        exit(1);
        break;

    case IDRETRY:
        assertResult = eAR_Retry;
        break;

    case IDIGNORE:
        assertResult = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? eAR_AlwaysIgnore : eAR_Ignore;
        break;
    }

    return assertResult;

#else
    std::cout << strMessage;
    return eAR_Retry;
#endif // OQPI_PLATFORM_WIN
}
//--------------------------------------------------------------------------------------------------
inline void debugger_break()
{
#if (OQPI_PLATFORM_WIN)
    __debugbreak();
#else
    asm("int $3");
#endif
}
//--------------------------------------------------------------------------------------------------
inline bool assert_and_return_false(const char *file, int line, const char *expression, const char *description = "", ...)
{
    const eAssertResult assertResult = assert_function(file, line, expression, description);
    if (assertResult == eAR_Retry)
    {
        debugger_break();
    }
    return false;
}
//--------------------------------------------------------------------------------------------------
inline bool assert_and_return_true(const char *file, int line, const char *expression, const char *description = "", ...)
{
    const eAssertResult assertResult = assert_function(file, line, expression, description);
    if (assertResult == eAR_Retry)
    {
        debugger_break();
    }
    return true;
}
//--------------------------------------------------------------------------------------------------
