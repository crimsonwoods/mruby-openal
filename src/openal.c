#include "openal.h"

void
mrb_mruby_openal_gem_init(mrb_state *mrb)
{
  mruby_openal_al_init(mrb);
  mruby_openal_alc_init(mrb);
  mruby_openal_alut_init(mrb);
  mruby_openal_common_init(mrb);
}

void
mrb_mruby_openal_gem_final(mrb_state *mrb)
{
  mruby_openal_common_final(mrb);
  mruby_openal_alut_final(mrb);
  mruby_openal_alc_final(mrb);
  mruby_openal_al_final(mrb);
}

