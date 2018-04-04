#include <haste/jvalue>
#include <haste/panic>
#include <haste/unittest>

namespace haste {

static const string null_string = "null";

jvalue::jvalue() {
  _data = null_string;
}

static jtype json_type(char c) {
  switch (c) {
    case 'n': return jtype::jnull;
    case 't': return jtype::jbool;
    case 'f': return jtype::jbool;
    case '-': return jtype::jnumber;
    case '0': return jtype::jnumber;
    case '1': return jtype::jnumber;
    case '2': return jtype::jnumber;
    case '3': return jtype::jnumber;
    case '4': return jtype::jnumber;
    case '5': return jtype::jnumber;
    case '6': return jtype::jnumber;
    case '7': return jtype::jnumber;
    case '8': return jtype::jnumber;
    case '9': return jtype::jnumber;
    case '"': return jtype::jstring;
    case '{': return jtype::jobject;
    case '[': return jtype::jarray;
    default: panic("invalid-json");
  }
}

jtype jvalue::type() const {
  return json_type(*_data.data());
}

bool jvalue::as_bool() const {
  return *_data.data() == 't';
}

i32 jvalue::as_i32() const {
  return 0;
}

u32 jvalue::as_u32() const {
  return 0;
}

i64 jvalue::as_i64() const {
  return 0;
}

u64 jvalue::as_u64() const {
  return 0;
}

f32 jvalue::as_f32() const {
  return 0;
}

f64 jvalue::as_f64() const {
  return 0;
}

string jvalue::as_string() const {
  return _data.slice(1, _data.size() - 1);
}

jvalue jvalue::at(const char*) const {
  return jvalue();
}

jvalue jvalue::at(const string&) const {
  return jvalue();
}

jvalue jvalue::at(usize index) const {
  return jvalue(_data.slice(_index.data()[index], _index.data()[index + 1] - 1));
}

jvalue::jvalue(const string& json)
  : _data(json) {
}


unittest() {
  assert_eq(jvalue().type(), jtype::jnull);



}

}
