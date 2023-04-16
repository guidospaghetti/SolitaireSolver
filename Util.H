
#ifndef _SOLITARE_UTIL_H_
#define _SOLITARE_UTIL_H_

#define LOG_OUT(format, ...) \
	printf("%s:%d>> " format "\n", __FILE__, __LINE__, ##__VA_ARGS__);
	
struct Enums {
	enum FrameType {
		RGB = 1,
		DEPTH = 2,
		IR = 4,
		RGB_DEPTH_REGISTERED = 8,
		DEPTH_RGB_REGISTERED = 16
	};
};

#endif
