/**
 * Macro for DLL export/import
 * When building the DLL (ASCORE_EXPORTS defined), classes/functions are exported
 * When using the DLL, classes/functions are imported
 *
 * In the Comet project, AScorePro is a static .lib so this is defined as empty
 */

#pragma once

#ifndef _ASCOREAPI_H_
#define _ASCOREAPI_H_

#define ASCORE_API

#endif // _ASCOREAPI_H_


/*
#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef ASCORE_EXPORTS
#define ASCORE_API __declspec(dllexport)
#else
#define ASCORE_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#define ASCORE_API __attribute__((visibility("default")))
#else
#define ASCORE_API
#endif
*/
