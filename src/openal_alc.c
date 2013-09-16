#include "openal.h"
#include "mruby/class.h"
#include "mruby/string.h"
#include "mruby/array.h"
#include <AL/al.h>
#include <AL/alc.h>

static struct RClass *mod_ALC = NULL;
static struct RClass *class_Context = NULL;
static struct RClass *class_Device = NULL;
static struct RClass *class_CaptureDevice = NULL;
static struct RClass *class_ALCError = NULL;

typedef struct mrb_alc_context_data_t {
  ALCcontext *context;
} mrb_alc_context_data_t;

typedef struct mrb_alc_device_data_t {
  ALCdevice *device;
} mrb_alc_device_data_t;

typedef struct mrb_alc_capturedevice_data_t {
  ALCdevice *device;
  ALint      frequency;
  ALenum     format;
  ALsizei    samples;
} mrb_alc_capturedevice_data_t;

static mrb_value specifier_to_array(mrb_state *mrb, ALchar const * const specifier);

static void
mrb_alc_context_free(mrb_state *mrb, void *p)
{
  mrb_alc_context_data_t *data = (mrb_alc_context_data_t*)p;
  if (NULL != data) {
    if (NULL != data->context) {
      alcDestroyContext(data->context);
    }
    mrb_free(mrb, data);
  }
}

static void
mrb_alc_device_free(mrb_state *mrb, void *p)
{
  mrb_alc_device_data_t *data = (mrb_alc_device_data_t*)p;
  if (NULL != data) {
    if (NULL != data->device) {
      alcCloseDevice(data->device);
    }
    mrb_free(mrb, data);
  }
}

static void
mrb_alc_capturedevice_free(mrb_state *mrb, void *p)
{
  mrb_alc_capturedevice_data_t *data = (mrb_alc_capturedevice_data_t*)p;
  if (NULL != data) {
    if (NULL != data->device) {
      alcCaptureCloseDevice(data->device);
    }
    mrb_free(mrb, data);
  }
}

static struct mrb_data_type const mrb_alc_context_data_type       = { "Context",       mrb_alc_context_free };
static struct mrb_data_type const mrb_alc_device_data_type        = { "Device",        mrb_alc_device_free };
static struct mrb_data_type const mrb_alc_capturedevice_data_type = { "CaptureDevice", mrb_alc_capturedevice_free };

static mrb_value
mrb_alc_context_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_alc_context_data_t *context_data =
    (mrb_alc_context_data_t*)DATA_PTR(self);
  mrb_value device;
  mrb_int argc;
  mrb_value *argv;
  mrb_get_args(mrb, "o*", &device, &argv, &argc);
  mrb_alc_device_data_t *device_data =
    (mrb_alc_device_data_t*)mrb_data_get_ptr(mrb, device, &mrb_alc_device_data_type);
  mrb_int i;
  ALCint attrs[argc + 1];
  for (i = 0; i < argc; ++i) {
    if (!mrb_fixnum_p(argv[i])) {
      if (mrb_respond_to(mrb, argv[i], mrb_intern2(mrb, "to_i", 4))) {
        argv[i] = mrb_funcall(mrb, argv[i], "to_i", 0);
      } else {
        mrb_raise(mrb, E_TYPE_ERROR, "attributes must be integer type.");
      }
    }
    attrs[i] = (ALCint)mrb_fixnum(argv[i]);
  }
  attrs[argc] = 0;
  ALCcontext *context = alcCreateContext(device_data->device, attrs);
  if (NULL == context) {
    mrb_raise(mrb, class_ALCError, alcGetString(device_data->device, alcGetError(device_data->device)));
  }
  if (NULL != context_data) {
    mrb_alc_context_free(mrb, context_data);
  }
  context_data = (mrb_alc_context_data_t*)mrb_malloc(mrb, sizeof(mrb_alc_context_data_t));
  if (NULL == context_data) {
    alcDestroyContext(context);
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }
  context_data->context = context;
  DATA_PTR(self) = context_data;
  DATA_TYPE(self) = &mrb_alc_context_data_type;
  return mrb_nil_value();
}

static mrb_value
mrb_alc_context_destroy(mrb_state *mrb, mrb_value self)
{
  mrb_alc_context_data_t *data =
    (mrb_alc_context_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_context_data_type);
  if (NULL  != data->context) {
    alcDestroyContext(data->context);
    data->context = NULL;
  }
  return mrb_nil_value();
}

static mrb_value
mrb_alc_context_is_destroyed(mrb_state *mrb, mrb_value self)
{
  mrb_alc_context_data_t *data =
    (mrb_alc_context_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_context_data_type);
  return (NULL == data->context) ? mrb_true_value() : mrb_false_value();
}

static mrb_value
mrb_alc_context_set_current(mrb_state *mrb, mrb_value self)
{
  mrb_alc_context_data_t *data =
    (mrb_alc_context_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_context_data_type);
  return alcMakeContextCurrent(data->context) == ALC_FALSE ? mrb_false_value() : mrb_true_value();
}

static mrb_value
mrb_alc_context_get_current(mrb_state *mrb, mrb_value self)
{
  ALCcontext *context = alcGetCurrentContext();
  if (NULL == context) {
    return mrb_nil_value();
  }
  mrb_alc_context_data_t *data =
    (mrb_alc_context_data_t*)mrb_malloc(mrb, sizeof(mrb_alc_context_data_t));
  if (NULL == data) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }
  data->context = context;
  return mrb_obj_value(Data_Wrap_Struct(mrb, class_Context, &mrb_alc_context_data_type, data));
}

static mrb_value
mrb_alc_context_get_device(mrb_state *mrb, mrb_value self)
{
  mrb_alc_context_data_t *data =
    (mrb_alc_context_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_context_data_type);
  if (NULL == data->context) {
    mrb_raise(mrb, class_ALCError, "context has already been destroyed.");
  }
  ALCdevice *device = alcGetContextsDevice(data->context);
  mrb_alc_device_data_t *device_data =
    (mrb_alc_device_data_t*)mrb_malloc(mrb, sizeof(mrb_alc_device_data_t));
  if (NULL == device_data) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }
  device_data->device = device;
  return mrb_obj_value(Data_Wrap_Struct(mrb, class_Device, &mrb_alc_device_data_type, device_data));
}

static mrb_value
mrb_alc_context_process(mrb_state *mrb, mrb_value self)
{
  mrb_alc_context_data_t *data =
    (mrb_alc_context_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_context_data_type);
  alcProcessContext(data->context);
  return mrb_nil_value();
}

static mrb_value
mrb_alc_context_suspend(mrb_state *mrb, mrb_value self)
{
  mrb_alc_context_data_t *data =
    (mrb_alc_context_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_context_data_type);
  alcSuspendContext(data->context);
  return mrb_nil_value();
}


static mrb_value
mrb_alc_device_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_alc_device_data_t *data =
    (mrb_alc_device_data_t*)DATA_PTR(self);
  mrb_value name;
  int const argc = mrb_get_args(mrb, "|o", &name);
  ALCdevice *device = NULL;
  if (1 == argc) {
    if (mrb_nil_p(name)) {
      device = alcOpenDevice(NULL);
    } else if (mrb_string_p(name)) {
      device = alcOpenDevice(RSTRING_PTR(name));
    } else if (mrb_respond_to(mrb, name, mrb_intern2(mrb, "to_s", 4))) {
      name = mrb_funcall(mrb, name, "to_s", 0);
      device = alcOpenDevice(RSTRING_PTR(name));
    } else {
      mrb_raise(mrb, E_TYPE_ERROR, "given argument cannot be converted to string.");
    }
    if (NULL == device) {
      mrb_raisef(mrb, class_ALCError, "cannot open device (%S).", name);
    }
  }
  if (NULL != data) {
    mrb_alc_device_free(mrb, data);
  }
  data = (mrb_alc_device_data_t*)mrb_malloc(mrb, sizeof(mrb_alc_device_data_t));
  if (NULL == data) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }
  data->device = device;
  DATA_PTR(self) = data;
  DATA_TYPE(self) = &mrb_alc_device_data_type;
  return self;
}

static mrb_value
mrb_alc_device_get_error(mrb_state *mrb, mrb_value self)
{
  mrb_alc_device_data_t *data =
    (mrb_alc_device_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_device_data_type);
  if (NULL == data->device) {
    mrb_raise(mrb, class_ALCError, "device is not opened.");
  }
  return mrb_fixnum_value(alcGetError(data->device));
}

static mrb_value
mrb_alc_device_get_string(mrb_state *mrb, mrb_value self)
{
  mrb_alc_device_data_t *data =
    (mrb_alc_device_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_device_data_type);
  if (NULL == data->device) {
    mrb_raise(mrb, class_ALCError, "device is not opened.");
  }
  mrb_int param;
  mrb_get_args(mrb, "i", &param);
  return mrb_str_new_cstr(mrb, alcGetString(data->device, (ALCenum)param));
}

static mrb_value
mrb_alc_device_open(mrb_state *mrb, mrb_value self)
{
  mrb_alc_device_data_t *data =
    (mrb_alc_device_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_device_data_type);
  if (NULL != data->device) {
    mrb_raise(mrb, class_ALCError, "device has already opened.");
  }
  mrb_value name = mrb_nil_value();
  int const argc = mrb_get_args(mrb, "|o", &name);
  ALCdevice *device = NULL;
  if (0 == argc) {
    device = alcOpenDevice(NULL);
  } else {
    if (mrb_nil_p(name)) {
      device = alcOpenDevice(NULL);
    } else if (mrb_string_p(name)) {
      device = alcOpenDevice(RSTRING_PTR(name));
    } else if (mrb_respond_to(mrb, name, mrb_intern2(mrb, "to_s", 4))) {
      name = mrb_funcall(mrb, name, "to_s", 0);
      device = alcOpenDevice(RSTRING_PTR(name));
    } else {
      mrb_raise(mrb, E_TYPE_ERROR, "given argument cannot be converted to string.");
    }
  }
  if (NULL == device) {
    mrb_raisef(mrb, class_ALCError, "cannot open device (%S).", name);
  }
  data->device = device;
  return self;
}

static mrb_value
mrb_alc_device_close(mrb_state *mrb, mrb_value self)
{
  mrb_alc_device_data_t *data =
    (mrb_alc_device_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_device_data_type);
  if (NULL != data->device) {
    if (alcCloseDevice(data->device) != ALC_FALSE) {
      data->device = NULL;
    } else {
      mrb_raise(mrb, class_ALCError, alcGetString(data->device, alcGetError(data->device)));
    }
  }
  return self;
}

static mrb_value
mrb_alc_device_is_extension_present(mrb_state *mrb, mrb_value self)
{
  mrb_alc_device_data_t *data =
    (mrb_alc_device_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_device_data_type);
  mrb_value name;
  mrb_get_args(mrb, "S", &name);
  if (mrb_nil_p(name)) {
    return mrb_false_value();
  }
  return alcIsExtensionPresent(data->device, RSTRING_PTR(name)) == ALC_FALSE ? mrb_false_value() : mrb_true_value();
}

static mrb_value
mrb_alc_device_get_enum_value(mrb_state *mrb, mrb_value self)
{
  mrb_alc_device_data_t *data =
    (mrb_alc_device_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_device_data_type);
  mrb_value name;
  mrb_get_args(mrb, "S", &name);
  if (mrb_nil_p(name)) {
    return mrb_false_value();
  }
  ALCenum const e = alcGetEnumValue(data->device, RSTRING_PTR(name));
  if (ALC_INVALID_ENUM == e) {
    mrb_raisef(mrb, class_ALCError, "no specified enum value (%S) is found.", name);
  }
  return mrb_fixnum_value(e);
}

static mrb_value
mrb_alc_device_get_device_specifier(mrb_state *mrb, mrb_value self)
{
  ALchar const * const specifiers = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
  if (NULL == specifiers) {
    return mrb_nil_value();
  }
  return specifier_to_array(mrb, specifiers);
}

static mrb_value
mrb_alc_device_get_default_device_specifier(mrb_state *mrb, mrb_value self)
{
  ALchar const * const specifiers = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
  if (NULL == specifiers) {
    return mrb_nil_value();
  }
  return specifier_to_array(mrb, specifiers);
}


static mrb_value
mrb_alc_capturedevice_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_alc_capturedevice_data_t *data =
    (mrb_alc_capturedevice_data_t*)DATA_PTR(self);
  mrb_value name;
  mrb_int freq, format, size;
  int const argc = mrb_get_args(mrb, "|oiii", &name, &freq, &format, &size);
  if ((0 != argc) && (4 != argc)) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "wrong number of arguments.");
  }
  ALCdevice *device = NULL;
  if (0 != argc) {
    device = alcCaptureOpenDevice(RSTRING_PTR(name), (ALCint)freq, (ALCenum)format, (ALCsizei)size);
    if (NULL == device) {
      mrb_raisef(mrb, class_ALCError, "cannot open capture device (%S).", name);
    }
  }
  if (NULL != data) {
    mrb_alc_capturedevice_free(mrb, data);
  }
  data = (mrb_alc_capturedevice_data_t*)mrb_malloc(mrb, sizeof(mrb_alc_capturedevice_data_t));
  if (NULL == data) {
    if (NULL != device) {
      alcCaptureCloseDevice(device);
    }
    mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
  }
  data->device = device;
  DATA_PTR(self) = data;
  DATA_TYPE(self) = &mrb_alc_capturedevice_data_type;
  return self;
}

static mrb_value
mrb_alc_capturedevice_open(mrb_state *mrb, mrb_value self)
{
  mrb_alc_capturedevice_data_t *data =
    (mrb_alc_capturedevice_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_capturedevice_data_type);
  if (NULL != data->device) {
    mrb_raise(mrb, class_ALCError, "capture device has already been opened.");
  }
  mrb_value name;
  mrb_int freq, format, size;
  mrb_get_args(mrb, "oiii", &name, &freq, &format, &size);
  ALCdevice *device = alcCaptureOpenDevice(RSTRING_PTR(name), (ALCint)freq, (ALCenum)format, (ALCsizei)size);
  if (NULL == device) {
    mrb_raisef(mrb, class_ALCError, "cannot open capture device (%S).", name);
  }
  data->device = device;
  data->frequency = freq;
  data->format = format;
  data->samples = size;
  return self;
}

static mrb_value
mrb_alc_capturedevice_close(mrb_state *mrb, mrb_value self)
{
  mrb_alc_capturedevice_data_t *data =
    (mrb_alc_capturedevice_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_capturedevice_data_type);
  if (NULL != data->device) {
    if (alcCaptureCloseDevice(data->device) == ALC_FALSE) {
      mrb_raise(mrb, class_ALCError, alcGetString(data->device, alcGetError(data->device)));
    }
    data->device = NULL;
  }
  return self;
}

static mrb_value
mrb_alc_capturedevice_start(mrb_state *mrb, mrb_value self)
{
  mrb_alc_capturedevice_data_t *data =
    (mrb_alc_capturedevice_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_capturedevice_data_type);
  if (NULL == data->device) {
    mrb_raise(mrb, class_ALCError, "no device is opened.");
  }
  alcCaptureStart(data->device);
  ALCenum const e = alcGetError(data->device);
  if (ALC_NO_ERROR != e) {
    mrb_raise(mrb, class_ALCError, alcGetString(data->device, e));
  }
  return self;
}

static mrb_value
mrb_alc_capturedevice_stop(mrb_state *mrb, mrb_value self)
{
  mrb_alc_capturedevice_data_t *data =
    (mrb_alc_capturedevice_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_capturedevice_data_type);
  if (NULL == data->device) {
    mrb_raise(mrb, class_ALCError, "no device is opened.");
  }
  alcCaptureStop(data->device);
  ALCenum const e = alcGetError(data->device);
  if (ALC_NO_ERROR != e) {
    mrb_raise(mrb, class_ALCError, alcGetString(data->device, e));
  }
  return self;
}

static mrb_value
mrb_alc_capturedevice_samples(mrb_state *mrb, mrb_value self)
{
  mrb_alc_capturedevice_data_t *data =
    (mrb_alc_capturedevice_data_t*)mrb_data_get_ptr(mrb, self, &mrb_alc_capturedevice_data_type);
  if (NULL == data->device) {
    mrb_raise(mrb, class_ALCError, "no device is opened.");
  }
  mrb_value buf;
  mrb_int sample;
  int const argc = mrb_get_args(mrb, "o|i", &buf, &sample);
  mrb_al_sample_buffer_data_t *buf_data =
    (mrb_al_sample_buffer_data_t*)mrb_data_get_ptr(mrb, buf, &mrb_al_sample_buffer_data_type);
  ALsizei coef = 1;
  switch (data->format) {
  case AL_FORMAT_MONO8:
    coef = 1;
    break;
  case AL_FORMAT_MONO16:
    coef = 2;
    break;
  case AL_FORMAT_STEREO8:
    coef = 2;
    break;
  case AL_FORMAT_STEREO16:
    coef = 4;
    break;
  default:
    mrb_raise(mrb, class_ALCError, "capture device is opened as unsupported format.");
    break;
  }
  ALsizei sample_count = buf_data->capacity / coef;
  if (1 < argc) {
    if (sample_count > (ALsizei)sample) {
      mrb_raise(mrb, E_ARGUMENT_ERROR, "too many sampling count is supplied.");
    }
    sample_count = (ALsizei)sample;
  }
  alcCaptureSamples(data->device, buf_data->buffer, sample_count);
  ALCenum const e = alcGetError(data->device);
  if (ALC_NO_ERROR != e) {
    mrb_raise(mrb, class_ALCError, alcGetString(data->device, e));
  }
  buf_data->size = sample_count * coef;
  return self;
}

static mrb_value
mrb_alc_capturedevice_get_device_specifier(mrb_state *mrb, mrb_value self)
{
  ALchar const * const specifiers = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
  if (NULL == specifiers) {
    return mrb_nil_value();
  }
  return specifier_to_array(mrb, specifiers);
}

static mrb_value
mrb_alc_capturedevice_get_default_device_specifier(mrb_state *mrb, mrb_value self)
{
  ALchar const * const specifiers = alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
  if (NULL == specifiers) {
    return mrb_nil_value();
  }
  return specifier_to_array(mrb, specifiers);
}

static mrb_value
specifier_to_array(mrb_state *mrb, ALchar const * const specifier)
{
  mrb_value array = mrb_ary_new(mrb);
  ALchar const *ptr = specifier;
  ALchar const *start = ptr;
  for (;;) {
    if ('\0' == *ptr++) {
      if ('\0' != *ptr) {
        mrb_ary_push(mrb, array, mrb_str_new_cstr(mrb, start));
      }
      start = ptr;
    }
    if ('\0' == *start) {
      break;
    }
  }
  return array;
}


void mruby_openal_alc_init(mrb_state *mrb)
{
  mod_ALC = mrb_define_module(mrb, "ALC");
  class_Context       = mrb_define_class_under(mrb, mod_ALC, "Context",       mrb->object_class);
  class_Device        = mrb_define_class_under(mrb, mod_ALC, "Device",        mrb->object_class);
  class_CaptureDevice = mrb_define_class_under(mrb, mod_ALC, "CaptureDevice", class_Device);
  class_ALCError      = mrb_define_class_under(mrb, mod_ALC, "ALCError",      mrb->eStandardError_class);

  MRB_SET_INSTANCE_TT(class_Context,       MRB_TT_DATA);
  MRB_SET_INSTANCE_TT(class_Device,        MRB_TT_DATA);
  MRB_SET_INSTANCE_TT(class_CaptureDevice, MRB_TT_DATA);

  mrb_define_method(mrb, class_Context, "initialize", mrb_alc_context_initialize,   ARGS_ANY());
  mrb_define_method(mrb, class_Context, "destroy",    mrb_alc_context_destroy,      ARGS_NONE());
  mrb_define_method(mrb, class_Context, "destroyed?", mrb_alc_context_is_destroyed, ARGS_NONE());
  mrb_define_method(mrb, class_Context, "device",     mrb_alc_context_get_device,   ARGS_NONE());
  mrb_define_method(mrb, class_Context, "process",    mrb_alc_context_process,      ARGS_NONE());
  mrb_define_method(mrb, class_Context, "suspend",    mrb_alc_context_suspend,      ARGS_NONE());
  mrb_define_class_method(mrb, class_Context, "current=", mrb_alc_context_set_current,  ARGS_REQ(1));
  mrb_define_class_method(mrb, class_Context, "current",  mrb_alc_context_get_current,  ARGS_NONE());

  mrb_define_method(mrb, class_Device, "initialize",         mrb_alc_device_initialize,           ARGS_OPT(1));
  mrb_define_method(mrb, class_Device, "error",              mrb_alc_device_get_error,            ARGS_NONE());
  mrb_define_method(mrb, class_Device, "string",             mrb_alc_device_get_string,           ARGS_REQ(1));
  mrb_define_method(mrb, class_Device, "open",               mrb_alc_device_open,                 ARGS_REQ(1));
  mrb_define_method(mrb, class_Device, "close",              mrb_alc_device_close,                ARGS_NONE());
  mrb_define_method(mrb, class_Device, "exntesion_present?", mrb_alc_device_is_extension_present, ARGS_REQ(1));
  mrb_define_method(mrb, class_Device, "enum_value",         mrb_alc_device_get_enum_value,       ARGS_REQ(1));
  mrb_define_class_method(mrb, class_Device, "device_specifier",         mrb_alc_device_get_device_specifier,         ARGS_NONE());
  mrb_define_class_method(mrb, class_Device, "default_device_specifier", mrb_alc_device_get_default_device_specifier, ARGS_NONE());

  mrb_define_method(mrb, class_CaptureDevice, "initialize", mrb_alc_capturedevice_initialize, ARGS_REQ(4));
  mrb_define_method(mrb, class_CaptureDevice, "open",       mrb_alc_capturedevice_open,       ARGS_REQ(4));
  mrb_define_method(mrb, class_CaptureDevice, "close",      mrb_alc_capturedevice_close,      ARGS_NONE());
  mrb_define_method(mrb, class_CaptureDevice, "start",      mrb_alc_capturedevice_start,      ARGS_NONE());
  mrb_define_method(mrb, class_CaptureDevice, "stop",       mrb_alc_capturedevice_stop,       ARGS_NONE());
  mrb_define_method(mrb, class_CaptureDevice, "samples",    mrb_alc_capturedevice_samples,    ARGS_REQ(2));
  mrb_define_class_method(mrb, class_CaptureDevice, "device_specifier",         mrb_alc_capturedevice_get_device_specifier,         ARGS_NONE());
  mrb_define_class_method(mrb, class_CaptureDevice, "default_device_specifier", mrb_alc_capturedevice_get_default_device_specifier, ARGS_NONE());

  mrb_define_const(mrb, mod_ALC, "FORMAT_MONO8",    mrb_fixnum_value(AL_FORMAT_MONO8));
  mrb_define_const(mrb, mod_ALC, "FORMAT_MONO16",   mrb_fixnum_value(AL_FORMAT_MONO16));
  mrb_define_const(mrb, mod_ALC, "FORMAT_STEREO8",  mrb_fixnum_value(AL_FORMAT_STEREO8));
  mrb_define_const(mrb, mod_ALC, "FORMAT_STEREO16", mrb_fixnum_value(AL_FORMAT_STEREO16));
}

void mruby_openal_alc_final(mrb_state *mrb)
{
}

