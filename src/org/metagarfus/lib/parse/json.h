#ifndef JSON_H
#define JSON_H
#include "string.h"
#include "sml.h"

#define JSON_NULL     0
#define JSON_OBJECT   1
#define JSON_ARRAY    2
#define JSON_STRING   3
#define JSON_BOOLEAN  4
#define JSON_NUMBER   5

#define JSON_IS_VALUE(X) ((X) == JSON_STRING || (X) == JSON_BOOLEAN || (X) == JSON_NUMBER)
struct json_data;

class JSONField;

class JSON
{
private:
  json_data *data;
  void free_data();
  void set_data(json_data *data);
  void ensure_data_uniqueness();
  void internal_set_value(const char *value, int type);
  JSONField *internal_get(const char *name);
  void internal_add(const char *name, JSON json);
public:
  JSON();
  JSON(const JSON &json);
  JSON(int value, int is_boolean);
  JSON(int value);
  JSON(double value);
  JSON(const char * value);
  JSON(JSON* values, int count);
  JSON(JSONField* values, int count);
  ~JSON();
  int set_type(int type);
  int get_type();
  void set_bool_value(int value);
  void set_value(int value);
  void set_value(double value);
  void set_value(const char *value);
  void clear();
  void add(const char *name, JSON json);
  void add(JSON json);
  int is_null_value();
  int is_integer_value();
  const char *get_str_value();
  int get_boolean_value();
  int get_integer_value();
  double get_double_value();
  int get(const char *name, JSON &ret);
  int get(int i, JSON &ret);
  int get_field(int i, JSONField &ret);
  int get_field_integer(const char *name, int def = 0);
  double get_field_double(const char *name, double def = 0);
  const char *get_field_str(const char *name, const char *def = "");
  int get_field_boolean(const char *name, int def = 0);
  int is_field_null(const char *name, int def = 0);
  JSON &operator=(const JSON &bro);
  string to_string();
  int from_string(const char *str);
  int count();
};


class JSONField
{
public:
  string name;
  JSON value;
};

JSON xmlrpc_to_json(SMLnode &xml);
void json_to_xmlrpc(JSON &json, SMLnode &root);
#endif // JSON_H
