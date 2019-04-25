/*! @file */
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ENABLE_DEBUG
	/** @brief Macro for error output.*/
	#define ERROR(fmt, args...)												\
	    do {																\
			fprintf(stderr, "[ERROR: %s] " fmt, __func__, ##args); 			\
		} while (0)
	/** @brief Macro for warning output.*/
	#define WARN(fmt, args...)                                            	\
	    do {																\
			fprintf(stdout, "[WARNING: %s] " fmt, __func__, ##args); 		\
	    } while (0)
	/** @brief Macro for debug output.*/
	#define DEBUG(fmt, args...)                                             \
	    do {																\
	        fprintf(stdout, "[DEBUG: %s] " fmt,  __func__, ##args);			\
	    } while (0)
#else
	/*	Stubbed functions, so that the compiler doesn't scream at us
		when we attempt to compile our program without these defined macros.	*/
	#define ERROR(fmt, args...)
	#define WARN(fmt, args...)
	#define DEBUG(fmt, args...)
#endif




int pet_log_file_init(char * filename);


#ifdef __cplusplus
}
#endif



#endif
