#ifndef LIB_ACL_FIBER_INCLUDE_H
#define LIB_ACL_FIBER_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/socket.h>

//typedef struct ACL_VSTREAM ACL_VSTREAM;

/**
 * 鍗忕▼缁撴瀯绫诲瀷
 */
typedef struct ACL_FIBER ACL_FIBER;

/**
 * 璁剧疆鏄惁闇€瑕� hook 绯荤粺涓殑 IO 鐩稿叧鐨� API锛屽唴閮ㄧ己鐪佸€间负 1
 * @param onoff {int} 鏄惁闇€瑕� hook
 */
void acl_fiber_hook_api(int onoff);

/**
 * 鍒涘缓涓€涓崗绋�
 * @param fn {void (*)(ACL_FIBER*, void*)} 鍗忕▼杩愯鏃剁殑鍥炶皟鍑芥暟鍦板潃
 * @param arg {void*} 鍥炶皟 fn 鍑芥暟鏃剁殑绗簩涓弬鏁�
 * @param size {size_t} 鎵€鍒涘缓鍗忕▼鎵€鍗犳爤绌洪棿澶у皬
 * @return {ACL_FIBER*}
 */
ACL_FIBER* acl_fiber_create(void (*fn)(ACL_FIBER*, void*),
	void* arg, size_t size);

/**
 * 杩斿洖褰撳墠绾跨▼涓浜庢秷浜＄姸鎬佺殑鍗忕▼鏁�
 * @retur {int}
 */
int acl_fiber_ndead(void);

void acl_fiber_check_timer(size_t max);

/**
 * 杩斿洖褰撳墠姝ｅ湪杩愯鐨勫崗绋嬪璞�
 * @retur {ACL_FIBER*} 杩斿洖 NULL 琛ㄧず褰撳墠娌℃湁姝ｅ湪杩愯鐨勫崗绋�
 */
ACL_FIBER* acl_fiber_running(void);

/**
 * 鑾峰緱鎵€缁欏崗绋嬬殑鍗忕▼ ID 鍙�
 * @param fiber {const ACL_FIBER*} acl_fiber_create 鍒涘缓鐨勫崗绋嬪璞★紝蹇呴』闈炵┖
 * @return {unsigned int} 鍗忕▼ ID 鍙�
 */
unsigned int acl_fiber_id(const ACL_FIBER* fiber);

/**
 * 鑾峰緱褰撳墠鎵€杩愯鐨勫崗绋嬬殑 ID 鍙�
 * @return {unsigned int} 褰撳墠杩愯鍗忕▼鐨� ID 鍙�
 */
unsigned int acl_fiber_self(void);

/**
 * 璁剧疆鎵€缁欏崗绋嬬殑閿欒鍙�
 * @param fiber {ACL_FIBER*} 鎸囧畾鐨勫崗绋嬪璞★紝涓� NULL 鍒欎娇鐢ㄥ綋鍓嶈繍琛岀殑鍗忕▼
 * @param errnum {int} 閿欒鍙�
 */
void acl_fiber_set_errno(ACL_FIBER* fiber, int errnum);

/**
 * 鑾峰緱鎸囧畾鍗忕▼鐨勯敊璇彿
 * @param fiber {ACL_FIBER*} 鎸囧畾鐨勫崗绋嬪璞★紝鑻ヤ负 NULL 鍒欎娇鐢ㄥ綋鍓嶅崗绋嬪璞�
 * @return {int} 鎵€缁欏崗绋嬮敊璇彿
 */
int acl_fiber_errno(ACL_FIBER* fiber);

/**
 * 鑾峰緱褰撳墠绯荤粺绾х殑 errno 鍙�
 * @return {int}
 */
int acl_fiber_sys_errno(void);

/**
 * 璁剧疆褰撳墠绯荤粺鐨� errno 鍙�
 * @param errnum {int}
 */
void acl_fiber_sys_errno_set(int errnum);

/**
 * 鏄惁淇濇寔鎵€鎸囧畾鍗忕▼鐨勯敊璇彿锛屽綋璁剧疆涓衡€滀繚鎸佲€濆悗锛屽垯璇ュ崗绋嬩粎淇濇寔褰撳墠鐘舵€佷笅鐨�
 * 閿欒鍙凤紝涔嬪悗璇ュ崗绋嬬殑閿欒鍙� errno 灏嗕笉鍐嶆敼鍙橈紝璧板埌鍐嶆璋冪敤鏈嚱鏁板彇娑堜繚鎸�
 * @param fiber {ACL_FIBER*} 鎸囧畾鐨勫崗绋嬪璞★紝涓� NULL 鍒欎娇鐢ㄥ綋鍓嶈繍琛岀殑鍗忕▼
 * @param yesno {int} 鏄惁淇濇寔
 */
void acl_fiber_keep_errno(ACL_FIBER* fiber, int yesno);

/**
 * 鑾峰緱鎸囧畾鍗忕▼鐨勫綋鍓嶇姸鎬�
 * @param fiber {const ACL_FIBER*} 鎸囧畾鐨勫崗绋嬪璞★紝涓� NULL 鍒欎娇鐢ㄥ綋鍓嶅崗绋�
 * @return {int} 鍗忕▼鐘舵€�
 */
int acl_fiber_status(const ACL_FIBER* fiber);

/**
 * 閫氱煡澶勪簬浼戠湢鐘舵€佺殑鍗忕▼閫€鍑� 
 * @param fiber {const ACL_FIBER*} 鎸囧畾鐨勫崗绋嬪璞★紝蹇呴』闈� NULL
 */
void acl_fiber_kill(ACL_FIBER* fiber);

/**
 * 妫€鏌ユ湰鍗忕▼鏄惁琚叾瀹冨崗绋嬮€氱煡閫€鍑�
 * @param fiber {const ACL_FIBER*} 鎸囧畾鐨勫崗绋嬪璞★紝鑻ヤ负 NULL 鍒欒嚜鍔ㄤ娇鐢ㄥ綋鍓�
 *  姝ｅ湪杩愯鐨勫崗绋�
 * @return {int} 杩斿洖鍊间负 0 琛ㄧず娌℃湁琚€氱煡閫€鍑猴紝闈� 0 琛ㄧず琚€氱煡閫€鍑�
 */
int acl_fiber_killed(ACL_FIBER* fiber);

/**
 * 鍞ら啋鍥� IO 绛夊師鍥犲浜庝紤鐪犵殑鍗忕▼
 * @param fiber {const ACL_FIBER*} 鍗忕▼瀵硅薄锛屽繀椤婚潪 NULL
 * @param signum {int} SIGINT, SIGKILL, SIGTERM ... 鍙傝€冪郴缁熶腑 bits/signum.h
 */
void acl_fiber_signal(ACL_FIBER* fiber, int signum);

/**
 * 鑾峰緱鍏跺畠鍗忕▼鍙戦€佺粰鎸囧畾鍗忕▼鐨勪俊鍙峰€�
 * @param fiber {const ACL_FIBER*} 鎸囧畾鐨勫崗绋嬪璞★紝涓� NULL 鏃跺垯浣跨敤褰撳墠鍗忕▼
 * @retur {int} 杩斿洖鎸囧畾鍗忕▼鏀跺埌鐨勪俊鍙峰€�
 */
int acl_fiber_signum(ACL_FIBER* fiber);

/**
 * 灏嗗綋鍓嶈繍琛岀殑鍗忕▼鎸傝捣锛岀敱璋冨害鍣ㄩ€夋嫨涓嬩竴涓渶瑕佽繍琛岀殑鍗忕▼
 * @return {int}
 */
int acl_fiber_yield(void);

/**
 * 灏嗘寚瀹氬崗绋嬪璞＄疆鍏ュ緟杩愯闃熷垪涓�
 * @param fiber {ACL_FIBER*} 鎸囧畾鍗忕▼锛屽繀椤婚潪 NULL
 */
void acl_fiber_ready(ACL_FIBER* fiber);

/**
 * 灏嗗綋鍓嶈繍琛岀殑鍗忕▼鎸傝捣锛屽悓鏃舵墽琛岀瓑寰呴槦鍒椾笅涓€涓緟杩愯鐨勫崗绋�
 */
void acl_fiber_switch(void);

/**
 * 璋冪敤鏈嚱鏁板惎鍔ㄥ崗绋嬬殑璋冨害杩囩▼
 */
void acl_fiber_schedule(void);

/**
 * 璋冪敤鏈嚱鏁版娴嬪綋鍓嶇嚎绋嬫槸鍚﹀浜庡崗绋嬭皟搴︾姸鎬�
 * @return {int} 0 琛ㄧず闈炲崗绋嬬姸鎬侊紝闈� 0 琛ㄧず澶勪簬鍗忕▼璋冨害鐘舵€�
 */
int acl_fiber_scheduled(void);

/**
 * 鍋滄鍗忕▼杩囩▼
 */
void acl_fiber_schedule_stop(void);

/**
 * 浣垮綋鍓嶈繍琛岀殑鍗忕▼浼戠湢鎸囧畾姣鏁�
 * @param milliseconds {unsigned int} 鎸囧畾瑕佷紤鐪犵殑姣鏁�
 * @return {unsigned int} 鏈崗绋嬩紤鐪犲悗鍐嶆琚敜閱掑悗鍓╀綑鐨勬绉掓暟
 */
unsigned int acl_fiber_delay(unsigned int milliseconds);

/**
 * 浣垮綋鍓嶈繍琛岀殑鍗忕▼浼戠湢鎸囧畾绉掓暟
 * @param seconds {unsigned int} 鎸囧畾瑕佷紤鐪犵殑绉掓暟
 * @return {unsigned int} 鏈崗绋嬩紤鐪犲悗鍐嶆琚敜閱掑悗鍓╀綑鐨勭鏁�
 */
unsigned int acl_fiber_sleep(unsigned int seconds);

/**
 * 鍒涘缓涓€涓崗绋嬬敤浣滃畾鏃跺櫒
 * @param milliseconds {unsigned int} 鎵€鍒涘缓瀹氭椂鍣ㄨ鍞ら啋鐨勬绉掓暟
 * @param size {size_t} 鎵€鍒涘缓鍗忕▼鐨勬爤绌洪棿澶у皬
 * @param fn {void (*)(ACL_FIBER*, void*)} 瀹氭椂鍣ㄥ崗绋嬭鍞ら啋鏃剁殑鍥炶皟鍑芥暟
 * @param ctx {void*} 鍥炶皟 fn 鍑芥暟鏃剁殑绗簩涓弬鏁�
 * @return {ACL_FIBER*} 鏂板垱寤虹殑瀹氭椂鍣ㄥ崗绋�
 */
ACL_FIBER* acl_fiber_create_timer(unsigned int milliseconds, size_t size,
	void (*fn)(ACL_FIBER*, void*), void* ctx);

/**
 * 鍦ㄥ畾鏃跺櫒鍗忕▼鏈鍞ら啋鍓嶏紝鍙互閫氳繃鏈嚱鏁伴噸缃鍗忕▼琚敜閱掔殑鏃堕棿
 * @param timer {ACL_FIBER*} 鐢� acl_fiber_create_timer 鍒涘缓鐨勫畾鏃跺櫒鍗忕▼
 * @param milliseconds {unsigned int} 鎸囧畾璇ュ畾鏃跺櫒鍗忕▼琚敜閱掔殑姣鏁�
 */
void acl_fiber_reset_timer(ACL_FIBER* timer, unsigned int milliseconds);

/**
 * 鏈嚱鏁拌缃� DNS 鏈嶅姟鍣ㄧ殑鍦板潃
 * @param ip {const char*} DNS 鏈嶅姟鍣� IP 鍦板潃
 * @param port {int} DNS 鏈嶅姟鍣ㄧ殑绔彛
 */
void acl_fiber_set_dns(const char* ip, int port);

/* for fiber specific */

/**
 * 璁惧畾褰撳墠鍗忕▼鐨勫眬閮ㄥ彉閲�
 * @param key {int*} 鍗忕▼灞€閮ㄥ彉閲忕殑绱㈠紩閿殑鍦板潃锛屽垵濮嬫椂璇ュ€煎簲 <= 0锛屽唴閮ㄤ細鑷姩
 *  鍒嗛厤涓€涓� > 0 鐨勭储寮曢敭锛屽苟缁欒鍦板潃璧嬪€硷紝鍚庨潰鐨勫崗绋嬪彲浠ュ鐢ㄨ鍊艰缃悇鑷殑
 *  灞€閮ㄥ彉閲忥紝璇ユ寚閽堝繀椤婚潪 NULL
 * @param ctx {void *} 鍗忕▼灞€閮ㄥ彉閲�
 * @param free_fn {void (*)(void*)} 褰撳崗绋嬮€€鍑烘椂浼氳皟鐢ㄦ鍑芥暟閲婃斁鍗忕▼灞€閮ㄥ彉閲�
 * @return {int} 杩斿洖鎵€璁剧疆鐨勫崗绋嬪眬閮ㄥ彉閲忕殑閿€硷紝杩斿洖 -1 琛ㄧず褰撳墠鍗忕▼涓嶅瓨鍦�
 */
int acl_fiber_set_specific(int* key, void* ctx, void (*free_fn)(void*));

/**
 * 鑾峰緱褰撳墠鍗忕▼灞€閮ㄥ彉閲�
 * @param key {int} 鐢� acl_fiber_set_specific 杩斿洖鐨勯敭鍊�
 * @retur {void*} 杩斿洖 NULL 琛ㄧず涓嶅瓨鍦�
 */
void* acl_fiber_get_specific(int key);

/* fiber locking */

/**
 * 鍗忕▼浜掓枼閿侊紝绾跨▼闈炲畨鍏紝鍙兘鐢ㄥ湪鍚屼竴绾跨▼鍐�
 */
typedef struct ACL_FIBER_MUTEX ACL_FIBER_MUTEX;

/**
 * 鍗忕▼璇诲啓閿侊紝绾跨▼闈炲畨鍏紝鍙兘鐢ㄥ湪鍚屼竴绾跨▼鍐�
 */
typedef struct ACL_FIBER_RWLOCK ACL_FIBER_RWLOCK;

/**
 * 鍒涘缓鍗忕▼浜掓枼閿侊紝绾跨▼闈炲畨鍏紝鍙兘鐢ㄥ湪鍚屼竴绾跨▼鍐�
 * @return {ACL_FIBER_MUTEX*}
 */
ACL_FIBER_MUTEX* acl_fiber_mutex_create(void);

/**
 * 閲婃斁鍗忕▼浜掓枼閿�
 * @param l {ACL_FIBER_MUTEX*} 鐢� acl_fiber_mutex_create 鍒涘缓鐨勫崗绋嬩簰鏂ラ攣
 */
void acl_fiber_mutex_free(ACL_FIBER_MUTEX* l);

/**
 * 瀵瑰崗绋嬩簰鏂ラ攣杩涜闃诲寮忓姞閿侊紝濡傛灉鍔犻攣鎴愬姛鍒欒繑鍥烇紝鍚﹀垯鍒欓樆濉�
 * @param l {ACL_FIBER_MUTEX*} 鐢� acl_fiber_mutex_create 鍒涘缓鐨勫崗绋嬩簰鏂ラ攣
 */
void acl_fiber_mutex_lock(ACL_FIBER_MUTEX* l);

/**
 * 瀵瑰崗绋嬩簰鏂ラ攣灏濊瘯鎬ц繘琛屽姞閿侊紝鏃犺鏄惁鎴愬姛鍔犻攣閮戒細绔嬪嵆杩斿洖
 * @param l {ACL_FIBER_MUTEX*} 鐢� acl_fiber_mutex_create 鍒涘缓鐨勫崗绋嬩簰鏂ラ攣
 * @return {int} 濡傛灉鍔犻攣鎴愬姛鍒欒繑鍥� 0 鍊硷紝鍚﹀垯杩斿洖 -1
 */
int acl_fiber_mutex_trylock(ACL_FIBER_MUTEX* l);

/**
 * 鍔犻攣鎴愬姛鐨勫崗绋嬭皟鐢ㄦ湰鍑芥暟杩涜瑙ｉ攣锛岃皟鐢ㄦ湰鍑芥暟鐨勫崗绋嬪繀椤绘槸璇ラ攣鐨勫睘涓伙紝鍚﹀垯
 * 鍐呴儴浼氫骇鐢熸柇瑷€
 * @param l {ACL_FIBER_MUTEX*} 鐢� acl_fiber_mutex_create 鍒涘缓鐨勫崗绋嬩簰鏂ラ攣
 */
void acl_fiber_mutex_unlock(ACL_FIBER_MUTEX* l);

/**
 * 鍒涘缓鍗忕▼璇诲啓閿侊紝绾跨▼闈炲畨鍏紝鍙兘鐢ㄥ湪鍚屼竴绾跨▼鍐�
 * @return {ACL_FIBER_RWLOCK*}
 */
ACL_FIBER_RWLOCK* acl_fiber_rwlock_create(void);

/**
 * 閲婃斁鍗忕▼璇诲啓閿�
 * @param l {ACL_FIBER_RWLOCK*} 鐢� acl_fiber_rwlock_create 鍒涘缓鐨勮鍐欓攣
 */
void acl_fiber_rwlock_free(ACL_FIBER_RWLOCK* l);

/**
 * 瀵瑰崗绋嬭鍐欓攣鍔犺閿侊紝濡傛灉璇ラ攣褰撳墠姝ｈ鍏跺畠鍗忕▼鍔犱簡璇婚攣锛屽垯鏈崗绋嬩緷鐒跺彲浠�
 * 姝ｅ父鍔犺閿侊紝濡傛灉璇ラ攣褰撳墠姝ｈ鍏跺畠鍗忕▼鍔犱簡鍐欓攣锛屽垯鏈崗绋嬭繘鍏ラ樆濉炵姸鎬侊紝鐩磋嚦
 * 鍐欓攣閲婃斁
 * @param l {ACL_FIBER_RWLOCK*} 鐢� acl_fiber_rwlock_create 鍒涘缓鐨勮鍐欓攣
 */
void acl_fiber_rwlock_rlock(ACL_FIBER_RWLOCK* l);

/**
 * 瀵瑰崗绋嬭鍐欓攣灏濊瘯鎬у姞璇婚攣锛屽姞閿佹棤璁烘槸鍚︽垚鍔熼兘浼氱珛鍗宠繑鍥�
 * @param l {ACL_FIBER_RWLOCK*} 鐢� acl_fiber_rwlock_create 鍒涘缓鐨勮鍐欓攣
 * @retur {int} 杩斿洖 1 琛ㄧず鍔犻攣鎴愬姛锛岃繑鍥� 0 琛ㄧず鍔犻攣澶辫触
 */
int acl_fiber_rwlock_tryrlock(ACL_FIBER_RWLOCK* l);

/**
 * 瀵瑰崗绋嬭鍐欓攣鍔犲啓閿侊紝鍙湁褰撹閿佹湭琚换浣曞崗绋嬪姞璇�/鍐欓攣鏃舵墠浼氳繑鍥烇紝鍚﹀垯闃诲锛�
 * 鐩磋嚦璇ラ攣鍙姞鍐欓攣
 * @param l {ACL_FIBER_RWLOCK*} 鐢� acl_fiber_rwlock_create 鍒涘缓鐨勮鍐欓攣
 */
void acl_fiber_rwlock_wlock(ACL_FIBER_RWLOCK* l);

/**
 * 瀵瑰崗绋嬭鍐欓攣灏濊瘯鎬у姞鍐欓攣锛屾棤璁烘槸鍚﹀姞閿佹垚鍔熼兘浼氱珛鍗宠繑鍥�
 * @param l {ACL_FIBER_RWLOCK*} 鐢� acl_fiber_rwlock_create 鍒涘缓鐨勮鍐欓攣
 * @return {int} 杩斿洖 1 琛ㄧず鍔犲啓閿佹垚鍔燂紝杩斿洖 0 琛ㄧず鍔犻攣澶辫触
 */
int acl_fiber_rwlock_trywlock(ACL_FIBER_RWLOCK* l);

/**
 * 瀵瑰崗绋嬭鍐欓攣鎴愬姛鍔犺閿佺殑鍗忕▼璋冪敤鏈嚱鏁拌В璇婚攣锛岃皟鐢ㄨ€呭繀椤绘槸涔嬪墠宸叉垚鍔熷姞璇�
 * 閿佹垚鍔熺殑鍗忕▼
 * @param l {ACL_FIBER_RWLOCK*} 鐢� acl_fiber_rwlock_create 鍒涘缓鐨勮鍐欓攣
 */
void acl_fiber_rwlock_runlock(ACL_FIBER_RWLOCK* l);
/**
 * 瀵瑰崗绋嬭鍐欓攣鎴愬姛鍔犲啓閿佺殑鍗忕▼璋冪敤鏈嚱鏁拌В鍐欓攣锛岃皟鐢ㄨ€呭繀椤绘槸涔嬪墠宸叉垚鍔熷姞鍐�
 * 閿佹垚鍔熺殑鍗忕▼
 * @param l {ACL_FIBER_RWLOCK*} 鐢� acl_fiber_rwlock_create 鍒涘缓鐨勮鍐欓攣
 */
void acl_fiber_rwlock_wunlock(ACL_FIBER_RWLOCK* l);

/* fiber_event.c */

/* 绾跨▼瀹夊叏鐨勫崗绋嬮攣锛屽彲浠ョ敤鍦ㄤ笉鍚岀嚎绋嬬殑鍗忕▼涔嬮棿鍙婁笉鍚岀嚎绋嬩箣闂寸殑浜掓枼 */
typedef struct ACL_FIBER_EVENT ACL_FIBER_EVENT;

/**
 * 鍒涘缓鍩轰簬浜嬩欢鐨勫崗绋�/绾跨▼娣峰悎閿�
 * @return {ACL_FIBER_EVENT *}
 */
ACL_FIBER_EVENT *acl_fiber_event_create(void);

/**
 * 閲婃斁浜嬩欢閿�
 * @param {ACL_FIBER_EVENT *}
 */
void acl_fiber_event_free(ACL_FIBER_EVENT *event);

/**
 * 绛夊緟浜嬩欢閿佸彲鐢�
 * @param {ACL_FIBER_EVENT *}
 * @return {int} 杩斿洖 0 琛ㄧず鎴愬姛锛�-1 琛ㄧず鍑洪敊
 */
int acl_fiber_event_wait(ACL_FIBER_EVENT *event);

/**
 * 灏濊瘯绛夊緟浜嬩欢閿佸彲鐢�
 * @param {ACL_FIBER_EVENT *}
 * @return {int} 杩斿洖 0 琛ㄧず鎴愬姛锛�-1 琛ㄧず閿佽鍗犵敤
 */
int acl_fiber_event_trywait(ACL_FIBER_EVENT *event);

/**
 * 浜嬩欢閿佹嫢鏈夎€呴€氱煡绛夊緟鑰呬簨浠堕攣鍙敤锛屽垯绛夊緟鑰呮敹鍒伴€氱煡鍚庡垯鍙幏寰椾簨浠堕攣
 * @param {ACL_FIBER_EVENT *}
 * @return {int} 杩斿洖 0 琛ㄧず鎴愬姛锛�-1 琛ㄧず鍑洪敊
 */
int acl_fiber_event_notify(ACL_FIBER_EVENT *event);

/* fiber semaphore */

typedef struct ACL_FIBER_SEM ACL_FIBER_SEM;

/**
 * 鍒涘缓鍗忕▼淇″彿閲忥紝鍚屾椂鍐呴儴浼氬皢褰撳墠绾跨▼涓庤淇″彿閲忕粦瀹�
 * @param num {int} 淇″彿閲忓垵濮嬪€硷紙蹇呴』 >= 0锛�
 * @return {ACL_FIBER_SEM *}
 */
ACL_FIBER_SEM* acl_fiber_sem_create(int num);

/**
 * 閲婃斁鍗忕▼淇″彿閲�
 * @param {ACL_FIBER_SEM *}
 */
void acl_fiber_sem_free(ACL_FIBER_SEM* sem);

/**
 * 鑾峰緱褰撳墠鍗忕▼淇″彿閲忔墍缁戝畾鐨勭嚎绋� ID
 * @param sem {ACL_FIBER_SEM*} 鍗忕▼淇″彿閲忓璞�
 * @return {acl_pthread_t}
 */
acl_pthread_t acl_fiber_sem_get_tid(ACL_FIBER_SEM* sem);

/**
 * 璁剧疆鎸囧畾鍗忕▼淇″彿閲忕殑鐨勭嚎绋� ID锛屽綋鏀瑰彉鏈崗绋嬩俊鍙烽噺鎵€灞炵殑绾跨▼鏃跺鏋滅瓑寰呯殑鍗忕▼
 * 鏁版嵁闈� 0 鍒欏唴閮ㄨ嚜鍔� fatal锛屽嵆褰撳崗绋嬩俊鍙烽噺涓婄瓑寰呭崗绋嬮潪绌烘椂绂佹璋冪敤鏈柟娉�
 * @param sem {ACL_FIBER_SEM*} 鍗忕▼淇″彿閲忓璞�
 * @param {acl_pthread_t} 绾跨▼ ID
 */
void acl_fiber_sem_set_tid(ACL_FIBER_SEM* sem, acl_pthread_t tid);

/**
 * 褰撳崗绋嬩俊鍙烽噺 > 0 鏃朵娇淇″彿閲忓噺 1锛屽惁鍒欑瓑寰呬俊鍙烽噺 > 0
 * @param sem {ACL_FIBER_SEM *}
 * @retur {int} 杩斿洖淇″彿閲忓綋鍓嶅€硷紝濡傛灉杩斿洖 -1 琛ㄦ槑褰撳墠绾跨▼涓庡崗绋嬩俊鍙烽噺鎵€灞炵嚎绋�
 *  涓嶆槸鍚屼竴绾跨▼锛屾鏃惰鏂规硶涓嶇瓑寰呯珛鍗宠繑鍥�
 */
int acl_fiber_sem_wait(ACL_FIBER_SEM* sem);

/**
 * 灏濊瘯浣垮崗绋嬩俊鍙烽噺鍑� 1
 * @param sem {ACL_FIBER_SEM *}
 * @retur {int} 鎴愬姛鍑� 1 鏃惰繑鍥炲€� >= 0锛岃繑鍥� -1 琛ㄧず褰撳墠淇″彿閲忎笉鍙敤锛屾垨褰撳墠
 *  璋冪敤鑰呯嚎绋嬩笌鍗忕▼淇″彿閲忔墍灞炵嚎绋嬩笉鏄悓涓€绾跨▼
 */
int acl_fiber_sem_trywait(ACL_FIBER_SEM* sem);

/**
 * 浣垮崗绋嬩俊鍙烽噺鍔� 1
 * @param sem {ACL_FIBER_SEM *}
 * @retur {int} 杩斿洖淇″彿閲忓綋鍓嶅€硷紝杩斿洖 -1 琛ㄧず褰撳墠璋冪敤鑰呯嚎绋嬩笌鍗忕▼淇″彿閲忔墍灞�
 *  绾跨▼涓嶆槸鍚屼竴绾跨▼
 */
int acl_fiber_sem_post(ACL_FIBER_SEM* sem);

/**
 * 鑾峰緱鎸囧畾鍗忕▼淇″彿閲忕殑褰撳墠鍊硷紝璇ュ€煎弽鏄犱簡鐩墠绛夊緟璇ヤ俊鍙烽噺鐨勬暟閲�
 * @param sem {ACL_FIBER_SEM*}
 * @retur {int}
 */
int acl_fiber_sem_num(ACL_FIBER_SEM* sem);

/* channel communication */

/**
 * 鍗忕▼闂撮€氫俊鐨勭閬�
 */
typedef struct ACL_CHANNEL ACL_CHANNEL;

/**
 * 鍒涘缓鍗忕▼閫氫俊绠￠亾
 * @param elemsize {int} 鍦� ACL_CHANNEL 杩涜浼犺緭鐨勫璞＄殑鍥哄畾灏哄澶у皬锛堝瓧鑺傦級
 * @param bufsize {int} ACL_CHANNEL 鍐呴儴缂撳啿鍖哄ぇ灏忥紝鍗冲彲浠ョ紦瀛� elemsize 灏哄澶у皬
 *  瀵硅薄鐨勪釜鏁�
 * @return {CHANNNEL*}
 */
ACL_CHANNEL* acl_channel_create(int elemsize, int bufsize);

/**
 * 閲婃斁鐢� acl_channel_create 鍒涘缓鐨勫崗绋嬮€氫俊绠￠亾瀵硅薄
 * @param c {ACL_CHANNEL*} 鐢� acl_channel_create 鍒涘缓鐨勭閬撳璞�
 */
void acl_channel_free(ACL_CHANNEL* c);

/**
 * 闃诲寮忓悜鎸囧畾 ACL_CHANNEL 涓彂閫佹寚瀹氱殑瀵硅薄鍦板潃
 * @param c {ACL_CHANNEL*} 鐢� acl_channel_create 鍒涘缓鐨勭閬撳璞�
 * @param v {void*} 琚彂閫佺殑瀵硅薄鍦板潃
 * @return {int} 杩斿洖鍊� >= 0
 */
int acl_channel_send(ACL_CHANNEL* c, void* v);

/**
 * 闈為樆濉炲紡鍚戞寚瀹� ACL_CHANNEL 涓彂閫佹寚瀹氱殑瀵硅薄锛屽唴閮ㄤ細鏍规嵁 acl_channel_create 涓寚瀹�
 * 鐨� elemsize 瀵硅薄澶у皬杩涜鏁版嵁鎷疯礉
 * @param c {ACL_CHANNEL*} 鐢� acl_channel_create 鍒涘缓鐨勭閬撳璞�
 * @param v {void*} 琚彂閫佺殑瀵硅薄鍦板潃
 */
int acl_channel_send_nb(ACL_CHANNEL* c, void* v);

/**
 * 浠庢寚瀹氱殑 ACL_CHANNEL 涓樆濉炲紡璇诲彇瀵硅薄锛�
 * @param c {ACL_CHANNEL*} 鐢� acl_channel_create 鍒涘缓鐨勭閬撳璞�
 * @param v {void*} 瀛樻斁缁撴灉鍐呭
 * @return {int} 杩斿洖鍊� >= 0 琛ㄧず鎴愬姛璇诲埌鏁版嵁
 */
int acl_channel_recv(ACL_CHANNEL* c, void* v);

/**
 * 浠庢寚瀹氱殑 ACL_CHANNEL 涓潪闃诲寮忚鍙栧璞★紝鏃犺鏄惁璇诲埌鏁版嵁閮戒細绔嬪嵆杩斿洖
 * @param c {ACL_CHANNEL*} 鐢� acl_channel_create 鍒涘缓鐨勭閬撳璞�
 * @param v {void*} 瀛樻斁缁撴灉鍐呭
 * @return {int} 杩斿洖鍊� >= 0 琛ㄧず鎴愬姛璇诲埌鏁版嵁锛屽惁鍒欒〃绀烘湭璇诲埌鏁版嵁
 */
int acl_channel_recv_nb(ACL_CHANNEL* c, void* v);

/**
 * 鍚戞寚瀹氱殑 ACL_CHANNEL 涓樆濉炲紡鍙戦€佹寚瀹氬璞＄殑鍦板潃
 * @param c {ACL_CHANNEL*} 鐢� acl_channel_create 鍒涘缓鐨勭閬撳璞�
 * @param v {void*} 琚彂閫佸璞＄殑鍦板潃
 * @return {int} 杩斿洖鍊� >= 0
 */
int acl_channel_sendp(ACL_CHANNEL* c, void* v);

/**
 * 浠庢寚瀹氱殑 CHANNLE 涓樆濉炲紡鎺ユ敹鐢� acl_channel_sendp 鍙戦€佺殑瀵硅薄鐨勫湴鍧€
 * @param c {ACL_CHANNEL*} 鐢� acl_channel_create 鍒涘缓鐨勭閬撳璞�
 * @return {void*} 杩斿洖闈� NULL锛屾寚瀹氭帴鏀跺埌鐨勫璞＄殑鍦板潃
 */
void* acl_channel_recvp(ACL_CHANNEL* c);

/**
 * 鍚戞寚瀹氱殑 ACL_CHANNEL 涓潪闃诲寮忓彂閫佹寚瀹氬璞＄殑鍦板潃
 * @param c {ACL_CHANNEL*} 鐢� acl_channel_create 鍒涘缓鐨勭閬撳璞�
 * @param v {void*} 琚彂閫佸璞＄殑鍦板潃
 * @return {int} 杩斿洖鍊� >= 0
 */
int acl_channel_sendp_nb(ACL_CHANNEL* c, void* v);

/**
 * 浠庢寚瀹氱殑 CHANNLE 涓樆濉炲紡鎺ユ敹鐢� acl_channel_sendp 鍙戦€佺殑瀵硅薄鐨勫湴鍧€
 * @param c {ACL_CHANNEL*} 鐢� acl_channel_create 鍒涘缓鐨勭閬撳璞�
 * @return {void*} 杩斿洖闈� NULL锛屾寚瀹氭帴鏀跺埌鐨勫璞＄殑鍦板潃锛屽鏋滆繑鍥� NULL 琛ㄧず
 *  娌℃湁璇诲埌浠讳綍瀵硅薄
 */
void* acl_channel_recvp_nb(ACL_CHANNEL* c);

/**
 * 鍚戞寚瀹氱殑 ACL_CHANNEL 涓彂閫佹棤绗﹀彿闀挎暣褰㈡暟鍊�
 * @param c {ACL_CHANNEL*} 鐢� acl_channel_create 鍒涘缓鐨勭閬撳璞�
 * @param val {unsigned long} 瑕佸彂閫佺殑鏁板€�
 * @return {int} 杩斿洖鍊� >= 0
 */
int acl_channel_sendul(ACL_CHANNEL* c, unsigned long val);

/**
 * 浠庢寚瀹氱殑 ACL_CHANNEL 涓帴鏀舵棤绗﹀彿闀挎暣褰㈡暟鍊�
 * @param c {ACL_CHANNEL*} 鐢� acl_channel_create 鍒涘缓鐨勭閬撳璞�
 * @return {unsigned long}
 */
unsigned long acl_channel_recvul(ACL_CHANNEL* c);

/**
 * 鍚戞寚瀹氱殑 ACL_CHANNEL 涓互闈為樆濉炴柟寮忓彂閫佹棤绗﹀彿闀挎暣褰㈡暟鍊�
 * @param c {ACL_CHANNEL*} 鐢� acl_channel_create 鍒涘缓鐨勭閬撳璞�
 * @param val {unsigned long} 瑕佸彂閫佺殑鏁板€�
 * @return {int} 杩斿洖鍊� >= 0
 */
int acl_channel_sendul_nb(ACL_CHANNEL* c, unsigned long val);

/**
 * 浠庢寚瀹氱殑 ACL_CHANNEL 涓互闈為樆濉炴柟寮忔帴鏀舵棤绗﹀彿闀挎暣褰㈡暟鍊�
 * @param c {ACL_CHANNEL*} 鐢� acl_channel_create 鍒涘缓鐨勭閬撳璞�
 * @return {unsigned long}
 */
unsigned long acl_channel_recvul_nb(ACL_CHANNEL* c);

/* master fibers server */

/**
 * 鍩轰簬鍗忕▼鐨勬湇鍔″櫒涓诲嚱鏁板叆鍙ｏ紝璇ユā鍧楀彲浠ュ湪 acl_master 鏈嶅姟鍣ㄦ帶鍒舵鏋朵笅杩愯
 * @param argc {int} 浣跨敤鑰呬紶鍏ョ殑鍙傛暟鏁扮粍 argv 鐨勫ぇ灏�
 * @param argv {char*[]} 鍙傛暟鏁扮粍澶у皬
 * @param service {void (*)(ACL_VSTREAM*, void*)} 鎺ユ敹鍒颁竴涓柊瀹㈡埛绔繛鎺ヨ姹�
 *  鍚庡垱寤轰竴涓崗绋嬪洖璋冩湰鍑芥暟
 * @param ctx {void*} service 鍥炶皟鍑芥暟鐨勭浜屼釜鍙傛暟
 * @param name {int} 鎺у埗鍙傛暟鍒楄〃涓殑绗竴涓帶鍒跺弬鏁�
 */
void acl_fiber_server_main(int argc, char* argv[],
	void (*service)(void*, ACL_VSTREAM*), void* ctx, int name, ...);
const char* acl_fiber_server_conf(void);

void acl_fiber_chat_main(int argc, char* argv[],
	int (*service)(ACL_VSTREAM*, void*), void* ctx, int name, ...);

/**************************** fiber iostuff *********************************/

ssize_t fiber_read(int fd, void* buf, size_t count);
ssize_t fiber_readv(int fd, const struct iovec* iov, int iovcnt);
ssize_t fiber_recv(int sockfd, void* buf, size_t len, int flags);
ssize_t fiber_recvfrom(int sockfd, void* buf, size_t len, int flags,
	struct sockaddr* src_addr, socklen_t* addrlen);
ssize_t fiber_recvmsg(int sockfd, struct msghdr* msg, int flags);

ssize_t fiber_write(int fd, const void* buf, size_t count);
ssize_t fiber_writev(int fd, const struct iovec* iov, int iovcnt);
ssize_t fiber_send(int sockfd, const void* buf, size_t len, int flags);
ssize_t fiber_sendto(int sockfd, const void* buf, size_t len, int flags,
	const struct sockaddr* dest_addr, socklen_t addrlen);
ssize_t fiber_sendmsg(int sockfd, const struct msghdr* msg, int flags);

/****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
