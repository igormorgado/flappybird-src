#ifndef C_TIME_TICKER_H
#define C_TIME_TICKER_H

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef DOXYGEN_SHOULD_IGNORE_THIS
/** @cond DOXYGEN_SHOULD_IGNORE_THIS */

/* Note: For Doxygen to produce clean output, you should set the 
 * PREDEFINED option to remove C_TIME_TICKER_DECLSPEC, C_TIME_TICKER_CALL, and
 * the DOXYGEN_SHOULD_IGNORE_THIS blocks.
 * PREDEFINED = DOXYGEN_SHOULD_IGNORE_THIS=1 C_TIME_TICKER_DECLSPEC= C_TIME_TICKER_CALL=
 */

/** Windows needs to know explicitly which functions to export in a DLL. */
#if defined(_WIN32)
	#if defined(C_TIME_TICKER_BUILD_LIBRARY)
		#define C_TIME_TICKER_DECLSPEC __declspec(dllexport)
	#else
		#define C_TIME_TICKER_DECLSPEC
	#endif
#else
	#if defined(C_TIME_TICKER_BUILD_LIBRARY)
		#if defined (__GNUC__) && __GNUC__ >= 4
			#define C_TIME_TICKER_DECLSPEC __attribute__((visibility("default")))
		#else
			#define C_TIME_TICKER_DECLSPEC
		#endif
	#else
		#define C_TIME_TICKER_DECLSPEC
	#endif
#endif

/* For Windows, by default, use the C calling convention */
#if defined(_WIN32)
	#define C_TIME_TICKER_CALL __cdecl
#else
	#define C_TIME_TICKER_CALL
#endif

/** @endcond DOXYGEN_SHOULD_IGNORE_THIS */
#endif /* DOXYGEN_SHOULD_IGNORE_THIS */

	
/* Optional API symbol name rewrite to help avoid duplicate symbol conflicts.
	For example:   -DTIME_TICKER_NAMESPACE_PREFIX=ALmixer
*/
	
#if defined(TIME_TICKER_NAMESPACE_PREFIX)
	#define TIME_TICKER_RENAME_PUBLIC_SYMBOL_WITH_NAMESPACE(namespace, symbol) namespace##symbol
	#define TIME_TICKER_RENAME_PUBLIC_SYMBOL(symbol) TIME_TICKER_RENAME_PUBLIC_SYMBOL_WITH_NAMESPACE(TIME_TICKER_NAMESPACE_PREFIX, symbol)
	
	#define TimeTicker_Create		TIME_TICKER_RENAME_PUBLIC_SYMBOL(TimeTicker_Create)
	#define TimeTicker_Free			TIME_TICKER_RENAME_PUBLIC_SYMBOL(TimeTicker_Free)
	#define TimeTicker_Start		TIME_TICKER_RENAME_PUBLIC_SYMBOL(TimeTicker_Start)
	#define TimeTicker_Stop			TIME_TICKER_RENAME_PUBLIC_SYMBOL(TimeTicker_Stop)
	#define TimeTicker_Reset		TIME_TICKER_RENAME_PUBLIC_SYMBOL(TimeTicker_Reset)
	#define TimeTicker_GetTime		TIME_TICKER_RENAME_PUBLIC_SYMBOL(TimeTicker_GetTime)
	#define TimeTicker_UpdateTime	TIME_TICKER_RENAME_PUBLIC_SYMBOL(TimeTicker_UpdateTime)
	#define TimeTicker_SetSpeed		TIME_TICKER_RENAME_PUBLIC_SYMBOL(TimeTicker_SetSpeed)
	#define TimeTicker_GetSpeed		TIME_TICKER_RENAME_PUBLIC_SYMBOL(TimeTicker_GetSpeed)

#endif /* defined(TIME_TICKER_NAMESPACE_PREFIX) */

#include "SDL.h"

struct TimeTicker;
typedef struct TimeTicker TimeTicker;

extern C_TIME_TICKER_DECLSPEC struct TimeTicker* C_TIME_TICKER_CALL TimeTicker_Create(void);
extern C_TIME_TICKER_DECLSPEC void C_TIME_TICKER_CALL TimeTicker_Free(struct TimeTicker* time_ticker);
extern C_TIME_TICKER_DECLSPEC void C_TIME_TICKER_CALL TimeTicker_Start(struct TimeTicker* time_ticker);
extern C_TIME_TICKER_DECLSPEC void C_TIME_TICKER_CALL TimeTicker_Stop(struct TimeTicker* time_ticker);
extern C_TIME_TICKER_DECLSPEC void C_TIME_TICKER_CALL TimeTicker_Reset(struct TimeTicker* time_ticker);
extern C_TIME_TICKER_DECLSPEC Uint32 C_TIME_TICKER_CALL TimeTicker_GetTime(struct TimeTicker* time_ticker);
extern C_TIME_TICKER_DECLSPEC Uint32 C_TIME_TICKER_CALL TimeTicker_UpdateTime(struct TimeTicker* time_ticker);
extern C_TIME_TICKER_DECLSPEC void C_TIME_TICKER_CALL TimeTicker_SetSpeed(struct TimeTicker* time_ticker, double new_speed);
extern C_TIME_TICKER_DECLSPEC double C_TIME_TICKER_CALL TimeTicker_GetSpeed(struct TimeTicker* time_ticker);

#ifdef __cplusplus
}
#endif

#endif /* C_TIME_TICKER_H */
