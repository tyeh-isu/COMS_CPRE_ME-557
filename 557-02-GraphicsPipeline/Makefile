########################################################################
####################### Makefile Template ##############################
########################################################################

DEBUG = -g
VERBOSE = -v
UNAME = $(shell uname -s)
APPNAME = Vulkan02

# Windiows definitions
ifeq ($(UNAME), Windows_NT)
	APP_NAME = $(APPNAME).exe
	GLM_PATH = $(VULKAN_SDK)/include
	GLFW_PATH = $(VULKAN_SDK)/glfw-3.3.9.bin.WIN64/include
	GLFW_LIB_PATH = $(VULKAN_SDK)/glfw-3.3.9.bin.WIN64/lib-mingw-w64
	LDFLAGS = -L$(VULKAN_SDK)/Lib -L$(GLFW_LIB_PATH) -lvulkan-1 -lglfw3 -lgdi32
	RUNSCRIP = ./compile-win.bat
	DEFINES = __WINDOWS__
endif

# MacOS definitions
ifeq ($(UNAME), Darwin)
	APP_NAME = $(APPNAME)
	GLM_PATH = $(VULKAN_SDK)/include
	GLFW_PATH = $(VULKAN_SDK)/glfw-3.3.9.bin.MACOS/include
	GLFW_LIB_PATH = $(VULKAN_SDK)/glfw-3.3.9.bin.MACOS/lib-x86_64
	LDFLAGS = -L$(VULKAN_SDK)/lib -L$(GLFW_LIB_PATH) -lvulkan -lglfw
	RUNSCRIP = ./compile-unx.bat
	DEFINES = __DARWIN__
endif

# Linux definitions
ifeq ($(UNAME), Linux)
	APP_NAME = $(APPNAME)
	GLM_PATH = /usr/include/glm-0.9.6.3/glm
	GLFW_PATH = /usr/include/GLFW
	GLFW_LIB_PATH = /usr/lib/x86_64-linux-gnu
	LDFLAGS = -L$(VULKAN_SDK)/lib -L$(GLFW_LIB_PATH) -lvulkan -lglfw
	RUNSCRIP = ./compile-unx.bat
	DEFINES = __LINUX__
endif

CFLAGS = -std=c++17 $(DEBUG) -I. -I$(VULKAN_SDK)/include -I$(GLM_PATH) -I$(GLFW_PATH)
include LOCAL_SRCS

$(APP_NAME): *.cpp
	g++ -D$(DEFINES) $(CFLAGS) -o $(APP_NAME) $(CPPSRCS) $(LDFLAGS)

shader:
ifeq ($(UNAME), Windows_NT)
else
	chmod 755 $(RUNSCRIP)
endif
	$(RUNSCRIP)

run: $(APP_NAME)
	./$(APP_NAME)

clean:
	rm -f $(APP_NAME)

test:
	@echo $(UNAME)