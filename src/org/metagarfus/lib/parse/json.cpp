#include <stdlib.h>
#include "json.h"
#include "geral.h"

#define JSON_EQUIVALENT_TYPES(X,Y) ((X) == (Y) || (JSON_IS_VALUE(X) && JSON_IS_VALUE(Y)))
struct substring {
  const char *str;
  int start, end;
};

static substring make_substring(substring str, int start, int end)
{
  str.end = end + str.start;
  str.start += start;
  return str;
}

static string make_string(substring str, int start, int end)
{
  string ret;
  str.end = end + str.start;
  str.start += start;
  ret.resize(str.end - str.start);
  safestrcpy(ret.c_str(), str.str + str.start, str.end - str.start + 1);
  return ret;
}

static char char_at(substring str, int i)
{
  return str.str[i + str.start];
}

static int len(substring str)
{
  return str.end - str.start;
}

class json_parsed {
public:
  JSON result;
  string value;
  char next;
  substring remaining;
  int error;
  json_parsed() {
    error = 0;
  }
};

struct json_data {
  int type;
  int ref_count;
  void *content;
};

json_data empty_object = {JSON_OBJECT, -1, NULL};
json_data empty_array = {JSON_ARRAY, -1, NULL};

void JSON::free_data()
{
  JSON *json;
  JSONField *field;
  list *ldata;
  if (data == NULL || data->ref_count < 0) {
    data = NULL;
    return;
  }
  data->ref_count--;
  if (data->ref_count) {
    data = NULL;
    return;
  }
  if (data->content) {
    if (data->type == JSON_ARRAY) {
      ldata = (list *) (data->content);
      for (int i = 0; i < ldata->num; i++) {
        json = (JSON *)(ldata->get(i));
        delete json;
      }
      delete ldata;
    } else if (data->type == JSON_OBJECT) {
      ldata = (list *) (data->content);
      for (int i = 0; i < ldata->num; i++) {
        field = (JSONField *)(ldata->get(i));
        delete field;
      }
      delete ldata;
    } else {
      delete ((string *)data->content);
    }
  }
  wrst_free(data);
  data = NULL;
}

void JSON::set_data(json_data *data)
{
  this->data = data;
  if (this->data != NULL && this->data->ref_count >= 0)
    this->data->ref_count++;
}

static void copy_to(int type, list &src, list &dest)
{
  JSONField *srcField, *destField;
  JSON *srcElem, *dstElem;
  for (int i = 0; i < src.num; i++) {
    switch(type) {
      case JSON_ARRAY:
        srcElem = (JSON *) src.get(i);
        dstElem = new JSON();
        *dstElem = *srcElem;
        dest.add(dstElem);
        break;
      case JSON_OBJECT:
        srcField = (JSONField *) src.get(i);
        destField = new JSONField();
        destField->name = srcField->name;
        destField->value = srcField->value;
        dest.add(destField);
        break;
    }
  }
}

void JSON::ensure_data_uniqueness()
{
  json_data *ndata;
  if (this->data != NULL && this->data->ref_count == 1)
    return;
  ndata = (json_data*) wrst_malloc(sizeof(json_data));
  memset(ndata, 0, (sizeof(*ndata)));
  if (data != NULL) {
    ndata->type = data->type;
    if (data->content != NULL) {
      switch(data->type) {
        case JSON_ARRAY:
        case JSON_OBJECT:
          ndata->content = new list();
          copy_to(data->type, *(list *)(data->content), *(list *)(ndata->content));
          break;
        default:
          ndata->content = new string();
          *((string*)ndata->content) = *((string *) data->content);
      }
    }
  }
  free_data();
  set_data(ndata);
}

JSON::JSON()
{
  this->data = NULL;
}

JSON::JSON(const JSON &json)
{
  set_data(json.data);
}

JSON::JSON(int value, int is_boolean)
{
  this->data = NULL;
  if (is_boolean)
    set_bool_value(value);
  else
    set_value(value);
}

JSON::JSON(int value)
{
  this->data = NULL;
  set_value(value);
}

JSON::JSON(double value)
{
  this->data = NULL;
  set_value(value);
}

JSON::JSON(const char *value)
{
  this->data = NULL;
  set_value(value);
}

JSON::JSON(JSON *values, int count)
{
  this->data = &empty_array;
  if (values == NULL)
    return;
  for (int i = 0; i < count; i++)
    add(values[i]);
}

JSON::JSON(JSONField *values, int count)
{
  this->data = &empty_object;
  if (values == NULL)
    return;
  for (int i = 0; i < count; i++)
    add(values[i].name, values[i].value);
}

JSON::~JSON()
{
  free_data();
}

int JSON::set_type(int type)
{
  if (type == JSON_NULL) {
    clear();
    return 1;
  }
  if (data != NULL && data->content != NULL && data->type != JSON_NULL && !JSON_EQUIVALENT_TYPES(data->type, type))
    return 0;
  ensure_data_uniqueness();
  this->data->type = type;
  return 1;
}

int JSON::get_type()
{
  return data == NULL ? JSON_NULL : data->type;
}

void JSON::set_bool_value(int value)
{
  internal_set_value(value ? "true" : "false", JSON_BOOLEAN);
}

void JSON::set_value(int value)
{
  char v[32];
  ssprintf(v, "%d", value);
  internal_set_value(v, JSON_NUMBER);
}

void JSON::set_value(double value)
{
  char v[32];
  ssprintf(v, "%.4lf", value);
  internal_set_value(v, JSON_NUMBER);
}

void JSON::set_value(const char *value)
{
  if (value == NULL)
    value = "";
  internal_set_value(value, JSON_STRING);
}

void JSON::internal_set_value(const char *value, int type)
{
  if (value == NULL)
    type = JSON_NULL;
  if (!set_type(type))
    return;
  if (data == NULL)
    return;
  ensure_data_uniqueness();
  if (data->content == NULL)
    data->content = new string();
  *(string *)(data->content) = value;
}

void JSON::clear()
{
 free_data();
}

void JSON::internal_add(const char *name, JSON json)
{
  list *children;
  JSONField *field;
  JSON *elem;
  ensure_data_uniqueness();
  if (data->content == NULL)
    data->content = new list();
  children = (list *) data->content;
  switch (data->type) {
    case JSON_ARRAY:
      elem = new JSON();
      *elem = json;
      children->add(elem);
      break;
    case JSON_OBJECT:
      field = internal_get(name);
      if (field != NULL) {
        field->value = json;
      } else {
        field = new JSONField();
        field->name = name;
        field->value = json;
        children->add(field);
      }
      break;
  }
}

void JSON::add(const char *name, JSON json)
{
  if (!set_type(JSON_OBJECT))
    return;
  internal_add(name, json);
}

void JSON::add(JSON json)
{
  if (!set_type(JSON_ARRAY))
    return;
  internal_add("", json);
}

int JSON::is_null_value()
{
  return data == NULL || data->type == JSON_NULL;
}

int JSON::is_integer_value()
{
  if (data == NULL || data->content == NULL || data->type != JSON_NUMBER)
    return false;
  return ((string *)data->content)->pos(".") < 0;
}

const char *JSON::get_str_value()
{
  if (data == NULL || data->content == NULL || !JSON_IS_VALUE(data->type))
    return "";
  return ((string *)data->content)->ro_c_str();
}

int JSON::get_boolean_value()
{
  if (data == NULL || data->content == NULL || !JSON_IS_VALUE(data->type))
    return 0;
  return ((string *)data->content)->compare("true") == 0;
}

int JSON::get_integer_value()
{
  if (data == NULL || data->content == NULL || !JSON_IS_VALUE(data->type))
    return 0;
  return ((string *)data->content)->to_int();
}

double JSON::get_double_value()
{
  if (data == NULL || data->content == NULL || !JSON_IS_VALUE(data->type))
    return 0;
  return ((string *)data->content)->to_double();
}

JSONField* JSON::internal_get(const char *name)
{
  JSONField *field;
  list *ldata;
  if (data == NULL || data->content == NULL || data->type != JSON_OBJECT)
    return NULL;
  ldata = (list*) data->content;
  for (int i = 0; i < ldata->num; i++) {
    field = (JSONField *)(ldata->get(i));
    if (field->name == name)
      return field;
  }
  return NULL;
}

int JSON::get(const char *name, JSON &ret)
{
  JSONField *field;
  ret.clear();
  field = internal_get(name);
  if (field == NULL)
    return false;
  ret = field->value;
  return true;
}

int JSON::get(int i, JSON &ret)
{
  list *ldata;
  ret.clear();
  if (data == NULL  || data->content == NULL || data->type != JSON_ARRAY)
    return false;
  ldata = (list*) data->content;
  if (ldata->num <= i)
    return false;
  ret = *(JSON *)(ldata->get(i));
  return true;
}

int JSON::get_field(int i, JSONField &ret)
{
  list *ldata;
  ret.name = "";
  ret.value.clear();
  if (data == NULL  || data->content == NULL || data->type != JSON_OBJECT)
    return false;
  ldata = (list*) data->content;
  if (ldata->num <= i)
    return false;
  ret = *(JSONField *)(ldata->get(i));
  return true;
}

int JSON::get_field_integer(const char *name, int def)
{
  JSON ret;
  if (!get(name, ret))
    return def;
  return ret.get_integer_value();
}

double JSON::get_field_double(const char *name, double def)
{
  JSON ret;
  if (!get(name, ret))
    return def;
  return ret.get_double_value();
}

const char *JSON::get_field_str(const char *name, const char *def)
{
  JSON ret;
  if (!get(name, ret))
    return def;
  return ret.get_str_value();
}

int JSON::get_field_boolean(const char *name, int def)
{
  JSON ret;
  if (!get(name, ret))
    return def;
  return ret.get_boolean_value();
}

int JSON::is_field_null(const char *name, int def)
{
  JSON ret;
  if (!get(name, ret))
    return def;
  return ret.is_null_value();
}

int JSON::count()
{
  list *ldata;
  if (data == NULL || data->content == NULL || (data->type != JSON_ARRAY && data->type != JSON_OBJECT))
    return 0;
  ldata = (list*) data->content;
  return ldata->num;
}

JSON &JSON::operator=(const JSON &bro)
{
  if (bro.data == data)
    return *this;
  free_data();
  set_data(bro.data);
  return *this;
}

string JSON::to_string()
{
  string ret, tmp;
  JSON *json;
  JSONField *field;
  list *ldata;
  int type;
  type = get_type();
  switch (type) {
    case JSON_NUMBER:
    case JSON_BOOLEAN:
      ret = *((string *) data->content);
      break;
    case JSON_NULL:
      ret = "null";
      break;
    case JSON_STRING:
      ret = ((string *) data->content)->replace("\\", "\\\\").quote('"');
      break;
    case JSON_ARRAY:
      ret = "[";
      ldata = (list *) data->content;
      for (int i = 0; ldata != NULL && i < ldata->num; i++) {
        if (i != 0)
          ret += ",";
        json = (JSON *)(ldata->get(i));
        ret += json->to_string();
      }
      ret += "]";
      break;
    case JSON_OBJECT:
      ret = "{";
      ldata = (list *) data->content;
      for (int i = 0; ldata != NULL && i < ldata->num; i++) {
        if (i != 0)
          ret += ",";
        field = (JSONField *)(ldata->get(i));
        if (field == NULL)
          continue;
        tmp = field->value.to_string();
        ret.push_printf("\"%s\":%s", field->name.replace("\\", "\\\\").replace("\"", "\\\"").ro_c_str(), tmp.ro_c_str());
      }
      ret += "}";
      break;
  }
  return ret;
}

static int is_white_space(char tchar)
{
  return tchar == ' ' || tchar == '\t' || tchar == '\n';
}

static int is_string(string value)
{
  return value.length() >= 2 && value.c_str()[0] == '"' && value.c_str()[value.length() - 1] == '"';
}

static int is_number(string value)
{
  const char *str;
  str = value.ro_c_str();
  for (int i = 0; i < value.length(); i++) {
    if (!((str[i] >= '0' && str[i] <= '9') || str[i] == '.'))
      return 0;
  }
  return 1;
}

static json_parsed consume_white_space(int start, substring source, json_parsed &ret){
  int t;
  ret.next = '\0';
  memset(&ret.remaining, 0, sizeof(substring));
  for (int i = start; i < len(source); i++) {
    if (!is_white_space(char_at(source, i))) {
      if (i + 1 >= len(source)) {
        t = len(source);
      } else {
        t = i + 1;
      }
      ret.next = char_at(source, i);
      ret.remaining = make_substring(source, t, len(source));
      return ret;
    }
  }
  return ret;
}

static json_parsed consume_string(substring source)
{
  int escaped;
  json_parsed ret;
  int j;
  consume_white_space(0, source, ret);
  if (ret.next != '"') {
    return ret;
  }
  j = 0;
  escaped = 0;
  ret.value = "\"";
  source = ret.remaining;
  for (int i = 0; i < len(source); i++) {
    if (!escaped) {
      if (char_at(source, i) == '"' ){
        consume_white_space(i + 1, source, ret);
        ret.value += make_string(source, j, i + 1);
        return ret;
      } else if (char_at(source, i) == '\\') {
        escaped = 1;
        ret.value += make_string(source, j, i);
        j = i + 1;
      }
    } else {
      escaped = 0;
    }
  }
  ret.value = source.str;
  ret.next = 0;
  memset(&ret.remaining, 0, sizeof(substring));
  return ret;
}

static json_parsed consume(substring source)
{
  int t;
  char chr;
  json_parsed ret;
  ret = consume_string(source);
  if (is_string(ret.value))
    return ret;
  for (int i = 0; i < len(source); i++) {
    chr = char_at(source, i);
    if (chr == '"' || chr == '[' || chr == ']' || chr== '}' || chr == '{' || chr == ',' || chr == ':') {
      if (i + 1 >= len(source)) {
        t = len(source);
      } else {
        t = i + 1;
      }
      ret.value = make_string(source, 0, i);
      ret.value.trim();
      ret.next = chr;
      ret.remaining = make_substring(source, t, len(source));
      return ret;
    }
  }
  ret.value = make_string(source, 0, len(source));
  ret.next = 0;
  memset(&ret.remaining, 0, sizeof(substring));
  return ret;
}

static JSON parse_string(string &value) {
  value = value.copy(1, value.length() - 2);
  return JSON(value.c_str());
}

static json_parsed parse_value(json_parsed ret)
{
  if (ret.value.length() == 0) {
    ret.result = JSON("");
    return ret;
  }
  ret.value.trim();
  if (is_string(ret.value))
    ret.result = parse_string(ret.value);
  else if (ret.value == "true" || ret.value == "false")
    ret.result = JSON(ret.value == "true", 1);
  else if (ret.value == "null")
    ret.result = JSON();
  else if (is_number(ret.value)) {
    if (ret.value.pos(".") < 0)
      ret.result = JSON(ret.value.to_int());
    else
    ret.result = JSON(ret.value.to_double());
  } else
    ret.error = 1;
  return ret;
}

static json_parsed parse(substring source);

static json_parsed parse_array(json_parsed ret)
{
  json_parsed tmp;
  JSON json;
  json.set_type(JSON_ARRAY);
  consume_white_space(0, ret.remaining, tmp);
  if (tmp.next == ']')
    ret = tmp;
  while (ret.next != ']') {
    ret = parse(ret.remaining);
    if (ret.error)
      return ret;
    if (ret.next != ',' && ret.next != ']') {
      ret.error = 1;
      return ret;
    }
    json.add(ret.result);
  }
  consume_white_space(0, ret.remaining, ret);
  ret.result = json;
  return ret;
}

static json_parsed parse_object(json_parsed ret)
{
  json_parsed tmp;
  string name;
  JSON json;
  json.set_type(JSON_OBJECT);
  consume_white_space(0, ret.remaining, tmp);
  if (tmp.next == '}')
    ret = tmp;
  while (ret.next != '}') {
    ret = consume(ret.remaining);
    if (!is_string(ret.value) || ret.next != ':') {
      ret.error = 1;
      return ret;
    }
    name = ret.value.copy(1, ret.value.length() - 2);
    ret = parse(ret.remaining);
    if (ret.error)
      return ret;
    if (ret.next != ',' && ret.next != '}') {
      ret.error = 1;
      return ret;
    }
    json.add(name, ret.result);
  }
  consume_white_space(0, ret.remaining, ret);
  ret.result = json;
  return ret;
}

static json_parsed parse(substring source) {
  json_parsed ret;
  ret = consume(source);
  if (ret.value.length() > 0) {
    return parse_value(ret);
  } else if (ret.next == '[') {
    return parse_array(ret);
  } else if (ret.next == '{') {
    return parse_object(ret);
  } else {
    ret.error = 1;
  }
  return ret;
}

int JSON::from_string(const char *str)
{
  substring sstr;
  json_parsed parsed;
  clear();
  sstr.str = str;
  sstr.start = 0;
  sstr.end = strlen(str);
  parsed = parse(sstr);
  if (parsed.error || parsed.next != '\0')
    return 0;
  *this = parsed.result;
  return 1;
}

JSON xmlrpc_value_to_json(SMLnode *value);

JSON xmlrpc_array_to_json(SMLnode *array)
{
  JSON ret;
  SMLnode *values, *value;
  cursor c;
  ret.set_type(JSON_ARRAY);
  values = array->child("data");
  if (values == NULL)
    return ret;
  value = values->init_ite(c);
  while (value != NULL) {
    ret.add(xmlrpc_value_to_json(value));
    value = values->iterate(c);
  }
  return ret;
}

JSON xmlrpc_struct_to_json(SMLnode *strct)
{
  JSON ret;
  SMLnode *member, *value, *name;
  cursor c;
  ret.set_type(JSON_OBJECT);
  member = strct->init_ite(c);
  while (member != NULL) {
    value = member->child("value");
    name = member->child("name");
    if (name == NULL || value == NULL)
      continue;
    ret.add(name->get_str_value(""), xmlrpc_value_to_json(value));
    member = strct->iterate(c);
  }
  return ret;
}

JSON xmlrpc_value_to_json(SMLnode *value)
{
  JSON ret;
  string data_type, str;
  if (value == NULL)
    return ret;
  value = value->child(0);
  if (value == NULL)
    return ret;
  data_type = value->get_name();
  if (data_type == "i4" || data_type == "int")
    ret.set_value(value->get_int_value());
  else if (data_type == "double")
    ret.set_value(value->get_double_value());
  else if (data_type == "boolean")
    ret.set_bool_value(value->get_int_value());
  else if (data_type == "nil")
    ret.clear();
  else if (data_type == "array")
    ret = xmlrpc_array_to_json(value);
  else if (data_type == "struct")
    ret = xmlrpc_struct_to_json(value);
  else {
    str = value->get_str_value();
    ret.set_value(str);
  }
  return ret;
}

JSON xmlrpc_to_json(SMLnode &xml)
{
  JSON ret, jparams;
  const char * method_name;
  SMLnode *params, *param, *value, *fault;
  cursor c;
  fault = xml.child("fault");
  //the fault string is a special case
  if (fault != NULL) {
    ret.add("fault", xmlrpc_value_to_json(fault->child("value")));
    return ret;
  }
  method_name = xml.get_leaf_str_value("methodName", NULL);
  if (method_name != NULL)
    ret.add("methodName", method_name);
  params = xml.child("params");
  if (params == NULL)
    return ret;
  jparams.set_type(JSON_ARRAY);
  param = params->init_ite(c);
  while (param != NULL) {
    value = param->child("value");
    if (value != NULL)
      jparams.add(xmlrpc_value_to_json(value));
    param = params->iterate(c);
  }
  ret.add("params", jparams);
  return ret;
}

void json_to_xmlrpc_value(JSON &jparam, SMLnode *param);

void json_to_xmlrpc_array(JSON &jarray, SMLnode *array)
{
  JSON jvalue;
  SMLnode *values;
  values = array->add_node("data");
  for (int i = 0; i < jarray.count(); i++) {
    if(!jarray.get(i, jvalue))
      continue;
    json_to_xmlrpc_value(jvalue, values);
  }
}

void json_to_xmlrpc_struct(JSON &jstruct, SMLnode *strct)
{
  JSONField field;
  SMLnode *member, *name;
  for (int i = 0; i < jstruct.count(); i++) {
    if (!jstruct.get_field(i, field))
      continue;
    member = strct->add_node("member");
    name = member->add_str_leaf("name", field.name);
    json_to_xmlrpc_value(field.value, member);
  }
}

void json_to_xmlrpc_value(JSON &jparam, SMLnode *param)
{
  SMLnode *value;
  int data_type;
  value = param->add_node("value");
  data_type = jparam.get_type();
  switch(data_type) {
    case JSON_BOOLEAN:
      value->add_int_leaf("boolean", jparam.get_boolean_value());
      break;
    case JSON_NUMBER:
      if (jparam.is_integer_value())
        value->add_int_leaf("i4", jparam.get_integer_value());
      else
        value->add_double_leaf("double", jparam.get_double_value());
      break;
    case JSON_ARRAY:
      json_to_xmlrpc_array(jparam, value->add_node("array"));
      break;
    case JSON_OBJECT:
      json_to_xmlrpc_struct(jparam, value->add_node("struct"));
      break;
    case JSON_NULL:
      value->add_node("nil");
      break;
    default:
      value->add_str_leaf("string", jparam.get_str_value());
  }
}

void json_to_xmlrpc(JSON &json, SMLnode &root)
{
  JSON ret, jparam;
  SMLnode *params, *param;
  if (json.get("methodName", ret))
    root.add_str_leaf("methodName", ret.get_str_value());
  if (!json.get("params", ret))
    return;
  params = root.add_node("params");
  for (int i = 0; i < ret.count(); i++) {
    if(!ret.get(i, jparam))
      continue;
    param = params->add_node("param");
    json_to_xmlrpc_value(jparam, param);
  }
}
