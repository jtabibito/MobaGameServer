#ifndef __LOGMANAGER_H__
#define __LOGMANAGER_H__

enum log_info {
	DEBUG = 0,
	WARNING,
	WRONG,
};

#define log_debug(msg, ...) logger::write2log(__FILE__, __LINE__, DEBUG, msg, ## __VA_ARGS__);
#define log_warning(msg, ...) logger::write2log(__FILE__, __LINE__, WARNING, msg, ## __VA_ARGS__);
#define log_error(msg, ...) logger::write2log(__FILE__, __LINE__, WRONG, msg, ## __VA_ARGS__);

class logger {
public:
	static void init(char* path, char* prefix, bool std_output = false);
	static void write2log(const char* file_name, int line_num, int modle, const char* msg, ...);
};

#endif // !__LOGMANAGER_H__