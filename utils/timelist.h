#ifndef __TIMELIST_H__
#define __TIMELIST_H__

#ifdef __cplusplus
extern "C" {
#endif
	struct TimeController;
	struct TimeController*
		schedule(void(*on_timer)(void* udata), 		// 回调, 触发timer时
			void* udata, 						// 自定义数据结构
			float after_sec, 					// 执行时间
			int repeat);						// 执行次数, if(repeat == -1) repeat infinitly

	void cancel_timer(struct TimeController* t);

	static int
		schedule_once(void(*on_timer)(void* udata),
			void* udata,
			float after_sec);
#ifdef __cplusplus
}
#endif


#endif // __TIMELIST_H__