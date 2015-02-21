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
#include "./camera.h"

v8::Handle<v8::Value> GPCamera::getWidgetValue(GPContext *context,
                                       CameraWidget *widget) {
  NanScope();
  const char *label;
  CameraWidgetType  type;
  int ret;
  v8::Local<v8::Object> value = NanNew<v8::Object>();

  ret = gp_widget_get_type(widget, &type);
  if (ret != GP_OK) return NanUndefined();
  ret = gp_widget_get_label(widget, &label);
  if (ret != GP_OK) return NanUndefined();
  value->Set(NanNew("label"), NanNew(label));
  value->Set(NanNew("type"), NanUndefined());
  // printf("Label: %s\n", label);
  // "Label:" is not i18ned, the "label" variable is
  switch (type) {
    case GP_WIDGET_TEXT: { /* char * */
      char *txt;
      ret = gp_widget_get_value(widget, &txt);
      if (ret == GP_OK) {
        value->Set(NanNew("type"), NanNew("string"));
        value->Set(NanNew("value"), NanNew(txt));
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
        value->Set(NanNew("type"), NanNew("range"));
        value->Set(NanNew("value"), NanNew(f));
        value->Set(NanNew("max"), NanNew(max));
        value->Set(NanNew("min"), NanNew(min));
        value->Set(NanNew("step"), NanNew(step));
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
        value->Set(NanNew("type"), NanNew("toggle"));
        value->Set(NanNew("value"), NanNew(t));
      } else {
        gp_context_error(context,
                         "Failed to retrieve values of toggle widget %s.",
                         label);
      }
      break;
    }
    case GP_WIDGET_DATE:  { /* int */
      int t;
      ret = gp_widget_get_value(widget, &t);
      if (ret != GP_OK) {
        gp_context_error(context,
                         "Failed to retrieve values of date/time widget %s.",
                         label);
        break;
      }
      value->Set(NanNew("type"), NanNew("date"));
      value->Set(NanNew("value"), NanNew<v8::Date>(t * 1000.0));
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

      v8::Local<v8::Array> choices = NanNew<v8::Array>(cnt);
      for (i = 0; i < cnt; i++) {
        const char *choice = NULL;

        ret = gp_widget_get_choice(widget, i, &choice);
        if (ret != GP_OK) continue;
        choices->Set(NanNew(i), NanNew(choice));
      }

      value->Set(NanNew("type"), NanNew("choice"));
      value->Set(NanNew("value"), NanNew(current));
      value->Set(NanNew("choices"), choices);
      break;
    }
    /* ignore: */
    case GP_WIDGET_WINDOW: {
      value->Set(NanNew("type"), NanNew("window"));
      break;
    }
    case GP_WIDGET_SECTION: {
      value->Set(NanNew("type"), NanNew("section"));
      break;
    }
    case GP_WIDGET_BUTTON: {
      value->Set(NanNew("type"), NanNew("button"));
    }
  }
  NanReturnValue(value);
}

int GPCamera::setWidgetValue(set_config_request *req) {
  int ret;
  CameraWidget *child;
  CameraWidget *rootconfig;
  ret = gp_camera_get_config(req->camera, &rootconfig, req->context);
  if (ret < GP_OK) return ret;

  ret = gp_widget_get_child_by_name(rootconfig, req->key.c_str(), &child);
  if (ret < GP_OK) return ret;

  switch (req->valueType) {
    case set_config_request::String: {
      ret = gp_widget_set_value(child, req->strValue.c_str());
      break;
    }
    case set_config_request::Integer: {
      ret = gp_widget_set_value(child, &req->intValue);
      break;
    }
    case set_config_request::Float: {
      ret = gp_widget_set_value(child, &req->fltValue);
    }
  }
  if (ret < GP_OK) return ret;

  ret = gp_camera_set_config(req->camera, rootconfig, req->context);
  if (ret < GP_OK) return ret;
  gp_widget_free(rootconfig);
  return ret;
}

int GPCamera::getConfigWidget(get_config_request *req, std::string name,
                              CameraWidget **child, CameraWidget **rootconfig) {
  int ret;
  GPContext *context = req->context;
  Camera *camera = req->camera;

  gp_camera_get_config(camera, rootconfig, context);
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
    // req->socket_path.c_str();
    snprintf(serv_addr.sun_path, sizeof(serv_addr.sun_path),
              "/tmp/preview.sock");

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
  if (retval == GP_OK && req->target_path.empty()) {
    retval = gp_file_get_data_and_size(file, &data, &req->length);
    if (retval == GP_OK && req->length != 0) {
      /* `gp_file_free` will call `free` on `file->data` pointer, save data */
      req->data = new char[req->length];
      memmove(const_cast<char *>(req->data), data, req->length);
    }
    data = NULL;
  }

  if (retval == GP_OK) {
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
