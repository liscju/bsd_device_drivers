#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/ioccom.h>
#include <sys/sysctl.h>

#define BUFFER_SIZE 256

MALLOC_DECLARE(M_ECHO);
MALLOC_DEFINE(M_ECHO, "echo_buffer", "buffer for echo driver");

#define ECHO_CLEAR_BUFFER       _IO('E', 1)
#define ECHO_SET_BUFFER_SIZE    _IOW('E', 2, int) 

static d_open_t  echo_open;
static d_close_t echo_close;
static d_read_t  echo_read;
static d_write_t echo_write;
static d_ioctl_t echo_ioctl;

static struct cdevsw echo_cdevsw = {
	.d_version  = D_VERSION,
	.d_open     = echo_open,
	.d_close    = echo_close,
	.d_read     = echo_read,
	.d_write    = echo_write,
	.d_ioctl    = echo_ioctl,
	.d_name     = "echo"
};

typedef struct echo {
	int buffer_size;
	char *buffer;
	int length;
} echo_t;

static echo_t *echo_message;
static struct cdev *echo_dev;

static struct sysctl_ctx_list clist;
static struct sysctl_oid *poid;

static int
echo_open(struct cdev *dev, int oflags, int devtype, struct thread *td) {
	uprintf("Opening echo device.\n");	
	return (0);
}

static int
echo_close(struct cdev *dev, int fflag, int devtype, struct thread *td) {
	uprintf("Closing echo device.\n");
	return (0);
}

static int
echo_write(struct cdev *dev, struct uio *uio, int ioflag) {
	int error = 0;
	int amount;

	amount = MIN(uio->uio_resid,
		(echo_message->buffer_size - 1 - uio->uio_offset > 0) ?
		echo_message->buffer_size - 1 - uio->uio_offset : 0);
	if (amount == 0)
		return (error);

	long long int offset = uio->uio_offset;
	error = uiomove(echo_message->buffer+offset, amount, uio);
	if (error != 0) {
		uprintf("Write failed.\n");
		return (error);
	}
	
	uprintf(
		"AFTER - SIZE: %d, OFFSET %lld\n"
		, uio->uio_resid
		, uio->uio_offset
	);

	echo_message->length = offset + amount;
	echo_message->buffer[echo_message->length] = '\0';

	return (error);
}

static int
echo_read(struct cdev *dev, struct uio *uio, int ioflag) {
	int error = 0;
	int amount;

	amount = MIN(uio->uio_resid,
		(echo_message->length - uio->uio_offset > 0) ?
		 echo_message->length - uio->uio_offset : 0);

	error = uiomove(
		echo_message->buffer + uio->uio_offset
		, amount
		, uio
	);
	if (error != 0)
		uprintf("Read failed.\n");

	return (error);
}

static int
echo_set_buffer_size(int size) {
	int error = 0;

	if (echo_message->buffer_size == size)
		return (error);

	if (size >= 128 && size <= 512) {
		echo_message->buffer = realloc(
			echo_message->buffer
			, size
			, M_ECHO
			, M_WAITOK
		);
		echo_message->buffer_size = size;

		if (echo_message->length >= size) {
			echo_message->length = size - 1;
			echo_message->buffer[size - 1] = '\0';
		}
	} else
		error = EINVAL;

	return (error);
}

static int
echo_ioctl(struct cdev *dev, u_long cmd, caddr_t data, int fflag,
		struct thread *td) {
	int error = 0;

	switch (cmd) {
		case ECHO_CLEAR_BUFFER:
			memset(
				echo_message->buffer
				, '\0'
				, echo_message->buffer_size
			);
			echo_message->length = 0;
			uprintf("Buffer cleared.\n");
			break;
		case ECHO_SET_BUFFER_SIZE:
			error = echo_set_buffer_size(
				*(int *) data
			);
			if (error == 0)
				uprintf("Buffer resized.\n");
			break;
		default:
			error = ENOTTY;
			break;
	}

	return (error);
}

static int
sysctl_set_buffer_size(SYSCTL_HANDLER_ARGS) {
	int error = 0;
	int size = echo_message->buffer_size;

	error = sysctl_handle_int(oidp, &size, 0, req);
	if (error || !req->newptr || echo_message->buffer_size == size)
		return (error);

	if (size >= 128 && size <= 512) {
		echo_message->buffer = realloc(
			echo_message->buffer
			, size
			, M_ECHO
			, M_WAITOK
		);
		echo_message->buffer_size = size;

		if (echo_message->length >= size) {
			echo_message->length = size - 1;
			echo_message->buffer[size - 1] = '\0';
		}
	} else
		error = EINVAL;

	return (error);
}

static int
echo_modevent(module_t mod __unused, int event, void *arg __unused) {
	int error = 0;

	switch (event) {
		case MOD_LOAD:
			echo_message = malloc(
				sizeof(echo_t)
				, M_ECHO
				, M_WAITOK
			);
			echo_message->buffer_size = 256;
			echo_message->buffer = malloc(
					echo_message->buffer_size
					, M_ECHO
					, M_WAITOK
			);

			sysctl_ctx_init(&clist);
			poid = SYSCTL_ADD_ROOT_NODE(
				&clist
				, OID_AUTO
				, "echo"
				, CTLFLAG_RW
				, 0
				, "echo root node"
			);
			SYSCTL_ADD_PROC(
				&clist
				, SYSCTL_CHILDREN(poid)
				, OID_AUTO
				, "buffer_size"
				, CTLTYPE_INT | CTLFLAG_RW
				, &echo_message->buffer_size
				, 0
				, sysctl_set_buffer_size
				, "I"
				, "echo buffer size"
			);

			echo_dev = make_dev(
				&echo_cdevsw, 0, UID_ROOT, GID_WHEEL,
				0600, "echo");
			uprintf("Echo driver loaded.\n");
			break;

		case MOD_UNLOAD:
			destroy_dev(echo_dev);
			sysctl_ctx_free(&clist);
			free(echo_message, M_ECHO);
			uprintf("Echo driver unloaded.\n");
			break;

		default:
			error = EOPNOTSUPP;
			break;
	}

	return (error);
}

DEV_MODULE(echo, echo_modevent, NULL);






