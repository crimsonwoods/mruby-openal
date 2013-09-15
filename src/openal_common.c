#include "openal.h"
#include "mruby/class.h"

static struct RClass *class_SampleBuffer = NULL;

static void
mrb_al_sample_buffer_free(mrb_state *mrb, void *p)
{
  mrb_al_sample_buffer_data_t *data =
    (mrb_al_sample_buffer_data_t*)p;
  if (NULL != data) {
    mrb_free(mrb, data->buffer);
    mrb_free(mrb, data);
  }
}

struct mrb_data_type const mrb_al_sample_buffer_data_type = { "SampleBuffer", mrb_al_sample_buffer_free };

static mrb_value
mrb_al_samplebuffer_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_al_sample_buffer_data_t *data =
    (mrb_al_sample_buffer_data_t*)DATA_PTR(self);
  mrb_int capacity;
  mrb_get_args(mrb, "i", &capacity);

  if (NULL == data) {
    data = (mrb_al_sample_buffer_data_t*)mrb_malloc(mrb, sizeof(mrb_al_sample_buffer_data_t));
    if (NULL == data) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
    }
    data->buffer = NULL;
    data->capacity = capacity;
    data->size = 0;
  }

  if ((data->capacity != capacity) && (NULL != data->buffer)) {
    void *buffer = mrb_realloc(mrb, data->buffer, capacity);
    if (NULL == buffer) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "insufficient memory.");
    }
    data->buffer = buffer;
  }

  data->capacity = capacity;
  data->size = 0;

  DATA_PTR(self) = data;
  DATA_TYPE(self) = &mrb_al_sample_buffer_data_type;

  return self;
}

static mrb_value
mrb_al_samplebuffer_get_capacity(mrb_state *mrb, mrb_value self)
{
  mrb_al_sample_buffer_data_t *data =
    (mrb_al_sample_buffer_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_sample_buffer_data_type);
  return mrb_fixnum_value(data->capacity);
}

static mrb_value
mrb_al_samplebuffer_get_size(mrb_state *mrb, mrb_value self)
{
  mrb_al_sample_buffer_data_t *data =
    (mrb_al_sample_buffer_data_t*)mrb_data_get_ptr(mrb, self, &mrb_al_sample_buffer_data_type);
  return mrb_fixnum_value(data->size);
}

void
mruby_openal_common_init(mrb_state *mrb)
{
  class_SampleBuffer = mrb_define_class_under(mrb, mod_AL, "SampleBuffer", mrb->object_class);

  MRB_SET_INSTANCE_TT(class_SampleBuffer, MRB_TT_DATA);

  mrb_define_method(mrb, class_SampleBuffer, "initialize", mrb_al_samplebuffer_initialize,   ARGS_REQ(1));
  mrb_define_method(mrb, class_SampleBuffer, "capacity",   mrb_al_samplebuffer_get_capacity, ARGS_NONE());
  mrb_define_method(mrb, class_SampleBuffer, "size",       mrb_al_samplebuffer_get_size,     ARGS_NONE());
}

void
mruby_openal_common_final(mrb_state *mrb)
{
}

