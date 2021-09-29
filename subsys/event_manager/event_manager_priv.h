/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Event manager private header.
 *
 * Although these defines are globally visible they must not be used directly.
 */

#ifndef _EVENT_MANAGER_PRIV_H_
#define _EVENT_MANAGER_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

#if !CONFIG_X86
#define _EM_FORCED_ALIGNMENT
#else
#define _EM_FORCED_ALIGNMENT __aligned(4)
#endif

/* There are 3 levels of priorities defining an order at which event listeners
 * are notified about incoming events.
 */

#define _SUBS_PRIO_FIRST  0
#define _SUBS_PRIO_NORMAL 1
#define _SUBS_PRIO_FINAL  2


/* Convenience macros generating section names. */

#define _SUBS_PRIO_ID(level) _CONCAT(_CONCAT(_prio, level), _)

#define _EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio)	_CONCAT(_CONCAT(event_subscribers_, ename), prio)

#define _EVENT_SUBSCRIBERS_SECTION_NAME(ename, prio)	STRINGIFY(_EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio))


/* Declare a zero-length subscriber. */
#define _EVENT_SUBSCRIBERS_EMPTY(ename, prio)							\
	const struct {} _CONCAT(_EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio), empty)		\
	__used _EM_FORCED_ALIGNMENT								\
	__attribute__((__section__(_EVENT_SUBSCRIBERS_SECTION_NAME(ename, prio)))) = {};


/* Convenience macros generating section start and stop markers. */

#define _EVENT_SUBSCRIBERS_START(ename, prio)	_CONCAT(__start_, _EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio))

#define _EVENT_SUBSCRIBERS_STOP(ename, prio)	_CONCAT(__stop_,  _EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, prio))


#define _EVENT_SUBSCRIBERS_DECLARE(ename)										\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FIRST))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_STOP(ename,  _SUBS_PRIO_ID(_SUBS_PRIO_FIRST))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_STOP(ename,  _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FINAL))[];	\
	extern const struct event_subscriber _EVENT_SUBSCRIBERS_STOP(ename,  _SUBS_PRIO_ID(_SUBS_PRIO_FINAL))[];


/* Macro defining empty subscribers on each priority level.
 * Each event type keeps an array of subscribers for every priority level.
 * It can happen that for a given priority no subscriber will be registered.
 * This macro declares zero-length subscriber that will cause required section
 * to be generated by the linker. If no subscriber is registered at this
 * level section will remain empty.
 */
#define _EVENT_SUBSCRIBERS_DEFINE(ename)					\
	_EVENT_SUBSCRIBERS_EMPTY(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FIRST))	\
	_EVENT_SUBSCRIBERS_EMPTY(ename, _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL))	\
	_EVENT_SUBSCRIBERS_EMPTY(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FINAL))


/* Subscribe a listener to an event. */
#define _EVENT_SUBSCRIBE(lname, ename, prio)							\
	const struct event_subscriber _CONCAT(_CONCAT(__event_subscriber_, ename), lname)	\
	__used _EM_FORCED_ALIGNMENT								\
	__attribute__((__section__(_EVENT_SUBSCRIBERS_SECTION_NAME(ename, prio)))) = {		\
		.listener = &_CONCAT(__event_listener_, lname),					\
	}


/* Pointer to event type definition is used as event type identifier. */
#define _EVENT_ID(ename) (&_CONCAT(__event_type_, ename))


/* Macro generates a function of name new_ename where ename is provided as
 * an argument. Allocator function is used to create an event of the given
 * ename type.
 */
#define _EVENT_ALLOCATOR_FN(ename)					\
	static inline struct ename *_CONCAT(new_, ename)(void)		\
	{								\
		struct ename *event = (struct ename *)k_malloc(sizeof(*event));\
		BUILD_ASSERT(offsetof(struct ename, header) == 0,	\
				 "");					\
		if (unlikely(!event)) {					\
			printk("Event Manager OOM error\n");		\
			LOG_PANIC();					\
			__ASSERT_NO_MSG(false);				\
			sys_reboot(SYS_REBOOT_WARM);			\
			return NULL;					\
		}							\
		event->header.type_id = _EVENT_ID(ename);		\
		return event;						\
	}


/* Macro generates a function of name new_ename where ename is provided as
 * an argument. Allocator function is used to create an event of the given
 * ename type.
 */
#define _EVENT_ALLOCATOR_DYNDATA_FN(ename)				\
	static inline struct ename *_CONCAT(new_, ename)(size_t size)	\
	{								\
		struct ename *event = (struct ename *)k_malloc(sizeof(*event) + size);\
		BUILD_ASSERT((offsetof(struct ename, dyndata) +		\
				  sizeof(event->dyndata.size)) ==	\
				 sizeof(*event), "");			\
		BUILD_ASSERT(offsetof(struct ename, header) == 0,	\
				 "");					\
		if (unlikely(!event)) {					\
			printk("Event Manager OOM error\n");		\
			LOG_PANIC();					\
			__ASSERT_NO_MSG(false);				\
			sys_reboot(SYS_REBOOT_WARM);			\
			return NULL;					\
		}							\
		event->header.type_id = _EVENT_ID(ename);		\
		event->dyndata.size = size;				\
		return event;						\
	}


/* Macro generates a function of name cast_ename where ename is provided as
 * an argument. Casting function is used to convert event_header pointer
 * into pointer to event matching the given ename type.
 */
#define _EVENT_CASTER_FN(ename)									\
	static inline struct ename *_CONCAT(cast_, ename)(const struct event_header *eh)	\
	{											\
		struct ename *event = NULL;							\
		if (eh->type_id == _EVENT_ID(ename)) {						\
			event = CONTAINER_OF(eh, struct ename, header);				\
		}										\
		return event;									\
	}


/* Macro generates a function of name is_ename where ename is provided as
 * an argument. Typecheck function is used to check if pointer to event_header
 * belongs to the event matching the given ename type.
 */
#define _EVENT_TYPECHECK_FN(ename) \
	static inline bool _CONCAT(is_, ename)(const struct event_header *eh)	\
	{									\
		return (eh->type_id == _EVENT_ID(ename));			\
	}


/* Wrappers used for defining event infos */
#ifdef CONFIG_EVENT_MANAGER_TRACE_EVENT_EXECUTION
#define EM_MEM_ADDRESS_LABEL "_em_mem_address_",
#define MEM_ADDRESS_TYPE PROFILER_ARG_U32,

#else
#define EM_MEM_ADDRESS_LABEL
#define MEM_ADDRESS_TYPE

#endif /* CONFIG_EVENT_MANAGER_TRACE_EVENT_EXECUTION */


#ifdef CONFIG_EVENT_MANAGER_PROFILE_EVENT_DATA
#define _ARG_LABELS_DEFINE(...) \
	{EM_MEM_ADDRESS_LABEL __VA_ARGS__}
#define _ARG_TYPES_DEFINE(...) \
	 {MEM_ADDRESS_TYPE __VA_ARGS__}

#else
#define _ARG_LABELS_DEFINE(...) \
	{EM_MEM_ADDRESS_LABEL}
#define _ARG_TYPES_DEFINE(...) \
	 {MEM_ADDRESS_TYPE}

#endif /* CONFIG_EVENT_MANAGER_PROFILE_EVENT_DATA */


/* Declarations and definitions - for more details refer to public API. */
#define _EVENT_INFO_DEFINE(ename, types, labels, profile_func)							\
	const static char *_CONCAT(ename, _log_arg_labels[]) __used = _ARG_LABELS_DEFINE(labels);		\
	const static enum profiler_arg _CONCAT(ename, _log_arg_types[]) __used = _ARG_TYPES_DEFINE(types);	\
	const static struct event_info _CONCAT(ename, _info) __used _EM_FORCED_ALIGNMENT			\
	__attribute__((__section__("event_infos"))) = {								\
				.profile_fn	= profile_func,							\
				.log_arg_cnt	= ARRAY_SIZE(_CONCAT(ename, _log_arg_labels)),			\
				.log_arg_labels	= _CONCAT(ename, _log_arg_labels),				\
				.log_arg_types	= _CONCAT(ename, _log_arg_types)				\
			}



#define _EVENT_LISTENER(lname, notification_fn)						\
	const struct event_listener _CONCAT(__event_listener_, lname)			\
	__used _EM_FORCED_ALIGNMENT							\
	__attribute__((__section__("event_listeners"))) = {				\
		.name = STRINGIFY(lname),						\
		.notification = (notification_fn),					\
	}


#define _EVENT_TYPE_DECLARE_COMMON(ename)				\
	extern const struct event_type _CONCAT(__event_type_, ename);	\
	_EVENT_SUBSCRIBERS_DECLARE(ename);				\
	_EVENT_CASTER_FN(ename);					\
	_EVENT_TYPECHECK_FN(ename)


#define _EVENT_TYPE_DECLARE(ename)					\
	_EVENT_TYPE_DECLARE_COMMON(ename);				\
	_EVENT_ALLOCATOR_FN(ename)


#define _EVENT_TYPE_DYNDATA_DECLARE(ename)				\
	_EVENT_TYPE_DECLARE_COMMON(ename);				\
	_EVENT_ALLOCATOR_DYNDATA_FN(ename)


#define _EVENT_TYPE_DEFINE(ename, init_log_en, log_fn, ev_info_struct)							\
	_EVENT_SUBSCRIBERS_DEFINE(ename);										\
	const struct event_type _CONCAT(__event_type_, ename) __used _EM_FORCED_ALIGNMENT				\
	__attribute__((__section__("event_types"))) = {									\
		.name				= STRINGIFY(ename),							\
		.subs_start	= {											\
			[_SUBS_PRIO_FIRST]	= _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FIRST)),	\
			[_SUBS_PRIO_NORMAL]	= _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL)),	\
			[_SUBS_PRIO_FINAL]	= _EVENT_SUBSCRIBERS_START(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FINAL)),	\
		},													\
		.subs_stop	= {											\
			[_SUBS_PRIO_FIRST]	= _EVENT_SUBSCRIBERS_STOP(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FIRST)),	\
			[_SUBS_PRIO_NORMAL]	= _EVENT_SUBSCRIBERS_STOP(ename, _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL)),	\
			[_SUBS_PRIO_FINAL]	= _EVENT_SUBSCRIBERS_STOP(ename, _SUBS_PRIO_ID(_SUBS_PRIO_FINAL)),	\
		},													\
		.init_log_enable		= init_log_en,								\
		.log_event			= log_fn,								\
		.ev_info			= ev_info_struct,							\
	}


/**
 * @brief Bitmask indicating event is displayed.
 */
struct event_manager_event_display_bm {
	ATOMIC_DEFINE(flags, CONFIG_EVENT_MANAGER_MAX_EVENT_CNT);
};

extern struct event_manager_event_display_bm _event_manager_event_display_bm;

#ifdef __cplusplus
}
#endif

#endif /* _EVENT_MANAGER_PRIV_H_ */
