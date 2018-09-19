#ifndef __TIMELIST_H__
#define __TIMELIST_H__

#ifdef __cplusplus
extern "C" {
#endif
	struct TimeController;
	struct TimeController*
	schedule_repeat(void(*on_timer)(void* udata), 	// �ص�, ����timerʱ
			void* udata, 							// �Զ������ݽṹ
			float after_sec, 						// ִ��ʱ��
			int repeat_count,						// ִ�д���, if(repeat == -1) repeat infinitly
			int repeat_sec);						// ���೤ʱ��ִ��

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