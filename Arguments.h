#pragma once

#include <unordered_map>
#include <string>
#include <iostream>

class Arguments {
    
    bool help = false;
    std::unordered_map<std::string, std::string> args;
    
    template<typename T>
    T fromString (const std::string& str) const {
        // error out upon no existing specialization - see below for implemented specializations
        std::string typeName = typeid(T).name();
        std::printf("No specialization of fromString<%s> in Arguments class, cannot read %s types!\n", typeName.c_str(), typeName.c_str());
        exit(1);
    }
    
    template<typename T>
    std::string typeName () const {
        return "unknown type";
    }
    
public:
    
    Arguments(int argc, char** argv) {
        std::string prevKey;
        bool hasPrevKey = false;
        for (int i = 1; i < argc; ++i) {
            std::string arg = std::string(argv[i]);
            if (arg.length() > 0) {
                
                // passing 'help' as an argument turns on help mode, printing each key/type and exiting early
                if (arg.compare("help") == 0 && !hasPrevKey) {
                    help = true;
                    printf("Usage:\n");
                    continue;
                }
                
                // if the argument starts with a dash, it's a new key; if not, it's the value to set for the last key
                if (arg[0] == '-') {
                    arg.erase(arg.begin()); // remove prefix dash
                    if (arg[0] == '-') arg.erase(arg.begin()); // allow 2 dashes instead of 1 optionally
                    prevKey = arg;
                    hasPrevKey = true;
                    args.insert({ arg, "true" });
                } else if (hasPrevKey) {
                    args[prevKey] = arg;
                    hasPrevKey = false;
                } else {
                    std::printf("Error reading arguments: value '%s' is not bound to a key (did you mean '-%s'?)\n", arg.c_str(), arg.c_str());
                    exit(1);
                }
                
            }
        }
    }
    
    virtual ~Arguments () {
        if (args.size() > 0) {
            std::printf("Unused arguments, are you sure you meant to include these?\n%s", toString().c_str());
            exit(1);
        }
        if (help) {
            exit(0);
        }
    }
    
    template<typename T>
    T read(const std::string& key, const T& defaultValue, bool required = false) {
        T val = defaultValue;
        const auto& found = args.find(key);
        if (found != args.end()) {
            const std::string& str = found->second;
            val = fromString<T>(str);
            args.erase(found);
        } else if (required && !help) {
            std::printf("No argument passed for required parameter -%s!\n", key.c_str());
            exit(1);
        }
        if (help) {
            printf("-%s: %s", key.c_str(), typeName<T>().c_str());
            if (required) printf(" (required)");
            else std::cout << " (default: " << defaultValue << ")";
            printf("\n");
        }
        return val;
    }
    
    template<typename T>
    T read(const std::string& key) {
        return read<T>(key, T{}, true);
    }
    
    std::string toString () const {
        std::string ret = "";
        for (const auto& pair : args) {
            ret += "- " + pair.first + ": " + pair.second + "\n";
        }
        return ret;
    }
    
};



template<>
std::string Arguments::fromString<std::string>(const std::string& val) const {
    return val;
}

template<>
bool Arguments::fromString<bool>(const std::string& val) const {
    if (val.rfind("true", 0) == 0 || val.rfind("1", 0) == 0 || val.rfind("True", 0) == 0) {
        return true;
    } else if (val.rfind("false", 0) == 0 || val.rfind("0", 0) == 0 || val.rfind("False", 0) == 0) {
        return false;
    } else {
        std::printf("Could not convert '%s' to bool; use 'true' (or '1') or 'false' (or '0')!\n", val.c_str());
        exit(1);
    }
}

#define FROM_STRING_STD_STO_T(T, stot) \
    template<> T Arguments::fromString<T>(const std::string& val) const { \
        try { \
            return std::stot(val); \
        } catch (const std::invalid_argument& e) { \
            std::printf("Could not convert '%s' to " #T " (invalid argument): %s\n", val.c_str(), e.what()); \
            exit(1); \
        } catch (const std::out_of_range& e ) { \
            std::printf("Could not convert '%s' to " #T " (out of range): %s\n", val.c_str(), e.what()); \
            exit(1); \
        } \
    }
FROM_STRING_STD_STO_T(int, stoi);
FROM_STRING_STD_STO_T(float, stof);
#undef FROM_STRING_STD_STO_T


#define TYPE_NAME(T) \
    template<> std::string Arguments::typeName<T>() const { return #T; }
TYPE_NAME(std::string);
TYPE_NAME(int);
TYPE_NAME(float);
TYPE_NAME(bool);
