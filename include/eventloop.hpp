#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ps2.hpp"

// class used by applications to handle keyboard state and event loop
class Frame;
class EventLoop {
private:
	friend Frame;
	virtual void frame_startup() = 0;
	virtual void frame_teardown() = 0;
public:
	virtual void poll() = 0;
	Frame get_frame(uint32_t frame_length_ms);
};

class Frame {
	EventLoop &owner;
	uint32_t frame_end;
public:
	Frame(EventLoop &owner, uint32_t frame_length);
	~Frame();

	Frame(const Frame&) = delete;
	Frame &operator=(const Frame&) = delete;
	Frame(Frame&&) = default;
	Frame &operator=(Frame&&) = default;
};

class EventQueueIterator;
class EventQueue {
	ps2::Event *event_buf;
	size_t head, tail;
	size_t size;
	size_t dropped;
public:
	using iterator = EventQueueIterator;
	using const_iterator = EventQueueIterator;

	EventQueue(size_t queue_size = 1024);
	~EventQueue();

	EventQueue(const EventQueue&) = delete;
	EventQueue &operator=(const EventQueue&) = delete;
	EventQueue(EventQueue&&) = default;
	EventQueue &operator=(EventQueue&&) = default;

	void push(ps2::Event event);
	bool empty() const;
	ps2::Event pop();
	size_t get_dropped() const;
	void clear();

	iterator begin() const;
	iterator end() const;
};
class EventQueueIterator {
	friend EventQueue;

	const ps2::Event *buf;
	size_t size;
	size_t at;
	size_t tail;

	EventQueueIterator(const ps2::Event *buf, size_t size, size_t at, size_t tail);
public:
	ps2::Event operator*() const;
	EventQueueIterator &operator++();
	bool operator!=(const EventQueueIterator &other) const;
};

class QueuedEventLoop : public EventLoop {
	EventQueue q0, q1;
	EventQueue *consumer, *producer;

	virtual void frame_startup() override;
	virtual void frame_teardown() override;
public:
	QueuedEventLoop(size_t queue_size = 1024);

	virtual void poll() override;

	EventQueue &events();
	const EventQueue &events() const;
};

class CallbackEventLoop : public EventLoop {
public:
	using cb_t = void(*)(void*, ps2::Event);

private:
	cb_t cb;
	void *arg;
	
	virtual void frame_startup() override;
	virtual void frame_teardown() override;
public:
	CallbackEventLoop(cb_t cb, void *arg);

	virtual void poll() override;
};

class IgnoreEventLoop : public EventLoop {
	virtual void frame_startup() override;
	virtual void frame_teardown() override;
public:
	IgnoreEventLoop() = default;

	virtual void poll() override;
};
