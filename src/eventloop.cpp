#include "eventloop.hpp"

#include <stdlib.h>

#include "pit.hpp"

Frame EventLoop::get_frame(uint32_t frame_length_ms) {
	return Frame(*this, frame_length_ms);
}

namespace {
void eventloop_poll_wrapper(void *eventloop) {
	static_cast<EventLoop*>(eventloop)->poll();
}
}

Frame::Frame(EventLoop &owner, uint32_t frame_length)
: owner(owner), frame_end(pit::millis + frame_length)
{
	owner.frame_startup();
}
Frame::~Frame() {
	owner.frame_teardown();
	pit::sleep_until<true>(frame_end, eventloop_poll_wrapper, &owner);
}

EventQueue::EventQueue(size_t queue_size) : size(queue_size), dropped(0) {
	if (size == 0) abort();
	event_buf = (ps2::Event*)calloc(size, sizeof(*event_buf));
	head = 0;
	tail = 0;
}
EventQueue::~EventQueue() {
	free(event_buf);
}

void EventQueue::push(ps2::Event event) {
	event_buf[tail++] = event;
	if (tail == size) tail = 0;
	if (tail == head) {
		++head;
		++dropped;
	}
}
bool EventQueue::empty() const {
	return head == tail;
}
ps2::Event EventQueue::pop() {
	if (empty()) abort();
	return event_buf[head++];
}
size_t EventQueue::get_dropped() const {
	return dropped;
}
void EventQueue::clear() {
	head = 0;
	tail = 0;
}

EventQueueIterator EventQueue::begin() const {
	return { event_buf, size, head, tail };
}
EventQueueIterator EventQueue::end() const {
	return { event_buf, size, tail, tail };
}

EventQueueIterator::EventQueueIterator(const ps2::Event *buf, size_t size, size_t at, size_t tail)
: buf(buf), size(size), at(at), tail(tail) {}

ps2::Event EventQueueIterator::operator*() const {
	return buf[at];
}
EventQueueIterator &EventQueueIterator::operator++() {
	++at;
	if (at == size) at = 0;
	return *this;
}
bool EventQueueIterator::operator!=(const EventQueueIterator &other) const {
	return buf != other.buf || size != other.size || at != other.at || tail != other.tail;
}

void QueuedEventLoop::frame_startup() {
	__asm__ volatile("cli" ::: "memory");
	EventQueue *tmp = consumer;
	consumer = producer;
	producer = tmp;
	producer->clear();
	__asm__ volatile("sti" ::: "memory");
}
void QueuedEventLoop::frame_teardown() { }

QueuedEventLoop::QueuedEventLoop(size_t queue_size)
: q0(queue_size), q1(queue_size) {
	consumer = &q0;
	producer = &q1;
}

void QueuedEventLoop::poll() {
	while (!ps2::events.empty()) {
		producer->push(ps2::events.pop());
	}
}

EventQueue &QueuedEventLoop::events() {
	return *consumer;
}
const EventQueue &QueuedEventLoop::events() const {
	return *consumer;
}

void CallbackEventLoop::frame_startup() { }
void CallbackEventLoop::frame_teardown() { }

CallbackEventLoop::CallbackEventLoop(CallbackEventLoop::cb_t cb, void *arg)
: cb(cb), arg(arg) { }

void CallbackEventLoop::poll() {
	while (!ps2::events.empty()) {
		cb(arg, ps2::events.pop());
	}
}

void IgnoreEventLoop::frame_startup() { }
void IgnoreEventLoop::frame_teardown() { }

void IgnoreEventLoop::poll() { }
