#include "utility.h"



//====================================================================================================================================
std::string readFileSrc( const std::string& _rstrFilePath )
{
    //--------------------------------------------------------------------------------------------------------------------
    // precondition
    ASSERT( _rstrFilePath.size() > 0 , "Invalid parameter _rstrFilePath(\"%s\") : size() must not be 0.\n", _rstrFilePath.c_str() );
    //-----------------------------------------------------------------------------

    LOG("Opening file %s\n", _rstrFilePath.c_str() );

    std::string strContent;


    std::ifstream file(_rstrFilePath.c_str());
    ASSERT( file, "Could not open file %s\n", _rstrFilePath.c_str() );

    file.seekg(0, std::ios::end);
    strContent.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    strContent.assign((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());


    //-----------------------------------------------------------------------------
    // postcondition
    ASSERT( strContent.size() > 0 ,  "Invalid content read strContent(\"%s\") : size() must not be 0.\n", strContent.c_str() );
    //-----------------------------------------------------------------------------

    return strContent;
}
