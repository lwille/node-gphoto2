/* Copyright contributors of the node-gphoto2 project */

#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>
#include <stdexcept>

#include "./camera.h"

v8::Local<v8::Object>
GPCamera::convertSettingsToObject(bool minify, GPContext *context, const A<TreeNode>::Tree &node) {
  Nan::EscapableHandleScope scope;
  v8::Local<v8::Object> prt = Nan::New<v8::Object>();

  for (const auto &pair : node) {
    const TreeNode &child_node = pair.second;
    v8::Local<v8::String> key = Nan::New<v8::String>(pair.first).ToLocalChecked();

    CameraWidgetType type;
    gp_widget_get_type(child_node.value, &type);

    if (!minify && (type == GP_WIDGET_WINDOW || type == GP_WIDGET_SECTION)) {
      const char *label;
      gp_widget_get_label(child_node.value, &label);

      const char *type_str = GP_WIDGET_WINDOW ? "window" : "section";

      v8::Local<v8::Object> child = Nan::New<v8::Object>();

      Nan::Set(child, Nan::New<v8::String>("type").ToLocalChecked(), Nan::New<v8::String>(type_str).ToLocalChecked());
      Nan::Set(child, Nan::New<v8::String>("label").ToLocalChecked(), Nan::New<v8::String>(label).ToLocalChecked());

      v8::Local<v8::Object> grandchildren = convertSettingsToObject(minify, context, child_node.subtree);
      Nan::Set(child, Nan::New<v8::String>("children").ToLocalChecked(), grandchildren);

      Nan::Set(prt, key, child);
    } else if (child_node.subtree.size() == 0) {
      Nan::Set(prt, key, getWidgetValue(context, child_node.value));
    } else {
      Nan::Set(prt, key, convertSettingsToObject(minify, context, child_node.subtree));
    }
  }

  return scope.Escape(prt);
}

v8::Local<v8::Value> GPCamera::getWidgetValue(GPContext *context, CameraWidget *widget) {
  Nan::EscapableHandleScope scope;
  const char *label;
  CameraWidgetType  type;
  int ret;
  v8::Local<v8::Object> value = Nan::New<v8::Object>();

  ret = gp_widget_get_type(widget, &type);
  if (ret != GP_OK) return Nan::Undefined();
  ret = gp_widget_get_label(widget, &label);
  if (ret != GP_OK) return Nan::Undefined();

  Nan::Set(value, Nan::New("label").ToLocalChecked(), Nan::New(label).ToLocalChecked());
  Nan::Set(value, Nan::New("type").ToLocalChecked(), Nan::Undefined());

  switch (type) {
  case GP_WIDGET_TEXT: { /* char * */
    char *txt;
    ret = gp_widget_get_value(widget, &txt);
    if (ret == GP_OK) {
      Nan::Set(value, Nan::New("type").ToLocalChecked(), Nan::New("string").ToLocalChecked());
      Nan::Set(value, Nan::New("value").ToLocalChecked(), Nan::New(txt).ToLocalChecked());
    } else {
      gp_context_error(context,
                       "Failed to retrieve value of text widget %s.",
                       label);
    }
    break;
  }
  case GP_WIDGET_RANGE: { /* float */
    float f, max, min, step;

    ret = gp_widget_get_range(widget, &min, &max, &step);
    if (ret == GP_OK) {
      ret = gp_widget_get_value(widget, &f);
      Nan::Set(value, Nan::New("type").ToLocalChecked(), Nan::New("range").ToLocalChecked());
      Nan::Set(value, Nan::New("value").ToLocalChecked(), Nan::New(f));
      Nan::Set(value, Nan::New("max").ToLocalChecked(), Nan::New(max));
      Nan::Set(value, Nan::New("min").ToLocalChecked(), Nan::New(min));
      Nan::Set(value, Nan::New("step").ToLocalChecked(), Nan::New(step));
    } else {
      gp_context_error(context,
                       "Failed to retrieve values of range widget %s.",
                       label);
    }
    break;
  }
  case GP_WIDGET_TOGGLE: { /* int */
    int t;
    ret = gp_widget_get_value(widget, &t);
    if (ret == GP_OK) {
      Nan::Set(value, Nan::New("type").ToLocalChecked(), Nan::New("toggle").ToLocalChecked());
      Nan::Set(value, Nan::New("value").ToLocalChecked(), Nan::New(t));
    } else {
      gp_context_error(context,
                       "Failed to retrieve values of toggle widget %s.",
                       label);
    }
    break;
  }
  case GP_WIDGET_DATE: { /* int */
    int t;
    ret = gp_widget_get_value(widget, &t);
    if (ret != GP_OK) {
      gp_context_error(context,
                       "Failed to retrieve values of date/time widget %s.",
                       label);
      break;
    }
    Nan::Set(value, Nan::New("type").ToLocalChecked(), Nan::New("date").ToLocalChecked());
    Nan::Set(value, Nan::New("value").ToLocalChecked(), Nan::New<v8::Date>(t * 1000.0).ToLocalChecked());
    break;
  }
  case GP_WIDGET_MENU:
  case GP_WIDGET_RADIO: { /* char * */
    int cnt, i;
    const char *current = NULL;

    ret = gp_widget_get_value(widget, &current);
    cnt = gp_widget_count_choices(widget);
    if (cnt < GP_OK) {
      ret = cnt;
      break;
    }
    ret = GP_ERROR_BAD_PARAMETERS;

    v8::Local<v8::Array> choices = Nan::New<v8::Array>(cnt);
    for (i = 0; i < cnt; i++) {
      const char *choice = NULL;

      ret = gp_widget_get_choice(widget, i, &choice);
      if (ret != GP_OK) continue;
      Nan::Set(choices, Nan::New(i), Nan::New(choice).ToLocalChecked());
    }

    Nan::Set(value, Nan::New("type").ToLocalChecked(), Nan::New("choice").ToLocalChecked());
    Nan::Set(value, Nan::New("value").ToLocalChecked(), Nan::New(current).ToLocalChecked());
    Nan::Set(value, Nan::New("choices").ToLocalChecked(), choices);
    break;
  }
  /* ignore: */
  case GP_WIDGET_WINDOW: {
    Nan::Set(value, Nan::New("type").ToLocalChecked(), Nan::New("window").ToLocalChecked());
    break;
  }
  case GP_WIDGET_SECTION: {
    Nan::Set(value, Nan::New("type").ToLocalChecked(), Nan::New("section").ToLocalChecked());
    break;
  }
  case GP_WIDGET_BUTTON: {
    Nan::Set(value, Nan::New("type").ToLocalChecked(), Nan::New("button").ToLocalChecked());
  }
  }
  return scope.Escape(value);
}

int GPCamera::setWidgetValue(set_config_request *req) {
  int ret, ro = 0;
  CameraWidget *child;
  CameraWidget *rootconfig;
  CameraWidgetType type;

  ret = getConfigWidget(req->context, req->camera, req->key, &child, &rootconfig);
  if (ret != GP_OK)
    goto finally;

  ret = gp_widget_get_readonly(child, &ro);
  if (ro == 1)
    ret = GP_ERROR_NOT_SUPPORTED;
  if (ret != GP_OK)
    goto finally;

  ret = gp_widget_get_type(child, &type);
  if (ret != GP_OK)
    goto finally;

  switch (req->valueType) {
  case set_config_request::String:
    ret = setWidgetValue(child, &type, req->strValue);
    break;

  case set_config_request::Integer:
    ret = setWidgetValue(child, &type, req->intValue);
    break;

  case set_config_request::Float:
    ret = setWidgetValue(child, &type, req->fltValue);
    break;
  }

  if (ret != GP_OK)
    goto finally;
  ret = gp_widget_set_changed(child, 1);
  if (ret != GP_OK)
    goto finally;
  ret = gp_camera_set_config(req->camera, rootconfig, req->context);
  if (ret != GP_OK)
    goto finally;

finally:
  gp_widget_free(rootconfig);
  return ret;
}

int GPCamera::getConfigWidget(GPContext *context, Camera *camera, std::string name,
                              CameraWidget **child, CameraWidget **rootconfig) {
  int ret;

  ret = gp_camera_get_config(camera, rootconfig, context);
  if (ret != GP_OK) return ret;

  ret = gp_widget_get_child_by_name(*rootconfig, name.c_str(), child);
  // name not found --> path specified
  // recurse until the specified child is found
  if (ret != GP_OK) {
    char *part, *s, *newname;

    newname = strdup(name.c_str());
    if (!newname) return GP_ERROR_NO_MEMORY;

    *child = *rootconfig;
    part = newname;
    while (part[0] == '/') {
      part++;
    }

    while (1) {
      CameraWidget *tmp;
      s = strchr(part, '/');
      if (s) {
        *s = '\0';
      }
      ret = gp_widget_get_child_by_name(*child, part, &tmp);
      if (ret != GP_OK) {
        ret = gp_widget_get_child_by_label(*child, part, &tmp);
      }
      if (ret != GP_OK) {
        break;
      }
      *child = tmp;
      if (!s) { /* end of path */
        break;
      }
      part = s + 1;
      while (part[0] == '/') {
        part++;
      }
    }

    if (s) { /* if we have stuff left over, we failed */
      gp_context_error(context, "%s not found in configuration tree.",
                       newname);
      free(newname);
      gp_widget_free(*rootconfig);
      return GP_ERROR;
    }
    free(newname);
  }
  return GP_OK;
}

int GPCamera::enumConfig(get_config_request *req, CameraWidget *root,
                         A<TreeNode>::Tree *tree) {
  int ret, n, i;
  char *label, *name, *uselabel;

  gp_widget_get_label(root, (const char**) &label);
  ret = gp_widget_get_name(root, (const char**) &name);

  TreeNode node(root, req->context);
  if (std::string((const char*) name).length()) {
    uselabel = name;
  } else {
    uselabel = label;
  }

  n = gp_widget_count_children(root);
  for (i = 0; i < n; i++) {
    CameraWidget *child;
    ret = gp_widget_get_child(root, i, &child);
    if (ret != GP_OK) continue;
    enumConfig(req, child, &node.subtree);
  }
  (*tree)[uselabel] = node;
  return GP_OK;
}

int GPCamera::getCameraFile(take_picture_request *req, CameraFile **file) {
  int retval = GP_OK;
  int fd;

  if (!req->target_path.empty()) {
    char *tmpname = strdup(req->target_path.c_str());
    fd = mkstemp(tmpname);
    req->target_path = tmpname;
  } else if (!req->socket_path.empty()) {
    struct sockaddr_un serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    snprintf(serv_addr.sun_path, sizeof(serv_addr.sun_path),
             "%s", req->socket_path.c_str());

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
      perror("Creating socket");
    }
    if (connect(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror(serv_addr.sun_path);
    }
  } else {
    return gp_file_new(file);
  }

  if (fd == -1) {
    if (errno == EACCES) gp_context_error(req->context, "Permission denied");
    return errno;
  }
  if (fd >= 0) {
    retval = gp_file_new_from_fd(file, fd);
    if (retval != GP_OK) ::close(fd);
  }
  return retval;
}

void GPCamera::downloadPicture(take_picture_request *req) {
  CameraFile *file;
  const char *data;
  char *component;
  char *saveptr;
  int retval;
  std::ostringstream folder;
  std::string name;

  component = strtok_r(const_cast<char *>(req->path.c_str()), "/", &saveptr);
  while (component) {
    char *next = strtok_r(NULL, "/", &saveptr);
    if (next) {
      folder << "/" << component;
    } else {
      name = component;
    }
    component = next;
  }
  if (folder.str().length() == 0) folder << "/";

  retval = getCameraFile(req, &file);

  if (retval == GP_OK) {
    retval = gp_camera_file_get(req->camera, folder.str().c_str(), name.c_str(),
                                GP_FILE_TYPE_NORMAL, file, req->context);
  } else {
    req->ret = retval;
    return;
  }

  /* Fallback to downloading into buffer */
  if (retval == GP_OK &&
      req->target_path.empty() &&
      req->socket_path.empty()) {
    retval = gp_file_get_data_and_size(file, &data, &req->length);
    if (retval == GP_OK && req->length != 0) {
      /* `gp_file_free` will call `free` on `file->data` pointer, save data */
      req->data = new char[req->length];
      memmove(const_cast<char *>(req->data), data, req->length);
    }
    data = NULL;
  }

  if (retval == GP_OK && !req->keep) {
    retval = gp_camera_file_delete(req->camera, folder.str().c_str(),
                                   name.c_str(), req->context);
  }

  gp_file_free(file);
  req->ret = retval;
}

void GPCamera::capturePreview(take_picture_request *req) {
  int retval;
  CameraFile *file;

  retval = getCameraFile(req, &file);

  if (retval == GP_OK) {
    retval = gp_camera_capture_preview(req->camera, file, req->context);
  }
  if (!req->target_path.empty() || !req->socket_path.empty()) {
    gp_file_free(file);
  }
  req->ret = retval;
}

void GPCamera::takePicture(take_picture_request *req) {
  int retval;
  CameraFilePath camera_file_path;

  /* NOP: This gets overridden in the library to /capt0000.jpg */
  snprintf(camera_file_path.folder, sizeof(camera_file_path.folder), "/");
  snprintf(camera_file_path.name, sizeof(camera_file_path.name), "foo.jpg");
  retval = gp_camera_capture(req->camera, GP_CAPTURE_IMAGE, &camera_file_path,
                             req->context);

  std::ostringstream path;
  if (std::string(camera_file_path.folder).compare("/") != 0) {
    path << camera_file_path.folder;
  }
  path << "/";
  path << camera_file_path.name;
  req->path = path.str();
  req->ret = retval;

  if (retval == GP_OK && req->download) {
    downloadPicture(req);
  }
}

int GPCamera::setWidgetValue(CameraWidget *child, CameraWidgetType* type, int intval) {
  switch (*type) {
  case GP_WIDGET_TEXT: {
    std::stringstream ss;
    ss << intval;
    return setWidgetValue(child, type, ss.str());
  }

  case GP_WIDGET_RANGE:
    return setWidgetValue(child, type, static_cast<float>(intval));

  case GP_WIDGET_TOGGLE:
  case GP_WIDGET_DATE:
    return gp_widget_set_value(child, &intval);

  case GP_WIDGET_MENU:
  case GP_WIDGET_RADIO: {
    const char *choice;

    if (gp_widget_get_choice(child, intval, &choice) == GP_OK) {
      return setWidgetValue(child, type, std::string(choice));
    } else {
      std::stringstream ss;
      ss << intval;
      return setWidgetValue(child, type, ss.str());
    }
  }

    /* ignore: */
  case GP_WIDGET_WINDOW:
  case GP_WIDGET_SECTION:
  case GP_WIDGET_BUTTON:
  default:
    return GP_ERROR_BAD_PARAMETERS;
  }
}

int GPCamera::setWidgetValue(CameraWidget *child, CameraWidgetType* type, std::string strval) {
  switch (*type) {
  case GP_WIDGET_TEXT:
    return gp_widget_set_value(child, strval.c_str());

  case GP_WIDGET_RANGE:
    return setWidgetValue(child, type, static_cast<float>(atof(strval.c_str())));

  case GP_WIDGET_TOGGLE:
    std::transform(strval.begin(), strval.end(), strval.begin(), ::tolower);

    if (strval == "on" || strval == "yes" || strval == "true" || strval == "1") {
      return setWidgetValue(child, type, 1);
    } else if (strval == "off" || strval == "no" || strval == "false" || strval == "0") {
      return setWidgetValue(child, type, 0);
    } else {
      return GP_ERROR_BAD_PARAMETERS;
    }

  case GP_WIDGET_DATE:
    return setWidgetValue(child, type, atoi(strval.c_str()));

  case GP_WIDGET_MENU:
  case GP_WIDGET_RADIO: {
    int count, i;

    count = gp_widget_count_choices(child);

    if (count < GP_OK)
      return count;

    // check the requested option matches one of the choices
    const char *choice;
    for (i = 0; i < count; i++) {
      if (gp_widget_get_choice (child, i, &choice) != GP_OK)
        continue;

      if (strval == std::string(choice))
        return gp_widget_set_value (child, choice);
    }

    if (i != count)
      return GP_ERROR_BAD_PARAMETERS;

    // string may contain an which can be used as an index into the options
    const char *start_ch = strval.c_str();
    char *end_ch;
    int index = strtol(start_ch, &end_ch, 10);
    if (index >= 0 && index < count && end_ch > start_ch && end_ch == start_ch + strval.length()) {
      if (gp_widget_get_choice(child, index, &choice) == GP_OK && gp_widget_set_value (child, choice) == GP_OK)
        return GP_OK;
    }

    // comment from gphoto2/actions.c
    // Let's just try setting the value directly, in case we have flexible setters, like PTP shutterspeed. */
    int ret = gp_widget_set_value(child, strval.c_str());
    printf("Return value: %d\n", ret);
    return ret;
  }

  /* ignore: */
  case GP_WIDGET_WINDOW:
  case GP_WIDGET_SECTION:
  case GP_WIDGET_BUTTON:
  default:
    return GP_ERROR_BAD_PARAMETERS;
  }
}

int GPCamera::setWidgetValue(CameraWidget *child, CameraWidgetType* type, float fltval) {
  switch (*type) {
  case GP_WIDGET_TEXT: {
    std::stringstream ss;
    ss << fltval;
    return setWidgetValue(child, type, ss.str());
  }

  case GP_WIDGET_RANGE: {
    float lo, hi, inc;

    int ret = gp_widget_get_range(child, &lo, &hi, &inc);

    if (ret != GP_OK)
      return ret;

    if (fltval < lo || fltval > hi)
      return GP_ERROR_BAD_PARAMETERS;

    return gp_widget_set_value(child, &fltval);
  }

  case GP_WIDGET_TOGGLE:
  case GP_WIDGET_DATE:
    return setWidgetValue(child, type, static_cast<int>(fltval));

  case GP_WIDGET_MENU:
  case GP_WIDGET_RADIO: {
    std::stringstream ss;
    ss << fltval;
    return setWidgetValue(child, type, ss.str());
  }

    /* ignore: */
  case GP_WIDGET_WINDOW:
  case GP_WIDGET_SECTION:
  case GP_WIDGET_BUTTON:
  default:
    return GP_ERROR_BAD_PARAMETERS;
  }
}
