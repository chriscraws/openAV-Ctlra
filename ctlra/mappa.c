
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "mappa.h"
#include "mappa_impl.h"

#include "ctlra.h"
#include "ini.h"

static int32_t dev_luts_flatten(struct dev_t *dev);

/* per ctlra-device we need a look-up table, which points to the
 * correct target function pointer to call based on the current layer.
 *
 * These layers of "ctlra-to-target" pairs are the mappings that need to
 * be saved/restored. Somehow, the uint32 group/item id pairs need to be
 * consistent in the host to re-connect correctly? Or use group_id / item_id?
 */

/* LAYER notes;
 * - Need to be able to display clearly to user
 * - Individual layers need to make sense
 * - "mask" layers that only overwrite certain parts
 * --- mask layer implementation returns which events were handled?
 * --- if not handled, go "down the stack" of layers?
 * --- enables "masked-masked" layers etc... complexity worth it?
 * - Display each layer as tab along top of mapping UI?
 * --- A tab button [+] to create a new layer?
 * --- Adding a new layer adds Group "mappa" : Item "Layer 1" ?
 */

/* mushes a single float to multiple feedback items. Eg: volume LEDs.
 * The application supplies a single float value, shown as VU LED strip
 */
static uint32_t
source_mush_value_to_array(float v, uint32_t idx, uint32_t total)
{
	return v > (idx / ((float)total)) ? -1 : 0xf;
}

void mappa_feedback_func(struct ctlra_dev_t *ctlra_dev, void *userdata)
{
	struct dev_t *dev = userdata;
	struct mappa_t *m = dev->self;
	struct lut_t *lut = dev->active_lut;

	/* iterate over the # of destinations, pulling sources for each */
	const struct ctlra_dev_info_t *info = dev->ctlra_dev_info;
	int count = info->control_count[CTLRA_FEEDBACK_ITEM];
	for(int i = 0; i < count; i++) {
		struct mappa_source_t *s = lut->sources[i];
		if(s && s->func) {
			float v;
			uint32_t token_size = 0;
			s->func(&v, 0, token_size, s->userdata);
			for(int j = 0; j < 7; j++) {
				ctlra_dev_light_set(ctlra_dev, i * 7 + j,
						    source_mush_value_to_array(v, j, 7));
			}
		}
	}
	ctlra_dev_light_flush(ctlra_dev, 1);

	/*
	int sidx = 0;
	struct source_t *s;
	TAILQ_FOREACH(s, &m->source_list, tailq) {
		float v;
		void *token = 0x0;
		if(s && s->source.func)
			s->source.func(token, &v, s->source.userdata);

	}
	ctlra_dev_light_flush(ctlra_dev, 1);
	*/
}

void mappa_event_func(struct ctlra_dev_t* ctlra_dev, uint32_t num_events,
		       struct ctlra_event_t** events, void *userdata)
{
	struct dev_t *dev = userdata;
	if(!dev) {
		printf("mappa programming error - dev is NULL\n");
		return;
	}
	struct mappa_t *m = dev->self;

	struct lut_t *lut = dev->active_lut;
	if(!lut) {
		MAPPA_ERROR(m, "programming error - lut is %p\n", lut);
		return;
	}

	struct target_t *t = 0;

	for(uint32_t i = 0; i < num_events; i++) {
		struct ctlra_event_t *e = events[i];
		float v = 0;
		uint32_t id = 0;

		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			id = e->button.id;
			t = lut->target_types[CTLRA_EVENT_BUTTON][id];
			v = e->button.pressed;
			break;
		case CTLRA_EVENT_ENCODER:
			// TODO: handle delta floats
			id = e->encoder.id;
			t = lut->target_types[CTLRA_EVENT_ENCODER][id];
			v = e->encoder.delta;
			break;
		case CTLRA_EVENT_SLIDER:
			id = e->slider.id;
			t = lut->target_types[CTLRA_EVENT_SLIDER][id];
			v = e->slider.value;
			break;
		case CTLRA_EVENT_GRID:
			break;
		default:
			break;
		};

		/* ensure event id is within known range. Badly written
		 * device code might send events that they don't advertise.
		 * Hence, we check if the event is in the advertised range,
		 * and bomb out if its not, notifying the dev.
		 */
		/* TODO: move to above loop */
		const struct ctlra_dev_info_t *info = dev->ctlra_dev_info;
		const uint32_t count = info->control_count[e->type];
		if(id >= count) {
			MAPPA_ERROR(m,
				"ctlra device error: type %u id %u >= control count %u\n",
				e->type, id, count);
			continue;
		}

		/* perform callback as currently mapped by lut */
		if(t && t->target.func) {
			float value = v * t->scale_range + t->scale_offset;
			t->target.func(t->id, value,
				       t->token_buf, t->token_size,
				       t->target.userdata);
#if 0
		} else {
			printf("event value %f has no target? t %p t->func %p\n",
			       value, t, t ? t->target.func : 0);
#endif
		}
	}
}

/* perform a deep copy of the target so it doesn't rely on app memory */
static struct target_t *
target_clone(const struct mappa_target_t *t, uint32_t token_size,
	     void *token)
{
	/* allocate a size for the list-pointer-enabled struct */
	struct target_t *n = malloc(sizeof(struct target_t) + token_size);
	/* dereference the current target, shallow copy values */
	n->target = *t;
	/* deep copy strings (to not depend on app provided strings) */
	if(t->name)
		n->target.name = strdup(t->name);
	/* intialize defaults */
	n->scale_range = 1.f;
	n->scale_offset = 0.f;
	/* store token */
	n->token_size = token_size;
	memcpy(n->token_buf, token, token_size);

	return n;
}

static void
target_destroy(struct target_t *t)
{
	assert(t != NULL);
	free(t->target.name);
	free(t);
}

static void
source_destroy(struct source_t *s)
{
	assert(s != NULL);
	free(s->source.name);
	free(s);
}

struct source_t *
source_deep_copy(const struct mappa_source_t *s)
{
	struct source_t *n = calloc(1, sizeof(struct source_t));
	n->source = *s;
	if(s->name)
		n->source.name = strdup(s->name);
	return n;
}

int32_t
mappa_source_add(struct mappa_t *m, struct mappa_source_t *t,
		 uint32_t *source_id, void *token, uint32_t token_size)
{
	struct source_t *n = source_deep_copy(t);
	TAILQ_INSERT_HEAD(&m->source_list, n, tailq);
	n->id = m->source_ids++;
	if(source_id)
		*source_id = n->id;
	return 0;
}

int32_t
mappa_source_remove(struct mappa_t *m, uint32_t source_id)
{
	struct source_t *s;
	TAILQ_FOREACH(s, &m->source_list, tailq) {
		if(s->id == source_id) {
			/* TODO: release any source bindings here */
			TAILQ_REMOVE(&m->source_list, s, tailq);
			source_destroy(s);
			return 0;
		}
	}
	return -EINVAL;
}

uint32_t
mappa_get_source_id(struct mappa_t *m, const char *sname)
{
	struct source_t *s;
	TAILQ_FOREACH(s, &m->source_list, tailq) {
		if(strcmp(s->source.name, sname) == 0) {
			return s->id;
		}
	}
	return 0;
}

int32_t
mappa_target_add(struct mappa_t *m,
		 struct mappa_target_t *t,
		 uint32_t *target_id,
		 void *token,
		 uint32_t token_size)
{
	struct target_t *n = target_clone(t, token_size, token);
	TAILQ_INSERT_HEAD(&m->target_list, n, tailq);
	n->id = m->target_ids++;
	if(target_id)
		*target_id = n->id;
	return 0;
}

uint32_t
mappa_get_target_id(struct mappa_t *m, const char *tname)
{
	struct target_t *t;
	TAILQ_FOREACH(t, &m->target_list, tailq) {
		if(strcmp(t->target.name, tname) == 0) {
			return t->id;
		}
	}
	return 0;
}

int32_t
mappa_target_set_range(struct mappa_t *m, uint32_t tid,
		       float max, float min)
{
	if(min > max)
		return -EINVAL;

	struct target_t *t;
	TAILQ_FOREACH(t, &m->target_list, tailq) {
		if(t->id == tid) {
			/* TODO: handle inversion here? if min > max? */
			float r = (max - min);
			t->scale_range = r;
			t->scale_offset = min;
			return 0;
		}
	}
	return -ENODEV;
}

int32_t
mappa_target_remove(struct mappa_t *m, uint32_t target_id)
{
	struct target_t *t;
	TAILQ_FOREACH(t, &m->target_list, tailq) {
		if(t->id == target_id) {
			/* TODO: nuke any controller mappings here */
			TAILQ_REMOVE(&m->target_list, t, tailq);
			target_destroy(t);
			return 0;
		}
	}

	return -EINVAL;
}

void dev_destroy(struct dev_t *dev);

void
mappa_remove_func(struct ctlra_dev_t *ctlra_dev, int unexpected_removal,
		  void *userdata)
{
	struct dev_t *mappa_dev = userdata;
	struct mappa_t *m = mappa_dev->self;

	struct ctlra_dev_info_t info;
	ctlra_dev_get_info(ctlra_dev, &info);
	MAPPA_INFO(m, "mappa_app: removing %s %s\n", info.vendor, info.device);

	/* remove dev from tailq and free it */
	TAILQ_REMOVE(&m->dev_list, mappa_dev, tailq);
	dev_destroy(mappa_dev);
}

#define DEV_FROM_MAPPA(m, dev, find_id) do {				\
	dev = 0;							\
	struct dev_t *__d = 0;						\
	TAILQ_FOREACH(__d, &m->dev_list, tailq) {			\
		if(__d->id == find_id) {				\
			dev = __d; break;				\
		}							\
	}								\
	if(!dev) {							\
		MAPPA_ERROR(m, "%s invalid dev id %u\n", __func__, find_id);\
		return -EINVAL;						\
	}								\
} while(0)

int32_t
mappa_bind_source_to_ctlra(struct mappa_t *m, uint32_t ctlra_dev_id,
			   const char *layer, uint32_t fb_id,
			   uint32_t source_id)
{
	struct dev_t *dev = 0;
	DEV_FROM_MAPPA(m, dev, ctlra_dev_id);

	const struct ctlra_dev_info_t *info = dev->ctlra_dev_info;
	int control_count = info->control_count[CTLRA_EVENT_BUTTON];
	if(fb_id >= control_count) {
		MAPPA_ERROR(m, "feedback id %u is out of range\n", fb_id);
		return -EINVAL;
	}


	/* iterate over sources by name, compare */
	int found = 0;
	struct source_t *s;
	TAILQ_FOREACH(s, &m->source_list, tailq) {
		if(s->id == source_id) {
			found = 1;
			break;
		}
	}
	if(!found)
		return -EINVAL;

	/* search for correct lut layer */
	struct lut_t *l;
	found = 0;
	TAILQ_FOREACH(l, &dev->lut_list, tailq) {
		if(strcmp(l->name, layer) == 0) {
			found = 1;
			break;
		}
	}
	if(!found)
		return -EINVAL;

	/* set source to feed into the feedback id */
	l->sources[fb_id] = &s->source;

	return 0;
}

void
lut_destroy(struct lut_t *lut)
{
	for(int i = 0; i < CTLRA_EVENT_T_COUNT; i++) {
		if(lut->target_types[i])
			free(lut->target_types[i]);
	}
	if(lut->name)
		free(lut->name);
	if(lut->sources)
		free(lut->sources);
	free(lut);
}

struct lut_t *
lut_create(struct dev_t *dev, const struct ctlra_dev_info_t *info,
	   const char *name)
{
	struct lut_t * lut = calloc(1, sizeof(*lut));
	if(!lut) {
		printf("failed to calloc lut mem\n");
		return 0;
	}

	/* allocate event input to target lookup arrays */
	for(int i = 0; i < CTLRA_EVENT_T_COUNT; i++) {
		uint32_t c = info->control_count[i];
		lut->target_types[i] = calloc(c, sizeof(void *));
		if(!lut->target_types[i])
			goto fail;
	}

	/* allocate feedback item lookup array */
	int items = info->control_count[CTLRA_FEEDBACK_ITEM];
	lut->sources = calloc(items, sizeof(struct mappa_source_t));
	if(!lut->sources)
		goto fail;

	lut->name = strdup(name);
	return lut;

fail:
	MAPPA_ERROR(dev->self, "failed to create LUT for %s %s\n",
		    info->vendor, info->device);
	lut_destroy(lut);
	return 0;
}

struct lut_t *
lut_create_add_to_dev(struct mappa_t *m,
			      struct dev_t *dev,
			      const struct ctlra_dev_info_t *info,
			      const char *name)
{
	/* add LUT to this dev */
	struct lut_t *lut = lut_create(dev, info, name);
	if(!lut) {
		MAPPA_ERROR(m, "lut create failed, returned %p\n", lut);
		return 0;
	}
	TAILQ_INSERT_TAIL(&dev->lut_list, lut, tailq);
	return lut;
}

struct dev_t *
dev_create(struct mappa_t *m, struct ctlra_dev_t *ctlra_dev,
	   const struct ctlra_dev_info_t *info)
{
	struct dev_t *dev = calloc(1, sizeof(*dev));
	if(!dev)
		return 0;

	TAILQ_INIT(&dev->lut_list);
	TAILQ_INIT(&dev->active_lut_list);

	/* store the dev info into the struct, for error checking later */
	dev->ctlra_dev_info = info;

	/* allocate memory for the the "flattened" lut of all the active
	 * layers */
	dev->active_lut = lut_create(dev, info, "active_lut");

	dev->self = m;
	dev->id = m->dev_ids++;
	TAILQ_INSERT_TAIL(&m->dev_list, dev, tailq);
	return dev;
}

void
dev_destroy(struct dev_t *dev)
{
	/* cleanup all LUTs */
	while(!TAILQ_EMPTY(&dev->lut_list)) {
		struct lut_t *l = TAILQ_FIRST(&dev->lut_list);
		TAILQ_REMOVE(&dev->lut_list, l, tailq);
		lut_destroy(l);
	}

	/* cleanup the active "flattened" lut */
	lut_destroy(dev->active_lut);

	/* cleanup the sources */
	free(dev);
}

int
mappa_accept_func(struct ctlra_t *ctlra, const struct ctlra_dev_info_t *info,
		  struct ctlra_dev_t *ctlra_dev, void *userdata)
{
	struct mappa_t *m = userdata;

	struct dev_t *dev = dev_create(m, ctlra_dev, info);
	if(!dev) {
		MAPPA_ERROR(m, "failed to create mappa dev_t for %s %s\n",
			    info->vendor, info->device);
		return 0;
	}

	/* TODO: check all available mappa config files to see if we want
	 * to accept this device? Use mappa_opts to configure this?
	 */

	ctlra_dev_set_event_func(ctlra_dev, mappa_event_func);
	ctlra_dev_set_feedback_func(ctlra_dev, mappa_feedback_func);
	ctlra_dev_set_remove_func(ctlra_dev, mappa_remove_func);

	/* the callback here is set per *DEVICE* - NOT the mappa pointer!
	 * The struct being passed is used directly to avoid lookup of the
	 * correct device from a list. The structure has a mappa_t back-
	 * pointer in order to communicate with "self" if required.
	 */
	ctlra_dev_set_callback_userdata(ctlra_dev, dev);

	MAPPA_INFO(m, "accepting device %s %s\n", info->vendor, info->device);
	return 1;
}

int32_t
mappa_iter(struct mappa_t *m)
{
	ctlra_idle_iter(m->ctlra);
	return 0;
}

/* struct that the token should be cast to when recieved */
struct mappa_layer_enable_t {
	/* ptr to device to apply this switch to */
	struct dev_t *dev;
	/* ptr to the lut to enable/disable based on value */
	struct lut_t *lut;
};

/* int cast of the float value will indicate the layer to switch to */
void mappa_layer_switch_target(uint32_t target_id, float value,
			       void *token, uint32_t token_size,
			       void *userdata)
{
	struct mappa_t *m = userdata;
	(void)m;
	assert(token_size == sizeof(struct mappa_layer_enable_t));
	struct mappa_layer_enable_t *le = token;

	struct dev_t *dev = le->dev;
	struct lut_t *lut = le->lut;

	/* TODO: allow config binding to set "actuate" value for
	 * "toggle" on press, or "momentary" (aka, revert on release)
	 * action types.. and possibly more. */
	if(value > 0.5) {
		lut->active = !lut->active;
		dev_luts_flatten(dev);
	}
}

struct mappa_t *
mappa_create(struct mappa_opts_t *opts)
{
	(void)opts;
	struct mappa_t *m = calloc(1, sizeof(struct mappa_t));
	if(!m)
		goto fail;

	if(opts)
		m->opts = *opts;

	char *mappa_debug = getenv("MAPPA_DEBUG");
	if(mappa_debug) {
		int debug_level = atoi(mappa_debug);
		m->opts.debug_level = debug_level;
		MAPPA_INFO(m, "debug level: %d\n", debug_level);
	}

	char *mappa_conf_dir = getenv("MAPPA_CONF_DIR");
	if(mappa_conf_dir) {
		MAPPA_INFO(m, "config files dir: %s\n", mappa_conf_dir);
	} else {
		MAPPA_INFO(m, "config files dir not set%s\n", "");
	}

	/* add default config search dir */
	const char *home = getenv("HOME");
	char conf_buf[256];
	snprintf(conf_buf, sizeof(conf_buf), "%s/.config/openAV/mappa/",
		 home);
	TAILQ_INIT(&m->conf_dir_list);
	mappa_add_config_dir(m, conf_buf);

	struct ctlra_create_opts_t c_opts = {
		.debug_level = m->opts.debug_level,
		.flags_usb_no_own_context = 1,
	};
	struct ctlra_t *c = ctlra_create(&c_opts);
	if(!c)
		goto fail;
	m->ctlra = c;

	/* 0 is the error return for uint32_t functions, so offset
	 * all targets by 1. They don't have an absolute meaning, so no
	 * issue in doing this */
	m->target_ids = 1;
	m->source_ids = 1;

	TAILQ_INIT(&m->target_list);
	TAILQ_INIT(&m->dev_list);

	/* TODO: Delay creation of targets until the layer becomes
	 * available. Make it possible to switch to it based on (dev,name)
	 * pair, as the layer applies only to a specific device instance,
	 * not any device in use.
	 *
	 * This allows a layer switch to be a "trigger" without args,
	 * avoiding any string parse/handling, or integer enumeration of
	 * layers.
	 *
	 * Internally, the target can register a token that will switch to
	 * the correct layer */
	/*
	struct mappa_target_t t = {
		.name = "mappa:layer switch",
		.func = mappa_layer_switch_target,
		.userdata = m,
	};

	uint32_t sid;
	int ret = mappa_target_add(m, &t, &sid, 0, 0);
	if(ret != 0)
		MAPPA_ERROR(m, "mappa target add %s failed\n", t.name);
	*/

	int num_devs = ctlra_probe(c, mappa_accept_func, m);
	MAPPA_INFO(m, "mappa connected to %d devices\n", num_devs);

	return m;
fail:
	if(m)
		free(m);
	return 0;
}

void
mappa_destroy(struct mappa_t *m)
{
	while(!TAILQ_EMPTY(&m->target_list)) {
		struct target_t *t = TAILQ_FIRST(&m->target_list);
		TAILQ_REMOVE(&m->target_list, t, tailq);
		target_destroy(t);
	}

	while(!TAILQ_EMPTY(&m->source_list)) {
		struct source_t *s = TAILQ_FIRST(&m->source_list);
		TAILQ_REMOVE(&m->source_list, s, tailq);
		source_destroy(s);
	}

	while(!TAILQ_EMPTY(&m->conf_dir_list)) {
		struct conf_dir_t *d = TAILQ_FIRST(&m->conf_dir_list);
		TAILQ_REMOVE(&m->conf_dir_list, d, tailq);
		free(d->dir);
		free(d);
	}

	/* iterate over all allocated resources and free them */
	ctlra_exit(m->ctlra);
	free(m);
}

static int32_t
config_file_binding_create(struct dev_t *dev, struct lut_t *lut,
			   uint32_t event_type, uint32_t control_id,
			   const char *layer, const char *control_name)
{
	const char *target = 0;
	struct mappa_t *m = dev->self;
	ini_t *config = m->ini_file;
	ini_sget(config, layer, control_name, NULL, &target);
	if(!target) {
		MAPPA_WARN(m, "Failed to bind %s %s to target %s\n",
			   layer, control_name, target);
		return 1;
	}

	/* Lookup the target in the mappa target list, create binding
	 * to target from the layer's lut. Note this is *NOT* the same
	 * as programming something to dev->active_lut. */
	struct target_t *t;
	TAILQ_FOREACH(t, &m->target_list, tailq) {
		if(t->target.name && strcmp(t->target.name, target) == 0) {
			lut->target_types[event_type][control_id] = t;
			MAPPA_INFO(m, "success %s %s => %s\n", layer,
				   control_name, target);
			return 0;
		}
	}

	/* TODO: think about how to handle this better. What if the app
	 * doesn't expose a target *YET* - but it could add it dynamically
	 * later. In that case we don't want to reload the whole file
	 * (overhead) - so storing the target name, but marking as invalid
	 * (null callback?) might be enough. When an app adds a target, we
	 * have to "re-walk" all the luts that are currently active, and
	 * fold them down to the active_lut again - but that's less work
	 * than re-loading the file, but gives the user the plug-and-play
	 * experience that is required.
	 *
	 * Impl: instead of having the app call a rebind() function, or
	 * performing it every time a target is added (overhead), use a
	 * two counter approach, and when an event arrives and if the
	 * target_add_counter != event_seen_counter, rebind */
	MAPPA_WARN(m,"target %s not found\n", target);
	return 1;
}

static int32_t
config_file_bind_layer_type(struct dev_t *dev, struct lut_t *lut,
			    const char *layer, uint32_t event_type)
{
	struct mappa_t *m = dev->self;
	/* for this event type, pull out control_count, and iterate over
	 * them. Pull the "layer", "name" pair from the config file, and
	 * call a function to map the control if it was non-NULL */
	const struct ctlra_dev_info_t *info = dev->ctlra_dev_info;
	uint32_t control_count = info->control_count[event_type];
	int32_t errors = 0;
	const uint32_t buf_size = 256;
	char buf[buf_size];

	for(uint32_t i = 0; i < control_count; i++) {
		const char *control_name = ctlra_info_get_name(info,
							       event_type,
							       i);
		if(!control_name) {
			MAPPA_ERROR(m, "control type %u, index %u of device %s %s has no name\n",
				    event_type, i, info->vendor, info->device);
			continue;
		}
		const char *prefix = ctlra_event_type_names[event_type];
		int written = snprintf(buf, sizeof(buf), "%s.%s",
				       prefix, control_name);
		if(written < 0 || written >= buf_size) {
			MAPPA_ERROR(m, "failed to print control name %s with prefix %s\n",
				    control_name, prefix);
			continue;
		}

		int ret = config_file_binding_create(dev, lut, event_type,
						     i, layer, buf);
		errors += !!ret;
	}

	return errors;
}

/* returns 0 on success, or # of FAILED bindings otherwise */
static int32_t
config_file_bind_layer(struct mappa_t *m, struct dev_t *dev,
		       struct lut_t *lut, const char *layer,
		       int32_t mapping_count)
{
	/* check the event type max, and pull the name of each control
	 * item from the Ctlra device code. Call a function to set up
	 * the mapping */
	int32_t total = 0;
	int32_t errors = 0;

	/* handle all event types here */
	const struct ctlra_dev_info_t *info = dev->ctlra_dev_info;
	for(int e = 0; e < CTLRA_EVENT_T_COUNT; e++) {
		total += info->control_count[e];
		errors += config_file_bind_layer_type(dev, lut, layer, e);
	}

	int32_t success = total - errors;

	if(mapping_count > 0 && success == mapping_count)
		return 0;

	if(errors)
		MAPPA_WARN(m, "Layer %s: %d bind errors, %d succesful bindings. "
			   "Mapping count from file is %d\n", layer, errors,
			   success, mapping_count);

	return errors ? -1 : 0;
}


static int32_t
dev_luts_flatten_event(struct dev_t *dev, struct lut_t *lut,
		       uint32_t event_type)
{
	const struct ctlra_dev_info_t *info = dev->ctlra_dev_info;
	uint32_t control_count = info->control_count[event_type];
	for(uint32_t i = 0; i < control_count; i++) {
		struct target_t *t = lut->target_types[event_type][i];
		if(t && t->target.func) {
			dev->active_lut->target_types[event_type][i] = t;
			/*
			printf("target %s (id %d) set for event %d id %d\n",
			       t->target.name, t->id, event_type, i);
			*/
		}
	}
	return 0;
}

static int32_t
dev_luts_flatten(struct dev_t *dev)
{
	/* iterate all enabled luts, and for each one blit all callbacks
	 * to the dev->active_lut. This enables that binding for each LUT,
	 * but given that the higher priority ones will be over-writing
	 * these bindings, we get the "mask" overlays for free */
	struct lut_t *l;
	/* TODO: reverse iterating is not correct: but solves the "unknown
	 * target" issue (file only refers to existing targets that came
	 * before it) and reverse order into the active_lut. Need to "delay"
	 * the target binding for internal items? Or allow target-hotplug
	 * to take case of this.
	 * Works as is though.
	 */
	TAILQ_FOREACH_REVERSE(l, &dev->lut_list, lut_list_t, tailq) {
		if(l->active) {
			printf("lut %s active\n", l->name);

			/* loop over each event type, each item, if it has
			 * a callback t and t->func, write it into the
			 * dev->active_lut, so it is used
			 */
			const struct ctlra_dev_info_t *info = dev->ctlra_dev_info;
			for(int e = 0; e < CTLRA_EVENT_T_COUNT; e++) {
				dev_luts_flatten_event(dev, l, e);
			}
		}
	}

	return 0;
}

int32_t
mappa_add_config_file(struct mappa_t *m, const char *file)
{
	if(!m)
		return -EINVAL;

	/* reset state first */
	m->ini_file = NULL;

	ini_t *config = ini_load(file);
	if(!config) {
		MAPPA_ERROR(m, "unable to load config file %s, does it exist?\n",
			   file);
		return -EINVAL;
	}

	/* TODO: cleanup this and pass along the config pointer instead */
	m->ini_file = config;

	/*
	const char *name = 0;
	const char *email = 0;
	const char *org = 0;
	ini_sget(config, "author", "name", NULL, &name);
	ini_sget(config, "author", "email", NULL, &email);
	ini_sget(config, "author", "organization", NULL, &org);
	*/

	const char *vendor = 0;
	const char *device = 0;
	const char *serial = 0;
	ini_sget(config, "hardware", "vendor", NULL, &vendor);
	ini_sget(config, "hardware", "device", NULL, &device);
	ini_sget(config, "hardware", "serial", NULL, &serial);
	if(!vendor || !device) {
		/* TODO: cleanup properly and close file */
		return -ENODATA;
	}

	/* lookup device by vendor/device/serial, apply config if matches */
	struct dev_t *dev = 0;
	TAILQ_FOREACH(dev, &m->dev_list, tailq) {
		const struct ctlra_dev_info_t *info = dev->ctlra_dev_info;
		assert(info);

		int match_required = 2; /* vendor + device */
		int vendor_match = strcmp(info->vendor, vendor) == 0;
		int device_match = strcmp(info->device, device) == 0;
		int serial_match = 0;
		if(serial && info->serial) {
			serial_match = strcmp(info->serial, serial) == 0;
			match_required = 3; /* include serial too */
		}

		int match_count = vendor_match + device_match + serial_match;

		printf("vendor match %s and %s = %d\n", info->vendor,
		       vendor, vendor_match);

		printf("dev %p serial is %s\n", dev, info->serial);

		if(match_count == match_required) {
			MAPPA_INFO(m, "%s %s serial %s matches with id %u\n",
				   vendor, device, serial, dev->id);
			break;
		}
	}
	if(!dev) {
		MAPPA_WARN(m, "No device found for this config: %s\n", file);
		/* TODO: cleanup close file */
		return -ENOSPC;
	}

	/* get layer name from the config file */
	const char *layer_count_str = 0;
	ini_sget(config, "mapping", "layer_count", NULL, &layer_count_str);
	if(!layer_count_str) {
		MAPPA_ERROR(m, "Config %s has no layer count - failing.\n",
			    file);
		goto fail;
	}
	int layer_count = atoi(layer_count_str);
	if(layer_count == 0) {
		MAPPA_ERROR(m, "Config %s has invalid layer count. "
			    "Fix the [mapping] layer_count = X field.\n",
			    file);
		goto fail;
	}

	for(uint32_t i = 0; i < layer_count; i++) {
		char layer_id[32];
		snprintf(layer_id, sizeof(layer_id), "layer.%u", i);

		/* get layer name from the config file */
		const char *layer_name = 0;
		ini_sget(config, layer_id, "name", NULL, &layer_name);
		if(!layer_name) {
			MAPPA_ERROR(m, "Layer %u has no name - skipping\n", i);
			continue;
		}

		/* (re)create the lut instance for this layer (resets it) */
		struct lut_t *lut = 0;
		TAILQ_FOREACH(lut, &dev->lut_list, tailq) {
			assert(lut->name);
			if(strcmp(lut->name, layer_name) == 0) {
				lut_destroy(lut);
				break;
			}
		}
		lut = lut_create_add_to_dev(m, dev, dev->ctlra_dev_info,
					    layer_name);
		if(!lut) {
			MAPPA_ERROR(m, "failed to create lut %s\n", "test");
			return -ENOMEM;
		}

		/* check if this layer should be active on loading */
		const char *active = 0;
		ini_sget(config, layer_id, "active_on_load", NULL, &active);
		if(active) {
			lut->active = atoi(active);
		}

		/* get error check value from config file if provided */
		const char *mapping_count_str = 0;
		ini_sget(config, layer_id, "mapping_count", NULL,
			 &mapping_count_str);
		int32_t mapping_count = 0;
		if(mapping_count_str) {
			int mc = atoi(mapping_count_str);
			mapping_count = mc;
		}

		/* load the various bindings for this layer */
		int ret = config_file_bind_layer(m, dev, lut, layer_id,
						 mapping_count);
		if(ret) {
			MAPPA_ERROR(m, "layer %s had binding failures\n",
				    layer_name);
		}

		/* register an internal target to switch to this layer */
		struct mappa_target_t t = {
			.func = mappa_layer_switch_target,
			.userdata = m,
		};
		char buf[64];
		snprintf(buf, sizeof(buf), "mappa:layer_enable:%s",
			 layer_name);
		t.name = buf;
		/* TODO store target_id in lut_t * for removal later */
		uint32_t tid;
		struct mappa_layer_enable_t le = {
			.dev = dev,
			.lut = lut,
		};
		printf("register layer %s\n", buf);
		ret = mappa_target_add(m, &t, &tid, &le, sizeof(le));
	}

	/* flatten layers into the active_lut now that they're loaded */
	int32_t ret = dev_luts_flatten(dev);
	if(ret)
		MAPPA_ERROR(m, "Failed to flatten layers for %p\n", dev);

	/* free and reset temporary handle */
	if(m->ini_file)
		ini_free(m->ini_file);
	m->ini_file = 0;

	return 0;

fail:
	if(m->ini_file)
		ini_free(m->ini_file);
	m->ini_file = 0;
	return -EINVAL;
}

void
mappa_add_config_dir(struct mappa_t *m, const char *abs_path)
{
	if(!m || !abs_path) {
		return;
	}

	/* this function adds the parameter abs_path as a directory to
	 * search for config files. Adding a directory will cause every
	 * file in the directory to be opened, and an attempt be made to
	 * extract the following part of the file:
	 *     [hardware]
	 *     vendor = vendor_name_here
	 *     device = device_name_here
	 *     serial = serial_code_optionally_here
	 */
	struct conf_dir_t *d = calloc(1, sizeof(*d));
	d->dir = strdup(abs_path);
	TAILQ_INSERT_HEAD(&m->conf_dir_list, d, tailq);
}
