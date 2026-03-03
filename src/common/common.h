#pragma once

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define FUNCTION_LINE __FILE__ ":" TOSTRING(__LINE__)
