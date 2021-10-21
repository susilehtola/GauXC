#pragma once
#include <gauxc/gauxc_config.hpp>
#include <stdexcept>
#include <string>
#include <sstream>
#include <string.h>

namespace GauXC {

// FWD decl all exception types for optional handling

#ifdef GAUXC_ENABLE_CUDA
class cuda_exception;
class cublas_exception;
#endif

#ifdef GAUXC_ENABLE_HIP
class hip_exception;
class hipblas_exception;
#endif

#ifdef GAUXC_ENABLE_MAGMA
class magma_exception;
#endif

class generic_gauxc_exception : public std::exception {

  std::string file_;
  std::string function_;
  int         line_;
  std::string msg_prefix_;

  const char* what() const noexcept override {
     std::stringstream ss;
     ss << "Generic GauXC Exception (" << msg_prefix_ << ")" << std::endl
        << "  File     " << file_ << std::endl
        << "  Function " << function_ << std::endl
        << "  Line     " << line_  << std::endl;
     auto msg = ss.str();

     return strdup( msg.c_str() );
  };

public:

  /**
   *  @brief Construct a generic_gauxc_exception object
   *
   *  @param[in] file File which contains the code that threw the exception
   *  @param[in] function Function which threw the exception
   *  @param[in] line Line number of file that threw exception
   *  @param[in] msg  General descriptor of task which threw exception
   */
  generic_gauxc_exception( std::string file, std::string function, int line, 
    std::string msg ) :
    file_(file), line_(line), function_(function), msg_prefix_(msg) {} 

};


}

#define GAUXC_GENERIC_EXCEPTION( MSG ) \
  throw generic_gauxc_exception( __FILE__, __PRETTY_FUNCTION__, __LINE__, MSG )

#define GAUXC_PIMPL_NOT_INITIALIZED() \
  GAUXC_GENERIC_EXCEPTION("PIMPL NOT INITIALIZED")

#define GAUXC_BAD_LWD_DATA_CAST() \
  GAUXC_GENERIC_EXCEPTION("BAD DATA CAST")

#define GAUXC_BAD_BACKEND_CAST() \
  GAUXC_GENERIC_EXCEPTION("BAD BACKEND CAST")

#define GAUXC_UNINITIALIZED_DEVICE_BACKEND() \
  GAUXC_GENERIC_EXCEPTION("UNINITIALIZED DEVICE BACKEND")
