#include "openal.h"
#include <AL/al.h>
#include <AL/alut.h>

static struct RClass *mod_ALUT;

static mrb_value
mrb_alut_init(mrb_state *mrb, mrb_value self)
{
  return alutInit(NULL, NULL) == AL_FALSE ? mrb_false_value() : mrb_true_value();
}

static mrb_value
mrb_alut_init_without_context(mrb_state *mrb, mrb_value self)
{
  return alutInitWithoutContext(NULL, NULL) == AL_FALSE ? mrb_false_value() : mrb_true_value();
}

static mrb_value
mrb_alut_exit(mrb_state *mrb, mrb_value self)
{
  return alutExit() == AL_FALSE ? mrb_false_value() : mrb_true_value();
}

static mrb_value
mrb_alut_sleep(mrb_state *mrb, mrb_value self)
{
  mrb_float duration;
  mrb_get_args(mrb, "f", &duration);
  return alutSleep(duration) == AL_FALSE ? mrb_false_value() : mrb_true_value();
}

static mrb_value
mrb_alut_get_error(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(alutGetError());
}

static mrb_value
mrb_alut_get_error_string(mrb_state *mrb, mrb_value self)
{
  mrb_int error;
  mrb_get_args(mrb, "i", &error);
  return mrb_str_new_cstr(mrb, alutGetErrorString((ALenum)error));
}

static mrb_value
mrb_alut_get_major_version(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(alutGetMajorVersion());
}

static mrb_value
mrb_alut_get_minor_version(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(alutGetMinorVersion());
}

void
mruby_openal_alut_init(mrb_state *mrb)
{
  mod_ALUT = mrb_define_module(mrb, "ALUT");
  mrb_define_module_function(mrb, mod_ALUT, "init_without_context", mrb_alut_init_without_context, ARGS_NONE());
  mrb_define_module_function(mrb, mod_ALUT, "init",          mrb_alut_init,              ARGS_NONE());
  mrb_define_module_function(mrb, mod_ALUT, "exit",          mrb_alut_exit,              ARGS_NONE());
  mrb_define_module_function(mrb, mod_ALUT, "sleep",         mrb_alut_sleep,             ARGS_REQ(1));
  mrb_define_module_function(mrb, mod_ALUT, "error",         mrb_alut_get_error,         ARGS_NONE());
  mrb_define_module_function(mrb, mod_ALUT, "error_string",  mrb_alut_get_error_string,  ARGS_REQ(1));
  mrb_define_module_function(mrb, mod_ALUT, "major_version", mrb_alut_get_major_version, ARGS_NONE());
  mrb_define_module_function(mrb, mod_ALUT, "minor_version", mrb_alut_get_minor_version, ARGS_NONE());
}

void
mruby_openal_alut_final(mrb_state *mrb)
{
}

