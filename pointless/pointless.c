#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/sysctl.h>

static long  a = 100;
static int   b = 200;
static char *c = "Are you suggesting cocnuts migrate?";

static struct sysctl_ctx_list clist;
static struct sysctl_oid *poid;

static int
sysctl_pointless_procedure(SYSCTL_HANDLER_ARGS) { // 1
	char *buf = "Not at all. They could be carried.";
	return (sysctl_handle_string(oidp, buf, strlen(buf), req));
}

static int
pointless_modevent(module_t mod __unused, int event, void *arg __unused) {
	int error = 0;

	switch (event) {
		case MOD_LOAD:
			sysctl_ctx_init(&clist); // 2

			poid = SYSCTL_ADD_ROOT_NODE( // 3
				&clist
				, OID_AUTO
				, "example"
				, CTLFLAG_RW
				, 0
				, "new top-level tree"
			);
			if (poid == NULL) {
				uprintf("SYSCTL_ADD_NODE failed.\n");
				return (EINVAL);
			}

			SYSCTL_ADD_LONG( // 4
				&clist
				, SYSCTL_CHILDREN(poid)
				, OID_AUTO
				, "long"
				, CTLFLAG_RW
				, &a
				, "new long leaf"
			);
			SYSCTL_ADD_INT( // 5
				&clist
				, SYSCTL_CHILDREN(poid)
				, OID_AUTO
				, "int"
				, CTLFLAG_RW
				, &b
				, 0
				, "new int leaf"
			);

			poid = SYSCTL_ADD_NODE( // 6
				&clist
				, SYSCTL_CHILDREN(poid)
				, OID_AUTO
				, "node"
				, CTLFLAG_RW
				, 0
				, "new tree under example"
			);
			if (poid == NULL) {
				uprintf("SYSCTL_ADD_NODE failed.\n");
				return (EINVAL);
			}
			SYSCTL_ADD_PROC( // 7
				&clist
				, SYSCTL_CHILDREN(poid)
				, OID_AUTO
				, "proc"
				, CTLTYPE_INT | CTLFLAG_RD
				, 0
				, 0
				, sysctl_pointless_procedure
				, "A"
				, "new proc leaf"
			);

			poid = SYSCTL_ADD_NODE( // 8
				&clist
				, SYSCTL_STATIC_CHILDREN(_debug)
				, OID_AUTO
				, "example"
				, CTLFLAG_RW
				, 0
				, "new tree under debug"
			);
			if (poid == NULL) {
				uprintf("SYSCTL_ADD_NODE failed.\n");
				return (EINVAL);
			}
			SYSCTL_ADD_STRING( // 9
				&clist
				, SYSCTL_CHILDREN(poid)
				, OID_AUTO
				, "string"
				, CTLFLAG_RD
				, c
				, 0
				, "new string leaf"
			);
			
			uprintf("Pointless module loaded.\n");
			break;
		case MOD_UNLOAD:
			if (sysctl_ctx_free(&clist)) { // 10
				uprintf("sysctl_ctx_free failed.\n");
				return (ENOTEMPTY);
			}
			uprintf("Pointless module unloaded.\n");
			break;
		default:
			error = EOPNOTSUPP;
			break;
	}

	return (error);
}

static moduledata_t pointless_mod = {
	"pointless"
	, pointless_modevent
	, NULL
};

DECLARE_MODULE(pointless, pointless_mod, SI_SUB_EXEC, SI_ORDER_ANY);
