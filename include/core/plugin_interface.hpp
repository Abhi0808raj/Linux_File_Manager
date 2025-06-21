#pragma once

#include<string>
#include<vector>

//base interface class for all file manager plugins
//Any plugin which is added must inherit from this class and
//implement all the virtual methods
class IFileManagerPlugin {
public:
    //Virtual destructor for automatic destruction
    //ensuring correct cleanup
    virtual ~IFileManagerPlugin() = default;

    //Function for returning the name
    //const=0 meaning this function is pure virtual
    virtual std::string name() const = 0;

    //Function for returning the plugin version
    virtual std::string version() const = 0;

    //Function for returning plugin description
    virtual std::string description() const = 0;

    //Returns vector of operations supported by plugin
    virtual std::vector<std::string> operations() const = 0;

    // Boolean function which returns true if operation was successful
    // Perform operation defined by a plugin
    //Takes String for the name of operations and Vector<string>
    //for the arguments needed for opertaion
    virtual bool execute(const std::string& operation, const std::vector<std::string>& args) = 0;
};

//extern "C" is used for disabling name mangling
//Name mangling means compiler changes names behind the scenes
//in order to account function overloading support
//extern "C" is telling the compiler to
//Use C linkage which does not support function overloading
//No function overloading is required because it is just the entry point
//
extern "C" {
    IFileManagerPlugin* create_plugin();
}