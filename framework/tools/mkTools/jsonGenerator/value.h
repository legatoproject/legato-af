
//--------------------------------------------------------------------------------------------------
/**
 * @file value.h
 */
//--------------------------------------------------------------------------------------------------

namespace json
{

namespace data
{



class Value_t;



// Setup STL mappings for the values data types.
using String_t = std::string;
using Object_t = std::map<String_t, Value_t>;
using Array_t = std::vector<Value_t>;



//--------------------------------------------------------------------------------------------------
/**
 * Ids for the types of data that can be stored in a value.
 */
//--------------------------------------------------------------------------------------------------
enum class Type_t
{
    Null,    ///< The value is empty.
    Object,  ///< The value is storing an object with sub-values.
    Array,   ///< The value holds a list of values.
    String,  ///< The value is a string.
    Bool,    ///< The value is either true or false.
    Number   ///< The value stores a number.
};



//--------------------------------------------------------------------------------------------------
/**
 * The core of the JSON library.  Data from the application is stored into the value data structure
 * which is then streamed in or out to provide interoperability.
 */
//--------------------------------------------------------------------------------------------------
class Value_t
{
    private:
        Type_t type;

        union Data_t
        {
            Object_t object;
            Array_t array;
            String_t string;
            bool boolean;
            double number;

            Data_t();
            ~Data_t();
        }
        data;

    public:
        Value_t();
        Value_t(const Object_t& object);
        Value_t(Object_t&& object);
        Value_t(const Array_t& array);
        Value_t(Array_t&& array);
        Value_t(const char* string);
        Value_t(const String_t& string);
        Value_t(String_t&& string);
        Value_t(bool boolean);
        Value_t(int number);
        Value_t(double number);
        Value_t(const std::initializer_list<Value_t>& arrayList);
        Value_t(const Value_t& value);
        Value_t(Value_t&& value);
        ~Value_t();

    public:
        Value_t& operator =(const Value_t& value);
        Value_t& operator =(Value_t&& value);

        bool operator ==(const Value_t& other);
        bool operator !=(const Value_t& other);
        bool operator >(const Value_t& other);
        bool operator <(const Value_t& other);
        bool operator >=(const Value_t& other);
        bool operator <=(const Value_t& other);

        operator bool() const;

        const Value_t& operator [](const char* index) const;
        Value_t& operator [](const char* index);

        const Value_t& operator [](const std::string& index) const;
        Value_t& operator [](const std::string& index);

        const Value_t& operator [](size_t index) const;
        Value_t& operator [](size_t index);

    public:
        Type_t Type() const;

        bool IsNull() const;
        bool IsObject() const;
        bool IsArray() const;
        bool IsString() const;
        bool IsBoolean() const;
        bool IsNumber() const;

    public:
        const Object_t& AsObject() const;
        Object_t& AsObject();
        const Array_t& AsArray() const;
        Array_t& AsArray();
        const String_t& AsString() const;
        String_t& AsString();
        bool AsBoolean() const;
        double AsNumber() const;

    private:
        std::string TypeName(Type_t type) const;
        void Expect(Type_t type) const;

        void Reset();
};



} // namespace data

} // namespace json
