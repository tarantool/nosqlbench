#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include "async_io.h"
#include <ev.h>
#include <stdbool.h>

#define DEFAULT_BUF_SIZE 1024

struct async_io_buf {
	char *data;
	size_t size;
	size_t iter;
};

struct async_io {
	struct async_io_if io_if;
	void *user_data;
	struct ev_loop *loop;
	bool is_shutdown;
	int sock;
	struct ev_io r_client;
	struct ev_io w_client;
	/* Needed count of requests per second. */
	uint32_t rps;
	/*
	 * Current writes per second. If current RPS less than
	 * needed then writes_ps is increasing else is decreasing.
	 */
	double writes_ps;
	struct ev_timer timeout_watcher;
	uint32_t req_per_timeout;
	/*
	 * This timer every second checks the current rps and
	 * updates writes_ps according to it.
	 */
	struct ev_timer rps_observer;
	/*
	 * Count of received answers at the previous timeout of
	 * rps_observer.
	 */
	uint32_t received_prev;
	struct async_io_buf read_buf;
	/*
	 * If the is_shutdown is set and received == need_to_receive then
	 * the event loop breaks.
	 */
	size_t received;
	size_t need_to_receive;

	struct async_io_buf write_buf;
	/* Count of really used bytes in write_buf. */
	size_t buf_busy;
};

static int
async_io_time_to_finish(struct async_io *obj)
{
	return obj->is_shutdown && obj->received == obj->need_to_receive;
}

static inline void
async_io_buf_create(struct async_io_buf *buf)
{
	buf->data = malloc(DEFAULT_BUF_SIZE);
	buf->size = DEFAULT_BUF_SIZE;
}

static inline void
async_io_buf_destroy(struct async_io_buf *buf)
{
	free(buf->data);
}

static void
rps_observer_cb(struct ev_loop *loop, struct ev_timer *timer, int revent);

static void
timeout_cb(struct ev_loop *loop, struct ev_timer *timer, int revent);

static void
write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

static void
read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

void *
async_io_get_user_data(struct async_io *obj)
{
	return obj->user_data;
}

static inline struct async_io *
async_io_new_impl(int sock, struct async_io_if *io_if, void *user_data)
{
	struct async_io *obj = calloc(1, sizeof(*obj));
	obj->user_data = user_data;
	obj->io_if = *io_if;
	obj->sock = sock;
	fcntl(sock, F_SETFL, O_NONBLOCK);
	async_io_buf_create(&obj->read_buf);
	async_io_buf_create(&obj->write_buf);
	obj->loop = ev_loop_new(0);
	ev_set_userdata(obj->loop, obj);
	return obj;
}

struct async_io *
async_io_new(int sock, struct async_io_if *io_if, void *user_data)
{
	struct async_io *obj = async_io_new_impl(sock, io_if, user_data);

	ev_io_init(&obj->r_client, read_cb, sock, EV_READ);
	ev_io_init(&obj->w_client, write_cb, sock, EV_WRITE);
	ev_io_start(obj->loop, &obj->r_client);
	ev_io_start(obj->loop, &obj->w_client);
	return obj;
}

struct async_io *
async_io_new_rps(int sock, struct async_io_if *io_if,
		 uint32_t rps, void *user_data)
{
	struct async_io *obj = async_io_new_impl(sock, io_if, user_data);

	obj->rps = rps;
	obj->writes_ps = rps;
	obj->req_per_timeout = 1;
	ev_io_init(&obj->r_client, read_cb, sock, EV_READ);
	ev_io_start(obj->loop, &obj->r_client);
	double send_interval = 1.0 / rps;
	if (send_interval < 0.001) {
		/*
		 * Some libev backends don't support precision less
		 * than millisecond.
		 */
		obj->req_per_timeout = 0.001 / send_interval;
		send_interval = 0.001;
	}
	ev_timer_init(&obj->timeout_watcher, timeout_cb, 0.0, send_interval);
	ev_timer_start(obj->loop, &obj->timeout_watcher);
	ev_timer_init(&obj->rps_observer, rps_observer_cb, 1.0, 1.0);
	ev_timer_start(obj->loop, &obj->rps_observer);
	return obj;
}

void
async_io_delete(struct async_io *obj)
{
	async_io_buf_destroy(&obj->read_buf);
	async_io_buf_destroy(&obj->write_buf);
	free(obj);
}

void
async_io_start(struct async_io *obj)
{
	ev_loop(obj->loop, 0);
	ev_loop_destroy(obj->loop);
}

void
read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	(void)watcher;
	(void)revents;
	struct async_io *io_obj;
	io_obj = (struct async_io *)ev_userdata(loop);
	struct async_io_buf *buffer = &io_obj->read_buf;
	/* Calculate count of free bytes. */
	size_t need_to_read = buffer->size - buffer->iter;
	/* Read new bytes in the buffer after already read bytes. */
	int bytes_read = recv(io_obj->sock, buffer->data +
			      buffer->iter, need_to_read, 0);
	if (bytes_read < 0) {
		/* If an error occured break the event loop. */
		goto break_loop;
	}
	if (bytes_read == 0) return;
	/*
	 * Move the buffer iterator for storing new data after this
	 * position.
	 */
	buffer->iter += bytes_read;
	/* Get len of the new message. */
	int need_len = io_obj->io_if.msg_len(io_obj, buffer->data,
					     buffer->iter);
	if (need_len == -1) {
		goto break_loop;
	}
	/* If need more bytes than already stored then wait for new bytes. */
	if ((size_t)need_len > buffer->iter) {
		/*
		 * Increase the buffer size if need more bytes than can
		 * be stored.
		 */
		if ((size_t)need_len > buffer->size) {
			buffer->data = realloc(buffer->data, (size_t)need_len);
			buffer->size = (size_t)need_len;
		}
		return;
	}
	size_t offset_one;
	size_t offset = 0;
	int rest;
	do {
		/* Count of not processed bytes. */
		rest = buffer->iter - offset;
		/* Process the next message. */
		if (io_obj->io_if.recv_from_buf(io_obj, buffer->data + offset,
						rest, &offset_one)) {
			goto break_loop;
		}
		/* Now offset_one contains length of processed message. */
		io_obj->received++;
		offset += offset_one;
		rest -= offset_one;
		if (rest == 0) break;
		/* Check if the buffer has one more message. */
		need_len = io_obj->io_if.msg_len(io_obj, buffer->data + offset,
						 rest);
		if (need_len == -1) {
			goto break_loop;
		}
	} while (rest >= need_len);
	/*
	 * All messages are processed. Need to save remained bytes for further
	 * processing.
	 */
	char *new_buf = malloc(DEFAULT_BUF_SIZE);
	memcpy(new_buf, buffer->data + offset, buffer->iter - offset);
	free(buffer->data);
	buffer->data = new_buf;
	buffer->size = DEFAULT_BUF_SIZE;
	buffer->iter = buffer->iter - offset;
	/*
	 * If no new messages for sending and all answers are received then
	 * break loop.
	 */
	if (async_io_time_to_finish(io_obj)) {
		goto break_loop;
	}
	return;
break_loop:
	ev_break(io_obj->loop, EVBREAK_ONE);
}

void
async_io_finish(struct async_io *obj)
{
	obj->is_shutdown = true;
}

void
write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	(void)watcher;
	(void)revents;
	struct async_io *io_obj;
	io_obj = (struct async_io *)ev_userdata(loop);
	struct async_io_buf *buffer = &io_obj->write_buf;
	int need_to_send = io_obj->buf_busy - buffer->iter;
	if (need_to_send == 0) {
		/* If no data for sending then get new message. */
		size_t size = 0;
		void *new_buf = io_obj->io_if.write(io_obj, &size);
		if (new_buf == NULL) {
			/* If no more messages then finish event loop. */
			async_io_finish(io_obj);
			if (async_io_time_to_finish(io_obj))
				goto break_loop;
			return;
		}
		io_obj->need_to_receive++;
		if (size > buffer->size) {
			buffer->data = realloc(buffer->data, size);
			buffer->size = size;
		}
		io_obj->buf_busy = size;
		buffer->iter = 0;
		memcpy(buffer->data, new_buf, size);
		need_to_send = size;
	}
	int bytes_send = send(io_obj->sock, buffer->data + buffer->iter,
			      need_to_send, 0);
	if (bytes_send < 0) {
		if (errno != EAGAIN)
			goto break_loop;
		return;
	}
	buffer->iter += bytes_send;
	return;
break_loop:
	ev_break(io_obj->loop, EVBREAK_ONE);
}

void
rps_observer_cb(struct ev_loop *loop, ev_timer *timer,
		     int revent)
{
	(void)timer;
	(void)revent;
	struct async_io *io_obj;
	io_obj = (struct async_io *)ev_userdata(loop);
	uint32_t one_second_received;
	/*
	 * Calculate how many answers are received since the current second
	 * beginning.
	 */
	one_second_received = io_obj->received - io_obj->received_prev;
	io_obj->received_prev = io_obj->received;
	/* Calculate difference between the needed rps and real rps. */
	double diff = (double)one_second_received - (double)io_obj->rps;
	io_obj->writes_ps -= diff;
	/* Count of writes per second can't be negative. */
	if (io_obj->writes_ps < 0)
		io_obj->writes_ps = io_obj->rps;
	double send_interval = 1.0 / io_obj->writes_ps;
	if (send_interval < 0.001) {
		io_obj->req_per_timeout = 0.001 / send_interval;
		send_interval = 0.001;
	}
	ev_timer_set(&io_obj->timeout_watcher, send_interval, send_interval);
	ev_timer_again(io_obj->loop, &io_obj->timeout_watcher);
}

void
timeout_cb(struct ev_loop *loop, ev_timer *timer, int revent)
{
	(void) timer;
	(void) revent;
	struct async_io *io_obj;
	io_obj = (struct async_io *)ev_userdata(loop);
	for (uint32_t i = 0; i < io_obj->req_per_timeout; ++i)
		write_cb(loop, &io_obj->w_client, 0);
}
