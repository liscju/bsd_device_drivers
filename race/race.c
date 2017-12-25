#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/ioccom.h>
#include <sys/queue.h>
#include "race_ioctl.h"

MALLOC_DEFINE(M_RACE, "race", "race object");

struct race_softc {
	LIST_ENTRY(race_softc) list; // 1
	int unit; // 2
};

static LIST_HEAD(, race_softc) race_list = // 3
	LIST_HEAD_INITIALIZER(&race_list); // 4

static struct race_softc * race_new(void);
static struct race_softc * race_find(int unit);
static void                race_destroy(struct race_softc *sc);
static d_ioctl_t           race_ioctl;

static struct cdevsw race_cdevsw = { // 5
	.d_version = D_VERSION,
	.d_ioctl   = race_ioctl,
	.d_name    = RACE_NAME // 6
};

static struct cdev *race_dev;

static int
race_ioctl( // 7
	struct cdev *dev,
	u_long cmd,
	caddr_t data,
	int fflag,
	struct thread *td) {
	/** **/
	struct race_softc *sc;
	int error = 0;

	switch (cmd) {
		case RACE_IOC_ATTACH:
			sc = race_new();
			*(int *) data = sc->unit;
			break;
		case RACE_IOC_DETACH:
			sc = race_find(*(int *) data);
			if (sc == NULL)
				return (ENOENT);
			race_destroy(sc);
			break;
		case RACE_IOC_QUERY:
			sc = race_find(*(int *) data);
			if (sc == NULL)
				return (ENOENT);
			break;
		case RACE_IOC_LIST:
			uprintf(" UNIT\n");
			LIST_FOREACH(sc, &race_list, list)
				uprintf(" %d\n", sc->unit);
			break;
		default:
			error = ENOTTY;
			break;
	}

	return (error);
}

static struct race_softc *
race_new(void) {
	struct race_softc *sc;
	int unit, max = -1;

	LIST_FOREACH(sc, &race_list, list) {
		if (sc->unit > max)
			max = sc->unit;
	}
	unit = max + 1;

	sc = (struct race_softc*) malloc(
		sizeof(struct race_softc),
		M_RACE,
		M_WAITOK | M_ZERO
	);
	sc->unit = unit;
	LIST_INSERT_HEAD(&race_list, sc, list);

	return (sc);
}

static struct race_softc *
race_find(int unit) {
	struct race_softc *sc;

	LIST_FOREACH(sc, &race_list, list) {
		if (sc->unit == unit)
			break;
	}

	return (sc);
}

static void
race_destroy(struct race_softc *sc) {
	LIST_REMOVE(sc, list);
	free(sc, M_RACE);
}

static int race_modevent(
	module_t mod __unused,
	int event,
	void *arg __unused) {
	/** **/
	int error = 0;

	switch (event) {
		case MOD_LOAD:
			race_dev = make_dev(
				&race_cdevsw,
				0,
				UID_ROOT,
				GID_WHEEL,
				0600,
				RACE_NAME
			);
			uprintf("Race driver loaded.\n");
			break;
		case MOD_UNLOAD:
			destroy_dev(race_dev);
			uprintf("Race driver unloaded.\n");
			break;
		case MOD_QUIESCE:
			if (!LIST_EMPTY(&race_list))
				error = EBUSY;
			break;
		default:
			error = EOPNOTSUPP;
			break;
	}

	return (error);
}

DEV_MODULE(race, race_modevent, NULL);
