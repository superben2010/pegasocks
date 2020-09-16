#include "pgs_applet.h"

#ifdef WITH_APPLET

static pgs_tray_t tray;

void pgs_tray_submenu_update(pgs_tray_context_t *ctx,
			     pgs_tray_menu_t *servers_submenu);

static void quit_cb(pgs_tray_menu_t *item)
{
	(void)item;
	pgs_tray_context_t *ctx = tray.menu[0].context;
	ctx->kill_workers(ctx->threads, ctx->thread_num);
}

static void pick_server_cb(pgs_tray_menu_t *item)
{
	(void)item;
	pgs_tray_context_t *ctx = tray.menu[0].context;
	pgs_logger_info(ctx->logger, "switched to server %s, index: %d",
			item->text, item->context);
	ctx->sm->cur_server_index = item->context;
	pgs_tray_submenu_update(ctx, tray.menu[0].submenu);
	tray_update(&tray);
}

static pgs_tray_t tray = {
	.icon = TRAY_ICON1,
	.menu = (pgs_tray_menu_t[]){ {
					     .text = "servers",
				     },
				     { .text = "-" },
				     { .text = "quit", .cb = quit_cb },
				     { .text = NULL } },
};

void pgs_tray_submenu_update(pgs_tray_context_t *ctx,
			     pgs_tray_menu_t *servers_submenu)
{
	for (int i = 0; i < ctx->sm->server_len * 3; i += 3) {
		int server_idx = i / 3;
		servers_submenu[i].text =
			ctx->sm->server_configs[server_idx].server_address;
		servers_submenu[i].checked =
			server_idx == ctx->sm->cur_server_index;
		servers_submenu[i].cb = pick_server_cb;
		servers_submenu[i].context = server_idx;
		servers_submenu[i].submenu = NULL;
		// TODO: metrics
		sprintf(&ctx->metrics_label[256 * server_idx],
			"(%s) g204: %.2f ms  connect: %.2f ms",
			ctx->sm->server_configs[server_idx].server_type,
			ctx->sm->server_stats[server_idx].g204_delay,
			ctx->sm->server_stats[server_idx].connect_delay);
		servers_submenu[i + 1].text =
			&ctx->metrics_label[256 * server_idx];
		servers_submenu[i + 1].disabled = 1;
		servers_submenu[i + 1].submenu = NULL;
		servers_submenu[i + 2].text = "-";
		servers_submenu[i + 2].submenu = NULL;
	}
	servers_submenu[ctx->sm->server_len * 3 - 1].text = NULL;
}

// init submenu
void pgs_tray_init(pgs_tray_context_t *ctx)
{
	pgs_logger_info(ctx->logger, "current server: %d, server length: %d",
			ctx->sm->cur_server_index, ctx->sm->server_len);
	pgs_tray_menu_t *servers_submenu =
		pgs_malloc(sizeof(pgs_tray_menu_t) * ctx->sm->server_len * 3);
	ctx->metrics_label =
		pgs_malloc(sizeof(char) * 256 * ctx->sm->server_len);
	pgs_tray_submenu_update(ctx, servers_submenu);
	tray.menu[0].submenu = servers_submenu;
	tray.menu[0].context = ctx;
}
// clean submenu
void pgs_tray_clean()
{
	if (tray.menu[0].submenu)
		pgs_free(tray.menu[0].submenu);
	pgs_tray_context_t *ctx = tray.menu[0].context;
	if (ctx->metrics_label)
		pgs_free(ctx->metrics_label);
}

void pgs_tray_start(pgs_tray_context_t *ctx)
{
	pgs_tray_init(ctx);
	if (tray_init(&tray) < 0) {
		printf("failed to create tray\n");
		return;
	}
	int iter = 0;
	while (tray_loop(1) == 0) {
		if (++iter % 5 == 0) {
			pgs_tray_submenu_update(ctx, tray.menu[0].submenu);
			tray_update(&tray);
			iter = 0;
		}
	}
	pgs_tray_clean();
}

#else
void pgs_tray_start(pgs_tray_context_t *ctx)
{
}

#endif
