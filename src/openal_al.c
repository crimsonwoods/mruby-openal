#include "openal.h"
#include "mruby/data.h"
#include "mruby/class.h"
#include "mruby/array.h"
#include "mruby/string.h"
#include <AL/al.h>
#include <AL/alut.h>
#include <stdbool.h>

struct RClass *mod_AL = NULL;

static struct RClass *class_Buffers = NULL;
static struct RClass *class_Buffer = NULL;
static struct RClass *class_Sources = NULL;
static struct RClass *class_Source = NULL;
static struct RClass *class_Listener = NULL;
static struct RClass *class_ALError = NULL;

static mrb_value
mrb_al_get_error(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(alGetError());
}

static mrb_value
mrb_al_get_string(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  mrb_get_args(mrb, "i", &value);
  return mrb_str_new_cstr(mrb, alGetString((ALenum)value));
}

static mrb_value
mrb_al_enable(mrb_state *mrb, mrb_value self)
{
  mrb_int capability;
  mrb_get_args(mrb, "i", &capability);
  alEnable(capability);
  return alGetError() != AL_NO_ERROR ? mrb_false_value() : mrb_true_value();
}

static mrb_value
mrb_al_disable(mrb_state *mrb, mrb_value self)
{
  mrb_int capability;
  mrb_get_args(mrb, "i", &capability);
  alDisable(capability);
  return alGetError() != AL_NO_ERROR ? mrb_false_value() : mrb_true_value();
}

static mrb_value
mrb_al_is_enabled(mrb_state *mrb, mrb_value self)
{
  mrb_int capability;
  mrb_get_args(mrb, "i", &capability);
  return alIsEnabled(capability) == AL_FALSE ? mrb_false_value() : mrb_true_value();
}

static mrb_value
mrb_al_get_boolean(mrb_state *mrb, mrb_value self)
{
  mrb_int param;
  mrb_get_args(mrb, "i", &param);
  return alGetBoolean(param) == AL_FALSE ? mrb_false_value() : mrb_true_value();
}

static mrb_value
mrb_al_get_integer(mrb_state *mrb, mrb_value self)
{
  mrb_int param;
  mrb_get_args(mrb, "i", &param);
  return mrb_fixnum_value(alGetInteger(param));
}

static mrb_value
mrb_al_get_float(mrb_state *mrb, mrb_value self)
{
  mrb_int param;
  mrb_get_args(mrb, "i", &param);
  return mrb_float_value(mrb, alGetFloat(param));
}

static mrb_value
mrb_al_doppler_factor(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_get_args(mrb, "f", &value);
  alDopplerFactor((ALfloat)value);
  return alGetError() == AL_NO_ERROR ? mrb_true_value() : mrb_false_value();
}

static mrb_value
mrb_al_doppler_velocity(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_get_args(mrb, "f", &value);
  alDopplerVelocity((ALfloat)value);
  return alGetError() == AL_NO_ERROR ? mrb_true_value() : mrb_false_value();
}

static mrb_value
mrb_al_speed_of_sound(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_get_args(mrb, "f", &value);
  alSpeedOfSound((ALfloat)value);
  return alGetError() == AL_NO_ERROR ? mrb_true_value() : mrb_false_value();
}

static mrb_value
mrb_al_distance_model(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  mrb_get_args(mrb, "i", &value);
  alDistanceModel((ALenum)value);
  return alGetError() == AL_NO_ERROR ? mrb_true_value() : mrb_false_value();
}

typedef struct mrb_al_buffers_data_t {
  ALsizei size;
  ALuint *buffers;
} mrb_al_buffers_data_t;

typedef struct mrb_al_buffer_data_t {
  bool   do_delete_on_free;
  ALuint buffer;
} mrb_al_buffer_data_t;

typedef struct mrb_al_sources_data_t {
  ALsizei size;
  ALuint *sources;
} mrb_al_sources_data_t;

typedef struct mrb_al_source_data_t {
  bool   do_delete_on_free;
  ALuint source;
} mrb_al_source_data_t;

static void
mrb_al_buffers_free(mrb_state *mrb, void *p)
{
  mrb_al_buffers_data_t *data = (mrb_al_buffers_data_t*)p;
  if (NULL != data) {
    alDeleteBuffers(data->size, data->buffers);
    mrb_free(mrb, data->buffers);
    mrb_free(mrb, data);
  }
}

static void
mrb_al_buffer_free(mrb_state *mrb, void *p)
{
  mrb_al_buffer_data_t *data = (mrb_al_buffer_data_t*)p;
  if (NULL != data) {
    if (data->do_delete_on_free) {
      alDeleteBuffers(1, &data->buffer);
    }
    mrb_free(mrb, data);
  }
}

static void
mrb_al_sources_free(mrb_state *mrb, void *p)
{
  mrb_al_sources_data_t *data = (mrb_al_sources_data_t*)p;
  if (NULL != data) {
    alDeleteSources(data->size, data->sources);
    mrb_free(mrb, data->sources);
    mrb_free(mrb, data);
  }
}

static void
mrb_al_source_free(mrb_state *mrb, void *p)
{
  mrb_al_source_data_t *data = (mrb_al_source_data_t*)p;
  if (NULL != data) {
    if (data->do_delete_on_free) {
      alDeleteSources(1, &data->source);
    }
    mrb_free(mrb, data);
  }
}

static struct mrb_data_type const mrb_al_buffers_data_type = { "Buffers", mrb_al_buffers_free };
static struct mrb_data_type const mrb_al_buffer_data_type  = { "Buffer",  mrb_al_buffer_free };
static struct mrb_data_type const mrb_al_sources_data_type = { "Sources", mrb_al_sources_free };
static struct mrb_data_type const mrb_al_source_data_type  = { "Source",  mrb_al_source_free };

static mrb_value
mrb_al_buffers_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_al_buffers_data_t *data =
    (mrb_al_buffers_data_t*)DATA_PTR(self);
  mrb_int size;
  int const argc = mrb_get_args(mrb, "|i", &size);

  if (NULL == data) {
    data = (mrb_al_buffers_data_t*)mrb_malloc(mrb, sizeof(mrb_al_buffers_data_t));
    if (NULL == data) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
    }
  }

  if (0 == argc) {
    data->size = 1;
    data->buffers = (ALuint*)mrb_malloc(mrb, sizeof(ALuint) * 1);
  } else {
    data->size = (ALsizei)size;
    data->buffers = (ALuint*)mrb_malloc(mrb, sizeof(ALuint) * size);
  }

  DATA_PTR(self)  = data;
  DATA_TYPE(self) = &mrb_al_buffers_data_type;

  if (NULL == data->buffers) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }

  alGenBuffers(data->size, data->buffers);
  ALenum const e = alGetError();
  if (AL_NO_ERROR != e) {
    mrb_raise(mrb, class_ALError, alGetString(e));
  }

  return self;
}

static mrb_value
mrb_al_buffers_each(mrb_state *mrb, mrb_value self)
{
  mrb_al_buffers_data_t *data =
    (mrb_al_buffers_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_buffers_data_type);
  mrb_value block;
  mrb_get_args(mrb, "&", &block);

  ALsizei const size = data->size;
  ALsizei i;
  mrb_al_buffer_data_t *buf;
  for (i = 0; i < size; ++i) {
    buf = (mrb_al_buffer_data_t*)mrb_malloc(mrb, sizeof(mrb_al_buffer_data_t));
    if (NULL == buf) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
    }
    buf->do_delete_on_free = false;
    buf->buffer = data->buffers[i];
    mrb_yield(
      mrb,
      block,
      mrb_obj_value(Data_Wrap_Struct(mrb, class_Buffer, &mrb_al_buffer_data_type, buf)));
  }
  return self;
}

static mrb_value
mrb_al_buffers_size(mrb_state *mrb, mrb_value self)
{
  mrb_al_buffers_data_t *data =
    (mrb_al_buffers_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_buffers_data_type);
  return mrb_fixnum_value(data->size);
}

static mrb_value
mrb_al_buffers_get_at(mrb_state *mrb, mrb_value self)
{
  mrb_al_buffers_data_t *data =
    (mrb_al_buffers_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_buffers_data_type);
  mrb_int index;
  mrb_get_args(mrb, "i", &index);

  if ((index < 0) || (index > (mrb_int)data->size)) {
    mrb_raise(mrb, E_INDEX_ERROR, "index is out of buffers.");
  }

  mrb_al_buffer_data_t *buf = mrb_malloc(mrb, sizeof(mrb_al_buffer_data_t));
  if (NULL == buf) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }
  buf->do_delete_on_free = false;
  buf->buffer = data->buffers[index];
  return mrb_obj_value(Data_Wrap_Struct(mrb, class_Buffer, &mrb_al_buffer_data_type, buf));
}

static mrb_value
mrb_al_buffer_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_al_buffer_data_t *data =
    (mrb_al_buffer_data_t*)DATA_PTR(self);

  if (NULL == data) {
    data = (mrb_al_buffer_data_t*)mrb_malloc(mrb, sizeof(mrb_al_buffer_data_t));
    if (NULL == data) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
    }
  }

  data->do_delete_on_free = true;
  data->buffer = 0;

  DATA_PTR(self) = data;
  DATA_TYPE(self) = &mrb_al_buffer_data_type;

  alGenBuffers(1, &data->buffer);
  ALenum const e = alGetError();
  if (AL_NO_ERROR != e) {
    mrb_raise(mrb, class_ALError, alGetString(e));
  }

  return self;
}

static mrb_value
mrb_al_buffer_get_xxx(mrb_state *mrb, mrb_value *self, ALenum param)
{
  mrb_al_buffer_data_t *data =
    (mrb_al_buffer_data_t*)mrb_data_get_ptr(mrb, *self, &mrb_al_buffer_data_type);
  ALint value;
  alGetBufferi(data->buffer, param, &value);
  ALenum const e = alGetError();
  if (AL_NO_ERROR != e) {
    mrb_raise(mrb, class_ALError, alGetString(e));
  }
  return mrb_fixnum_value(value);
}

static mrb_value
mrb_al_buffer_get_size(mrb_state *mrb, mrb_value self)
{
  return mrb_al_buffer_get_xxx(mrb, &self, AL_SIZE);
}

static mrb_value
mrb_al_buffer_get_frequency(mrb_state *mrb, mrb_value self)
{
  return mrb_al_buffer_get_xxx(mrb, &self, AL_FREQUENCY);
}

static mrb_value
mrb_al_buffer_get_channels(mrb_state *mrb, mrb_value self)
{
  return mrb_al_buffer_get_xxx(mrb, &self, AL_CHANNELS);
}

static mrb_value
mrb_al_buffer_get_bits(mrb_state *mrb, mrb_value self)
{
  return mrb_al_buffer_get_xxx(mrb, &self, AL_BITS);
}

static mrb_value
mrb_al_buffer_create_hello_world(mrb_state *mrb, mrb_value self)
{
  ALuint name = alutCreateBufferHelloWorld();
  if (0 == name) {
    return mrb_nil_value();
  }
  mrb_al_buffer_data_t *buf = (mrb_al_buffer_data_t*)mrb_malloc(mrb, sizeof(mrb_al_buffer_data_t));
  if (NULL == buf) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }
  buf->do_delete_on_free = true;
  buf->buffer = name;
  return mrb_obj_value(Data_Wrap_Struct(mrb, class_Buffer, &mrb_al_buffer_data_type, buf));
}

static mrb_value
mrb_al_buffer_create_from_file(mrb_state *mrb, mrb_value self)
{
  mrb_value file;
  mrb_get_args(mrb, "S", &file);
  ALuint name = alutCreateBufferFromFile(RSTRING_PTR(file));
  if (0 == name) {
    ALenum const e = alutGetError();
    if (e == AL_NO_ERROR) {
      return mrb_nil_value();
    }
    mrb_raise(mrb, class_ALError, alutGetErrorString(e));
  }
  mrb_al_buffer_data_t *buf = (mrb_al_buffer_data_t*)mrb_malloc(mrb, sizeof(mrb_al_buffer_data_t));
  if (NULL == buf) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }
  buf->do_delete_on_free = true;
  buf->buffer = name;
  return mrb_obj_value(Data_Wrap_Struct(mrb, class_Buffer, &mrb_al_buffer_data_type, buf));
}

static mrb_value
mrb_al_buffer_create_waveform(mrb_state *mrb, mrb_value self)
{
  mrb_int shape;
  mrb_float freq, phase, duration;
  mrb_get_args(mrb, "ifff", &shape, &freq, &phase, &duration);
  ALuint name = alutCreateBufferWaveform((ALenum)shape, (ALfloat)freq, (ALfloat)phase, (ALfloat)duration);
  if (0 == name) {
    return mrb_nil_value();
  }
  mrb_al_buffer_data_t *buf = (mrb_al_buffer_data_t*)mrb_malloc(mrb, sizeof(mrb_al_buffer_data_t));
  if (NULL == buf) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }
  buf->do_delete_on_free = true;
  buf->buffer = name;
  return mrb_obj_value(Data_Wrap_Struct(mrb, class_Buffer, &mrb_al_buffer_data_type, buf));
}

static mrb_value
mrb_al_sources_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_al_sources_data_t *data =
    (mrb_al_sources_data_t*)DATA_PTR(self);
  mrb_int size;
  int const argc = mrb_get_args(mrb, "|i", &size);

  if (NULL == data) {
    data = (mrb_al_sources_data_t*)mrb_malloc(mrb, sizeof(mrb_al_sources_data_t));
    if (NULL == data) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
    }
  }

  if (0 == argc) {
    data->size = 1;
    data->sources = (ALuint*)mrb_malloc(mrb, sizeof(ALuint) * 1);
  } else {
    data->size = (ALsizei)size;
    data->sources = (ALuint*)mrb_malloc(mrb, sizeof(ALuint) * size);
  }

  DATA_PTR(self)  = data;
  DATA_TYPE(self) = &mrb_al_sources_data_type;

  if (NULL == data->sources) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }

  alGenSources(data->size, data->sources);
  ALenum const e = alGetError();
  if (AL_NO_ERROR != e) {
    mrb_raise(mrb, class_ALError, alGetString(e));
  }

  return self;
}

static mrb_value
mrb_al_sources_each(mrb_state *mrb, mrb_value self)
{
  mrb_al_sources_data_t *data =
    (mrb_al_sources_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_sources_data_type);
  mrb_value block;
  mrb_get_args(mrb, "&", &block);

  ALsizei const size = data->size;
  ALsizei i;
  mrb_al_source_data_t *src;
  for (i = 0; i < size; ++i) {
    src = (mrb_al_source_data_t*)mrb_malloc(mrb, sizeof(mrb_al_source_data_t));
    if (NULL == src) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
    }
    src->do_delete_on_free = false;
    src->source = data->sources[i];
    mrb_yield(
      mrb,
      block,
      mrb_obj_value(Data_Wrap_Struct(mrb, class_Source, &mrb_al_source_data_type, src)));
  }
  return self;
}

static mrb_value
mrb_al_sources_size(mrb_state *mrb, mrb_value self)
{
  mrb_al_sources_data_t *data =
    (mrb_al_sources_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_sources_data_type);
  return mrb_fixnum_value(data->size);
}

static mrb_value
mrb_al_sources_get_at(mrb_state *mrb, mrb_value self)
{
  mrb_al_sources_data_t *data =
    (mrb_al_sources_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_sources_data_type);
  mrb_int index;
  mrb_get_args(mrb, "i", &index);

  if ((index < 0) || (index > (mrb_int)data->size)) {
    mrb_raise(mrb, E_INDEX_ERROR, "index is out of buffers.");
  }

  mrb_al_source_data_t *buf = mrb_malloc(mrb, sizeof(mrb_al_source_data_t));
  if (NULL == buf) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }
  buf->do_delete_on_free = false;
  buf->source = data->sources[index];
  return mrb_obj_value(Data_Wrap_Struct(mrb, class_Source, &mrb_al_source_data_type, buf));
}

static mrb_value
mrb_al_source_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)DATA_PTR(self);

  if (NULL == data) {
    data = (mrb_al_source_data_t*)mrb_malloc(mrb, sizeof(mrb_al_source_data_t));
    if (NULL == data) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
    }
  }

  data->do_delete_on_free = true;
  data->source = 0;

  DATA_PTR(self) = data;
  DATA_TYPE(self) = &mrb_al_source_data_type;

  alGenSources(1, &data->source);
  ALenum const e = alGetError();
  if (AL_NO_ERROR != e) {
    mrb_raise(mrb, class_ALError, alGetString(e));
  }

  return self;
}

static mrb_value
mrb_al_source_get_xxx_i(mrb_state *mrb, mrb_value *self, ALenum param)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, *self, &mrb_al_source_data_type);
  ALint value;
  alGetSourcei(data->source, param, &value);
  ALenum const e = alGetError();
  if (AL_NO_ERROR != e) {
    mrb_raise(mrb, class_ALError, alGetString(e));
  }
  return mrb_fixnum_value(value);
}

static void
mrb_al_source_set_xxx_i(mrb_state *mrb, mrb_value *self, ALenum param, mrb_int value)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, *self, &mrb_al_source_data_type);
  alSourcei(data->source, param, (ALint)value);
  ALenum const e = alGetError();
  if (AL_NO_ERROR != e) {
    mrb_raise(mrb, class_ALError, alGetString(e));
  }
}

static mrb_value
mrb_al_source_get_xxx_b(mrb_state *mrb, mrb_value *self, ALenum param)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, *self, &mrb_al_source_data_type);
  ALint value;
  alGetSourcei(data->source, param, &value);
  ALenum const e = alGetError();
  if (AL_NO_ERROR != e) {
    mrb_raise(mrb, class_ALError, alGetString(e));
  }
  return (value == AL_FALSE) ? mrb_false_value() : mrb_true_value();
}

static void
mrb_al_source_set_xxx_b(mrb_state *mrb, mrb_value *self, ALenum param, mrb_bool value)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, *self, &mrb_al_source_data_type);
  alSourcei(data->source, param, value ? AL_TRUE : AL_FALSE);
  ALenum const e = alGetError();
  if (AL_NO_ERROR != e) {
    mrb_raise(mrb, class_ALError, alGetString(e));
  }
}

static mrb_value
mrb_al_source_get_xxx_f(mrb_state *mrb, mrb_value *self, ALenum param)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, *self, &mrb_al_source_data_type);
  ALfloat value;
  alGetSourcef(data->source, param, &value);
  ALenum const e = alGetError();
  if (AL_NO_ERROR != e) {
    mrb_raise(mrb, class_ALError, alGetString(e));
  }
  return mrb_float_value(mrb, value);
}

static void
mrb_al_source_set_xxx_f(mrb_state *mrb, mrb_value *self, ALenum param, mrb_float value)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, *self, &mrb_al_source_data_type);
  alSourcef(data->source, param, (ALfloat)value);
  ALenum const e = alGetError();
  if (AL_NO_ERROR != e) {
    mrb_raise(mrb, class_ALError, alGetString(e));
  }
}

static mrb_value
mrb_al_source_get_xxx_3f(mrb_state *mrb, mrb_value *self, ALenum param)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, *self, &mrb_al_source_data_type);
  ALfloat values[3] = { 0.0f, 0.0f, 0.0f };
  alGetSourcefv(data->source, param, values);
  ALenum const e = alGetError();
  if (AL_NO_ERROR != e) {
    mrb_raise(mrb, class_ALError, alGetString(e));
  }
  mrb_value array_values[3] = {
    mrb_float_value(mrb, values[0]),
    mrb_float_value(mrb, values[1]),
    mrb_float_value(mrb, values[2])
  };
  return mrb_ary_new_from_values(mrb, 3, array_values);
}

static mrb_value
mrb_al_source_is_relative(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_b(mrb, &self, AL_SOURCE_RELATIVE);
}

static mrb_value
mrb_al_source_set_relative(mrb_state *mrb, mrb_value self)
{
  mrb_bool value;
  mrb_get_args(mrb, "b", &value);
  mrb_al_source_set_xxx_b(mrb, &self, AL_SOURCE_RELATIVE, value);
  return value ? mrb_true_value() : mrb_false_value();
}

static mrb_value
mrb_al_source_get_type(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_i(mrb, &self, AL_SOURCE_TYPE);
}

static mrb_value
mrb_al_source_set_type(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  mrb_get_args(mrb, "i", &value);
  mrb_al_source_set_xxx_i(mrb, &self, AL_SOURCE_TYPE, (ALCint)value);
  return self;
}

static mrb_value
mrb_al_source_is_looping(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_b(mrb, &self, AL_LOOPING);
}

static mrb_value
mrb_al_source_set_looping(mrb_state *mrb, mrb_value self)
{
  mrb_bool value;
  mrb_get_args(mrb, "b", &value);
  mrb_al_source_set_xxx_b(mrb, &self, AL_LOOPING, value);
  return value ? mrb_true_value() : mrb_false_value();
}

static mrb_value
mrb_al_source_get_buffer(mrb_state *mrb, mrb_value self)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_source_data_type);
  ALuint value;
  alGetSourcei(data->source, AL_BUFFER, (ALint*)&value);
  mrb_al_buffer_data_t *buf = mrb_malloc(mrb, sizeof(mrb_al_buffer_data_t));
  if (NULL == buf) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }
  buf->do_delete_on_free = false;
  buf->buffer = value;
  return mrb_obj_value(Data_Wrap_Struct(mrb, class_Buffer, &mrb_al_buffer_data_type, buf));
}

static mrb_value
mrb_al_source_set_buffer(mrb_state *mrb, mrb_value self)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_source_data_type);
  mrb_value buffer;
  mrb_get_args(mrb, "o", &buffer);
  mrb_al_buffer_data_t *buf_data =
    (mrb_al_buffer_data_t*)mrb_data_get_ptr(mrb, buffer, &mrb_al_buffer_data_type);
  alSourcei(data->source, AL_BUFFER, buf_data->buffer);
  return buffer;
}

static mrb_value
mrb_al_source_get_buffers_queued(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_i(mrb, &self, AL_BUFFERS_QUEUED);
}

static mrb_value
mrb_al_source_get_buffers_processed(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_i(mrb, &self, AL_BUFFERS_PROCESSED);
}

static mrb_value
mrb_al_source_get_min_gain(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_f(mrb, &self, AL_MIN_GAIN);
}

static mrb_value
mrb_al_source_set_min_gain(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_get_args(mrb, "f", &value);
  mrb_al_source_set_xxx_f(mrb, &self, AL_MIN_GAIN, value);
  return self;
}

static mrb_value
mrb_al_source_get_max_gain(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_f(mrb, &self, AL_MAX_GAIN);
}

static mrb_value
mrb_al_source_set_max_gain(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_get_args(mrb, "f", &value);
  mrb_al_source_set_xxx_f(mrb, &self, AL_MAX_GAIN, value);
  return mrb_float_value(mrb, value);
}

static mrb_value
mrb_al_source_get_reference_distance(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_f(mrb, &self, AL_REFERENCE_DISTANCE);
}

static mrb_value
mrb_al_source_set_reference_distance(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_get_args(mrb, "f", &value);
  mrb_al_source_set_xxx_f(mrb, &self, AL_REFERENCE_DISTANCE, value);
  return self;
}

static mrb_value
mrb_al_source_get_rolloff_factor(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_f(mrb, &self, AL_ROLLOFF_FACTOR);
}

static mrb_value
mrb_al_source_set_rolloff_factor(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_get_args(mrb, "f", &value);
  mrb_al_source_set_xxx_f(mrb, &self, AL_ROLLOFF_FACTOR, value);
  return self;
}

static mrb_value
mrb_al_source_get_max_distance(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_f(mrb, &self, AL_MAX_DISTANCE);
}

static mrb_value
mrb_al_source_set_max_distance(mrb_state *mrb, mrb_value self)
{
   mrb_float value;
  mrb_get_args(mrb, "f", &value);
  mrb_al_source_set_xxx_f(mrb, &self, AL_MAX_DISTANCE, value);
  return self; 
}

static mrb_value
mrb_al_source_get_pitch(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_f(mrb, &self, AL_PITCH);
}

static mrb_value
mrb_al_source_set_pitch(mrb_state *mrb, mrb_value self)
{
   mrb_float value;
  mrb_get_args(mrb, "f", &value);
  mrb_al_source_set_xxx_f(mrb, &self, AL_PITCH, value);
  return self; 
}

static mrb_value
mrb_al_source_get_direction(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_3f(mrb, &self, AL_DIRECTION);
}

static mrb_value
mrb_al_source_get_cone_inner_angle(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_f(mrb, &self, AL_CONE_INNER_ANGLE);
}

static mrb_value
mrb_al_source_get_cone_outer_angle(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_f(mrb, &self, AL_CONE_OUTER_ANGLE);
}

static mrb_value
mrb_al_source_get_cone_outer_gain(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_f(mrb, &self, AL_CONE_OUTER_GAIN);
}

static mrb_value
mrb_al_source_get_sec_offset(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_f(mrb, &self, AL_SEC_OFFSET);
}

static mrb_value
mrb_al_source_get_sample_offset(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_f(mrb, &self, AL_SAMPLE_OFFSET);
}

static mrb_value
mrb_al_source_get_byte_offset(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_f(mrb, &self, AL_BYTE_OFFSET);
}

static mrb_value
mrb_al_source_is_playing(mrb_state *mrb, mrb_value self)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_source_data_type);
  ALint state = 0;
  alGetSourcei(data->source, AL_SOURCE_STATE, &state);
  return (state == AL_PLAYING) ? mrb_true_value() : mrb_false_value();
}

static mrb_value
mrb_al_source_set_playing(mrb_state *mrb, mrb_value self)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_source_data_type);
  mrb_bool state;
  mrb_get_args(mrb, "b", &state);
  if (state) {
    alSourcePlay(data->source);
  } else {
    alSourceStop(data->source);
  }
  return mrb_al_source_is_playing(mrb, self);
}

static mrb_value
mrb_al_source_get_state(mrb_state *mrb, mrb_value self)
{
  return mrb_al_source_get_xxx_i(mrb, &self, AL_SOURCE_STATE);
}

static mrb_value
mrb_al_source_play(mrb_state *mrb, mrb_value self)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_source_data_type);
  alSourcePlay(data->source);
  return mrb_nil_value();
}

static mrb_value
mrb_al_source_stop(mrb_state *mrb, mrb_value self)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_source_data_type);
  alSourceStop(data->source);
  return mrb_nil_value();
}

static mrb_value
mrb_al_source_pause(mrb_state *mrb, mrb_value self)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_source_data_type);
  alSourcePause(data->source);
  return mrb_nil_value();
}

static mrb_value
mrb_al_source_rewind(mrb_state *mrb, mrb_value self)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_source_data_type);
  alSourceRewind(data->source);
  return mrb_nil_value();
}

static mrb_value
mrb_al_source_queue_buffers(mrb_state *mrb, mrb_value self)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_source_data_type);
  mrb_value arg;
  mrb_int offset = 0;
  mrb_int count = 0;
  int const argc = mrb_get_args(mrb, "o|ii", &arg, &offset, &count);
  if (mrb_type(arg) != MRB_TT_DATA) {
    mrb_raise(mrb, E_TYPE_ERROR, "given argument is not a data object.");
  }
  if (DATA_TYPE(arg) == &mrb_al_buffer_data_type) {
    mrb_al_buffer_data_t *bdata = (mrb_al_buffer_data_t*)DATA_PTR(arg);
    alSourceQueueBuffers(data->source, 1, &bdata->buffer);
    ALenum const e = alGetError();
    if (AL_NO_ERROR != e) {
      mrb_raise(mrb, class_ALError, alGetString(e));
    }
  } else if (DATA_TYPE(arg) == &mrb_al_buffers_data_type) {
    mrb_al_buffers_data_t *bdata = (mrb_al_buffers_data_t*)DATA_PTR(arg);
    ALsizei size = bdata->size;
    if (2 <= argc) {
      if (bdata->size <= offset) {
        mrb_raise(mrb, E_INDEX_ERROR, "index is out of buffers.");
      }
      size = bdata->size - offset;
    }
    if (3 <= argc) {
      if (bdata->size <= offset + count) {
        mrb_raise(mrb, E_INDEX_ERROR, "index is out of buffers.");
      }
      size = count;
    }
    alSourceQueueBuffers(data->source, size, &bdata->buffers[offset]);
    ALenum const e = alGetError();
    if (AL_NO_ERROR != e) {
      mrb_raise(mrb, class_ALError, alGetString(e));
    }
  } else {
    mrb_raise(mrb, E_TYPE_ERROR, "given argument is not a buffer.");
  }
  return self;
}

static mrb_value
mrb_al_source_unqueue_buffers(mrb_state *mrb, mrb_value self)
{
  mrb_al_source_data_t *data =
    (mrb_al_source_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_source_data_type);
  mrb_value arg;
  mrb_int offset = 0;
  mrb_int count = 0;
  int const argc = mrb_get_args(mrb, "o|ii", &arg, &offset, &count);
  if (mrb_type(arg) != MRB_TT_DATA) {
    mrb_raise(mrb, E_TYPE_ERROR, "given argument is not a data object.");
  }
  if (DATA_TYPE(arg) == &mrb_al_buffer_data_type) {
    mrb_al_buffer_data_t *bdata = (mrb_al_buffer_data_t*)DATA_PTR(arg);
    alSourceUnqueueBuffers(data->source, 1, &bdata->buffer);
    ALenum const e = alGetError();
    if (AL_NO_ERROR != e) {
      mrb_raise(mrb, E_RUNTIME_ERROR, alGetString(e));
    }
  } else if (DATA_TYPE(arg) == &mrb_al_buffers_data_type) {
    mrb_al_buffers_data_t *bdata = (mrb_al_buffers_data_t*)DATA_PTR(arg);
    ALsizei size = bdata->size;
    if (2 <= argc) {
      if (bdata->size <= offset) {
        mrb_raise(mrb, E_INDEX_ERROR, "index is out of buffers.");
      }
      size = bdata->size - offset;
    }
    if (3 <= argc) {
      if (bdata->size <= offset + count) {
        mrb_raise(mrb, E_INDEX_ERROR, "index is out of buffers.");
      }
      size = count;
    }
    alSourceUnqueueBuffers(data->source, size, &bdata->buffers[offset]);
    ALenum const e = alGetError();
    if (AL_NO_ERROR != e) {
      mrb_raise(mrb, class_ALError, alGetString(e));
    }
  } else {
    mrb_raise(mrb, E_TYPE_ERROR, "given argument is not a buffer.");
  }
  return self;
}

void
mruby_openal_al_init(mrb_state *mrb)
{
  mod_AL = mrb_define_module(mrb, "AL");
  class_Buffers  = mrb_define_class_under(mrb, mod_AL, "Buffers",  mrb->object_class);
  class_Buffer   = mrb_define_class_under(mrb, mod_AL, "Buffer",   mrb->object_class);
  class_Sources  = mrb_define_class_under(mrb, mod_AL, "Sources",  mrb->object_class);
  class_Source   = mrb_define_class_under(mrb, mod_AL, "Source",   mrb->object_class);
  class_Listener = mrb_define_class_under(mrb, mod_AL, "Listener", mrb->object_class);
  class_ALError  = mrb_define_class_under(mrb, mod_AL, "ALError",  mrb->eStandardError_class);

  MRB_SET_INSTANCE_TT(class_Buffers,  MRB_TT_DATA);
  MRB_SET_INSTANCE_TT(class_Buffer,   MRB_TT_DATA);
  MRB_SET_INSTANCE_TT(class_Sources,  MRB_TT_DATA);
  MRB_SET_INSTANCE_TT(class_Source,   MRB_TT_DATA);
  MRB_SET_INSTANCE_TT(class_Listener, MRB_TT_DATA);

  mrb_define_module_function(mrb, mod_AL, "get_error",         mrb_al_get_error,        ARGS_NONE());
  mrb_define_module_function(mrb, mod_AL, "enable",            mrb_al_enable,           ARGS_REQ(1));
  mrb_define_module_function(mrb, mod_AL, "disable",           mrb_al_disable,          ARGS_REQ(1));
  mrb_define_module_function(mrb, mod_AL, "eanbled?",          mrb_al_is_enabled,       ARGS_REQ(1));
  mrb_define_module_function(mrb, mod_AL, "boolean",           mrb_al_get_boolean,      ARGS_REQ(1));
  mrb_define_module_function(mrb, mod_AL, "integer",           mrb_al_get_integer,      ARGS_REQ(1));
  mrb_define_module_function(mrb, mod_AL, "float",             mrb_al_get_float,        ARGS_REQ(1));
  mrb_define_module_function(mrb, mod_AL, "string",            mrb_al_get_string,       ARGS_REQ(1));
  mrb_define_module_function(mrb, mod_AL, "doppler_factor=",   mrb_al_doppler_factor,   ARGS_REQ(1));
  mrb_define_module_function(mrb, mod_AL, "doppler_velocity=", mrb_al_doppler_velocity, ARGS_REQ(1));
  mrb_define_module_function(mrb, mod_AL, "speed_of_sound=",   mrb_al_speed_of_sound,   ARGS_REQ(1));
  mrb_define_module_function(mrb, mod_AL, "distance_model=",   mrb_al_distance_model,   ARGS_REQ(1));

  mrb_define_method(mrb, class_Buffers, "initialize", mrb_al_buffers_initialize, ARGS_REQ(1));
  mrb_define_method(mrb, class_Buffers, "each",       mrb_al_buffers_each,       ARGS_NONE());
  mrb_define_method(mrb, class_Buffers, "size",       mrb_al_buffers_size,       ARGS_NONE());
  mrb_define_method(mrb, class_Buffers, "[]",         mrb_al_buffers_get_at,     ARGS_REQ(1));

  mrb_define_method(mrb, class_Buffer, "initialize", mrb_al_buffer_initialize,    ARGS_NONE());
  mrb_define_method(mrb, class_Buffer, "size",       mrb_al_buffer_get_size,      ARGS_NONE());
  mrb_define_method(mrb, class_Buffer, "frequency",  mrb_al_buffer_get_frequency, ARGS_NONE());
  mrb_define_method(mrb, class_Buffer, "channels",   mrb_al_buffer_get_channels,  ARGS_NONE());
  mrb_define_method(mrb, class_Buffer, "bits",       mrb_al_buffer_get_bits,      ARGS_NONE());
  mrb_define_class_method(mrb, class_Buffer, "hello_world", mrb_al_buffer_create_hello_world, ARGS_NONE());
  mrb_define_class_method(mrb, class_Buffer, "from_file",   mrb_al_buffer_create_from_file,   ARGS_REQ(1));
  mrb_define_class_method(mrb, class_Buffer, "waveform",    mrb_al_buffer_create_waveform,    ARGS_REQ(4));

  mrb_define_method(mrb, class_Sources, "initialize", mrb_al_sources_initialize, ARGS_REQ(1));
  mrb_define_method(mrb, class_Sources, "each",       mrb_al_sources_each,       ARGS_NONE());
  mrb_define_method(mrb, class_Sources, "size",       mrb_al_sources_size,       ARGS_NONE());
  mrb_define_method(mrb, class_Sources, "[]",         mrb_al_sources_get_at,     ARGS_REQ(1));

  mrb_define_method(mrb, class_Source, "initialize",          mrb_al_source_initialize,             ARGS_NONE());
  mrb_define_method(mrb, class_Source, "relative?",           mrb_al_source_is_relative,            ARGS_NONE());
  mrb_define_method(mrb, class_Source, "relative=",           mrb_al_source_set_relative,           ARGS_REQ(1));
  mrb_define_method(mrb, class_Source, "type",                mrb_al_source_get_type,               ARGS_NONE());
  mrb_define_method(mrb, class_Source, "type=",               mrb_al_source_set_type,               ARGS_REQ(1));
  mrb_define_method(mrb, class_Source, "looping?",            mrb_al_source_is_looping,             ARGS_NONE());
  mrb_define_method(mrb, class_Source, "looping=",            mrb_al_source_set_looping,            ARGS_REQ(1));
  mrb_define_method(mrb, class_Source, "buffer",              mrb_al_source_get_buffer,             ARGS_NONE());
  mrb_define_method(mrb, class_Source, "buffer=",             mrb_al_source_set_buffer,             ARGS_REQ(1));
  mrb_define_method(mrb, class_Source, "buffers_queued",      mrb_al_source_get_buffers_queued,     ARGS_NONE());
  mrb_define_method(mrb, class_Source, "buffers_processed",   mrb_al_source_get_buffers_processed,  ARGS_NONE());
  mrb_define_method(mrb, class_Source, "min_gain",            mrb_al_source_get_min_gain,           ARGS_NONE());
  mrb_define_method(mrb, class_Source, "min_gain=",           mrb_al_source_set_min_gain,           ARGS_REQ(1));
  mrb_define_method(mrb, class_Source, "max_gain",            mrb_al_source_get_max_gain,           ARGS_NONE());
  mrb_define_method(mrb, class_Source, "max_gain=",           mrb_al_source_set_max_gain,           ARGS_REQ(1));
  mrb_define_method(mrb, class_Source, "reference_distance",  mrb_al_source_get_reference_distance, ARGS_NONE());
  mrb_define_method(mrb, class_Source, "reference_distance=", mrb_al_source_set_reference_distance, ARGS_REQ(1));
  mrb_define_method(mrb, class_Source, "rolloff_factor",      mrb_al_source_get_rolloff_factor,     ARGS_NONE());
  mrb_define_method(mrb, class_Source, "rolloff_factor=",     mrb_al_source_set_rolloff_factor,     ARGS_REQ(1));
  mrb_define_method(mrb, class_Source, "max_distance",        mrb_al_source_get_max_distance,       ARGS_NONE());
  mrb_define_method(mrb, class_Source, "max_distance=",       mrb_al_source_set_max_distance,       ARGS_REQ(1));
  mrb_define_method(mrb, class_Source, "pitch",               mrb_al_source_get_pitch,              ARGS_NONE());
  mrb_define_method(mrb, class_Source, "pitch=",              mrb_al_source_set_pitch,              ARGS_REQ(1));
  mrb_define_method(mrb, class_Source, "direction",           mrb_al_source_get_direction,          ARGS_NONE());
  mrb_define_method(mrb, class_Source, "cone_inner_angle",    mrb_al_source_get_cone_inner_angle,   ARGS_NONE());
  mrb_define_method(mrb, class_Source, "cone_outer_angle",    mrb_al_source_get_cone_outer_angle,   ARGS_NONE());
  mrb_define_method(mrb, class_Source, "cone_outer_gain",     mrb_al_source_get_cone_outer_gain,    ARGS_NONE());
  mrb_define_method(mrb, class_Source, "sec_offset",          mrb_al_source_get_sec_offset,         ARGS_NONE());
  mrb_define_method(mrb, class_Source, "sample_offset",       mrb_al_source_get_sample_offset,      ARGS_NONE());
  mrb_define_method(mrb, class_Source, "byte_offset",         mrb_al_source_get_byte_offset,        ARGS_NONE());
  mrb_define_method(mrb, class_Source, "playing?",            mrb_al_source_is_playing,             ARGS_NONE());
  mrb_define_method(mrb, class_Source, "playing=",            mrb_al_source_set_playing,            ARGS_REQ(1));
  mrb_define_method(mrb, class_Source, "state",               mrb_al_source_get_state,              ARGS_NONE());
  mrb_define_method(mrb, class_Source, "play",                mrb_al_source_play,                   ARGS_NONE());
  mrb_define_method(mrb, class_Source, "stop",                mrb_al_source_stop,                   ARGS_NONE());
  mrb_define_method(mrb, class_Source, "pause",               mrb_al_source_pause,                  ARGS_NONE());
  mrb_define_method(mrb, class_Source, "rewind",              mrb_al_source_rewind,                 ARGS_NONE());
  mrb_define_method(mrb, class_Source, "queue_buffers",       mrb_al_source_queue_buffers,          ARGS_REQ(1));
  mrb_define_method(mrb, class_Source, "unqueue_buffers",     mrb_al_source_unqueue_buffers,        ARGS_REQ(1));

  mrb_define_const(mrb, class_Source, "UNDETERMINED", mrb_fixnum_value(AL_UNDETERMINED));
  mrb_define_const(mrb, class_Source, "STATIC",       mrb_fixnum_value(AL_UNDETERMINED));
  mrb_define_const(mrb, class_Source, "STREAMING",    mrb_fixnum_value(AL_UNDETERMINED));

  mrb_define_const(mrb, class_Buffer, "WAVEFORM_SINE",       mrb_fixnum_value(ALUT_WAVEFORM_SINE));
  mrb_define_const(mrb, class_Buffer, "WAVEFORM_SQUARE",     mrb_fixnum_value(ALUT_WAVEFORM_SQUARE));
  mrb_define_const(mrb, class_Buffer, "WAVEFORM_SAWTOOTH",   mrb_fixnum_value(ALUT_WAVEFORM_SAWTOOTH));
  mrb_define_const(mrb, class_Buffer, "WAVEFORM_WHITENOISE", mrb_fixnum_value(ALUT_WAVEFORM_WHITENOISE));
  mrb_define_const(mrb, class_Buffer, "WAVEFORM_IMPULSE",    mrb_fixnum_value(ALUT_WAVEFORM_IMPULSE));
}

void
mruby_openal_al_final(mrb_state *mrb)
{
}

