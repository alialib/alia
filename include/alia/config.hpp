#ifndef ALIA_CONFIG_HPP
#define ALIA_CONFIG_HPP

// compile-time configuration options for alia

// alia requires that an application's UI function maintain consistent control
// flow within a frame and inform alia of possible variations between frames.
// (See the section on control flow in the documentation for more information.)
// When an application fails to do this, it compromises alia's data management
// facilities and an exception is generated.  However, checking for these
// potential errors every frame takes time.  If you're sure that your UI is
// free from errors and you want to eliminate these checks, comment out this
// symbol.  Note that if you comment this out and you actually do have errors,
// your application will symbol crash without explanation.
#define ALIA_SAFE

// There are other situations that represent improper control flow consistency,
// but which are not dangerous to alia.  If you want alia to generate
// exceptions in these situations as well, uncomment the symbol below.
//#define ALIA_PEDANTIC

#endif
