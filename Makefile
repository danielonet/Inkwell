## Inkwell - Haiku makefile using Haiku's makefile-engine.
## Build on Haiku with:  make   (binary lands in objects.*/Inkwell)

NAME = Inkwell
TYPE = APP
APP_MIME_SIG = application/x-vnd.Inkwell

SRCS = \
	src/main.cpp \
	src/MainWindow.cpp \
	src/SidebarView.cpp \
	src/EditorView.cpp \
	src/MarkdownRender.cpp \
	src/MarkdownView.cpp

RDEFS = Inkwell.rdef

# localestub provides the Locale Kit catalog lookup used by B_TRANSLATE.
LIBS = be tracker localestub $(STDCPPLIBS)
LIBPATHS =
LOCAL_INCLUDE_PATHS = src

# Add language codes here once locales/<code>.catkeys exist, e.g. LOCALES = en he
LOCALES =

OPTIMIZE := FULL
DEFINES =
WARNINGS = ALL
SYMBOLS :=
DEBUGGER :=
COMPILER_FLAGS = -std=c++11
LINKER_FLAGS =

## Include the makefile-engine
DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
