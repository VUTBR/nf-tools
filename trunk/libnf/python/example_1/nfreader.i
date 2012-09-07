%module nfreader

%typemap(in) (int argc, char **argv) {
  int i;
  if (!PyList_Check($input)) {
    PyErr_SetString(PyExc_ValueError, "Expecting a list");
    return NULL;
  }
  $1 = PyList_Size($input);
  $2 = (char **) malloc(($1+1)*sizeof(char *));
  for (i = 0; i < $1; i++) {
    PyObject *s = PyList_GetItem($input,i);
    if (!PyString_Check(s)) {
        free($2);
        PyErr_SetString(PyExc_ValueError, "List items must be strings");
        return NULL;
    }
    $2[i] = PyString_AsString(s);
  }
  $2[i] = 0;
}

%typemap(freearg) (int argc, char *argv[]) {
   if ($2) free($2);
}


%{
  #include <setjmp.h>
  #include <signal.h>

  static sigjmp_buf timeout;

  static void backout(int sig) {
    siglongjmp(timeout, sig);
  }

	int main( int argc, char **argv );
	#include "../nfdump/bin/nffile.h"
	#include "../nfdump/bin/nfx.h"
	#include "../nfdump/bin/util.h"
	#include "../nfdump/bin/flist.h"
%}

%include <exception.i>

%exception {
  if (!sigsetjmp(timeout, 1)) {
    signal(SIGINT,backout); // Check return?
    $action
  }
  else {
    // raise a Python exception
    SWIG_exception(SWIG_RuntimeError, "Timeout in $decl");
  }
}


int main( int argc, char **argv );

%exception;
