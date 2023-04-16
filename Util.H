
#ifndef _SOLITARE_UTIL_H_
#define _SOLITARE_UTIL_H_

#define LOG_OUT(format, ...) \
	printf("%s:%d>> " format "\n", __FILE__, __LINE__, ##__VA_ARGS__);

#endif
