#ifndef __TIMELIST_H__
#define __TIMELIST_H__

#ifdef __cplusplus
extern "C" {
#endif
	struct TimeController;
	struct TimeController*
	schedule_repeat(void(*on_timer)(void* udata), 	// 回调, 触发timer时
			void* udata, 							// 自定义数据结构
			float after_sec, 						// 执行时间
			int repeat_count,						// 执行次数, if(repeat == -1) repeat infinitly
			int repeat_sec);						// 隔多长时间执行

	void cancel_timer(struct TimeController* t);

	struct TimeController*
	schedule_once(void(*on_timer)(void* udata),
			void* udata,
			float after_sec);

	void* get_timer_udata(struct TimeController* t);
#ifdef __cplusplus
}
#endif


#endif // __TIMELIST_H__