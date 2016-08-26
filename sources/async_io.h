#ifndef ASYNC_IO_H_INCLUDED
#define ASYNC_IO_H_INCLUDED

/*
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * There is described API for making asynchronous reading and writing
 * to socket in one thread.
 *
 * async_io_object - a main structure that is used for managing async io.
 */
struct async_io_object;

/**
 * This structure represents an interface
 *   - for fetching messages from the given buffer
 *   - for getting new messages and send them to network
 *   - for parsing this messages
 * The user of async_io_object must implement this interface.
 */
struct async_io_if {
	/**
	 * Return length of the message beginnig of that
	 * stored in given buffer.
	 */
	int (*msg_len)(struct async_io_object *obj, void *buf, size_t size);
	/**
	 * Return a new message for sending it to network and write size
	 * of message to @arg size. If there are no new messages return NULL.
	 */
	void *(*write)(struct async_io_object *io_obj, size_t *size);
	/**
	 * Process new message that stored in the given buffer (@arg buf)
	 * and write length of the processed message to @arg off.
	 */
	int (*recv_from_buf)(struct async_io_object *obj, char *buf,
			     size_t size, size_t *off);
};

/**
 * Allocate and initialize new async_io_object.
 * @arg sock      - socket for reading and writing to it.
 * @arg io_if     - an implemented interface for working with the bytes stream
 *   (@sa struct async_io_if).
 * @arg user_data - any user defined data or NULL. Further the user_data can
 *   be retreived in all functions of @arg io_if by calling
 *   async_io_get_user_data.
 */
struct async_io_object *async_io_new(int sock, struct async_io_if *io_if, void *user_data);

/**
 * Allocate and initialize new async_io_object that after starting will be
 * trying to support defined RPS (requests per second).
 * @arg sock      - socket for reading and writing to it.
 * @arg io_if     - an implemented interface for working with the bytes stream
 *   (@sa struct async_io_if).
 * @arg rps       - needed count of requests per second
 * @arg user_data - any user defined data or NULL. Further the user_data can
 *   be retreived in all functions of @arg io_if by calling
 *   async_io_get_user_data.
 */
struct async_io_object *async_io_new_rps(int sock, struct async_io_if *io_if,
					 uint32_t rps, void *user_data);

/**
 * Return the data that was passed to async_io_new as @arg user_data.
 */
void *async_io_get_user_data(struct async_io_object *obj);

/**
 * Start event loop that blocks current thread and listens socket on
 * writing or reading bytes capability.
 */
void async_io_start(struct async_io_object *obj);

/**
 * Prepare to finish event loop. After this function is called
 * and all answers on all requests will be received the event loop will break.
 */
void async_io_finish(struct async_io_object *obj);

/**
 * Free memory allocated in async_io_new.
 */
void async_io_delete(struct async_io_object *obj);

#endif
