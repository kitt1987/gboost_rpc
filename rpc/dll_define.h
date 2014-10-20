#pragma once

#ifdef WIN32
#ifdef LIBRPC_EXPORTS
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif
#else
#define DLL_EXPORT
#endif
