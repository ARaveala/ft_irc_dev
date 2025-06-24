
## C++ STYLE

Let's try to enforce some standards for our coding to make things more uniform and easy to read.
Using <a href="https://google.github.io/styleguide/cppguide.html" target="_blank">Google's C++ Style Guide</a> for inspiration with some changes.

Some general principles:
<ul>
  <li><b>C++17</b> standard.</li>
  <li>Using <b>smart pointers</b> instead of raw pointers.</li>
  <li>Use as much private stuff as possible, add underscore to the end of the name. Example: private_variable_</li>
  <li>Header files will have .h suffix and don't contain code implementation.</li>
  <li>Each module should be in its own subfolder in the repo, under the src/ -directory. Including it's header files for the module. </li>
  <li>Each .cpp file should include only it's own header file. No cross-referencing.</li>
  <li>Try to write short functions. Separate single tasks into single functions that do only one thing.</li>
  <li>Use const variables as much as possible. If you have a function that takes a variable that it doesn't need to modify, take it in as a const ref if possible.</li>
  <li>Try to declare local variables close to first use in the function. Initialize variables when declaring them.</li>
</ul>

### Naming

Let's try to be as descriptive with naming as we can, even if the names can become long. In short functions and for loops _n_ or _i_ could be fine but as a Class member, please god no.

Formatting:

**Classes**: UpperCamelCase

**Functions**: lowerCamelCase

**Variables**: snake_case. class data memberse set to private will have underscore(_) in the end.

**Header guard**: __HEADER_H__

**EXAMPLE**
```
#ifndef __CLIENT_H__
#define __CLIENT_H__

class Client{
  private:
  int            some_variable_;
  std::string    other_variable_;
  std::string    nickname_;

  public:
  // getters
  std::string getNickname();
  int         getSomeVariable();

  // setters
  void        setNickname(std::string const &new_nick);
  void        setSomeVariable(int const &value);

  // member functions
  void        doSomethingUseful()

#endif
```

