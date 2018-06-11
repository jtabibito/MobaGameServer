#ifndef __TIMELIST_H__
#define __TIMELIST_H__

#ifdef __cplusplus
extern "C" {
#endif
	struct TimeController;
	struct TimeController*
		schedule(void(*on_timer)(void* udata), 		// �ص�, ����timerʱ
			void* udata, 						// �Զ������ݽṹ
			float after_sec, 					// ִ��ʱ��
			int repeat);						// ִ�д���, if(repeat == -1) repeat infinitly

	void cancel_timer(struct TimeController* t);

	static int
		schedule_once(void(*on_timer)(void* udata),
			void* udata,
			float after_sec);
#ifdef __cplusplus
}
#endif


#endif // __TIMELIST_H__