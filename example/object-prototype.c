/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>

#include "jerryscript.h"
#include "jerryscript-port.h"
#include "jerryscript-ext/handler.h"

struct View {
  jerry_value_t proto_object;
  int x;
  int y;
  char id[16];
  struct View* children[8];
};

struct ImageView {
  struct View view;
  char src[16];
};

struct ButtonView {
  struct View view;
  char text[16];
};

jerry_value_t g_view_prototype_object;
jerry_value_t g_image_prototype_object;
jerry_value_t g_button_prototype_object;
struct View* g_root_view = NULL;

void init_view_tree(void);
void button_initiator(struct View* button);
void image_initiator(struct View* image);
void view_initiator(struct View* view);
void init_view_prototype(void);
void init_image_prototype(void);
struct View* get_element_by_id(const char* id);

static jerry_value_t
view_x_getter_handler(const jerry_value_t func_value, /**< function object */
                 const jerry_value_t this_value, /**< this arg */
                 const jerry_value_t * args_p, /**< function arguments */
                 const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void)this_value;
  (void)(func_value);
  (void)args_p;
  (void)args_cnt;
  printf("%s\n","view_getter");
  struct View* view = NULL;
  jerry_get_object_native_pointer(this_value, (void**)&view, NULL);
  if (view) {
    return jerry_create_number(view->x);
  }

  return jerry_create_number_nan();
} 
static jerry_value_t
view_x_setter_handler(const jerry_value_t func_value, /**< function object */
                 const jerry_value_t this_value, /**< this arg */
                 const jerry_value_t * args_p, /**< function arguments */
                 const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void)this_value;
  (void)(func_value);
  //(void)args_p;
  //(void)args_cnt;
  struct View* view = NULL;
  jerry_get_object_native_pointer(this_value, (void**)&view, NULL);

  if (view) {
    if (jerry_value_is_number(args_p[0])) {
      double d = jerry_get_number_value(args_p[0]);
      view->x = (int)d;
#if 0
      jerry_size_t req_sz = jerry_get_string_size (args_p[0]);
      jerry_char_t str_buf_p[req_sz];

      jerry_string_to_char_buffer (args_p[0], str_buf_p, req_sz);
      printf("%s,value:%s,count=%d\n","view_setter",str_buf_p, args_cnt);
#endif
    }
    else {
      printf("view_setter,count=%d\n", args_cnt);
    }
  }
  return jerry_create_undefined();
}

struct View* get_element_by_id(const char* id) {
  int i=0;
  for(i=0;i<8;i++) {
    if (g_root_view->children[i] && strcmp(g_root_view->children[i]->id,id)==0) {
      return g_root_view->children[i]; 
    }
  }
  return NULL;
}

static jerry_value_t
get_element_by_id_handler(const jerry_value_t func_value, /**< function object */
                 const jerry_value_t this_value, /**< this arg */
                 const jerry_value_t * args_p, /**< function arguments */
                 const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void)(func_value);
  (void)this_value;
  (void)args_p;
  (void)args_cnt;

  if (!jerry_value_is_string(args_p[0])) {
    return jerry_create_undefined();
  }
  jerry_size_t req_sz = jerry_get_string_size (args_p[0]);
  jerry_char_t str_buf_p[req_sz+1];

  jerry_string_to_char_buffer (args_p[0], str_buf_p, req_sz);
  str_buf_p[req_sz] = 0;
  printf("getElementById,id:%s\n",(const char*)str_buf_p);

  struct View *view = get_element_by_id((const char*)str_buf_p);
  if (!view) {
    return jerry_create_undefined();
  }
  jerry_value_t retobject = jerry_create_object();
  jerry_set_object_native_pointer(retobject, view, NULL);
  jerry_release_value(jerry_set_prototype(retobject,
        view->proto_object));
  return retobject;
}

static jerry_value_t
src_getter_handler(const jerry_value_t func_value, /**< function object */
                 const jerry_value_t this_value, /**< this arg */
                 const jerry_value_t *args_p, /**< function arguments */
                 const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void)func_value;
  (void)args_p;
  (void)args_cnt;
  struct ImageView *image = NULL;
  jerry_get_object_native_pointer(this_value, (void**)&image, NULL);
  if (image) {
    return jerry_create_string((const jerry_char_t *)image->src);
  }
  return jerry_create_undefined();
} 

static jerry_value_t
src_setter_handler(const jerry_value_t func_value, /**< function object */
                 const jerry_value_t this_value, /**< this arg */
                 const jerry_value_t *args_p, /**< function arguments */
                 const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void)func_value;
  (void)args_cnt;
  struct ImageView *image = NULL;
  jerry_get_object_native_pointer(this_value, (void**)&image, NULL);
  if (image) {
    if (jerry_value_is_string(args_p[0])) {
      jerry_size_t req_sz = jerry_get_string_size (args_p[0]);
      jerry_char_t str_buf_p[req_sz+1];
      jerry_string_to_char_buffer (args_p[0], str_buf_p, req_sz);
      str_buf_p[req_sz] = 0;
      strcpy(image->src,(const char*)str_buf_p);
    }
  }
  return jerry_create_undefined();
}

void init_view_prototype(void) {
  //create view prototype
  g_view_prototype_object = jerry_create_object ();
  jerry_property_descriptor_t x_prop_desc;
  jerry_init_property_descriptor_fields (&x_prop_desc);
  x_prop_desc.is_get_defined = true;
  x_prop_desc.is_set_defined = true;
  x_prop_desc.getter = jerry_create_external_function (view_x_getter_handler);
  x_prop_desc.setter = jerry_create_external_function (view_x_setter_handler);

  jerry_value_t x_prop_name = jerry_create_string ((const jerry_char_t *) "x");
  jerry_release_value(jerry_define_own_property(g_view_prototype_object , x_prop_name, &x_prop_desc));
}

void init_image_prototype(void) {
  g_image_prototype_object = jerry_create_object ();
  jerry_property_descriptor_t src_prop_desc;
  jerry_init_property_descriptor_fields (&src_prop_desc);
  src_prop_desc.is_get_defined = true;
  src_prop_desc.is_set_defined = true;
  src_prop_desc.getter = jerry_create_external_function (src_getter_handler);
  src_prop_desc.setter = jerry_create_external_function (src_setter_handler);;

  jerry_value_t src_prop_name = jerry_create_string ((const jerry_char_t *) "src");
  jerry_release_value(jerry_define_own_property(g_image_prototype_object , src_prop_name, &src_prop_desc));

#if 0
  jerry_release_value(jerry_set_property (g_image_prototype_object, 
        jerry_create_string ((const jerry_char_t *) "prototype"), 
        g_view_prototype_object));
#else
  jerry_release_value(jerry_set_prototype(g_image_prototype_object,
        g_view_prototype_object));
#endif
}

void view_initiator(struct View* view) {
  memset(view, 0, sizeof(struct View));
  view->proto_object = g_view_prototype_object;
  sprintf(view->id, "%d", 0);
  view->x = 100;
  view->y = 100;
}

void image_initiator(struct View* image) {
  memset(image, 0, sizeof(struct ImageView));
  image->proto_object = g_image_prototype_object;
  sprintf(image->id, "%d", 1);
  sprintf(((struct ImageView*)image)->src, "%s", "hwsrc");
  image->x = 20;
  image->y = 20;
}

void button_initiator(struct View* button) {
  memset(button, 0, sizeof(struct ButtonView));
  button->proto_object = g_button_prototype_object;
  sprintf(button->id, "%d", 2);
  sprintf(((struct ButtonView*)button)->text, "%s", "hwbtn");
  button->x = 10;
  button->y = 10;
}

void init_view_tree(void) {
  g_root_view = (struct View*)malloc(sizeof(struct View));
  view_initiator(g_root_view);

  struct ImageView *image = (struct ImageView*)malloc(sizeof(struct ImageView));
  image_initiator((struct View*)image);

  struct ButtonView *button = (struct ButtonView*)malloc(sizeof(struct ButtonView));
  button_initiator((struct View*)button);

  g_root_view->children[0] = (struct View*)image;
  g_root_view->children[1] = (struct View*)button;
}

static void *
context_alloc_fn (size_t size, void *cb_data)
{
  (void) cb_data;
  return malloc (size);
}

int
main (int argc,
      char **argv)
{
  (void)argc;
  (void)argv;
  jerry_create_context (1024 * 1024,
                        context_alloc_fn,
                        NULL);
  jerry_init (JERRY_INIT_EMPTY);
  jerryx_handler_register_global ((const jerry_char_t *) "print",
                                  jerryx_handler_print);
  init_view_prototype();
  init_image_prototype();
  init_view_tree();

  jerry_value_t document_object_value = jerry_create_object ();
  jerry_release_value(jerry_set_property (document_object_value, 
        jerry_create_string ((const jerry_char_t *) "getElementById"), 
        jerry_create_external_function(get_element_by_id_handler)));

  jerry_value_t glob_obj_val = jerry_get_global_object ();
  jerry_release_value (jerry_set_property (glob_obj_val , 
        jerry_create_string ((const jerry_char_t *) "document"), 
        document_object_value));

  const jerry_char_t script[] = " \
    var node = document.getElementById(\"1\"); \
    node.x = 25; \
    print(node.x); \
    node.src = \"hwsrc1\"; \
    print(node.src); \
  ";

  /* Evaluate script */
  jerry_eval (script, sizeof (script) - 1, JERRY_PARSE_NO_OPTS);
  //jerry_cleanup ();
}

