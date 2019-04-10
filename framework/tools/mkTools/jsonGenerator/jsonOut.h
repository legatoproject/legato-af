
//--------------------------------------------------------------------------------------------------
/**
 * @file jsonOut.h
 */
//--------------------------------------------------------------------------------------------------

namespace json
{

namespace data
{



//--------------------------------------------------------------------------------------------------
/**
 * Write a json object as properly formated json text to the given output stream.
 *
 * @return A reference to the input stream for further pipelining.
 */
//--------------------------------------------------------------------------------------------------
std::ostream& operator <<
(
    std::ostream& out,      ///< [IN] The output stream to write to.
    const Object_t& object  ///< [IN] The object to write as json.
);



//--------------------------------------------------------------------------------------------------
/**
 * Write a json array as properly formated json text to the given output stream.
 *
 * @return A reference to the input stream for further pipelining.
 */
//--------------------------------------------------------------------------------------------------
std::ostream& operator <<
(
    std::ostream& out,    ///< [IN] The output stream to write to.
    const Array_t& array  ///< [IN] The array to write as json.
);



//--------------------------------------------------------------------------------------------------
/**
 * Write a json value as properly formated json text to the given output stream.
 *
 * @return A reference to the input stream for further pipelining.
 */
//--------------------------------------------------------------------------------------------------
std::ostream& operator <<
(
    std::ostream& out,    ///< [IN] The output stream to write to.
    const Value_t& value  ///< [IN] The value to write as json.
);



} // namespace data

} // namespace json
