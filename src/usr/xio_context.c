/*
 * Copyright (c) 2013 Mellanox Technologies®. All rights reserved.
 *
 * This software is available to you under a choice of one of two licenses.
 * You may choose to be licensed under the terms of the GNU General Public
 * License (GPL) Version 2, available from the file COPYING in the main
 * directory of this source tree, or the Mellanox Technologies® BSD license
 * below:
 *
 *      - Redistribution and use in source and binary forms, with or without
 *        modification, are permitted provided that the following conditions
 *        are met:
 *
 *      - Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *      - Neither the name of the Mellanox Technologies® nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "xio_os.h"
#include "libxio.h"
#include "xio_observer.h"
#include "xio_common.h"
#include "xio_context.h"

/*---------------------------------------------------------------------------*/
/* xio_context_add_observer						     */
/*---------------------------------------------------------------------------*/
int xio_context_reg_observer(struct xio_context *ctx,
			     struct xio_observer *observer)
{
	xio_observable_reg_observer(&ctx->observable, observer);

	return 0;
}

/*---------------------------------------------------------------------------*/
/* xio_context_remove_observer		                                     */
/*---------------------------------------------------------------------------*/
void xio_context_unreg_observer(struct xio_context *ctx,
				struct xio_observer *observer)
{
	xio_observable_unreg_observer(&ctx->observable, observer);
}

/*---------------------------------------------------------------------------*/
/* xio_ctx_open                                                              */
/*---------------------------------------------------------------------------*/
struct xio_context *xio_ctx_open(struct xio_loop_ops *loop_ops,
		void *ev_loop,
		int polling_timeout)
{
	struct xio_context		*ctx = NULL;
	int				cpu;

	xio_read_logging_level();

	cpu = sched_getcpu();
	if (cpu == -1)
		return NULL;

	/* allocate new context */
	ctx = calloc(1, sizeof(struct xio_context));
	if (ctx == NULL) {
		xio_set_error(ENOMEM);
		ERROR_LOG("calloc failed. %m\n");
		return NULL;
	}

	if (loop_ops == NULL)  {
		/* use default implementation */
		ctx->loop_ops.ev_loop_add_cb = xio_ev_loop_add;
		ctx->loop_ops.ev_loop_del_cb = xio_ev_loop_del;
	} else {
		ctx->loop_ops.ev_loop_add_cb = loop_ops->ev_loop_add_cb;
		ctx->loop_ops.ev_loop_del_cb = loop_ops->ev_loop_del_cb;
	}
	ctx->ev_loop = ev_loop;

	ctx->cpuid		= cpu;
	ctx->nodeid		= xio_get_nodeid(cpu);
	ctx->polling_timeout	= polling_timeout;

	ctx->worker = (uint64_t) pthread_self();

	XIO_OBSERVABLE_INIT(&ctx->observable, ctx);

	return ctx;
}

/*---------------------------------------------------------------------------*/
/* xio_ctx_close	                                                     */
/*---------------------------------------------------------------------------*/
void xio_ctx_close(struct xio_context *ctx)
{
	xio_observable_notify_all_observers(&ctx->observable,
					    XIO_CONTEXT_EVENT_CLOSE, NULL);
	xio_observable_unreg_all_observers(&ctx->observable);

	free(ctx);
	ctx = NULL;
}
