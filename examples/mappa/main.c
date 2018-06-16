#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

#include "mappa.h"

static volatile uint32_t done;

void sighndlr(int signal)
{
	done = 1;
	printf("\n");
}

void sw_target_float_func(uint32_t group_id, uint32_t target_id,
			  float value, void *userdata)
{
	printf("%s: (%d : %d) %f\n", __func__, group_id, target_id, value);
}

void tests(void);

int main(int argc, char **argv)
{
	tests();

	int ret;
	signal(SIGINT, sighndlr);

	/* create mappa context */
	struct mappa_t *m = mappa_create(NULL);
	if(!m)
		return -1;

	for(int i = 0; i < 12; i++) {
		char grp_buf[64];
		char itm_buf[64];

		int group = i / 8;
		int item = i % 8;
		snprintf(grp_buf, sizeof(grp_buf), "group_%d", group);
		snprintf(itm_buf, sizeof(itm_buf), "item_%d", item);

		struct mappa_sw_target_t tar = {
			.group_name = grp_buf,
			.item_name = itm_buf,
			.group_id = group,
			.item_id = item,
			.func = sw_target_float_func,
			.userdata = 0x0,
		};

		/* add target */
		ret = mappa_sw_target_add(m, &tar);
		assert(ret == 0);
	}

	/* valid remove works */
	ret = mappa_sw_target_remove(m, 0, 0);
	assert(ret == 0);

	/* no invalid removes */
	ret = mappa_sw_target_remove(m, 2, 0);
	assert(ret == -EINVAL);
	ret = mappa_sw_target_remove(m, 0, 20);
	assert(ret == -EINVAL);

	/* valid remove, but not double remove */
	ret = mappa_sw_target_remove(m, 0, 6);
	assert(ret == 0);
	ret = mappa_sw_target_remove(m, 0, 6);
	assert(ret == -EINVAL);

	ret = mappa_sw_target_remove(m, 1, 0);
	assert(ret == 0);

	/* map a few controls */
	int dev = 0;
	int control = 2;
	int group = 0;
	int item = 7;
	int layer = 0;
	ret = mappa_bind_ctlra_to_target(m, dev, CTLRA_EVENT_SLIDER, control,
					 group, item, layer);

	control = 13;
	group = 1;
	item = 1;
	ret = mappa_bind_ctlra_to_target(m, dev, CTLRA_EVENT_SLIDER, control,
					 group, item, layer);

	control = 2;
	group = 1;
	item = 2;
	ret = mappa_bind_ctlra_to_target(m, dev, CTLRA_EVENT_BUTTON, control,
					 group, item, layer);

	/* loop for testing */
	while(!done) {
		mappa_iter(m);
		usleep(10 * 1000);
	}

	mappa_destroy(m);

	return 0;
}

void tests(void)
{
	int ret = 0;
	ret = setenv("CTLRA_VIRTUAL_VENDOR", "Native Instruments", 1);
	printf("errno %d\n", errno);
	assert(!ret);
	ret = setenv("CTLRA_VIRTUAL_DEVICE", "Kontrol Z1", 1);
	//ret = setenv("CTLRA_VIRTUAL_DEVICE", "Maschine Mikro Mk2", 1);
	printf("errno %d\n", errno);
	assert(!ret);

	struct mappa_t *m = mappa_create(NULL);
	assert(m);


	mappa_destroy(m);
}
