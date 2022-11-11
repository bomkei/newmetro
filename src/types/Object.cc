#include "types/Object.h"
#include "Utils.h"

template <>
std::string ObjImmediate<bool, TYPE_Bool>::to_string() const
{
  return this->value ? "true" : "false";
}

template <TypeKind k, char begin, char end>
ObjList<k, begin, end>::ObjList()
    : Object(k)
{
}

template <TypeKind k, char begin, char end>
std::string ObjList<k, begin, end>::to_string() const
{
  return begin +
         Utils::join<Object*>(", ", this->elements,
                              [](auto x) { return x->to_string(); }) +
         end;
}

template <TypeKind k, char begin, char end>
ObjList<k, begin, end>* ObjList<k, begin, end>::clone() const
{
  auto x = new ObjList<k, begin, end>;

  for (auto&& elem : this->elements) {
    x->elements.emplace_back(elem->clone());
  }

  return x;
}

Object::Object(Type const& type)
    : type(type),
      ref_count(0)
{
}

ObjNone::ObjNone()
    : Object(TYPE_None)
{
}

std::string ObjNone::to_string() const
{
  return "none";
}

ObjNone* ObjNone::clone() const
{
  return new ObjNone;
}

ObjString::ObjString(std::wstring&& val)
    : Object(TYPE_String),
      value(std::move(val))
{
}

ObjString::ObjString(std::wstring const& val)
    : Object(TYPE_String),
      value(val)
{
}

void ObjString::append(wchar_t ch)
{
  this->value.push_back(ch);
}

void ObjString::append(std::wstring const& s)
{
  this->value.append(s);
}

std::string ObjString::to_string() const
{
  return Utils::Converter::to_utf8(this->value);
}

ObjString* ObjString::clone() const
{
  return new ObjString(this->value);
}

ObjFloat::ObjFloat(float val)
    : Object(TYPE_Float),
      value(val)
{
}

std::string ObjFloat::to_string() const
{
  auto s = std::to_string(this->value);

  while (s.length() >= 2 && *(s.rbegin() + 1) != '.' &&
         *s.rbegin() == '0') {
    s.pop_back();
  }

  return s;
}

ObjFloat* ObjFloat::clone() const
{
  return new ObjFloat(this->value);
}

ObjRange::ObjRange(ValueType begin, ValueType end)
    : Object(TYPE_Range),
      begin(begin),
      end(end)
{
}

std::string ObjRange::to_string() const
{
  return Utils::format("range(%lu, %lu)", this->begin, this->end);
}

ObjRange* ObjRange::clone() const
{
  return new ObjRange(begin, end);
}

ObjFunction::ObjFunction(Node* func)
    : Object(TYPE_Function),
      is_builtin(false),
      func(func),
      builtin(nullptr)
{
}

std::string ObjFunction::to_string() const
{
  return Utils::format(
      "<%sfunc 0x%X>", this->builtin ? "builtin-" : "",
      std::hash<void*>()(this->is_builtin ? (void*)this->builtin
                                          : this->func));
}

ObjFunction* ObjFunction::clone() const
{
  return new ObjFunction(*this);
}

ObjFunction* ObjFunction::from_builtin(BuiltinFunc const& b)
{
  auto x = new ObjFunction(nullptr);

  x->is_builtin = true;
  x->builtin = &b;

  return x;
}

template struct ObjList<TYPE_Tuple, '(', ')'>;
template struct ObjList<TYPE_Vector, '[', ']'>;
