#include <ngx_config.h>
#include <ngx_core.h>

#include <sys/capability.h>

ngx_module_t ngx_drop_privs_module;

typedef struct {
	ngx_str_t caps;
	ngx_str_t chroot;
} ngx_dropprivs_conf_t;

static void* create_conf(ngx_cycle_t* cycle)
{
	return ngx_pcalloc(cycle->pool, sizeof(ngx_dropprivs_conf_t));
}

static ngx_int_t init_module(ngx_cycle_t* cycle)
{
	if (0 == geteuid()) {
		cap_t caps;
		char* buf;
		int res;
		ngx_dropprivs_conf_t* conf;

		conf = (ngx_dropprivs_conf_t*)ngx_get_conf(cycle->conf_ctx, ngx_drop_privs_module);
		if (NULL == conf) {
			return NGX_ERROR;
		}

		if (conf->chroot.len > 0) {
			buf = (char*)ngx_pcalloc(cycle->pool, conf->chroot.len+1);
			if (NULL == buf) {
				return NGX_ERROR;
			}

			memcpy(buf, conf->chroot.data, conf->chroot.len);

			res = chdir(buf)
			ngx_log_debug2(NGX_LOG_DEBUG_CORE, cycle->log, ngx_errno, "chdir(%s): %d", buf, res);


			if (-1 == res) {
				ngx_log_error_core(NGX_LOG_EMERG, cycle->log, ngx_errno, "chdir(%s) failed", buf);
				ngx_pfree(cycle->pool, buf);
				return NGX_ERROR;
			}

			res = chroot(buf);
			ngx_log_debug2(NGX_LOG_DEBUG_CORE, cycle->log, ngx_errno, "chroot(%s): %d", buf, res);

			if (-1 == res) {
				ngx_log_error_core(NGX_LOG_EMERG, cycle->log, ngx_errno, "chroot(%s) failed", buf);
				ngx_pfree(cycle->pool, buf);
				return NGX_ERROR;
			}

			ngx_pfree(cycle->pool, buf);
		}

		if (conf->caps.len > 0) {
			buf = (char*)ngx_pcalloc(cycle->pool, conf->caps.len+1);
			if (NULL == buf) {
				return NGX_ERROR;
			}

			memcpy(buf, conf->caps.data, conf->caps.len);

			caps = cap_from_text(buf);
			ngx_pfree(cycle->pool, buf);

			if (NULL == caps) {
				ngx_log_error_core(NGX_LOG_EMERG, cycle->log, ngx_errno, "cap_from_text() failed");
				return NGX_ERROR;
			}

			res = cap_set_proc(caps);
			cap_free(caps);
			if (-1 == res) {
				ngx_log_error_core(NGX_LOG_EMERG, cycle->log, ngx_errno, "cap_set_proc() failed");
				return NGX_ERROR;
			}
		}
	}

	return NGX_OK;
}

static ngx_command_t ngx_dropprivs_commands[] = {
	{
		ngx_string("dropprivs_set_caps"),
		NGX_MAIN_CONF | NGX_DIRECT_CONF | NGX_CONF_TAKE1,
		ngx_conf_set_str_slot,
		0,
		offsetof(ngx_dropprivs_conf_t, caps),
		NULL
	},

	{
		ngx_string("dropprivs_chroot"),
		NGX_MAIN_CONF | NGX_DIRECT_CONF | NGX_CONF_TAKE1,
		ngx_conf_set_str_slot,
		0,
		offsetof(ngx_dropprivs_conf_t, chroot),
		NULL
	},

	ngx_null_command
};

static ngx_core_module_t ngx_dropprivs_ctx = {
	ngx_string("dropprivs"),
	create_conf,
	NULL
};

ngx_module_t ngx_drop_privs_module = {
	NGX_MODULE_V1,
	&ngx_dropprivs_ctx,
	ngx_dropprivs_commands,
	NGX_CORE_MODULE,
	NULL, /* init master */
	init_module, /* init module */
	NULL, /* init process */
	NULL, /* init thread */
	NULL, /* exit thread */
	NULL, /* exit process */
	NULL, /* exit master */
	NGX_MODULE_V1_PADDING
};

