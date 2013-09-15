#ifndef MRUBY_OPENAL_H
#define MRUBY_OPENAL_H

#include "mruby.h"
#include "mruby/data.h"
#include <stddef.h>

typedef struct mrb_al_sample_buffer_data_t {
  void  *buffer;
  size_t capacity;
  size_t size;
} mrb_al_sample_buffer_data_t;

extern struct mrb_data_type const mrb_al_sample_buffer_data_type;

extern struct RClass *mod_AL;

extern void mruby_openal_common_init(mrb_state *mrb);
extern void mruby_openal_common_final(mrb_state *mrb);

extern void mruby_openal_al_init(mrb_state *mrb);
extern void mruby_openal_alc_init(mrb_state *mrb);
extern void mruby_openal_alut_init(mrb_state *mrb);
extern void mruby_openal_al_final(mrb_state *mrb);
extern void mruby_openal_alc_final(mrb_state *mrb);
extern void mruby_openal_alut_final(mrb_state *mrb);

#endif /* end of MRUBY_OPENAL_H */

