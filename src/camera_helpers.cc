#include "camera.h"
#include <ctime>
#include <sstream>


namespace cv = cvv8;
Handle<Value> GPCamera::getWidgetValue(GPContext *context, CameraWidget *widget) {
  HandleScope scope;
	const char *label;
	CameraWidgetType	type;
  int ret;
  Local<Object> value = Object::New();
  
	ret = gp_widget_get_type (widget, &type);
	if (ret != GP_OK)
		return Undefined();
	ret = gp_widget_get_label (widget, &label);
	if (ret != GP_OK)
		return Undefined();
  value->Set(cv::CastToJS("label"), cv::CastToJS(label));
  value->Set(cv::CastToJS("type"), Undefined());
//	printf ("Label: %s\n", label); /* "Label:" is not i18ned, the "label" variable is */
	switch (type) {
	case GP_WIDGET_TEXT: {		/* char *		*/
		char *txt;
		ret = gp_widget_get_value (widget, &txt);
		if (ret == GP_OK) {
      value->Set(cv::CastToJS("type"), cv::CastToJS("string"));
      value->Set(cv::CastToJS("value"), cv::CastToJS(txt));
		} else {
			gp_context_error (context, "Failed to retrieve value of text widget %s.", label);
		}
		break;
	}
	case GP_WIDGET_RANGE: {	/* float		*/
		float	f, max,min,step;

		ret = gp_widget_get_range (widget, &min, &max, &step);
		if (ret == GP_OK)
			ret = gp_widget_get_value (widget, &f);
		if (ret == GP_OK) {
		  value->Set(cv::CastToJS("type"), cv::CastToJS("range"));
      value->Set(cv::CastToJS("value"), cv::CastToJS(f));
      value->Set(cv::CastToJS("max"), cv::CastToJS(max));
      value->Set(cv::CastToJS("min"), cv::CastToJS(min));
      value->Set(cv::CastToJS("step"), cv::CastToJS(step));
      break;
		} else {
			gp_context_error (context, "Failed to retrieve values of range widget %s.", label);
		}
		break;
	}
	case GP_WIDGET_TOGGLE: {	/* int		*/
		int	t;    
		ret = gp_widget_get_value (widget, &t);
		if (ret == GP_OK) {		  
		  value->Set(cv::CastToJS("type"), cv::CastToJS("toggle"));
      value->Set(cv::CastToJS("value"), cv::CastToJS(t));
      break;
		} else {
			gp_context_error (context, "Failed to retrieve values of toggle widget %s.", label);
		}
		break;
	}
	case GP_WIDGET_DATE:  {		/* int			*/
		int	t;
		time_t	xtime;
		struct tm *xtm;
		char	timebuf[200];

		ret = gp_widget_get_value (widget, &t);
		if (ret != GP_OK) {
			gp_context_error (context, "Failed to retrieve values of date/time widget %s.", label);
			break;
		}
	  value->Set(cv::CastToJS("type"), cv::CastToJS("date"));
    value->Set(cv::CastToJS("value"), Date::New(t*1000.0));
    break;
	}
	case GP_WIDGET_MENU:
	case GP_WIDGET_RADIO: { /* char *		*/
    int cnt, i;
    const char *current = NULL;
    
    
    ret = gp_widget_get_value (widget, &current);
    
    
		cnt = gp_widget_count_choices (widget);
		if (cnt < GP_OK) {
			ret = cnt;
			break;
		}
		ret = GP_ERROR_BAD_PARAMETERS;
		
    Local<Array> choices = Array::New(cnt);
		for ( i=0; i<cnt; i++) {
			const char *choice = NULL;
      
			ret = gp_widget_get_choice (widget, i, &choice);
			if (ret != GP_OK)
				continue;				
      choices->Set(cv::CastToJS(i), cv::CastToJS(choice));    	  
		}
   		
	  value->Set(cv::CastToJS("type"), cv::CastToJS("choice"));
    value->Set(cv::CastToJS("value"), cv::CastToJS(current));
    value->Set(cv::CastToJS("choices"), choices);
		break;
	}

	/* ignore: */
	case GP_WIDGET_WINDOW:
    value->Set(cv::CastToJS("type"), cv::CastToJS("window"));
    break;
	case GP_WIDGET_SECTION:
    value->Set(cv::CastToJS("type"), cv::CastToJS("section"));
  break;
	case GP_WIDGET_BUTTON:
    value->Set(cv::CastToJS("type"), cv::CastToJS("button"));    
		break;
	} 
  return scope.Close(value);
}

int GPCamera::getConfigWidget(get_config_request *req, std::string name, CameraWidget **child, CameraWidget **rootconfig){
  int ret;
  GPContext *context = req->context;
  Camera    *camera  = req->camera;
  
  ret = gp_camera_get_config(camera, rootconfig, context);
  ret = gp_widget_get_child_by_name(*rootconfig, name.c_str(), child);
  
  // name not found --> path specified
  // recurse until the specified child is found
  if (ret != GP_OK) {
		char		*part, *s, *newname;

		newname = strdup(name.c_str());
		if (!newname)
			return GP_ERROR_NO_MEMORY;
		*child = *rootconfig;
		part = newname;
		while (part[0] == '/')
			part++;
			while (1) {
  			CameraWidget *tmp;

  			s = strchr (part,'/');
  			if (s)
  				*s='\0';
  			ret = gp_widget_get_child_by_name (*child, part, &tmp);
  			if (ret != GP_OK)
  				ret = gp_widget_get_child_by_label (*child, part, &tmp);
  			if (ret != GP_OK)
  				break;
  			*child = tmp;
  			if (!s) /* end of path */
  				break;
  			part = s+1;
  			while (part[0] == '/')
  				part++;
  		}
  		if (s) { /* if we have stuff left over, we failed */
  			gp_context_error (context, "%s not found in configuration tree.", newname);
  			free (newname);
  			gp_widget_free (*rootconfig);
  			return GP_ERROR;
  		}
  		free (newname);
  	}
  	return GP_OK;  
}

int GPCamera::enumConfig(get_config_request* req, CameraWidget *root, A<TreeNode>::Tree &tree){
  int ret,n,i;
  char* label, *name, *uselabel;
  
  gp_widget_get_label (root,(const char**)&label);
  ret = gp_widget_get_name (root, (const char**)&name);
  
	TreeNode node;	
  node.value = root;
  node.context  = req->context;
	if (std::string((const char*)name).length())
		uselabel = name;
	else
		uselabel = label;

	n = gp_widget_count_children(root);
	for (i=0; i<n; i++) {
		CameraWidget *child;
		ret = gp_widget_get_child(root, i, &child);
		if (ret != GP_OK)
			continue;			
		enumConfig(req, child, node.subtree);
	}
  tree[uselabel] = node;
  return GP_OK;  
}
