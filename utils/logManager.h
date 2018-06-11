#ifndef __LOGMANAGER_H__
#define __LOGMANAGER_H__

enum {
	DEBUG = 0,
	WARING,
	WRONG,
};

#define log_debug(msg, ...) log::write2log(__FILE__, __LINE__, DEBUG, msg, ## __VA_ARGS__);
#define log_waring(msg, ...) log::write2log(__FILE__, __LINE__, WARING, msg, ## __VA_ARGS__);
#define log_wrong(msg, ...) log::write2log(__FILE__, __LINE__, WRONG, msg, ## __VA_ARGS__);

class log {
public:
	static void init(char* path, char* prefix, bool std_output = false);
	static void write2log(const char* file_name, int line_num, int modle, const char* msg, ...);
};

#endif // !__LOGMANAGER_H__